# WaterMethanol Controller V4.7 - User Guide

Active sketch: `WaterMethanolController/WaterMethanolController.ino`
Locked behavior checklist: `WaterMethanolController/LOCKED_BEHAVIOR_CHECKLIST.md`
Calibration reference: `WaterMethanolController/CALIBRATION_REFERENCE.md`
Test results master: `WaterMethanolController/TEST_RESULTS_MASTER.md`
mDNS failure modes: `WaterMethanolController/MDNS_FAILURE_MODES.md`
mDNS lessons learned: `WaterMethanolController/MDNS_LESSONS_LEARNED.md`

This guide is written for a first-time user. It covers wiring, setup, and validation from zero to working.

## Safety First
- This system can cut boost and timing. Test safely.
- Always bench test before vehicle test.
- Use proper fusing, relays/SSRs, and wiring gauge for pump and solenoids.
- Share grounds between ESP32, sensors, and SSRs.

## What This Controller Does
- Reads MAP and rail pressure and computes injector dP using active baro compensation (`rail psig + baro psia - MAP psia`).
- Active baro reference is boot-captured by default and can be temporarily overridden in Test Section with `Force Baro (kPa abs)`.
- Uses one rail pressure sensor in the rail, and the three meth injectors are fed from that same rail.
- Uses a 10-point MAP kPa -> duty curve to command injector PWM.
- Runs pump and only enables spray after injector dP reaches the target injector dP.
- If injector dP cannot reach the target injector dP before the pressure-ready timeout, a real pressure-ready timeout fault latches.
- Enforces safeties: low level, rail dP fault, timing cut, boost cut.
- Low level before active spray causes boost cut only; low level during active spray causes timing cut and boost cut.
- Injector dP monitoring has a dP delay and both normal and critical low-pressure fast fault paths.
- Provides a web UI with live data and settings.
- Boost control is not part of this controller; boost cut is safety-only.

## Firmware Tabs (For Learning)
- `WaterMethanolController/WaterMethanolController.ino`: global settings, pins, setup, and shared runtime state.
- `WaterMethanolController/ControlLoop.ino`: main control task that reads sensors, applies safety, and updates outputs.
- `WaterMethanolController/DpMonitor.ino`: dedicated injector differential pressure safety monitor with highly descriptive comments, including dP delay behavior, pending state, normal fault path, critical low-pressure fast fault path, timing-cut auto-clear, and retained boost hold behavior.
- `WaterMethanolController/Api.ino`: status and configuration endpoints used by the web UI and bench test controls.

## Hardware You Need
- ESP32 (ESP32-WROOM-DA module target).
- Pump SSR or relay.
- Injector bank SSR (PWM capable).
- IAT ground SSR (timing cut).
- Boost solenoid SSR (boost cut).
- Onboard status light on GPIO2 (active controller connectivity indicator).
- MAP sensor (5V or 3.3V).
- Rail pressure sensor (5V or 3.3V).
- Level switch (float).
- 12V to 5V buck converter.
- Proper fuses, wiring, and connectors.

## Wiring (ESP32 GPIO Map)
Pins are defined near the top of `WaterMethanolController/WaterMethanolController.ino`.

| Signal | GPIO | Direction | Notes |
| --- | ---: | --- | --- |
| Injector Bank SSR PWM | 16 | Output | Software-timed (esp_timer), no LEDC. |
| Pump SSR | 26 | Output | Runs with spray request and pressure build unless forced. |
| Red LED | 33 | Output | ON during boost cut only. |
| Blue LED | 14 | Output | ON when tank low. |
| Green LED | 25 | Output | ON when methanol is actively spraying. |
| IAT Ground SSR | 32 | Output | Fixed failsafe polarity: HIGH = healthy, LOW grounds IAT during timing cut. |
| Boost Solenoid SSR | 23 | Output | Fixed: HIGH = boost allowed, LOW = boost cut. |
| Onboard Wi-Fi LED | 2 | Output | ON when STA is connected with a valid IP or when at least one AP client is connected to the controller. |
| Level Switch | 27 | Input (pull-up) | Fixed: grounded = level OK, open = low level. |
| MAP ADC | 34 | ADC input | Use divider if sensor is 5V. |
| Rail Pressure ADC | 35 | ADC input | Use divider if sensor is 5V. |

