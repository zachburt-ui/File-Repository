// ======================== PERSISTENCE ============================
// ================================================================
//
// This file handles saving and loading configuration from flash.
// The ESP32 Preferences library stores key and value pairs in non volatile memory.

// Build tag used to detect a newly flashed firmware image.
// This value changes on each compile because it uses compile date and time macros.
// When the tag changes, firmware defaults are loaded once and persisted as the new baseline.
static String firmwareBuildTag(){
  return String(__DATE__) + " " + String(__TIME__);
}

// Normalize the stored fault history text so the status card always has a valid display value.
static void sanitizeFaultHistoryText(){
  fault_history_text.trim();
  if(fault_history_text.length() == 0) fault_history_text = "None";
}

// Persist fault history immediately when a new active reason appears.
// This keeps last-known fault context visible after reboot without persisting active latches.
void persistFaultHistory(){
  sanitizeFaultHistoryText();
  prefs.begin("methctl", false);
  prefs.putString("fault_hist", fault_history_text);
  prefs.end();
}

// Load settings from flash.
// If a key does not exist yet, the current global value is used as a default.
static void loadPrefs(){
  prefs.begin("methctl", false); // Open preference namespace used by this controller.
  const String currentBuildTag = firmwareBuildTag(); // Build identifier for the firmware currently running on the controller.
  const String storedBuildTag = prefs.getString("fw_build", ""); // Last build identifier that stored preferences were created under.
  if(storedBuildTag != currentBuildTag){
    // A newly flashed firmware build is running.
    // Clear old saved values so firmware defaults become the active settings baseline.
    prefs.clear();
    prefs.putString("fw_build", currentBuildTag); // Persist current build tag so the defaults are not re-applied on every reboot.
    prefs.end(); // Close namespace and return with compile-time defaults still active in memory.
    if(serial_debug_enable){
      Serial.printf("[PREF] New firmware build detected. Loading firmware defaults for this flash: %s\n", currentBuildTag.c_str());
    }
    return;
  }
  map_div_ratio  = prefs.getFloat("map_div",  map_div_ratio); // Restore manifold sensor divider ratio.
  rail_div_ratio = prefs.getFloat("rail_div", rail_div_ratio); // Restore rail sensor divider ratio.
  // Restore shared ADC input gain and offset correction.
  adc_input_gain = prefs.getFloat("adc_ag", adc_input_gain);
  adc_input_offset_v = prefs.getFloat("adc_ao", adc_input_offset_v);
  // Restore shared ADC curve midpoint values (endpoints are fixed and sanitized).
  for(uint8_t i = 0; i < ADC_CURVE_PTS; ++i){
    const String key = String("adc_c") + String(i);
    adc_curve_out_v[i] = prefs.getFloat(key.c_str(), adc_curve_out_v[i]);
  }
  wifiSsid = prefs.getString("ap_ssid", wifiSsid); // Restore access point network name.
  mdnsHost = prefs.getString("mdns", mdnsHost); // Restore multicast domain name system host name.
  wifi_mode = (uint8_t)prefs.getUChar("wifi_mode", wifi_mode); // Restore wireless mode selection.
  sta_ssid = prefs.getString("sta_ssid", sta_ssid); // Restore station network name.
  sta_pass = prefs.getString("sta_pass", sta_pass); // Restore station password.
  // Manifold absolute pressure is stored as linear and offset; derive the per volt slope.
  map_kpa_linear_hpt = prefs.getFloat("map_hlin", map_kpa_linear_hpt);
  map_kpa_offset_hpt = prefs.getFloat("map_hoff", map_kpa_offset_hpt);
  // Ensure the MAP calibration and divider ratio are within reasonable bounds.
  sanitizeMapCalibration();
  rail_psi_per_v  = prefs.getFloat("rail_s", rail_psi_per_v);
  rail_psi_offset = prefs.getFloat("rail_o", rail_psi_offset);
  // Rail calibration must be sanitized after loading persisted rail linear and offset.
  sanitizeRailCalibration();
  // ADC input gain and offset correction must also be sanitized after loading.
  sanitizeAdcInputCalibration();
  // Shared ADC curve correction values must be sanitized after loading.
  sanitizeAdcCurveCalibration();
  inj_pwm_hz  = prefs.getFloat("inj_hz", inj_pwm_hz); // Restore injector pulse width modulation frequency.
  inj_lbhr_at_58psi = prefs.getFloat("inj_lb", inj_lbhr_at_58psi); // Restore injector size reference at fifty-eight pounds per square inch.
  mix_meth_pct      = prefs.getFloat("mix_pct", mix_meth_pct); // Restore methanol mixture percentage for flow math.
  flow_ref_dp_psi   = prefs.getFloat("flow_dp", flow_ref_dp_psi); // Restore reference differential pressure for flow math.
  // Recompute mix density after loading the methanol percentage.
  mix_density_g_per_cc = ((mix_meth_pct/100.0f)*0.792f) + ((1.0f - mix_meth_pct/100.0f)*0.998f);
  min_dp_psi            = prefs.getFloat("dp_min", min_dp_psi);
  rail_fault_delay_ms   = prefs.getUInt("pf_dly", rail_fault_delay_ms);
  timing_cut_auto_clear_ms = prefs.getUInt("tc_auto", timing_cut_auto_clear_ms);
  dp_arm_duty_pct       = prefs.getFloat("dp_arm", dp_arm_duty_pct);
  dp_recover_margin_psi = prefs.getFloat("dp_rec", dp_recover_margin_psi);
  dp_critical_psi       = prefs.getFloat("dp_crit", dp_critical_psi);
  dp_critical_ms        = prefs.getUInt("dp_critms", dp_critical_ms);
  dp_arm_settle_ms      = prefs.getUInt("dp_armstl", dp_arm_settle_ms);
  if(!isfinite(dp_critical_psi) || dp_critical_psi < 0.0f || dp_critical_psi > 300.0f){
    dp_critical_psi = 20.0f;
  }
  if(dp_critical_ms > 10000){
    dp_critical_ms = 100;
  }
  if(dp_arm_settle_ms > 10000){
    dp_arm_settle_ms = 300;
  }
  level_debounce_ms     = prefs.getUInt("lvl_db", level_debounce_ms); // Restore level switch debounce time.
  pressure_ready_timeout_ms = prefs.getUInt("prdy_to", pressure_ready_timeout_ms); // Restore pressure-ready timeout.
  duty_max              = prefs.getFloat("dmax", duty_max); // Restore maximum duty clamp.
  serial_debug_enable     = prefs.getBool("sdbg", serial_debug_enable); // Restore serial debug enable flag.
  serial_status_period_ms = prefs.getUInt("sper", serial_status_period_ms); // Restore serial status print interval.
  fault_history_text = prefs.getString("fault_hist", fault_history_text); // Restore last non-clear safety or fault reason shown as history.
  sanitizeFaultHistoryText();
  // Restore each spray-curve point from preferences.
  for(int i=0;i<CURVE_PTS;i++){
    String k="k"+String(i);
    String d="d"+String(i);
    curve_kpa[i]  = prefs.getFloat(k.c_str(), curve_kpa[i]);
    curve_duty[i] = prefs.getFloat(d.c_str(), curve_duty[i]);
  }
  prefs.end(); // Close preference namespace after loading all settings.
  // Print a summary when serial debug is enabled.
  if(serial_debug_enable){
    Serial.println("[PREF] Loaded:");
    Serial.printf("       AP SSID: %s | MDNS: %s\n", wifiSsid.c_str(), mdnsHost.c_str());
    Serial.printf("       MAP span/offset (plain five-volt calibration): %.3f / %.3f (per-volt slope %.3f)\n", map_kpa_linear_hpt, map_kpa_offset_hpt, map_kpa_per_v);
    Serial.printf("       Rail psi/V offset: %.3f / %.3f\n", rail_psi_per_v, rail_psi_offset);
    Serial.printf("       ADC calibration (shared gain/offset V for MAP and rail): %.4f / %.4f\n", adc_input_gain, adc_input_offset_v);
    Serial.printf("       ADC curve midpoints V (0.5/1.0/1.5/2.0/2.5/3.0): %.4f / %.4f / %.4f / %.4f / %.4f / %.4f\n",
      adc_curve_out_v[1], adc_curve_out_v[2], adc_curve_out_v[3], adc_curve_out_v[4], adc_curve_out_v[5], adc_curve_out_v[6]);
    Serial.printf("       Fault history: %s\n", fault_history_text.c_str());
    Serial.printf("       Duty clamp: %.1f%% | inj Hz: %.1f\n", duty_max, inj_pwm_hz);
    Serial.printf("       Differential pressure min/arm/recover/delay/auto clear: %.1f / %.1f / %.1f / %lu ms / %lu ms\n", min_dp_psi, dp_arm_duty_pct, dp_recover_margin_psi, (unsigned long)rail_fault_delay_ms, (unsigned long)timing_cut_auto_clear_ms);
    Serial.printf("       Differential pressure critical/critical delay/arm settle: %.1f / %lu ms / %lu ms\n", dp_critical_psi, (unsigned long)dp_critical_ms, (unsigned long)dp_arm_settle_ms);
    Serial.printf("       Level debounce: %lu ms | pressure-ready timeout: %lu ms\n", (unsigned long)level_debounce_ms, (unsigned long)pressure_ready_timeout_ms);
    Serial.printf("       Spray-enable/flow reference differential pressure: %.1f psi | Meth mix: %.1f%%\n", flow_ref_dp_psi, mix_meth_pct);
  }
}
// Save all tunable settings to flash.
static void savePrefs(){
  prefs.begin("methctl", false); // Open preference namespace used by this controller.
  prefs.putString("fw_build", firmwareBuildTag()); // Persist firmware build tag so settings stay stable across normal reboots on this build.
  prefs.putFloat("map_div", map_div_ratio); // Persist manifold sensor divider ratio.
  prefs.putFloat("rail_div", rail_div_ratio); // Persist rail sensor divider ratio.
  prefs.putFloat("adc_ag", adc_input_gain); // Persist shared ADC gain correction used by both MAP and rail inputs.
  prefs.putFloat("adc_ao", adc_input_offset_v); // Persist shared ADC offset correction used by both MAP and rail inputs.
  // Persist shared ADC correction curve values for all fixed breakpoints.
  for(uint8_t i = 0; i < ADC_CURVE_PTS; ++i){
    const String key = String("adc_c") + String(i);
    prefs.putFloat(key.c_str(), adc_curve_out_v[i]);
  }
  // Remove old per-input ADC keys now that one shared ADC correction pair is the only supported format.
  if(prefs.isKey("map_ag")) prefs.remove("map_ag");
  if(prefs.isKey("map_ao")) prefs.remove("map_ao");
  if(prefs.isKey("rail_ag")) prefs.remove("rail_ag");
  if(prefs.isKey("rail_ao")) prefs.remove("rail_ao");
  prefs.putString("ap_ssid", wifiSsid); // Persist access point network name.
  prefs.putString("mdns", mdnsHost); // Persist multicast domain name system host name.
  prefs.putUChar("wifi_mode", wifi_mode); // Persist wireless mode selection.
  prefs.putString("sta_ssid", sta_ssid); // Persist station network name.
  prefs.putString("sta_pass", sta_pass); // Persist station password.
  prefs.putFloat("map_hlin", map_kpa_linear_hpt); // Persist manifold calibration linear value.
  prefs.putFloat("map_hoff", map_kpa_offset_hpt); // Persist manifold calibration offset value.
  prefs.putFloat("rail_s", rail_psi_per_v); // Persist rail calibration slope.
  prefs.putFloat("rail_o", rail_psi_offset); // Persist rail calibration offset.
  prefs.putFloat("inj_hz", inj_pwm_hz); // Persist injector pulse width modulation frequency.
  prefs.putFloat("inj_lb", inj_lbhr_at_58psi); // Persist injector size reference.
  prefs.putFloat("mix_pct", mix_meth_pct); // Persist methanol percentage for flow calculations.
  prefs.putFloat("flow_dp", flow_ref_dp_psi); // Persist reference differential pressure for flow calculations.
  prefs.putFloat("dp_min", min_dp_psi); // Persist normal differential pressure fault threshold.
  prefs.putUInt("pf_dly", rail_fault_delay_ms); // Persist normal differential pressure fault delay.
  prefs.putUInt("tc_auto", timing_cut_auto_clear_ms); // Persist timing cut auto-clear delay.
  prefs.putFloat("dp_arm", dp_arm_duty_pct); // Persist differential pressure arm duty threshold.
  prefs.putFloat("dp_rec", dp_recover_margin_psi); // Persist differential pressure recovery margin.
  prefs.putFloat("dp_crit", dp_critical_psi); // Persist critical differential pressure fault threshold.
  prefs.putUInt("dp_critms", dp_critical_ms); // Persist critical differential pressure fault delay.
  prefs.putUInt("dp_armstl", dp_arm_settle_ms); // Persist differential pressure arm settle delay.
  prefs.putUInt("lvl_db", level_debounce_ms); // Persist level switch debounce delay.
  prefs.putUInt("prdy_to", pressure_ready_timeout_ms); // Persist pressure-ready timeout.
  if(prefs.isKey("prime_ms")) prefs.remove("prime_ms"); // Remove old key now that pressure-ready timeout has replaced prime delay.
  prefs.putFloat("dmax", duty_max); // Persist duty clamp limit.
  // Multicast domain name system is always on (no preference key needed).

  prefs.putBool("sdbg", serial_debug_enable); // Persist serial debug enable flag.
  prefs.putUInt("sper", serial_status_period_ms); // Persist serial status print interval.
  sanitizeFaultHistoryText();
  prefs.putString("fault_hist", fault_history_text); // Persist last non-clear safety or fault reason for the Status card history tile.
  // Persist each spray-curve point under stable key names (k0..k9 and d0..d9).
  for(int i=0;i<CURVE_PTS;i++){
    String k="k"+String(i);
    String d="d"+String(i);
    prefs.putFloat(k.c_str(), curve_kpa[i]);
    prefs.putFloat(d.c_str(), curve_duty[i]);
  }
  prefs.end(); // Close preference namespace after all values are written.
}
// ================================================================
