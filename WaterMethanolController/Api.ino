// =================== APPLICATION INTERFACE ======================
// ================================================================
//
// This file defines the web application programming interface that the web interface uses.
// The interface has two main responsibilities:
// 1) Serve the single page web interface.
// 2) Provide JavaScript Object Notation data for live status and configuration.

// Log incoming requests to the serial monitor when debug is enabled.
static void logReq(const char* path){
  if(!serial_debug_enable) return;
  Serial.printf("[HTTP] %s from %s\n", path, server.client().remoteIP().toString().c_str());
}
// Send no-cache headers so browsers always fetch the current firmware user interface and status payloads.
// This prevents stale interface markup from hiding new status tiles after firmware updates.
static void sendNoCacheHeaders(){
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
}
// Serve the embedded web page markup at the root path.
static void handleRoot(){
  logReq("/");
  sendNoCacheHeaders();
  server.send_P(200, "text/html", INDEX_HTML);
}
// Build the live status JavaScript Object Notation payload sent to the user interface and WebSocket clients.
// This pulls from current sensor values and safety states.
void buildStatusJson(JsonDocument& d){
  const float mapPsi = mapKpa * 0.1450377377f;       // Convert manifold pressure from kilopascal absolute to pounds per square inch absolute.
  const float baroKpaNow = effectiveBaroKpa(); // Active barometric reference for status and differential pressure reporting.
  const float baroPsiNow = baroKpaNow * 0.1450377377f; // Convert active barometric reference from kilopascal absolute to pounds per square inch absolute.
  const float dpPsiNow = (railPsi + baroPsiNow) - mapPsi; // Injector differential pressure equals rail gauge pressure plus active barometric pressure minus manifold absolute pressure.
  bool levelLow = levelLowState;               // Start from debounced level switch state.
  if(force_level_mode==1) levelLow = true;     // Bench test override that forces low level.
  else if(force_level_mode==2) levelLow = false; // Bench test override that forces level okay.
  const bool levelLowEffective = levelLow && !force_level_fault_bypass; // Effective low-level safety input after optional bypass.
  const bool levelOkNow = !levelLowEffective;   // Convenience flag used by status and safety summaries.
  const bool railDpFaultRaw = faultLatched || force_dp_fault; // Raw rail differential-pressure timing-cut source before dedicated rail bypass.
  const bool railDpFaultNow = railDpFaultRaw && !force_dp_fault_bypass; // Effective rail differential-pressure timing-cut source after dedicated rail bypass.
  const bool railDpFaultManualNow = force_dp_fault && !force_dp_fault_bypass; // Manual rail differential-pressure timing-cut source after dedicated bypass.
  const bool railDpFaultRealNow = faultLatched && !force_dp_fault_bypass; // Real rail differential-pressure timing-cut latch after dedicated bypass.
  const bool pressureReadyFaultRaw = pressureReadyFaultLatched || force_pressure_ready_fault; // Raw pressure-ready timeout timing-cut source before dedicated pressure-ready bypass.
  const bool pressureReadyFaultNow = pressureReadyFaultRaw && !force_pressure_ready_fault_bypass; // Effective pressure-ready timeout timing-cut source after dedicated pressure-ready bypass.
  const bool railDpBoostHoldRaw = dpBoostCutHold || force_dp_boost_hold; // Raw retained rail differential-pressure boost-hold source before dedicated rail hold bypass.
  const bool railDpBoostHoldNow = railDpBoostHoldRaw && !force_dp_boost_hold_bypass; // Effective retained rail differential-pressure boost-hold source after dedicated rail hold bypass.
  const bool railDpBoostHoldManualNow = force_dp_boost_hold && !force_dp_boost_hold_bypass; // Manual retained rail differential-pressure boost-hold source after dedicated bypass.
  const bool railDpBoostHoldRealNow = dpBoostCutHold && !force_dp_boost_hold_bypass; // Real retained rail differential-pressure boost hold after dedicated bypass.
  const bool pressureReadyBoostHoldRaw = pressureReadyBoostCutHold || force_pressure_ready_boost_hold; // Raw retained pressure-ready boost-hold source before dedicated pressure-ready hold bypass.
  const bool pressureReadyBoostHoldNow = pressureReadyBoostHoldRaw && !force_pressure_ready_boost_hold_bypass; // Effective retained pressure-ready boost-hold source after dedicated pressure-ready hold bypass.
  const bool railDpPendingNow = (railLowSince > 0) && !railDpFaultNow && dpArmedNow; // Rail differential-pressure pending timer state after effective fault evaluation.
  const char* railDpStateDetail = "NONE"; // Human-readable rail differential-pressure state detail that distinguishes normal-low and critical-low paths.
  if(railDpFaultManualNow){
    railDpStateDetail = "FAULT (MANUAL)";
  }else if(railDpFaultRealNow){
    railDpStateDetail = (railDpLastRealFaultPath == RAIL_DP_PATH_CRITICAL_LOW) ? "FAULT (CRITICAL LOW)" : "FAULT (NORMAL LOW)";
  }else if(railDpBoostHoldManualNow){
    railDpStateDetail = "BOOST HOLD (MANUAL)";
  }else if(railDpBoostHoldRealNow){
    if(railDpLastRealFaultPath == RAIL_DP_PATH_CRITICAL_LOW){
      railDpStateDetail = "BOOST HOLD (CRITICAL LOW)";
    }else if(railDpLastRealFaultPath == RAIL_DP_PATH_NORMAL_LOW){
      railDpStateDetail = "BOOST HOLD (NORMAL LOW)";
    }else{
      railDpStateDetail = "BOOST HOLD";
    }
  }else if(railDpPendingNow){
    railDpStateDetail = "PENDING (LOW)";
  }
  const char* reason = "None";                 // Human-readable current fault status used uniformly across status outputs.
  if(force_timing_cut) reason = "Manual Timing Cut"; // Manual timing cut latch takes highest priority.
  else if(force_boost_cut) reason = "Manual Boost Cut"; // Manual boost cut latch is next priority.
  else if(force_pressure_ready_fault && !force_pressure_ready_fault_bypass) reason = "Pressure-Ready Timeout Fault (Manual)"; // Manual pressure-ready timeout fault command for bench testing.
  else if(pressureReadyFaultNow) reason = "Pressure-Ready Timeout Fault"; // Real pressure-ready timeout timing-cut latch.
  else if(railDpFaultManualNow) reason = "Rail dP Fault (Manual)"; // Manual rail differential-pressure timing-cut command for bench testing.
  else if(railDpFaultRealNow) reason = (railDpLastRealFaultPath == RAIL_DP_PATH_CRITICAL_LOW) ? "Rail dP Fault (Critical Low)" : "Rail dP Fault (Normal Low)"; // Real rail differential-pressure timing-cut latch with path detail.
  else if(pressureReadyBoostHoldNow) reason = "Pressure-Ready Timeout Fault (Boost Hold)"; // Retained boost-hold from pressure-ready timeout path.
  else if(railDpBoostHoldManualNow) reason = "Rail dP Fault (Manual Boost Hold)"; // Manual retained rail differential-pressure boost-hold command for bench testing.
  else if(railDpBoostHoldRealNow){
    if(railDpLastRealFaultPath == RAIL_DP_PATH_CRITICAL_LOW){
      reason = "Rail dP Fault (Boost Hold - Critical Low)";
    }else if(railDpLastRealFaultPath == RAIL_DP_PATH_NORMAL_LOW){
      reason = "Rail dP Fault (Boost Hold - Normal Low)";
    }else{
      reason = "Rail dP Fault (Boost Hold)";
    }
  }
  else if(timingCutOn) reason = "Low Level (Timing Cut)"; // Low level timing cut while spray was active.
  else if(levelLowEffective) reason = "Low Level"; // Low level boost cut before spray.
  // The flow model treats three injectors as one bank.
  // This is display only and does not affect control.
  const float scale = (dpPsiNow > 0.1f) ? sqrtf(dpPsiNow / 58.0f) : 0.0f; // Standard injector square-root pressure scaling model.
  const float flowLbHr = (inj_lbhr_at_58psi * (float)inj_count) * scale * (cmdDuty / 100.0f); // Total injector bank mass flow in pounds per hour.
  const float flowCcMin = (flowLbHr * 453.59237f / 3600.0f) / mix_density_g_per_cc * 60.0f; // Convert mass flow to cubic centimeters per minute.
  const float flowGph = (flowCcMin * 60.0f) / 3785.411784f; // Convert cubic centimeters per minute to gallons per hour.
  // The network line displayed in the user interface is built here.
  String n;
  if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA){
    n += String("AP: ") + wifiSsid + "  IP: " + WiFi.softAPIP().toString();
  }
  if(WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA){
    if(n.length()) n += "  |  ";
    n += String("STA: ") + (WiFi.isConnected() ? WiFi.localIP().toString() : "connecting...");
  }
  netLine = n; // Save network summary string for user interface status card.
  // Injector output status is reported from the actual pulse command so bench force modes are reflected correctly.
  const bool injOnNow = (injOnUs > 0); // True means the injector pulse generator is currently commanding on-time.
  if(modeNow==MODE_IAT_CUT) d["stateText"] = "Timing Cut"; // Primary state banner for user interface.
  else if(!boostOn) d["stateText"] = "Boost Cut"; // Secondary state banner when boost cut is active without timing cut.
  else d["stateText"] = ""; // No banner when system is healthy.

  // -------- Live sensor and output values shown in gauges and status tiles --------
  d["mapKpa"] = (double)mapKpa;
  d["mapKpaRaw"] = (double)mapKpaRaw;
  d["railPsi"] = (double)railPsi;
  d["baroKpa"] = (double)baroKpaNow;
  d["baroPsi"] = (double)baroPsiNow;
  d["dpPsi"] = (double)dpPsiNow;
  d["sprayPct"] = (double)cmdDuty;
  d["curvePct"] = (double)curvePctLive;
  d["dutyClamp"] = (double)duty_max;
  d["pumpOn"] = pumpOn;
  d["injOn"]  = injOnNow;
  d["sprayActive"] = injOnNow && pumpOn;
  d["levelOk"] = levelOkNow;
  d["redLedOn"]    = redLedOn;
  d["blueLedOn"]   = blueLedOn;
  d["greenLedOn"]  = greenLedOn;
  d["boostOn"] = boostOn;
  d["levelLow"] = levelLow;
  d["iatOn"] = iatGroundOn;

  // -------- Safety state and latch visibility fields --------
  d["levelFault"] = levelLowEffective;
  d["dpFault"] = railDpFaultNow;
  d["railDpStateDetail"] = railDpStateDetail;
  d["pressureReadyFault"] = pressureReadyFaultNow;
  d["dpBoostHold"] = railDpBoostHoldNow;
  d["pressureReadyBoostHold"] = pressureReadyBoostHoldNow;
  d["faultActive"] = (strcmp(reason, "None") != 0);
  d["forceBoostCut"] = force_boost_cut;
  d["forceTimingCut"] = force_timing_cut;
  d["forceDpFault"] = force_dp_fault;
  d["forcePressureReadyFault"] = force_pressure_ready_fault;
  d["forceDpBoostHold"] = force_dp_boost_hold;
  d["forcePressureReadyBoostHold"] = force_pressure_ready_boost_hold;
  d["dpMonitorOverride"] = force_dp_monitor_override;
  d["pressureReadyOverride"] = force_pressure_ready_override;
  d["levelFaultBypass"] = force_level_fault_bypass;
  d["dpFaultBypass"] = force_dp_fault_bypass;
  d["dpBoostHoldBypass"] = force_dp_boost_hold_bypass;
  d["pressureReadyFaultBypass"] = force_pressure_ready_fault_bypass;
  d["pressureReadyBoostHoldBypass"] = force_pressure_ready_boost_hold_bypass;

  // -------- Differential pressure monitor diagnostics --------
  d["dpPending"] = railDpPendingNow;
  d["dpState"] = dpStateName(dpStateNow);
  d["dpLowMs"] = (uint32_t)dpLowForMsNow;
  d["dpMinSeen"] = (double)dpMinSeenPsiNow;
  d["dpArmed"] = dpArmedNow;
  d["dpArmSettleRemainingMs"] = (uint32_t)dpArmSettleRemainingMsNow;
  d["levelDebounceMs"] = level_debounce_ms;
  d["pressureReadyTimeoutMs"] = pressure_ready_timeout_ms;
  d["pressureReady"] = pressureReady;
  d["levelRaw"] = levelRawLast;

  // -------- Tunable values mirrored back to the user interface --------
  d["dpMinPsi"] = min_dp_psi;
  d["dpArmPct"] = dp_arm_duty_pct;
  d["dpRecover"] = dp_recover_margin_psi;
  d["dpCriticalPsi"] = dp_critical_psi;
  d["dpCriticalMs"] = dp_critical_ms;
  d["dpArmSettleMs"] = dp_arm_settle_ms;
  d["lvlDebounce"] = level_debounce_ms;
  d["pressureReadyTimeoutMs"] = pressure_ready_timeout_ms;
  d["injLb"] = inj_lbhr_at_58psi;
  d["mixMethPct"] = mix_meth_pct;
  d["targetInjectorDp"] = flow_ref_dp_psi;
  const char* safetyState = timingCutOn ? "Timing Cut" : (!boostOn ? "Boost Cut" : "OK"); // Compact safety state label for status tile.
  const char* safetyReason = (strcmp(reason, "None") == 0) ? "" : reason; // Keep safety reason synchronized with the unified current-fault text.
  d["safetyState"] = safetyState;
  d["safetyReason"] = safetyReason;
  d["currentFaultStatus"] = reason;
  d["faultReason"] = reason;
  if(strcmp(reason, "None") != 0){
    const String newFaultHistory = String(reason); // Build once so change detection is clear and explicit.
    if(fault_history_text != newFaultHistory){
      fault_history_text = newFaultHistory;
      persistFaultHistory(); // Persist history when a new active reason appears.
    }
  }
  d["faultHistory"] = fault_history_text;

  // -------- Flow model outputs and network summary --------
  d["flowLbHr"] = (double)flowLbHr;
  d["flowCcMin"] = (double)flowCcMin;
  d["flowGph"] = (double)flowGph;
  d["netLine"] = netLine;
}
// This handler serves the status endpoint at /api/status.
// It returns live status data for the user interface.
static void handleStatus(){
  logReq("/api/status");
  JsonDocument d; // Scratch JavaScript Object Notation document for status response.
  buildStatusJson(d); // Populate the status response with current runtime values.
  String out; serializeJson(d, out); // Serialize JavaScript Object Notation document to string.
  sendNoCacheHeaders();
  server.send(200, "application/json", out); // Return status payload to browser.
}
// This handler serves the configuration endpoint at /api/config.
// It returns the current configuration so the user interface can populate fields.
static void handleGetConfig(){
  logReq("/api/config");
  JsonDocument d; // Scratch JavaScript Object Notation document for configuration response.

  // -------- Core tunables --------
  d["injHz"] = inj_pwm_hz;
  d["dutyClamp"] = duty_max;
  d["dpMinPsi"] = min_dp_psi;
  d["dpFaultMs"] = rail_fault_delay_ms;
  d["dpArmPct"] = dp_arm_duty_pct;
  d["dpRecover"] = dp_recover_margin_psi;
  d["dpCriticalPsi"] = dp_critical_psi;
  d["dpCriticalMs"] = dp_critical_ms;
  d["dpArmSettleMs"] = dp_arm_settle_ms;
  d["timingCutAutoMs"] = timing_cut_auto_clear_ms;
  d["lvlDebounce"] = level_debounce_ms;
  d["pressureReadyTimeoutMs"] = pressure_ready_timeout_ms;
  d["injLb"] = inj_lbhr_at_58psi;
  d["mixMethPct"] = mix_meth_pct;
  d["targetInjectorDp"] = flow_ref_dp_psi;

  // -------- Sensor calibration --------
  d["mapLin"] = map_kpa_linear_hpt;
  d["mapOff"] = map_kpa_offset_hpt;
  d["pLin"] = rail_psi_per_v;
  d["pOff"] = rail_psi_offset;

  // -------- Network settings --------
  d["wifiMode"] = wifi_mode;
  d["apSsid"] = wifiSsid;
  d["mdnsHost"] = mdnsHost;
  d["staSsid"] = sta_ssid;
  d["staPass"] = sta_pass;

  // -------- Test section force values --------
  d["forcePump"] = (int)force_pump_mode;
  d["forceInj"]  = (int)force_inj_mode;
  d["forceDuty"] = force_duty;
  d["forceLevel"] = (int)force_level_mode;
  d["forceMapKpa"] = force_map_kpa;
  d["forceRailPsi"] = force_rail_psi;
  d["forceDpPsi"] = force_dp_psi;
  d["forceBaroKpa"] = force_baro_kpa;
  d["mapDiv"] = map_div_ratio;
  d["railDiv"] = rail_div_ratio;
  // Shared ADC trim is the primary interface used by the current user interface.
  d["adcGain"] = adc_input_gain;
  d["adcOffset"] = adc_input_offset_v;
  JsonArray adcCurveX = d["adcCurveX"].to<JsonArray>();
  JsonArray adcCurveY = d["adcCurveY"].to<JsonArray>();
  for(uint8_t i = 0; i < ADC_CURVE_PTS; ++i){
    adcCurveX.add(adc_curve_in_v[i]);
    adcCurveY.add(adc_curve_out_v[i]);
  }
  d["forceBoostCut"] = force_boost_cut;
  d["forceTimingCut"] = force_timing_cut;
  d["forceDpFault"] = force_dp_fault;
  d["forcePressureReadyFault"] = force_pressure_ready_fault;
  d["forceDpBoostHold"] = force_dp_boost_hold;
  d["forcePressureReadyBoostHold"] = force_pressure_ready_boost_hold;
  d["forceDpMonitorOverride"] = force_dp_monitor_override;
  d["forcePressureReadyOverride"] = force_pressure_ready_override;
  d["forceLevelFaultBypass"] = force_level_fault_bypass;
  d["forceDpFaultBypass"] = force_dp_fault_bypass;
  d["forceDpBoostHoldBypass"] = force_dp_boost_hold_bypass;
  d["forcePressureReadyFaultBypass"] = force_pressure_ready_fault_bypass;
  d["forcePressureReadyBoostHoldBypass"] = force_pressure_ready_boost_hold_bypass;
  d["sdbg"] = serial_debug_enable;
  d["sper"] = serial_status_period_ms;
  // Spray curve points (manifold absolute pressure in kilopascal to spray duty percent).
  JsonArray a = d["curve"].to<JsonArray>();
  for(int i=0;i<CURVE_PTS;i++){
    JsonObject p = a.add<JsonObject>();
    p["kpa"] = curve_kpa[i];
    p["pct"] = curve_duty[i];
  }
  String out; serializeJson(d, out); // Serialize JavaScript Object Notation document to string.
  sendNoCacheHeaders();
  server.send(200, "application/json", out); // Return configuration payload to browser.
}
// This handler accepts configuration updates at /api/config.
// It accepts a JavaScript Object Notation object of settings and applies them to runtime and preferences.
static void handlePostConfig(){
  logReq("/api/config:POST");
  if(!server.hasArg("plain")){
    server.send(400, "text/plain", "Missing body");
    return;
  }
  JsonDocument d; // Scratch JavaScript Object Notation document for decoded post body.
  DeserializationError e = deserializeJson(d, server.arg("plain"));
  if(e){
    server.send(400, "text/plain", String("JSON parse error: ")+e.c_str());
    return;
  }
  // Basic controls are applied here.
  if(!d["injHz"].isNull())      inj_pwm_hz = constrain((float)d["injHz"], 1.0f, 400.0f);
  if(!d["dutyClamp"].isNull())  duty_max   = constrain((float)d["dutyClamp"], 0.0f, 100.0f);
  if(!d["dpMinPsi"].isNull())   min_dp_psi = max(0.0f, (float)d["dpMinPsi"]);
  if(!d["dpFaultMs"].isNull())  rail_fault_delay_ms = (uint32_t)max(0, (int)d["dpFaultMs"]);
  if(!d["dpArmPct"].isNull())   dp_arm_duty_pct = max(0.0f, (float)d["dpArmPct"]);
  if(!d["dpRecover"].isNull())  dp_recover_margin_psi = max(0.0f, (float)d["dpRecover"]);
  if(!d["dpCriticalPsi"].isNull()) dp_critical_psi = max(0.0f, (float)d["dpCriticalPsi"]);
  if(!d["dpCriticalMs"].isNull())  dp_critical_ms = (uint32_t)max(0, (int)d["dpCriticalMs"]);
  if(!d["dpArmSettleMs"].isNull()) dp_arm_settle_ms = (uint32_t)max(0, (int)d["dpArmSettleMs"]);
  if(!d["timingCutAutoMs"].isNull()) timing_cut_auto_clear_ms = (uint32_t)max(0, (int)d["timingCutAutoMs"]);
  if(!d["lvlDebounce"].isNull()) level_debounce_ms = (uint32_t)max(0, (int)d["lvlDebounce"]);
  if(!d["pressureReadyTimeoutMs"].isNull()) pressure_ready_timeout_ms = (uint32_t)max(0, (int)d["pressureReadyTimeoutMs"]);
  if(!d["injLb"].isNull())       inj_lbhr_at_58psi = max(1.0f, (float)d["injLb"]);
  if(!d["mixMethPct"].isNull()){
    mix_meth_pct = constrain((float)d["mixMethPct"], 0.0f, 100.0f);
    mix_density_g_per_cc = ((mix_meth_pct/100.0f)*0.792f) + ((1.0f - mix_meth_pct/100.0f)*0.998f);
  }
  if(!d["targetInjectorDp"].isNull()) flow_ref_dp_psi = max(0.0f, (float)d["targetInjectorDp"]);
  else if(!d["desiredRailDp"].isNull()) flow_ref_dp_psi = max(0.0f, (float)d["desiredRailDp"]);
  // Sensor scaling (five volt sensor volts before divider) is applied here.
  if(!d["mapLin"].isNull()) { map_kpa_linear_hpt  = (float)d["mapLin"]; }
  if(!d["mapOff"].isNull()) { map_kpa_offset_hpt = (float)d["mapOff"]; }
  if(!d["pLin"].isNull())   rail_psi_per_v  = (float)d["pLin"];
  if(!d["pOff"].isNull())   rail_psi_offset = (float)d["pOff"];
  // Wireless network settings are applied here, and they trigger a reboot when changed.
  bool wifiChanged = false; // Tracks whether network settings changed and reboot is required.
  if(!d["wifiMode"].isNull()){ wifi_mode = (uint8_t)constrain((int)d["wifiMode"], 0, 2); wifiChanged=true; }
  if(!d["apSsid"].isNull()){
    wifiSsid = String((const char*)d["apSsid"]);
    if(wifiSsid.length() == 0) wifiSsid = "watermeth";
    wifiChanged = true;
  }
  if(!d["mdnsHost"].isNull()){
    mdnsHost = String((const char*)d["mdnsHost"]);
    if(mdnsHost.length() == 0) mdnsHost = "watermeth";
    wifiChanged = true;
  }
  if(!d["staSsid"].isNull()) { sta_ssid = String((const char*)d["staSsid"]); wifiChanged=true; }
  if(!d["staPass"].isNull()) { sta_pass = String((const char*)d["staPass"]); wifiChanged=true; }
  // Test modes (bench overrides) are applied here.
  if(!d["forcePump"].isNull()) force_pump_mode = (uint8_t)constrain((int)d["forcePump"], 0, 2);
  if(!d["forceInj"].isNull())  force_inj_mode  = (uint8_t)constrain((int)d["forceInj"], 0, 2);
  if(!d["forceDuty"].isNull()) force_duty      = constrain((float)d["forceDuty"], 0.0f, 100.0f);
  if(!d["forceLevel"].isNull()) force_level_mode = (uint8_t)constrain((int)d["forceLevel"], 0, 2);
  if(!d["forceMapKpa"].isNull()) force_map_kpa = (float)d["forceMapKpa"];
  if(!d["forceRailPsi"].isNull()) force_rail_psi = (float)d["forceRailPsi"];
  if(!d["forceDpPsi"].isNull()) force_dp_psi = (float)d["forceDpPsi"];
  if(!d["forceBaroKpa"].isNull()) force_baro_kpa = (float)d["forceBaroKpa"];
  // Installer and debug settings are applied here.
  if(!d["mapDiv"].isNull())  map_div_ratio  = max(0.01f, (float)d["mapDiv"]);
  if(!d["railDiv"].isNull()) rail_div_ratio = max(0.01f, (float)d["railDiv"]);
  // Shared ADC trim keys used by current user interface.
  if(!d["adcGain"].isNull()) adc_input_gain = (float)d["adcGain"];
  if(!d["adcOffset"].isNull()) adc_input_offset_v = (float)d["adcOffset"];
  if((bool)(d["adcCurveReset"] | false)){
    resetAdcCurveCalibration();
  }
  JsonArray adcCurveY = d["adcCurveY"];
  if(!adcCurveY.isNull()){
    const uint8_t count = (uint8_t)min((int)adcCurveY.size(), (int)ADC_CURVE_PTS);
    for(uint8_t i = 0; i < count; ++i){
      if(!adcCurveY[i].isNull()){
        adc_curve_out_v[i] = (float)adcCurveY[i];
      }
    }
  }
  // Apply calibration sanity checks after updates.
  sanitizeMapCalibration();
  sanitizeRailCalibration();
  sanitizeAdcInputCalibration();
  sanitizeAdcCurveCalibration();
  if(!d["forceBoostCut"].isNull()) force_boost_cut = (bool)d["forceBoostCut"];
  if(!d["forceTimingCut"].isNull()) force_timing_cut = (bool)d["forceTimingCut"];
  if(!d["forceDpFault"].isNull()) force_dp_fault = (bool)d["forceDpFault"];
  if(!d["forcePressureReadyFault"].isNull()) force_pressure_ready_fault = (bool)d["forcePressureReadyFault"];
  if(!d["forceDpBoostHold"].isNull()) force_dp_boost_hold = (bool)d["forceDpBoostHold"];
  if(!d["forcePressureReadyBoostHold"].isNull()) force_pressure_ready_boost_hold = (bool)d["forcePressureReadyBoostHold"];
  if(!d["forceDpMonitorOverride"].isNull()) force_dp_monitor_override = (bool)d["forceDpMonitorOverride"];
  if(!d["forcePressureReadyOverride"].isNull()) force_pressure_ready_override = (bool)d["forcePressureReadyOverride"];
  if(!d["forceLevelFaultBypass"].isNull()) force_level_fault_bypass = (bool)d["forceLevelFaultBypass"];
  if(!d["forceDpFaultBypass"].isNull()) force_dp_fault_bypass = (bool)d["forceDpFaultBypass"];
  if(!d["forceDpBoostHoldBypass"].isNull()) force_dp_boost_hold_bypass = (bool)d["forceDpBoostHoldBypass"];
  if(!d["forcePressureReadyFaultBypass"].isNull()) force_pressure_ready_fault_bypass = (bool)d["forcePressureReadyFaultBypass"];
  if(!d["forcePressureReadyBoostHoldBypass"].isNull()) force_pressure_ready_boost_hold_bypass = (bool)d["forcePressureReadyBoostHoldBypass"];
  // Serial controls are kept writable from the user interface so bench sessions can adjust verbosity and cadence.
  if(!d["sdbg"].isNull())   serial_debug_enable   = (bool)d["sdbg"];
  if(!d["sper"].isNull())   serial_status_period_ms = (uint32_t)max(0, (int)d["sper"]);
  // Spray curve points (must remain monotonic by kilopascal) are applied here.
  JsonArray a = d["curve"];
  if(!a.isNull()){
    for(int i=0;i<CURVE_PTS && i<(int)a.size();i++){
      JsonObject p = a[i];
      float k = p["kpa"] | curve_kpa[i];
      float c = p["pct"] | curve_duty[i];
      curve_kpa[i]  = constrain(k, 0.0f, 600.0f);
      curve_duty[i] = constrain(c, 0.0f, 100.0f);
    }
  }
  // Enforce strictly increasing manifold absolute pressure values for the spray curve.
  // This prevents zero-width segments that can cause divide-by-zero during interpolation.
  for(int i=1;i<CURVE_PTS;i++){
    if(curve_kpa[i] <= curve_kpa[i-1]){
      curve_kpa[i] = min(600.0f, curve_kpa[i-1] + 0.1f);
    }
  }
  // Persist all settings to flash storage.
  savePrefs();

  JsonDocument out; // Response payload sent after configuration apply and save.
  out["ok"] = true; // Simple acknowledge flag consumed by user interface save flow.
  String sOut; serializeJson(out, sOut); // Serialize JavaScript Object Notation document to string.
  server.send(200, "application/json", sOut); // Return success response before optional reboot.
  // If wireless network settings changed, reboot to apply them cleanly.
  if(wifiChanged){
    Serial.println("[WIFI] settings changed - rebooting to apply...");
    delay(300);
    ESP.restart();
  }
}
// Captive portal handlers are used so phones show the web page immediately.
static void handleCaptiveOk(){
  // Return a minimal success payload for probe endpoints that only need an HTTP 200 response.
  server.send(200, "text/plain", "OK");
}
static void handleCaptive(){
  // Redirect all unknown captive-portal paths to the controller root page.
  IPAddress targetIp = server.client().localIP();
  if(targetIp[0] == 0){
    if(WiFi.isConnected() && WiFi.localIP()[0] != 0) targetIp = WiFi.localIP();
    else targetIp = WiFi.softAPIP();
  }
  server.sendHeader("Location", String("http://") + targetIp.toString(), true);
  server.send(302, "text/plain", "");
}
// ================================================================