Wiring notes:
- Use a proper 12V to 5V buck for the ESP32.
- Share grounds between ESP32, sensors, and all SSRs.
- If sensors are 5V, use a divider to protect the ESP32 ADC (default ratio 0.66).
- Pump SSR output is held low with a pulldown at boot.
- Red/Blue/Green LED outputs are held low with pulldowns at boot.
- MAP and rail analog inputs use ADC1 pins.

## Flash the Firmware
1. Compile and upload the sketch `WaterMethanolController/WaterMethanolController.ino`.
2. Target board: `esp32:esp32:esp32da`.
3. On first boot after each newly flashed firmware build, the controller loads firmware default settings once, then persists user changes across normal reboots.

## Connect to the Web UI
1. Power the ESP32.
2. Connect to AP SSID `watermeth` (open by default).
3. Open `http://192.168.4.1/`.
4. Top right shows network and WebSocket status. `WS: ON` means live data is streaming, `WS: STALE` means no recent socket data, and `WS: OFF` means disconnected.
Wi-Fi works in both AP and STA modes.
If you are on STA Wi-Fi and your configured `.local` host name is not resolving, open the device by its STA IP address instead.
Use explicit protocol when opening host names: `http://<mdnsHost>.local/`.
If the browser auto-upgrades to `https://<mdnsHost>.local/`, it will fail because the controller serves HTTP on port 80 only.
The multicast domain name system host name is sanitized on boot (trimmed, `.local` removed, and spaces replaced) to avoid resolution failures.
Wi-Fi mode, STA SSID/password, AP SSID, and multicast domain name system host are user editable in the UI and persist after save on normal reboots.
mDNS starts after STA is connected and reports a non-zero IP address. STA reconnect retry uses `WiFi.reconnect()` first and falls back to explicit `WiFi.begin(...)` only when needed, while keeping the selected Wi-Fi mode unchanged. STA auto-reconnect is enabled for stable recovery after link drops. Wi-Fi sleep is disabled for STA stability. The web task heartbeat reports whether mDNS is running. The captive DNS server only runs when AP is enabled so STA DNS lookups are not intercepted.
The mDNS responder is health-checked in runtime, and if the responder state is invalid it is restarted with the saved host name. On responder start, STA link-up, and STA IP change, the controller sends an explicit IPv4 mDNS announcement. While connected, it also refreshes that announcement on a periodic interval to improve first-hit `.local` resolution consistency.
Live gauges update from WebSocket messages when connected; polling only runs when WebSocket data is stale.
MAP and rail pressure are filtered in firmware for smooth control behavior; the UI shows live values.
MAP and rail input voltage conversion uses averaged calibrated millivolt ADC reads (`analogReadMilliVolts(...)`) for better cross-board consistency; no fixed board-specific lookup table is applied. Full calibration formulas and conversion details are maintained in `CALIBRATION_REFERENCE.md`.
The UI uses safe setters so missing elements do not break live updates.

## UI Layout (Top to Bottom)
1. Setup Quickstart
2. Status
3. Methanol Controller Live Data
4. Methanol Spray Curve
5. Methanol Controller Settings
6. Sensor Calibration
7. Wi-Fi Settings
8. Test Section

Cards are collapsed by default. Click the card title strip only to expand or collapse a card. Cards should always open; if they do not, check for escaped newlines in the `toggleCard` script.

## First-Time Setup (Step by Step)
### 1) Sensor Calibration
Open **Sensor Calibration**.
- The card is organized into three sections in this required order: Analog Input Calibration, Preset Selection, then Sensor Equation Values.
- Complete Analog Input Calibration first before fitting presets and sensor equation values.
- The card now includes a live calibration gauge pair with two decimal places for MAP (kPa abs) and rail pressure (psig), using the same live status signals as the Live Data card.
- Calibration gauge readability update: the two-decimal MAP and rail gauges in Sensor Calibration use a short display-only smoothing layer to reduce flicker while preserving live tracking.
- This smoothing applies only to the Sensor Calibration two-decimal gauges and does not change control-loop math, safety logic, or the main Live Data gauges.
- MAP Preset: choose your sensor or enter Span/Offset.
  - Span = total kPa over 0-5V
  - Offset = kPa at 0V
