// ========================== CONTROL LOOP =========================
// ================================================================
//
// This task is the heart of the software.
// It continuously:
// 1) Reads sensors,
// 2) Applies safety rules,
// 3) Computes spray duty,
// 4) Updates pulse width modulation timers for the outputs.

// These event trackers ensure we only print state changes once.
// This keeps the serial monitor readable.
static bool lastLevelLow    = false;
static bool lastTimingCut   = false;
static bool lastFaultLatched= false;
static bool lastPressureReadyFaultLatched = false;
static bool lastPressureReady = false;
static bool lastPumpOn      = false;
// Spray request hysteresis reduces flicker around the duty threshold.
static bool sprayReqHold = false;
// Spray request debounce removes fast on/off chatter when the MAP signal is noisy around curve start.
// The on-delay is short so spray still responds quickly when boost rises.
// The off-delay is a little longer so small dips do not rapidly toggle relays.
static bool sprayReqDebounced = false;
static uint32_t sprayReqDebounceStartMs = 0;
static const uint32_t sprayReqOnDebounceMs = 200;
static const uint32_t sprayReqOffDebounceMs = 800;
// dP helper declarations live in DpMonitor.ino.
// Keeping declarations here makes control flow explicit for anyone reading this file first.
static bool dpPendingActive(bool dpFaultNow);
static void updateDpMonitor(float dpPsiNow, bool dpArmCondition, uint32_t nowMs, bool &faultBefore, bool &latchedByCritical);
static void updateDpStateMachine(bool dpFaultNow, bool dpBoostHoldNow);
static const char* timingCutReasonText(bool levelLowEmergency);
static void controlTask(void*){
  // This is used to throttle status prints so we do not spam serial output.
  uint32_t lastPrint = 0;

  for(;;){
    // Sensor reads always come first.
    // Read raw analog to digital converter values, undo the voltage divider, then apply linear and offset math.
    // This gives us manifold absolute pressure in kilopascal and rail pressure in pounds per square inch.
    float mapV_adc_raw  = adcVolts(PIN_ADC_MAP);   // Raw volts at manifold pressure analog to digital converter pin.
    float railV_adc_raw = adcVolts(PIN_ADC_RAIL);  // Raw volts at rail pressure analog to digital converter pin.
    // Apply one shared board-level analog to digital converter gain and offset correction before divider math.
    // A shared correction is used because both inputs run through the same analog to digital converter front-end behavior.
    // Manufacturer sensor linear and offset values remain untouched while this layer compensates board-level measurement bias.
    // The corrected value is intentionally not clamped again so zero and full-scale endpoint calibration remains meaningful.
    // Raw reads are already bounded in adcVolts, and calibration sanitation constrains gain and offset ranges.
    const float mapV_adc_linear  = (mapV_adc_raw  * adc_input_gain) + adc_input_offset_v;
    const float railV_adc_linear = (railV_adc_raw * adc_input_gain) + adc_input_offset_v;
    // Apply shared piecewise analog correction after linear shared trim.
    // This corrects board-specific midrange bow that cannot be removed by gain and offset alone.
    float mapV_adc  = applyAdcCurveCorrection(mapV_adc_linear);
    float railV_adc = applyAdcCurveCorrection(railV_adc_linear);

    // Undo the divider to get sensor-side volts.
    // The checks prevent divide-by-zero if the ratio is misconfigured.
    float mapV_sens  = (map_div_ratio  > 0.01f) ? (mapV_adc  / map_div_ratio)  : 0.0f;
    float railV_sens = (rail_div_ratio > 0.01f) ? (railV_adc / rail_div_ratio) : 0.0f;
    // Convert volts to engineering units using each sensor's calibration format.
    // MAP uses plain five-volt sensor calibration: linear span over zero to five volts plus offset.
    // Rail pressure sensor uses direct slope in gauge pounds per square inch per volt plus offset.
    float kpa_raw = map_kpa_per_v * mapV_sens + map_kpa_offset_hpt;
    float psi = rail_psi_per_v * railV_sens + rail_psi_offset;
    const float baroPsiRef = effectiveBaroPsi(); // Active barometric reference used for differential pressure conversion (boot capture unless force override is active).
    // Apply test overrides before filtering so forced values drive the outputs.
    if(force_map_kpa >= 0.0f) kpa_raw = force_map_kpa; // Forced manifold pressure test value overrides sensor math.
    if(force_rail_psi >= 0.0f) psi = force_rail_psi;   // Forced rail pressure test value overrides sensor math.
    if(force_dp_psi >= 0.0f){
      const float mapPsi = kpa_raw * 0.1450377377f; // Convert forced manifold pressure from kilopascal to pounds per square inch.
      psi = (mapPsi + force_dp_psi) - baroPsiRef;   // Rebuild forced rail gauge pressure from forced injector differential pressure plus manifold pressure minus the active barometric reference.
    }
    // Test overrides let you force values for bench troubleshooting.
    mapKpaRaw = kpa_raw;
    // Low-pass filter on MAP and rail pressure for smoother control behavior.
    // Uses a simple exponential moving average.
    static bool mapFilterInit = false;
    static bool railFilterInit = false;
    const float alpha = 0.15f; // Filter strength (higher means faster response and less smoothing).
    if(!mapFilterInit){
      mapKpaFilt = kpa_raw;
      mapFilterInit = true;
    }else{
      mapKpaFilt = (mapKpaFilt * (1.0f - alpha)) + (kpa_raw * alpha);
    }
    if(!railFilterInit){
      railPsiFilt = psi;
      railFilterInit = true;
    }else{
      railPsiFilt = (railPsiFilt * (1.0f - alpha)) + (psi * alpha);
    }
    // Publish live sensor values for the user interface and serial status.
    mapKpa  = mapKpaFilt;
    railPsi = railPsiFilt;
    // Level switch processing is debounced.
    // Default assumption: a normally closed float switch means a high logic level indicates low level.
    // You can flip the polarity in the user interface if your wiring is different.
    const bool levelPinHigh = (digitalRead(PIN_LEVEL_SWITCH) == HIGH);
    bool levelLowRaw = level_active_high ? levelPinHigh : !levelPinHigh;
    bool levelLow = false;
    // Debounce edges before accepting a new level state.
    uint32_t now = millis();
    if(levelLowRaw != levelRawLast){
      levelRawLast = levelLowRaw;
      levelLastChangeMs = now;
    }
    if((now - levelLastChangeMs) >= level_debounce_ms){
      levelLowState = levelLowRaw;
    }
    levelLow = levelLowState;
    // Test overrides for bench testing.
    if(force_level_mode == 1){ // Force low level regardless of the switch.
      levelLow = true;
      levelLowState = true;
      levelRawLast = true;
    }else if(force_level_mode == 2){ // Force high level regardless of the switch.
      levelLow = false;
      levelLowState = false;
      levelRawLast = false;
    }
    // Spray command computation includes curve lookup and force overrides.
    // Force modes are explicit bench overrides.
    float rawCurve = curveLookupDuty(mapKpaFilt); // This is zero to one hundred percent from the curve.
    float dutyCmd  = 0.0f;
    const bool injectorsForcedOff = (force_inj_mode == 1); // Bench override that always closes injectors.
    const bool injectorsForcedOn = (force_inj_mode == 2);  // Bench override that always commands injector duty.
    const float forcedInjectorDuty = clampf(force_duty, 0.0f, 100.0f); // Forced duty is bounded only by physical zero to one hundred percent limits.
    if(injectorsForcedOff){
      dutyCmd = 0.0f; // Force OFF means zero injector duty.
    }else if(injectorsForcedOn){
      dutyCmd = forcedInjectorDuty; // Force ON means use the forced duty directly.
    }else{
      // In normal mode, curve duty is clamped by the duty clamp slider.
      dutyCmd = clampf(rawCurve, 0.0f, duty_max);
    }
    // Curve duty is saved separately so the user interface can show it directly.
    float curvePctNow = clampf(rawCurve, 0.0f, 100.0f);
    if(injectorsForcedOn){
      // Forced injector command must apply immediately and must not wait for spray-request debounce.
      sprayReqHold = (dutyCmd > 0.0f);
      sprayReqDebounced = sprayReqHold;
      sprayReqDebounceStartMs = 0;
    }else{
      if(dutyCmd > 0.5f) sprayReqHold = true;       // Set spray request once duty rises above the high threshold.
      else if(dutyCmd < 0.2f) sprayReqHold = false; // Clear spray request only after duty falls below the low threshold.
      // Debounce the hysteresis output to prevent relay chatter when MAP jitters around curve start.
      if(sprayReqHold != sprayReqDebounced){
        if(sprayReqDebounceStartMs == 0){
          sprayReqDebounceStartMs = now; // Start timing the candidate edge.
        }
        const uint32_t debounceMs = sprayReqHold ? sprayReqOnDebounceMs : sprayReqOffDebounceMs; // Use asymmetric delays for better driveability.
        if((now - sprayReqDebounceStartMs) >= debounceMs){
          sprayReqDebounced = sprayReqHold; // Accept edge only after it stayed stable long enough.
          sprayReqDebounceStartMs = 0;      // Reset edge timer after acceptance.
        }
      }else{
        sprayReqDebounceStartMs = 0; // Candidate disappeared, so clear pending debounce timer.
      }
    }
    bool sprayRequested = sprayReqDebounced; // Use debounced spray request for pressure-build and output decisions.
    // Compute injector differential pressure from the current filtered readings.
    // This value is used both for pressure-ready spray gating and for dP safety.
    const float mapPsiNow = mapKpaFilt * 0.1450377377f;       // Convert filtered manifold pressure from kilopascal absolute to pounds per square inch absolute.
    const float dpPsiNow = (railPsi + baroPsiRef) - mapPsiNow; // Injector differential pressure equals rail gauge pressure plus active barometric pressure minus manifold absolute pressure.
    // Low level should only trigger timing cut if it first happens during active spray.
    // Once triggered, hold that low-level timing-cut intent while level remains low.
    // This prevents a one-loop timing-cut pulse from clearing immediately when pressure-ready resets.
    const bool sprayActiveForSafety = sprayRequested && pressureReady; // True only when spray demand exists and pressure-ready gate is satisfied.
    if(levelLow && sprayActiveForSafety){
      levelLowSprayTimingLatched = true; // Latch low-level timing-cut trigger when low level is detected during active spray.
    }
    if(!levelLow){
      levelLowSprayTimingLatched = false; // Clear low-level timing-cut latch as soon as level returns to OK.
    }
    // Safety logic chooses the active mode.
    // Timing cut is active if effective differential pressure faults are active,
    // or if low level was detected during active spray and level is still low.
    const bool levelLowEffective = levelLow && !force_level_fault_bypass; // Optional level bypass can suppress low-level safety effects without changing the raw level input.
    const bool dpFaultEffective = !force_dp_fault_bypass && (faultLatched || force_dp_fault); // Rail differential pressure timing-cut source after dedicated rail differential pressure bypass.
    const bool pressureReadyFaultEffective = !force_pressure_ready_fault_bypass && (pressureReadyFaultLatched || force_pressure_ready_fault); // Pressure-ready timeout timing-cut source after dedicated pressure-ready bypass.
    const bool dpBoostHoldEffective = !force_dp_boost_hold_bypass && (dpBoostCutHold || force_dp_boost_hold); // Rail differential pressure retained boost-hold source after dedicated rail hold bypass.
    const bool pressureReadyBoostHoldEffective = !force_pressure_ready_boost_hold_bypass && (pressureReadyBoostCutHold || force_pressure_ready_boost_hold); // Pressure-ready retained boost-hold source after dedicated pressure-ready hold bypass.
    bool levelLowEmergency = levelLowEffective && levelLowSprayTimingLatched; // Latched low-level emergency remains active while effective low level persists.
    bool timingCutActive = dpFaultEffective || pressureReadyFaultEffective || levelLowEmergency; // Timing cut activates from effective fault sources or effective low-level emergency.
    if(force_timing_cut) timingCutActive = true; // Force the timing cut latch on for testing.
    modeNow = timingCutActive ? MODE_IAT_CUT : MODE_NORMAL; // Publish active operating mode for all downstream logic.
    // Safety state is updated here.
    // This also drives the boost cut decision and timing cut output.
    timingCutOn = timingCutActive;              // Differential pressure fault or low level while spraying.
    // Build one explicit boost-cut request using current-cycle safety inputs.
    // This avoids relying on the previous loop's boost output state.
    const bool boostCutRequestedPreDp = levelLowEffective || timingCutActive || force_boost_cut || dpBoostHoldEffective || pressureReadyBoostHoldEffective;
    // Spray enable gating is pressure-based.
    // The pump must build injector differential pressure to the configured target before injectors can open.
    // The target uses the Target Injector dP setting so users can set one pressure goal for both flow reference and spray enable.
    // Pressure-build timing is tied to spray request and safety state, not to pump force mode.
    // This allows bench testing to force the pump off and still verify timeout fault behavior.
    const bool pressureBuildRequested = (modeNow == MODE_NORMAL) && sprayRequested && !boostCutRequestedPreDp;
    const bool pressureReadyGateBypassed = force_pressure_ready_override || injectorsForcedOn; // Forced injectors and explicit pressure-ready override both bypass pressure-ready injector blocking and timeout fault latching.
    const float sprayEnableDpTargetPsi = max(0.0f, flow_ref_dp_psi); // Zero means no pressure gate.
    const bool pressureReadyForSpray = (sprayEnableDpTargetPsi <= 0.0f) ? true : (dpPsiNow >= sprayEnableDpTargetPsi);
    if(!pressureBuildRequested){
      pressureReady = false;      // Pressure-ready state is reset whenever spray request is no longer valid.
      pressureReadySinceMs = 0;   // Clear the build timer when no spray request is active.
    }else if(pressureReadyGateBypassed){
      // Bench override behavior: treat pressure-ready as satisfied while spray is requested.
      // This allows injector output testing from MAP demand without waiting for differential pressure target.
      pressureReady = true;
      pressureReadySinceMs = 0;
    }else if(!pressureReady){
      if(pressureReadyForSpray){
        pressureReady = true;       // Latch pressure-ready once target dP is reached during an active spray request.
        pressureReadySinceMs = 0;   // Clear the build timer once pressure target is reached.
      }else{
        if(pressureReadySinceMs == 0){
          pressureReadySinceMs = now; // Start timing how long the system has been trying to build pressure.
        }
        // If pressure cannot reach the target in time, latch a real fault so timing cut and retained boost hold engage.
        if(!pressureReadyFaultLatched && pressure_ready_timeout_ms > 0 && (now - pressureReadySinceMs) >= pressure_ready_timeout_ms){
          pressureReadyFaultLatched = true;
          faultLatchedAtMs = now; // Start the shared timing-cut auto-clear timer for this newly latched pressure-ready timeout fault.
          pressureReadyBoostCutHold = true;
          timingCutActive = true;
          timingCutOn = true;
          modeNow = MODE_IAT_CUT;
          pressureReadySinceMs = 0;
          if(serial_debug_enable){
            Serial.printf("[FAULT] pressure_ready_timeout dp=%.1f target=%.1f timeout=%lu ms\n",
              (double)dpPsiNow, (double)sprayEnableDpTargetPsi, (unsigned long)pressure_ready_timeout_ms);
          }
        }
      }
    }
    // Commands are chosen by mode.
    // In timing cut, pump and injectors are off.
    // In normal mode, we use curve duty with pressure-ready gating and overrides.
    bool localPump = false;
    if(modeNow == MODE_IAT_CUT){
      cmdDuty = 0.0f;   // Timing cut mode always closes injectors.
      localPump = false; // Timing cut mode always turns the pump off.
    }else{
      curvePctLive = curvePctNow; // Publish raw curve value for live gauge visibility.
      // Apply the debounced spray request gate before commanding outputs.
      // This prevents pump and injector chatter when MAP noise causes fast duty oscillation near curve start.
      cmdDuty = sprayRequested ? dutyCmd : 0.0f; // Hold duty at zero until spray request has been stable for debounce timing.
      localPump = pump_in_spray_mode && sprayRequested; // Normal pump request follows the debounced spray request.
    }
    // Output force modes are absolute bench overrides.
    if(force_pump_mode == 2){
      localPump = true; // Force pump ON even when automatic safety logic would normally block it.
    }else if(force_pump_mode == 1){
      localPump = false; // Force pump OFF regardless of automatic requests.
    }
    // Any active boost-cut request forces pump and injectors off immediately.
    // This uses current safety inputs, not the previous loop's boost output.
    if(boostCutRequestedPreDp){
      if(!injectorsForcedOn){
        cmdDuty = 0.0f; // Normal injector path is blocked by boost-cut conditions.
      }
      if(force_pump_mode != 2){
        localPump = false; // Normal pump path is blocked by boost-cut conditions.
      }
    }
    // Enforce pressure-ready gating before allowing injector duty greater than zero.
    if(!pressureReadyGateBypassed && sprayRequested && cmdDuty > 0.0f && !pressureReady){
      localPump = true;  // Run the pump while building to the pressure target.
      cmdDuty = 0.0f;    // Hold injectors closed until pressure-ready is achieved.
    }
    pumpOn = localPump; // Publish final pump output command.
    gpioFast(PIN_PUMP_SSR, pumpOn); // Drive pump solid state relay output pin.
    if(serial_debug_enable){
      if(pressureReady != lastPressureReady){
        Serial.printf("[EVENT] pressure_ready %s\n", pressureReady ? "DONE" : "RESET");
        lastPressureReady = pressureReady;
      }
      if(pumpOn != lastPumpOn){
        Serial.printf("[EVENT] pump %s\n", pumpOn ? "ON" : "OFF");
        lastPumpOn = pumpOn;
      }
    }
    // Differential pressure monitor now runs through helper functions in DpMonitor.ino.
    // Keeping this call site short makes the control loop easier to understand.
    bool faultBefore = faultLatched;                       // Snapshot used to detect new real dP latch edges.
    bool latchedByCritical = false;                        // True only when the critical low-pressure path caused this new latch.
    const uint32_t dpNowMs = millis();                     // One timestamp for this monitor pass.
    const bool dpArmCondition = (modeNow == MODE_NORMAL) && pumpOn && pressureReady && (cmdDuty > dp_arm_duty_pct); // Arm gate for dP checks.
    updateDpMonitor(dpPsiNow, dpArmCondition, dpNowMs, faultBefore, latchedByCritical); // Run dP arm, fault paths, and retention.
    // Shared timing-cut auto-clear path:
    // one timer setting and one timer state are used for any real timing-cut latch source.
    // Retained boost-hold latches still remain latched until power cycle unless bypassed.
    const bool anyRealTimingCutLatch = faultLatched || pressureReadyFaultLatched;
    if(anyRealTimingCutLatch && timing_cut_auto_clear_ms > 0){
      if(faultLatchedAtMs == 0){
        faultLatchedAtMs = dpNowMs; // Capture the first active timing-cut latch timestamp for shared auto-clear timing.
      }
      if((dpNowMs - faultLatchedAtMs) >= timing_cut_auto_clear_ms){
        const bool hadRailDpLatch = faultLatched;
        const bool hadPressureReadyLatch = pressureReadyFaultLatched;
        faultLatched = false; // Clear rail differential-pressure timing-cut latch.
        pressureReadyFaultLatched = false; // Clear pressure-ready timeout timing-cut latch.
        faultLatchedAtMs = 0; // Reset shared timing-cut auto-clear timer.
        railLowSince = 0; // Clear normal low-pressure timer so next dP arm starts clean.
        railCriticalSince = 0; // Clear critical low-pressure timer so next dP arm starts clean.
        dpLowForMsNow = 0; // Clear published dP low-time telemetry after auto-clear.
        if(serial_debug_enable){
          if(hadRailDpLatch && hadPressureReadyLatch){
            Serial.printf("[EVENT] timing_cut AUTO_CLEAR (rail dP + pressure-ready) after %u ms\n", (unsigned)timing_cut_auto_clear_ms);
          }else if(hadPressureReadyLatch){
            Serial.printf("[EVENT] pressure_ready_timing_cut AUTO_CLEAR after %u ms\n", (unsigned)timing_cut_auto_clear_ms);
          }else{
            Serial.printf("[EVENT] timing_cut AUTO_CLEAR after %u ms\n", (unsigned)timing_cut_auto_clear_ms);
          }
        }
      }
    }else if(!anyRealTimingCutLatch){
      faultLatchedAtMs = 0; // Keep shared timer cleared when no real timing-cut latch is active.
    }

    // Log exactly when a new real dP fault latches and which path caused it.
    if(!faultBefore && faultLatched && serial_debug_enable){
      if(latchedByCritical){
        Serial.printf("[FAULT] Rail differential pressure critical low (%.1f psi) for %u ms -> timing cut latched\n",
          (double)dpPsiNow, (unsigned)dp_critical_ms);
      }else{
        Serial.printf("[FAULT] Rail differential pressure low (%.1f psi) for %u ms -> timing cut latched\n",
          (double)dpPsiNow, (unsigned)rail_fault_delay_ms);
      }
    }
    if(serial_debug_enable && (faultLatched != lastFaultLatched)){
      Serial.printf("[EVENT] fault_latch %s\n", faultLatched ? "ON" : "OFF");
      lastFaultLatched = faultLatched;
    }
    if(serial_debug_enable && (pressureReadyFaultLatched != lastPressureReadyFaultLatched)){
      Serial.printf("[EVENT] pressure_ready_fault_latch %s\n", pressureReadyFaultLatched ? "ON" : "OFF");
      lastPressureReadyFaultLatched = pressureReadyFaultLatched;
    }
    // Re-evaluate timing-cut state immediately after differential pressure monitor update.
    // This guarantees a newly latched real differential pressure fault turns timing cut on in the same control-loop pass.
    const bool dpFaultEffectivePostDp = !force_dp_fault_bypass && (faultLatched || force_dp_fault); // Recompute effective rail differential-pressure fault after monitor update.
    const bool pressureReadyFaultEffectivePostDp = !force_pressure_ready_fault_bypass && (pressureReadyFaultLatched || force_pressure_ready_fault); // Recompute effective pressure-ready timeout fault after monitor update.
    const bool dpBoostHoldEffectivePostDp = !force_dp_boost_hold_bypass && (dpBoostCutHold || force_dp_boost_hold); // Recompute effective retained rail differential-pressure boost hold after monitor update.
    const bool pressureReadyBoostHoldEffectivePostDp = !force_pressure_ready_boost_hold_bypass && (pressureReadyBoostCutHold || force_pressure_ready_boost_hold); // Recompute effective retained pressure-ready boost hold after monitor update.
    timingCutActive = dpFaultEffectivePostDp || pressureReadyFaultEffectivePostDp || levelLowEmergency;
    if(force_timing_cut){
      timingCutActive = true;
    }
    modeNow = timingCutActive ? MODE_IAT_CUT : MODE_NORMAL;
    // Safety outputs are updated here.
    // Timing cut grounds the intake air temperature input and cuts boost.
    // The intake air temperature relay output is wired failsafe, so the healthy electrical state stays high and timing cut pulls the line low.
    // Low level cuts boost only.
    timingCutOn = timingCutActive; // Publish final timing cut state for status and output logic.
    const bool boostCutRequestedNow = levelLowEffective || timingCutOn || force_boost_cut || dpBoostHoldEffectivePostDp || pressureReadyBoostHoldEffectivePostDp; // Final boost-cut request after monitor and pressure-ready paths are updated this loop.
    if(boostCutRequestedNow){
      // Force outputs are absolute for bench testing.
      // Normal outputs are still synchronized to safety state changes in this same pass.
      if(!injectorsForcedOn){
        cmdDuty = 0.0f;
      }
      if(force_pump_mode != 2){
        if(pumpOn){
          pumpOn = false;
          gpioFast(PIN_PUMP_SSR, pumpOn);
        }
      }
    }
    // Final injector force application happens after all automatic safety calculations so force mode stays absolute.
    if(injectorsForcedOn){
      cmdDuty = forcedInjectorDuty;
    }else if(injectorsForcedOff){
      cmdDuty = 0.0f;
    }
    iatGroundOn  = timingCutOn;                       // Logical timing-cut request: true means the intake air temperature line must be grounded to pull timing.
    boostOn      = !boostCutRequestedNow;             // Boost is cut when any boost-cut request source is active.
    const bool dpFaultNow = dpFaultEffectivePostDp;
    const bool dpBoostHoldNow = dpBoostHoldEffectivePostDp;
    updateDpStateMachine(dpFaultNow, dpBoostHoldNow); // Publish one normalized dP state for UI and serial readers.
    gpioFast(PIN_IAT_SSR, iat_ssr_active_high ? iatGroundOn : !iatGroundOn); // Drive the failsafe timing-cut relay using configured polarity (healthy high, timing-cut low).
    gpioFast(PIN_BOOST_SSR, boost_ssr_active_high ? boostOn : !boostOn);      // Drive boost relay using configured polarity.
    // Injector pulse width modulation timing is updated here.
    // The timer callback uses these values to toggle the injector solid state relay.
    float hz = clampf(inj_pwm_hz, 1.0f, 500.0f);  // Constrain injector pulse width modulation frequency to safe bounds.
    uint32_t per = (uint32_t)(1000000.0f / hz);   // Convert frequency to period in microseconds.
    uint32_t on  = (uint32_t)(per * (cmdDuty / 100.0f)); // Convert duty percent to on-time in microseconds.
    if((!boostOn || timingCutActive) && !injectorsForcedOn) on = 0; // Keep normal injector safety cut behavior, but do not block explicit force-on bench command.
    if(on > per) on = per;                        // Clamp on-time to period to avoid invalid timing.
    // LEDs are updated after final injector and pump output decisions are known.
    // Blue LED means low level only.
    // Red LED means boost cut is active.
    // Green LED means methanol is actually being sprayed, so it requires both pump output and injector on-time.
    blueLedOn  = levelLow;               // Blue LED mirrors low level condition only.
    redLedOn   = !boostOn;               // Red LED mirrors boost cut condition only.
    greenLedOn = pumpOn && (on > 0);     // Green LED mirrors the final real spray output state.
    gpioFast(PIN_BLUE_LED, blueLedOn);   // Drive blue LED output pin.
    gpioFast(PIN_RED_LED, redLedOn);     // Drive red LED output pin.
    gpioFast(PIN_GREEN_LED, greenLedOn); // Drive green LED output pin.
    injPeriodUs = per;                            // Publish period for timer callback.
    injOnUs     = on;                             // Publish on-time for timer callback.
    // Serial status output is throttled.
    if(serial_debug_enable){
      uint32_t now = millis();
      if(now - lastPrint >= serial_status_period_ms){
        lastPrint = now;
        const bool dpPending = dpPendingActive(dpFaultNow); // Pending means delay timer running while armed and no fault latched.
        const char* faultState = (faultLatched || pressureReadyFaultLatched) ? "ACTIVE" : "OK";
        Serial.printf(
          "[STAT] mode=%s dpState=%s fault=%s map=%.1fkPa mapRaw=%.1fkPa rail=%.1fpsi dp=%.1fpsi duty=%.1f%% curve=%.1f%% pump=%d injOn=%d boostCut=%d timingCut=%d lvlLow=%d lvlRaw=%d redLed=%d blueLed=%d greenLed=%d dpLatch=%d prLatch=%d dpPend=%d dpLowMs=%lu dpMin=%.1f arm=%d armSettleMs=%lu ready=%d injHz=%.1f injOnUs=%lu fPump=%u fInj=%u fLvl=%u fMap=%.1f fRail=%.1f fDp=%.1f ovrDp=%d ovrPr=%d bypLvl=%d bypDp=%d bypPr=%d bypHold=%d bypPrHold=%d\n",
          modeName((Mode)modeNow),
          dpStateName(dpStateNow),
          faultState,
          (double)mapKpa,
          (double)mapKpaRaw,
          (double)railPsi,
          (double)(railPsi - (mapKpaFilt*0.1450377377f)),
          (double)cmdDuty,
          (double)curvePctLive,
          pumpOn ? 1 : 0,
          (cmdDuty > 0.5f) ? 1 : 0,
          boostOn ? 0 : 1,
          timingCutOn ? 1 : 0,
          levelLow ? 1 : 0,
          levelRawLast ? 1 : 0,
          redLedOn ? 1 : 0,
          blueLedOn ? 1 : 0,
          greenLedOn ? 1 : 0,
          faultLatched ? 1 : 0,
          pressureReadyFaultLatched ? 1 : 0,
          dpPending ? 1 : 0,
          (unsigned long)dpLowForMsNow,
          (double)dpMinSeenPsiNow,
          dpArmedNow ? 1 : 0,
          (unsigned long)dpArmSettleRemainingMsNow,
          pressureReady ? 1 : 0,
          (double)inj_pwm_hz,
          (unsigned long)injOnUs,
          (unsigned)force_pump_mode,
          (unsigned)force_inj_mode,
          (unsigned)force_level_mode,
          (double)force_map_kpa,
          (double)force_rail_psi,
          (double)force_dp_psi,
          force_dp_monitor_override ? 1 : 0,
          force_pressure_ready_override ? 1 : 0,
          force_level_fault_bypass ? 1 : 0,
          force_dp_fault_bypass ? 1 : 0,
          force_pressure_ready_fault_bypass ? 1 : 0,
          force_dp_boost_hold_bypass ? 1 : 0,
          force_pressure_ready_boost_hold_bypass ? 1 : 0
        );
        if(levelLow != lastLevelLow){
          Serial.printf("[EVENT] level_low %s\n", levelLow?"ON":"OFF");
          lastLevelLow = levelLow;
        }
        if(timingCutOn != lastTimingCut){
          const char* tcReason = timingCutReasonText(levelLowEmergency); // Keep timing-cut reason strings centralized in DpMonitor.ino.
          Serial.printf("[EVENT] timing_cut %s reason=%s dp=%.1f armDuty=%.1f\n",
            timingCutOn?"ON":"OFF",
            tcReason,
            (double)(railPsi - (mapKpaFilt*0.1450377377f)),
            (double)dp_arm_duty_pct
          );
          lastTimingCut = timingCutOn;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
// ================================================================
