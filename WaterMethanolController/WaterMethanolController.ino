/***********************************************************************
  WaterMethanol Controller V4.7

  High-level behavior:
  - Reads manifold absolute pressure and rail pressure, then commands spray duty from a ten point curve.
  - Captures barometric pressure at boot and uses it in injector differential pressure math.
  - Test Section can temporarily override barometric reference for bench diagnostics.
  - Pump runs before injectors open until the configured target injector differential pressure is reached.
  - Safety uses rail differential pressure ((rail gauge pressure + active barometric pressure) minus manifold absolute pressure) and tank level:
      * Low level always forces boost cut and spray off.
      * Rail differential pressure fault or low level while spraying triggers timing cut:
        - Intake air temperature is grounded to pull timing, boost is cut, and pump and injectors are off.
        - Differential pressure timing cut auto clears after the configured delay.
        - Boost cut from a differential pressure fault only clears once the level switch reads OK.
  - Blue LED indicates low level only. Red LED indicates boost cut only. Green LED indicates active spray output.

  Pulse width modulation implementation:
  - Uses esp_timer edge scheduling, not the hardware pulse width modulation peripheral.
  - The control loop updates period and on time; timer callbacks toggle solid state relay pins.

  Tasks:
  - Web server and domain name system run on core 0 for wireless network stack stability.
  - Control loop runs on core 1 for math, safety, and outputs.

  User interface:
  - Single page web interface with live status, curve editor, settings, and test tools.
***********************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <math.h>
// Forward declaration for the embedded web interface markup (defined in WebUI.ino).
extern const char INDEX_HTML[];
// ================================================================
// ======================= USER CONFIG (TOP) ======================
// ================================================================

// ---------------- PINS (EDIT HERE) ----------------
static const int PIN_INJ_SSR        = 16;  // Injector bank solid state relay pulse width modulation output: active high means injectors on.
static const int PIN_PUMP_SSR       = 26;  // Pump solid state relay output: active high means pump on.
static const int PIN_RED_LED        = 33;  // Red LED output: active high means boost cut.
static const int PIN_BLUE_LED       = 14;  // Blue LED output: active high means low level.
static const int PIN_GREEN_LED      = 25;  // Green LED output: active high means methanol is actively spraying.
static const int PIN_IAT_SSR        = 32;  // Timing cut relay solid state relay: active low grounds intake air temperature on fault for failsafe behavior.
static const int PIN_BOOST_SSR      = 23;  // Boost cut relay solid state relay: active high allows boost, low cuts boost.
static const int PIN_WIFI_LED       = 2;   // Onboard status light output: active high means station Wi-Fi connected.
static const int PIN_LEVEL_SWITCH   = 27;  // Tank low level switch input (default: high means low level).
static const int PIN_ADC_MAP        = 34;  // Manifold absolute pressure sensor analog input.
static const int PIN_ADC_RAIL       = 35;  // Rail pressure sensor analog input.

// ---------------- Wireless Access Point and Multicast Domain Name System ----------------
// Access point name and multicast domain name system host name are user editable in the user interface and saved in preferences.
static String wifiSsid = "watermeth"; // Open access point (no password).
static bool   wifiUsePassword = false;     // Keep open for testing.
static String wifiPass = "";               // Ignored when open.
// Firmware Wi-Fi defaults are used as first-boot values before preferences exist.
// After first save, user interface values persist and remain user-editable across boots.
static const uint8_t FW_WIFI_MODE = 1;
static const char*   FW_STA_SSID  = "Unit#29_Starlink";
static const char*   FW_STA_PASS  = "03Silverado";

static const int   AP_CH     = 1;  // Access point channel.
static const bool  AP_HIDDEN = false; // Access point visibility flag.
static const int   AP_MAX    = 4;  // Maximum clients allowed on the access point.
// Wireless mode options are listed below.
// 0 = access point only (default, phone connects directly to controller)
// 1 = station only (controller joins home wireless network)
// 2 = access point plus station (both)
static uint8_t wifi_mode = FW_WIFI_MODE;
static String  sta_ssid  = FW_STA_SSID; // Default station network name, overridden by saved preferences.
static String  sta_pass  = FW_STA_PASS; // Default station password, overridden by saved preferences.
// Network status line shown in the user interface status endpoint.
static String netLine = "";
// Captive portal domain name system server (matches Bumpbox behavior).
DNSServer dnsServer;
// ---------------- ANALOG TO DIGITAL CONVERSION AND SCALING ----------------
// Sensors are scaled by divider ratios, then linear and offset conversion is applied.
static const int   ADC_BITS = 12;
static const float ADC_VREF = 3.30f;
// Divider ratios (volts at analog to digital converter divided by volts at sensor).
// Example: five volt sensor into three point three volt analog to digital converter is 0.66.
static float map_div_ratio  = 0.66f;
static float rail_div_ratio = 0.66f;
// Analog to digital converter input calibration is separate from sensor manufacturer calibration.
// Gain and offset are applied to measured analog to digital converter pin voltage before divider math.
// Keeping this layer separate allows manufacturer sensor linear and offset values to remain unchanged.
// One shared correction pair is used for both MAP and rail inputs because bench data showed a common analog front-end bias.
// Defaults are neutral so newly flashed boards start from a portable baseline and only apply user trim when needed.
static float adc_input_gain = 1.0000f;
static float adc_input_offset_v = 0.000f;
// Shared multi-point analog to digital converter correction curve in pin-voltage space.
// Input breakpoints are fixed and represent corrected linear volts after shared gain and offset.
// Output values are user-tunable to flatten board-specific midrange bow while keeping shared correction for MAP and rail.
static const uint8_t ADC_CURVE_PTS = 8;
static const float adc_curve_in_v[ADC_CURVE_PTS] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.3f};
static float adc_curve_out_v[ADC_CURVE_PTS] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.3f};
// Linear and offset conversion (units = slope * volts + offset).
// Plain five-volt manifold absolute pressure scaling: Span = total kilopascal over zero to five volts, Offset = kilopascal at zero volts.
static float map_kpa_linear_hpt  = 312.5f;  // Default General Motors three bar (LSA and LS9).
static float map_kpa_offset_hpt  = -11.25f;
static float map_kpa_per_v       = map_kpa_linear_hpt / 5.0f;  // Derived slope used internally.

static float rail_psi_per_v  = 32.5f;  // Pounds per square inch per volt (zero to one hundred thirty pounds per square inch over zero point five to four point five volts).
static float rail_psi_offset = -16.25f; // Pounds per square inch offset (zero pounds per square inch at zero point five volts).

// ---------------- Flow Model Constants ----------------
// The flow model is for the user interface only and does not change spray output.
// The bank uses three General Motors flex fuel injectors as one combined injector.
static const uint8_t inj_count = 3;
static float   inj_lbhr_at_58psi = 36.0f;     // Pounds per hour per injector at fifty eight pounds per square inch differential (adjustable size only).
// User selectable mix density (methanol and water percentage) used for flow estimates.
static float   mix_meth_pct = 60.0f;          // Percent methanol by volume.
static float   mix_density_g_per_cc = 0.8744f; // Computed: methanol percent times 0.792 plus water percent times 0.998.
// Desired rail differential pressure (pounds per square inch) used for flow reference; zero uses live differential pressure.
static float   flow_ref_dp_psi = 60.0f;       // Desired rail differential pressure for the flow table (user selectable).

// Host name for multicast domain name system.
static String mdnsHost = "watermeth";
static bool mdnsRunning = false;
// ---------------- Injector Pulse Width Modulation ----------------
// Software pulse width modulation using esp_timer edge scheduling (no hardware pulse width modulation peripheral).
static float inj_pwm_hz = 50.0f;      // Adjustable.


// ---------------- Safety and State ----------------
// Differential pressure safety uses rail gauge pressure plus active barometric reference minus manifold absolute pressure. A fault latches timing cut.
static float min_dp_psi              = 50.0f; // Rail differential pressure fault threshold in pounds per square inch.
static uint32_t rail_fault_delay_ms  = 3000;
static uint32_t timing_cut_auto_clear_ms = 4000; // Timing cut auto clear delay in milliseconds. Zero disables auto clear.
static float dp_arm_duty_pct         = 5.0f; // Require duty above this before checking differential pressure.
static float dp_recover_margin_psi   = 1.5f; // Margin above minimum differential pressure to clear pending fault.
static float dp_critical_psi         = 25.0f; // Critical low differential pressure threshold for fast fault response.
static uint32_t dp_critical_ms       = 150;  // Time below the critical low differential pressure threshold before latching a fault.
static uint32_t dp_arm_settle_ms     = 500;  // Delay after arm conditions are met before differential pressure checks begin.
static uint32_t level_debounce_ms    = 300;   // Debounce time for the float switch.
static uint32_t pressure_ready_timeout_ms = 3000; // Maximum time allowed to reach target injector dP before latching a pressure-ready fault.

// Level switch wiring is fixed: pull-up input, grounded = level OK, open = low level.
static const bool level_active_high = true;  // A high logic level means low level.
static const bool boost_ssr_active_high = true; // Active high = boost allowed, low = boost cut.
static const bool iat_ssr_active_high = false;  // Active low: low grounds intake air temperature for timing cut, while healthy state stays high for failsafe behavior.
static const bool wifi_led_active_high = true;  // Active high turns onboard status light on when station Wi-Fi is connected.

// Forced latch states (bench testing). Not persisted; reset on reboot.
static bool force_boost_cut = false;
static bool force_timing_cut = false;
static bool force_dp_fault = false;
static bool force_pressure_ready_fault = false; // Bench latch that manually simulates a pressure-ready timeout fault condition.
static bool force_dp_boost_hold = false;
static bool force_pressure_ready_boost_hold = false; // Bench latch that manually simulates retained boost hold after a pressure-ready timeout fault.
static bool force_dp_monitor_override = false; // Bench override that disables automatic differential pressure monitoring while enabled.
static bool force_pressure_ready_override = false; // Bench override that bypasses pressure-ready injector blocking and timeout fault latching while enabled.
static bool force_level_fault_bypass = false; // Bench override that ignores low-level safety cut behavior while enabled.
static bool force_dp_fault_bypass = false; // Bench override that ignores injector differential pressure fault timing-cut behavior while enabled.
static bool force_dp_boost_hold_bypass = false; // Bench override that ignores retained differential pressure boost-hold behavior while enabled.
static bool force_pressure_ready_fault_bypass = false; // Bench override that ignores pressure-ready timeout timing-cut behavior while enabled.
static bool force_pressure_ready_boost_hold_bypass = false; // Bench override that ignores retained pressure-ready timeout boost-hold behavior while enabled.
// Pump behavior.
static bool pump_in_spray_mode = true;
// ---------------- Spray Curve (10 points) ----------------
// Curve defines commanded spray duty versus manifold absolute pressure; linear interpolation between points.
static const int CURVE_PTS = 10;
// Default curve: onset around six pounds per square inch, ramping to around eighteen to twenty pounds per square inch maximum (evenly spaced kilopascal).
// Kilopascal values: 145, 156, 167, 178, 189, 200, 211, 222, 233, 245.
static float curve_kpa[CURVE_PTS]  = {145, 156, 167, 178, 189, 200, 211, 222, 233, 245};
static float curve_duty[CURVE_PTS] = {  5,  10,  18,  28,  40,  55,  70,  85,  95, 100};
// Duty clamp.
// Hard ceiling on commanded spray duty.
static float duty_max = 60.0f;
// Serial output behavior.
static bool serial_debug_enable = true;
static uint32_t serial_status_period_ms = 250;
static uint32_t serial_print_heartbeat_ms = 1000; // Heartbeat interval.
// Last non-clear safety or fault reason shown in the Status card as fault history.
// This is informational history only and does not indicate an active latch by itself.
static String fault_history_text = "None";

// ================================================================
// ======================= INTERNAL STATE ==========================
// ================================================================
//
// Everything below is runtime state.
// These values change as the controller runs and are used by the user interface.

// Web server and preference storage.
WebServer server(80);
Preferences prefs;
// Operating mode: normal operation or timing cut active.
enum Mode : uint8_t { MODE_NORMAL = 0, MODE_IAT_CUT = 1 };
volatile Mode modeNow = MODE_NORMAL;
// Differential pressure monitor state for live status and diagnostics.
enum DpMonitorState : uint8_t {
  DP_STATE_IDLE = 0,
  DP_STATE_ARMED = 1,
  DP_STATE_PENDING = 2,
  DP_STATE_TIMING_CUT = 3,
  DP_STATE_BOOST_HOLD = 4,
  DP_STATE_OVERRIDE = 5
};
// Rail differential-pressure real-fault path classification.
// This keeps normal low-pressure and critical low-pressure latches distinguishable in status reporting.
enum RailDpFaultPath : uint8_t {
  RAIL_DP_PATH_NONE = 0,
  RAIL_DP_PATH_NORMAL_LOW = 1,
  RAIL_DP_PATH_CRITICAL_LOW = 2
};

// esp_timer edge schedulers (no hardware timers, no hardware pulse width modulation peripheral).
// This keeps the wireless network and the web server responsive while generating pulse width modulation.
static esp_timer_handle_t injTimer  = nullptr;
// Pulse width modulation timing for injectors (volatile).
// Updated in control loop; used by timer callback to toggle injector solid state relay.
volatile uint32_t injPeriodUs = 20000;
volatile uint32_t injOnUs     = 0;
volatile bool     injHigh     = false;
// Live values shared between control loop and status or user interface.
volatile float mapKpa    = 0.0f;
volatile float railPsi   = 0.0f;
volatile float baroKpa   = 101.325f; // Boot-captured barometric reference in kilopascal absolute.
volatile float baroPsi   = 14.6959f; // Boot-captured barometric reference in pounds per square inch absolute.
volatile float cmdDuty   = 0.0f;
volatile float curvePctLive = 0.0f; // Raw curve duty (zero to one hundred) before clamp and test overrides.
volatile float mapKpaRaw = 0.0f;    // Raw manifold absolute pressure before test overrides.
volatile float mapKpaFilt = 0.0f;   // Filtered manifold absolute pressure for duty smoothing.
volatile float railPsiFilt = 0.0f;  // Filtered rail pressure for smoother differential pressure.
volatile bool pumpOn      = false;
volatile bool redLedOn    = false;
// Live flags updated in the control loop.
volatile bool blueLedOn   = false;     // Blue LED indicates low level.
volatile bool greenLedOn  = false;     // Green LED indicates methanol is actively spraying.
volatile bool levelLowState = false;   // Debounced level switch state.
volatile bool boostOn     = true;      // Boost solenoid allowed.
volatile bool timingCutOn = false;    // Timing cut active (differential pressure fault or low level while spraying).
volatile bool iatGroundOn  = false;    // Intake air temperature ground solid state relay state.
static uint32_t levelLastChangeMs = 0;
static bool levelRawLast = false;
static bool pressureReady = false;
static uint32_t pressureReadySinceMs = 0;
static bool levelLowSprayTimingLatched = false; // Holds low-level timing-cut intent once low level occurs during active spray, and clears only when level returns OK.
volatile bool faultLatched = false;    // Timing cut latch (rail differential pressure fault) with optional auto clear.
volatile uint32_t faultLatchedAtMs = 0;  // Timestamp for timing cut auto clear tracking.
volatile bool dpBoostCutHold = false;    // Boost cut latch set by real differential pressure faults and held until power cycle.
volatile bool pressureReadyFaultLatched = false; // Timing cut latch set by a real pressure-ready timeout fault and released by shared timing-cut auto-clear when enabled.
volatile bool pressureReadyBoostCutHold = false; // Boost cut latch set by real pressure-ready timeout faults and held until power cycle.
volatile uint8_t dpStateNow = DP_STATE_IDLE; // Live differential pressure monitor state.
volatile uint8_t railDpLastRealFaultPath = RAIL_DP_PATH_NONE; // Last real rail differential-pressure latch path (normal low or critical low).
volatile uint32_t dpLowForMsNow = 0;         // Time spent below the normal differential pressure threshold while armed.
volatile float dpMinSeenPsiNow = -1.0f;      // Lowest observed differential pressure while armed.
volatile bool dpArmedNow = false;            // Differential pressure monitor arm status after settle delay.
volatile uint32_t dpArmSettleRemainingMsNow = 0; // Remaining arm settle delay before differential pressure checks start.
uint32_t railLowSince = 0;             // Timer for low differential pressure detection.
uint32_t railCriticalSince = 0;        // Timer for critical low differential pressure detection.
uint32_t dpArmSinceMs = 0;             // Timer for differential pressure arm settle delay.

// Forward declaration for multicast domain name system host sanitization.
void sanitizeMdnsHost();
// Forward declaration for boot-time barometric reference capture.
static void captureBaroReferenceAtBoot();

// Capture barometric pressure from the MAP signal once at boot.
// This runs early so differential pressure math has a stable ambient reference for the whole run.
static void captureBaroReferenceAtBoot(){
  const uint8_t sampleCount = 20; // Average several samples for a stable boot reference.
  float kpaSum = 0.0f;
  uint8_t validCount = 0;
  for(uint8_t i = 0; i < sampleCount; ++i){
    const float mapV_adc_raw = adcVolts(PIN_ADC_MAP); // Read calibrated MAP pin voltage at the analog input pin.
    const float mapV_adc_linear = (mapV_adc_raw * adc_input_gain) + adc_input_offset_v; // Apply shared analog gain and offset trim.
    const float mapV_adc = applyAdcCurveCorrection(mapV_adc_linear); // Apply shared analog correction curve after linear trim.
    const float mapV_sens = (map_div_ratio > 0.01f) ? (mapV_adc / map_div_ratio) : 0.0f; // Convert controller pin volts to sensor-side volts.
    const float mapKpaNow = (map_kpa_per_v * mapV_sens) + map_kpa_offset_hpt; // Convert volts to manifold absolute pressure.
    if(isfinite(mapKpaNow) && mapKpaNow > 60.0f && mapKpaNow < 130.0f){
      kpaSum += mapKpaNow;
      validCount++;
    }
    delay(2);
  }
  float capturedKpa = (validCount > 0) ? (kpaSum / (float)validCount) : 101.325f; // Use standard atmosphere when no valid sample is available.
  capturedKpa = clampf(capturedKpa, 70.0f, 115.0f); // Keep captured barometric reference in a realistic ambient range.
  baroKpa = capturedKpa;
  baroPsi = baroKpa * 0.1450377377f;
  if(serial_debug_enable){
    Serial.printf("[BARO] Boot capture: %.2f kPa abs (%.2f psia)\n", (double)baroKpa, (double)baroPsi);
  }
}

// Start multicast domain name system once Wi-Fi is up.
void startMdnsOnce(){
  if(mdnsRunning) return;
  // Ensure the host name is valid before starting multicast domain name system.
  sanitizeMdnsHost();
  const char* host = mdnsHost.c_str();
  if(MDNS.begin(host)){
    MDNS.addService("http", "tcp", 80);
    mdnsRunning = true;
    Serial.printf("[MDNS] Started: %s.local\n", host);
  }else{
    Serial.println("[MDNS] Start failed.");
  }
}
// Clean up the multicast domain name system host name.
void sanitizeMdnsHost(){
  mdnsHost.trim();
  if(mdnsHost.length() == 0) mdnsHost = "watermeth";
  if(mdnsHost.endsWith(".local")) mdnsHost = mdnsHost.substring(0, mdnsHost.length() - 6);
  mdnsHost.toLowerCase();
  String clean;
  for(size_t i=0;i<mdnsHost.length();i++){
    char c = mdnsHost[i];
    if((c>='a' && c<='z') || (c>='0' && c<='9') || c=='-'){
      clean += c;
    }else if(c=='_' || c==' '){
      clean += '-';
    }
  }
  if(clean.length() == 0) clean = "watermeth";
  mdnsHost = clean;
}


// Test overrides (user interface force modes).
// These override sensors and outputs for troubleshooting only.
// Output force commands are absolute by design for bench testing.
static uint8_t force_pump_mode   = 0; // 0 normal, 1 force off, 2 force on.
static uint8_t force_inj_mode    = 0; // 0 normal, 1 force off, 2 force on.
static float   force_duty        = 0.0f;
static uint8_t force_level_mode  = 0; // 0 normal, 1 force low, 2 force high.
static float   force_map_kpa     = -1.0f; // Values below zero use sensor, otherwise overrides.
static float   force_rail_psi    = -1.0f; // Values below zero use sensor, otherwise overrides.
static float   force_dp_psi      = -1.0f; // Values below zero use calculated differential pressure, otherwise overrides.
static float   force_baro_kpa    = -1.0f; // Values below zero use boot-captured barometric reference, otherwise overrides.

// ================================================================
// ============================= SETUP ============================
// ================================================================

void setup(){
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("=== WaterMethanol Controller V4.7 BOOT ===");
  Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());
  Serial.printf("CPU: %lu MHz | Heap: %lu\n", (unsigned long)ESP.getCpuFreqMHz(), (unsigned long)ESP.getFreeHeap());
  // Configure general purpose input and output directions and set safe defaults.
  // This prevents pumps, injectors, and relays from turning on at boot.
  pinMode(PIN_INJ_SSR, OUTPUT);
  pinMode(PIN_PUMP_SSR, OUTPUT);
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_BLUE_LED, OUTPUT);
  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_IAT_SSR, OUTPUT);
  pinMode(PIN_BOOST_SSR, OUTPUT);
  pinMode(PIN_WIFI_LED, OUTPUT);
  pinMode(PIN_LEVEL_SWITCH, INPUT_PULLUP);
  gpioFast(PIN_INJ_SSR, false);
  gpioFast(PIN_PUMP_SSR, false);
  gpioFast(PIN_RED_LED, false);
  gpioFast(PIN_BLUE_LED, false);
  gpioFast(PIN_GREEN_LED, false);
  gpioFast(PIN_IAT_SSR, iat_ssr_active_high ? false : true);
  gpioFast(PIN_BOOST_SSR, boost_ssr_active_high ? true : false);
  gpioFast(PIN_WIFI_LED, wifi_led_active_high ? false : true); // Keep onboard status light off until station Wi-Fi is connected.
  // Analog to digital converter configuration for the manifold absolute pressure and rail pressure sensors.
  analogReadResolution(ADC_BITS);
  analogSetPinAttenuation(PIN_ADC_MAP, ADC_11db);
  analogSetPinAttenuation(PIN_ADC_RAIL, ADC_11db);
  // Enable pulldowns on the ADC pins to prevent floating inputs when a sensor is unplugged.
  // These pins are input-only, so a weak pulldown is safe and avoids stale readings.
  rtc_gpio_pulldown_en((gpio_num_t)PIN_ADC_MAP);
  rtc_gpio_pullup_dis((gpio_num_t)PIN_ADC_MAP);
  rtc_gpio_pulldown_en((gpio_num_t)PIN_ADC_RAIL);
  rtc_gpio_pullup_dis((gpio_num_t)PIN_ADC_RAIL);

  // Load persisted settings.
  loadPrefs();
  // Capture barometric reference immediately after preferences load and before network startup.
  // This gives differential pressure math a consistent ambient reference from the start of runtime.
  captureBaroReferenceAtBoot();
  sanitizeMdnsHost();
  // Wi-Fi mode, station credentials, and multicast domain name system host now come from saved preferences.
  // Firmware constants above are used only as first-boot defaults.
  // Serial debug default remains enabled from firmware defaults unless a saved preference overrides it.
  if(mdnsHost.length() == 0) mdnsHost = "watermeth";
  // Apply output idle states again after loading preferences.
  // This ensures any polarity settings take effect before the control loop starts.
  gpioFast(PIN_INJ_SSR, false);
  gpioFast(PIN_PUMP_SSR, false);
  gpioFast(PIN_IAT_SSR, iat_ssr_active_high ? false : true);
  gpioFast(PIN_BOOST_SSR, boost_ssr_active_high ? true : false);
  gpioFast(PIN_RED_LED, false);
  gpioFast(PIN_BLUE_LED, false);
  gpioFast(PIN_GREEN_LED, false);
  gpioFast(PIN_WIFI_LED, wifi_led_active_high ? false : true); // Re-assert off state before tasks start.
  // Ensure the pump SSR output is held low at boot.
  gpio_pulldown_en((gpio_num_t)PIN_PUMP_SSR);
  gpio_pullup_dis((gpio_num_t)PIN_PUMP_SSR);
  // Hold LED outputs low at boot.
  gpio_pulldown_en((gpio_num_t)PIN_RED_LED);
  gpio_pullup_dis((gpio_num_t)PIN_RED_LED);
  gpio_pulldown_en((gpio_num_t)PIN_BLUE_LED);
  gpio_pullup_dis((gpio_num_t)PIN_BLUE_LED);
  gpio_pulldown_en((gpio_num_t)PIN_GREEN_LED);
  gpio_pullup_dis((gpio_num_t)PIN_GREEN_LED);

  // ========================
  // Wireless network and multicast domain name system (always on).
  // ========================
  WiFi.persistent(false);
  WiFi.disconnect(false);
  delay(50);
  WiFi.setSleep(false);
  // Build helper for access point startup so AP-only and AP+STA modes share identical initialization.
  auto startAP = [&](){
    Serial.printf("[WIFI] Starting access point name=%s\n", wifiSsid.c_str());
    if(wifiUsePassword && strlen(wifiPass.c_str()) >= 8){
      WiFi.softAP(wifiSsid.c_str(), wifiPass.c_str(), AP_CH, AP_HIDDEN, AP_MAX);
  }else{
    WiFi.softAP(wifiSsid.c_str(), nullptr, AP_CH, AP_HIDDEN, AP_MAX);
  }
  delay(50);
  Serial.printf("[WIFI] Access point IP address: %s\n", WiFi.softAPIP().toString().c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());
  };
  // Build helper for station startup so STA-only and AP+STA modes share identical initialization.
  auto startSTA = [&](){
    if(sta_ssid.length() == 0){
      Serial.println("[WIFI] Station mode requested but network name is blank (skipping).");
      return;
    }
  sanitizeMdnsHost();
  WiFi.setHostname(mdnsHost.c_str());
  Serial.printf("[WIFI] Starting station mode network name=%s\n", sta_ssid.c_str());
  WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
  uint32_t t0 = millis();
  while(!WiFi.isConnected() && (millis()-t0) < 8000){
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  if(WiFi.isConnected()){
    const IPAddress ip = WiFi.localIP();
    Serial.printf("[WIFI] Station IP address: %s\n", ip.toString().c_str());
    if(ip[0] != 0){
      startMdnsOnce();
    }
  }else{
    Serial.println("[WIFI] Station connect timeout (will keep trying in background).");
  }
  };
  if(wifi_mode == 0){
    WiFi.mode(WIFI_AP);
    WiFi.setAutoReconnect(true);
    startAP();
  }else if(wifi_mode == 1){
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    startSTA();
  }else{
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    startAP();
    startSTA();
  }
  // Register HTTP routes for the user interface, API, and captive portal probes.
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  // Captive probe endpoints for Android, iOS, and Windows are registered below.
  server.on("/generate_204", handleCaptiveOk);
  server.on("/gen_204", handleCaptiveOk);
  server.on("/ncsi.txt", handleCaptiveOk);
  server.on("/connecttest.txt", handleCaptiveOk);
  server.on("/hotspot-detect.html", handleCaptiveOk);
  server.on("/library/test/success.html", handleCaptiveOk);
  server.on("/success.txt", handleCaptiveOk);
  server.on("/fwlink", handleCaptive);
  server.onNotFound(handleCaptive);
  server.begin();
  // mDNS is started when Wi-Fi reports a valid STA connection.
  wsBegin();
  Serial.println("[HTTP] Server started.");
  // Start pulse width modulation edge schedulers.
  pwmInit();
  Serial.println("[PWM] esp_timer edge scheduler started (injectors).");
  // Start tasks.
  // Note: The wireless network stack lives primarily on core 0 in the ESP32 software development framework, so we pin web and domain name system tasks to core 0.
  // Control and math run on core 1 to keep the web server responsive.
  BaseType_t okWeb = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 2, nullptr, 0);
  BaseType_t okCtl = xTaskCreatePinnedToCore(controlTask, "ctrl", 8192, nullptr, 1, nullptr, 1);
  Serial.printf("[TASK] web(core0)=%s ctrl(core1)=%s\n", okWeb==pdPASS?"OK":"FAIL", okCtl==pdPASS?"OK":"FAIL");
}
void loop(){
  // Nothing here on purpose. The web stack is on the core 0 task (proven working pattern).
  vTaskDelay(pdMS_TO_TICKS(1000));
}