- Rail Pressure Sensor Preset: choose a known rail pressure sensor or enter slope/offset directly.
- Default rail preset is GM 0-130 psig oil/rail (32.5 / -16.25).
- Legacy two-point rail quick-calc controls (`P1/V1`, `P2/V2`) were removed; use direct `pLin/pOff` entry or the rail point-entry fit table.
- Keep manufacturer sensor linear/offset values in place for your sensor family.
- MAP point-entry table is provided in Sensor Equation Values:
  - The table heading line is intentionally larger for quick scan during bench calibration.
  - Enter sensor-side voltage (`0.5-4.5 V`) and MAP (`kPa`) points.
  - The third table column shows expected MAP (`kPa`) from current `mapLin/mapOff` for each entered voltage.
  - Click `Fit MAP Span/Offset` to update `mapLin` and `mapOff` from entered points.
- Rail point-entry table is provided in Sensor Equation Values:
  - The table heading line is intentionally larger for quick scan during bench calibration.
  - Enter sensor-side voltage (`0.5-4.5 V`) and rail pressure (`psig`) points.
  - The third table column shows expected rail pressure (`psig`) from current `pLin/pOff` for each entered voltage.
  - Click `Fit Rail Slope/Offset` to update `pLin` and `pOff` from entered points.
- Sensor quick check is split into two tables in Sensor Equation Values:
  - `MAP Quick Check`: enter measured MAP (`kPa`) at each check voltage.
  - `Rail Quick Check`: enter measured rail (`psig`) at each check voltage.
  - Each table computes expected values and `% error` for quick validation.
  - If a measured cell is left blank, `% error` stays `-` and the rest of the UI continues running normally.
- Table voltage measurement note:
  - Measure at the divider input (sensor side). Do not apply more than `3.3 V` at the controller ADC input.
  - With default divider `0.66`, `0.5-4.5 V` at the divider input corresponds to `0.33-2.97 V` at the controller input.
- In the Analog Input Calibration section, use the shared ADC trim sliders for board-level correction:
  - Shared ADC Gain Trim (MAP and rail)
  - Shared ADC Offset Trim (V) (MAP and rail)
  - `Shared ADC Curve Table` heading uses the same larger table-title style for readability.
  - Shared ADC Curve (multi-point) table with fixed breakpoints (`0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3 V`) and per-row MAP/rail capture inputs.
  - Use **Fit Shared ADC Curve** after entering held readings at breakpoints to flatten midrange bow that gain/offset alone cannot remove.
  - Use **Reset ADC Curve to Identity** to return curve outputs to one-to-one mapping.
  - Both sliders include visible tick marks and numeric scale labels for repeatable tuning.
  - Slider helper text now includes a quick end-user method: apply `0.0 V` and `3.3 V` at the sensor input, then adjust shared gain/offset until low and high readings match expected endpoints.
  - Slider span for cross-board correction:
    - Gain: `0.8000` to `1.2000`
    - Offset: `-0.500 V` to `+0.500 V`
  - Startup defaults are gain `1.0000` and offset `0.000 V`.
  - Shared ADC curve startup defaults are identity:
    - `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`
  - Matching MAP and rail correction remains intentional because both inputs use one shared board-level trim pair.
  - Slider control supports mouse click/drag and keyboard keys (`Arrow`, `Page Up/Down`, `Home`, `End`).
  - Endpoint calibration note: shared ADC trim now keeps endpoint behavior linear after correction, so `0.0 V` and `3.3 V` can be used as valid anchors.
  - Shared ADC curve endpoints are locked to `0.0 V` and `3.3 V`, so endpoint anchors remain valid while curve midpoints are adjusted.
- Divider ratios:
  - Default is `0.66` for a 5V sensor into 3.3V ADC.
  - Use `1.00` for 3.3V sensors.
- MAP and rail calibration values are sanity-checked on boot and after saves; invalid values are restored to safe defaults.
- MAP and rail live inputs use lightweight smoothing for stable gauges and stable duty command behavior while still tracking normal input changes.
- The firmware sanity-checks MAP linear/offset and divider ratio at boot and after saves. If invalid values are stored (0, NaN, or out of range), it restores safe defaults (3-bar GM: 312.5 / -11.25 and 0.66 divider).
- Bench validation note: on replacement COM5 hardware, a locked 3-point rail pressure sensor check measured `-16.25 psi` at `0.0 V`, `57.0 to 57.4 psi` at `1.5 V`, and `140.56 psi` at `3.3 V`. Verify your own board with a 3-point check before final vehicle testing.
- Active bench-session note (COM5): board-level correction now uses ADC trim sliders so manufacturer sensor constants can stay intact.
- If a board needs correction, tune ADC gain/offset first.
- Keep MAP in plain 5V span/offset form (`mapLin` kPa over 0-5V, `mapOff` kPa at 0V).
- Keep rail pressure sensor in plain slope/offset form (`pLin` in psig/V and `pOff` in psig) from measured sensor data.
- Bench validation note: on replacement COM5 hardware, a locked 3-point MAP check measured `-11.25 kPa` at `0.0 V`, `194.7 to 196.2 kPa` at `2.2 V`, and `290.31 kPa` at `3.3 V`.
- For full historical and active calibration evidence coverage (8-point sweeps, 3-point locks, and active shared-trim expected curves), see `TEST_RESULTS_MASTER.md` Section 7 and `CALIBRATION_REFERENCE.md` Section 4.

### 2) Verify Live Data
Open **Methanol Controller Live Data** and **Status**.
- Status tiles and Live Data gauges use matching category color accents to make safety and pressure-monitor states easier to spot at a glance.
- Status `Fault History` shows the last active safety or fault reason and persists as informational history across reboot.
- Status `Current Fault Status` shows the active fault reason right now, or `None` when no active fault is present.
- Status now shows separate rail dP and pressure-ready fault tiles, and all fault text uses one shared `Current Fault Status` source across Status and Live Data.
- Status `Rail dP State Detail` identifies rail dP path/state with explicit labels such as `PENDING (LOW)`, `FAULT (NORMAL LOW)`, `FAULT (CRITICAL LOW)`, `BOOST HOLD (NORMAL LOW)`, and `BOOST HOLD (CRITICAL LOW)`.
- Live Data `Fault Status` continues to show the current active reason.
- Live Data gauge layout uses a 4-wide baseline and fills row width automatically when a row has fewer gauges.
- Live Data shows MAP in both `kPa abs` and `psia` gauges.
- Live Data shows active barometric reference in both `Baro (kPa abs)` and `Baro (psia)` gauges (boot capture unless `Force Baro` is set in Test Section).
- Live Data shows `Boost (psi)` computed as `MAP psia - Baro psia`.
- MAP should look reasonable for your engine and altitude at idle.
- Rail pressure is shown as `psig` in Live Data.
- Rail psig should be near 0 when pump is off.
- Injector dP should follow `rail psig + baro psia - MAP psia` (equivalent to `rail psig - MAP psig`).
- Live Data `Fault Status` is rendered as a full-width bottom gauge tile for clear active-fault visibility.

### 3) Set the Methanol Spray Curve
Open **Methanol Spray Curve**.
- The card is organized into three themed sections: Curve Axis and Ceiling, Curve Preview and Table, and Live Curve Summary.
- Set Curve Start and Curve Max to your desired MAP window.
- Use **Spread X** to auto-space points.
- Drag points to shape the duty curve.
- Spray command is forced to 0% below Curve Start kPa.
- Duty Clamp limits maximum duty.
- Flow and GPH estimates use **Target Injector dP** when it is set; otherwise they use live injector dP.

### 4) Configure Methanol Controller Settings
Open **Methanol Controller Settings**.
For first startup, use **Apply Default Settings** to load a conservative baseline, then fine tune and press **Save**.
Default settings are:
- dP min `50 psi`, dP fault delay `3000 ms`, dP critical `25 psi` for `150 ms`
- dP arm `5%`, dP delay `500 ms`, dP recover `1.5 psi`
- Timing cut auto-clear `4000 ms`
- Injector `36 lb/hr @58 psi`, injector PWM `50 Hz`, meth mix `60%`
- Target injector dP `60 psi`, pressure-ready timeout `3000 ms`, level debounce `300 ms`, MAP/Rail divider `0.66`
- GM 3-bar MAP calibration `312.5 / -11.25`, GM 0-130 psi rail calibration `32.5 / -16.25`
Use the on-card tune order and sections to reduce setup mistakes:
- **Step 1 - Safety - Fault Thresholds**
  - Pressure Fault Threshold and Pressure Fault Delay define the normal dP fault path.
  - Critical dP Threshold and Critical dP Delay define the critical low-pressure fast fault path.
  - Timing Cut Auto-Clear sets timing cut release delay (set to 0 to disable auto clear).
- **Step 2 - Safety - dP Monitoring Behavior**
  - dP Arm Duty and dP Delay control when monitoring starts.
  - dP Recover Margin controls when a pending low-pressure timer is allowed to clear.
  - Target Injector dP / Spray Enable Target is used as both the pressure-ready spray gate and the flow reference pressure (default 60 psi).
  - Pressure-Ready Timeout sets how long pressure build is allowed before a real pressure-ready timeout fault is latched.
- **Step 3 - Flow and Spray Enable**
  - Injector Size and Injector PWM drive flow model and output behavior.
  - Spray is blocked until live injector dP reaches the target injector dP from Step 2.
  - Methanol Mix is used for flow calculations.
- **Step 4 - Signal Conditioning**
  - Level Debounce filters tank switch transitions (default 300 ms).
  - MAP Divider Ratio and Rail Divider Ratio match your voltage divider harness.
- **Step 5 - Serial Diagnostics**
  - Serial Debug turns runtime logging on or off.
  - Serial Status Period controls how often status lines print.

### 5) Configure Wi-Fi (Optional)
Open **Wi-Fi Settings**.
- Use the three themed sections in order: Wi-Fi Mode, AP and mDNS Identity, then Station Credentials.
- Set AP SSID and mDNS host.
- Set STA SSID/pass if you want station mode.
- Save.

### 6) Save
Press **Save** in each card as you finish. Save applies all settings on the page and settings persist across normal reboots unless changed in the UI or firmware. Each newly flashed firmware build loads firmware defaults once on first boot, then uses saved values afterward. Serial Debug is enabled by default and forced ON at boot for bench visibility.
Each card includes a Defaults line at the top showing the shipping values. Live Data and Status cards note that they show live values (no defaults).

## Test Section (Bench Testing)
Open **Test Section**.
The card is organized into three themed sections: Sensor Overrides, Output Overrides, and Manual Fault Controls and Overrides.
Section 1 and Section 2 fields are intentionally aligned so force-value entries are easier to scan during bench testing.
You can force values to validate behavior without wiring:
- Force Level Switch
- Force Pump
- Force Injectors + Forced Duty
- Force MAP, Rail, Baro, and Injector dP
For the full step-by-step bench procedure with expected outcomes and locked behavior tracking, see `LOCKED_BEHAVIOR_CHECKLIST.md`.
For consolidated historical results from assistant self-tests, user bench tests, and host validation runs, see `TEST_RESULTS_MASTER.md`.

Important:
- In Test Section, output force commands are absolute bench overrides.
- Force Pump (`Force ON` or `Force OFF`) always applies while selected, even during timing cut, boost cut, low-level, and pressure-ready blocks.
- Force Injectors (`Force ON`) always applies while selected at the forced duty value, even during timing cut, boost cut, low-level, and pressure-ready blocks.
- Force Baro (`kPa abs`) overrides barometric reference used in dP math and in the Baro gauge while active; leave blank or use a negative value to return to boot-captured baro.
- Real rail dP and pressure-ready timeout faults both use the same timing-cut auto-clear setting.
- Real rail dP and pressure-ready timeout boost-hold latches retain boost cut until power cycle (unless their dedicated bypass is enabled for bench testing).
- Status shows dP monitor states: `IDLE`, `ARMED`, `PENDING`, `TIMING_CUT`, and `BOOST_HOLD`.
- Rail dP fault text and state-detail labels now distinguish `normal low` and `critical low` paths in both active timing-cut and retained boost-hold conditions.
- In the normal non-forced output path, pressure-ready state resets immediately when spray is no longer allowed (for example level low, timing cut, retained boost hold, or Force Pump OFF).
- In the normal non-forced output path, if pressure-ready does not complete before timeout while spray is requested, the controller latches a real pressure-ready timeout fault.
- Exception: when **Pressure-Ready Override** is ON, pressure-ready injector blocking and pressure-ready timeout fault latching are bypassed for bench testing.
- If you want map-only spray bench testing without rail dP safety intervention, enable **Pressure-Ready Override** and **dP Monitor Override** together.
- Bench-force sequencing note: if you switch from a high forced rail value to a low forced rail value and then test pressure-ready or timeout behavior, set Force Injectors to OFF briefly first and wait for rail psig to settle low before restarting the scenario. This prevents stale pressure-ready state from the previous forced-rail condition.
- Manual fault controls and per-error overrides:
  - Controls are grouped as **Manual Fault Triggers**, **Monitor Overrides**, and **Safety Bypasses** for faster bench setup.
  - **Manual Boost Cut**
  - **Manual Timing Cut**
  - **Rail dP Fault**
  - **Pressure-Ready Fault**
  - **Rail dP Boost Hold**
  - **Pressure-Ready Boost Hold**
  - **dP Monitor Override** (test only, pauses automatic rail dP fault monitoring)
  - **Pressure-Ready Override** (test only, bypasses pressure-ready injector blocking and pressure-ready timeout fault latching)
  - **Level Fault Bypass** (test only, bypasses low-level safety cut behavior)
  - **dP Fault Bypass** (test only, bypasses rail dP timing-cut behavior)
  - **Pressure-Ready Fault Bypass** (test only, bypasses pressure-ready timeout timing-cut behavior)
  - **dP Hold Bypass** (test only, bypasses retained dP boost-hold behavior)
  - **Pressure-Ready Hold Bypass** (test only, bypasses retained pressure-ready boost-hold behavior)
- Manual fault controls are bench overrides and clear when toggled OFF.
- Runtime fault states clear on reboot.

## What the LEDs Mean
- BLUE on: tank low, spray off.
- GREEN on: methanol is actively spraying.
- RED on: boost cut is active. Timing cut also turns RED on because timing cut asserts boost cut at the same time. IAT is grounded only during timing cut, and the timing-cut output is failsafe (`HIGH` healthy, `LOW` timing cut).

## Setup Checklist (Sanity Checks)
1. Idle MAP is reasonable for your engine/altitude.
2. Rail pressure sensor reads near 0 psig with pump off, then rises until Pressure Ready shows READY when spray is requested.
3. Injector dP stays near your target when spray is commanded.
4. BLUE LED indicates low level. GREEN LED indicates active spray. RED LED indicates boost cut.
5. When level goes low during boost/spray, boost cut and timing cut are active.

## Timing Cut Tuning Tips
- In your ECU, put a flat negative value (example: -8) in the hottest IAT column across boosted MAP rows.
- Ensure Spark Minimum allows that pull.
- Keep non-boost rows at 0 if you do not want off-boost pull.

## Serial Monitor (115200)
- Heartbeat from web task every second with heap/IP.
- `[STAT]` lines every 250 ms: mode, dP monitor state, MAP kPa, rail psig, duty, pump, LEDs, timing cut, dP timers, and pressure-ready status.
- `[EVENT]` lines for level low transitions, timing cut on/off, pump on/off, pressure-ready done/reset, fault latch on/off.
- `[FAULT]` when injector dP normal path, critical low-pressure path, or pressure-ready timeout path latches timing cut.

## Troubleshooting
- No live data:
  - Confirm Wi-Fi connected and `WS: ON` (or `WS: STALE` if polling is active).
  - Check that `http://192.168.4.1/` loads.
- Missing Pressure Ready tile in Status:
  - Reload using a cache-buster URL, for example `http://<controller-ip>/?v=20260223`.
  - Firmware now sends no-cache headers so browsers fetch current Status markup and payloads after firmware updates.
- `.local` opens by IP but not reliably in browser:
  - Confirm host from `http://<controller-ip>/api/config` (`mdnsHost`), then open `http://<mdnsHost>.local/`.
  - Clear OS DNS cache (`ipconfig /flushdns`).
  - Clear browser DNS host cache (`edge://net-internals/#dns` or `chrome://net-internals/#dns`).
  - Restart browser and retry `.local`; if still unstable, use controller IP for tuning on that session.
- MAP/rail stuck at 0:
  - Check sensor wiring and divider ratio.
  - Verify MAP span/offset and rail slope/offset values.
- Immediate timing cut:
  - Verify MAP and rail inputs are not swapped.
  - Check dP threshold and delay are reasonable.
  - Confirm injector dP reaches the target injector dP.
- Low level stuck:
  - Check float wiring (grounded = OK, open = low).
  - In Test Section, Force Level Switch uses the same fixed logic: open = low, grounded = OK.
  - Use Force Level Switch in Test Section to isolate.
- Boost cut/timing cut not doing anything:
  - Verify SSR polarity settings in Test Section.
  - Confirm wiring to boost solenoid and IAT SSR.
- Flow table looks wrong:
  - Verify injector size, mix %, and reference dP.
  - Confirm duty clamp is not limiting.

## Quick Glossary
- Injector dP: rail pressure minus MAP pressure.
- Duty Clamp: hard max duty applied to the curve.
- Timing Cut: grounds IAT to force timing reduction (`HIGH` healthy output, `LOW` active timing cut output).
- Boost Cut: opens boost solenoid path to reduce boost.

