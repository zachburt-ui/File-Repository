// ======================= DIFFERENTIAL PRESSURE ====================
// ================================================================
//
// This tab intentionally contains only injector differential pressure safety logic.
// Isolating this logic in one tab makes safety behavior easier to read and audit.
//
// Definitions used in this file:
// - Differential pressure = rail gauge pressure plus active barometric reference minus manifold absolute pressure.
//   Active barometric reference is the boot capture unless Test Section Force Baro override is enabled.
// - Armed = spray conditions are valid and pressure monitoring is allowed.
// - Pending = differential pressure is low but delay timer has not yet latched a fault.
// - Fault latched = timing cut safety is active from a real pressure fault.
// - Boost hold = boost remains cut until power cycle after a real pressure fault.
//
// Manual bench-test latches remain outside this file.
// This file only controls real differential-pressure-derived latches and timers.

// Return true when the monitor is waiting out the normal low-pressure delay.
// This helper is shared by serial status printing and API status reporting.
static bool dpPendingActive(bool dpFaultNow){
  // Pending requires an active low-pressure timer,
  // no current differential pressure fault,
  // and an armed monitor.
  return (railLowSince > 0) && !dpFaultNow && dpArmedNow;
}

// Reset arm and fault window timers.
// This function does not clear retained boost hold, by design.
static void resetDpMonitorWindow(){
  dpArmSinceMs = 0;                  // Clear the arm-delay start timestamp.
  dpArmedNow = false;                // Publish that monitoring is not armed.
  dpArmSettleRemainingMsNow = 0;     // No delay countdown while disarmed.
  railLowSince = 0;                  // Clear normal low-pressure timer start.
  railCriticalSince = 0;             // Clear critical low-pressure timer start.
  dpLowForMsNow = 0;                 // Clear live low-pressure elapsed time.
  dpMinSeenPsiNow = -1.0f;           // Reset tracked minimum differential pressure for next arm window.
}

// Update monitor arming state using the configured dP delay.
// Monitoring is only "armed" after the arm condition remains true for the full delay.
static void updateDpArmState(bool dpArmCondition, uint32_t nowMs){
  if(dpArmCondition){
    // Start delay timer on first cycle where arm condition is true.
    if(dpArmSinceMs == 0){
      dpArmSinceMs = nowMs;
    }

    // Measure how long arm condition has remained continuously true.
    const uint32_t armElapsed = nowMs - dpArmSinceMs;

    // Mark monitor armed only after the configured dP delay has fully elapsed.
    if(armElapsed >= dp_arm_settle_ms){
      dpArmedNow = true;
      dpArmSettleRemainingMsNow = 0;
    }else{
      dpArmedNow = false;
      dpArmSettleRemainingMsNow = dp_arm_settle_ms - armElapsed;
    }
  }else{
    // Any loss of arm condition fully resets the monitor window.
    resetDpMonitorWindow();
  }
}

// Evaluate both fault paths while monitoring is armed.
// The normal path uses the long delay.
// The critical low-pressure path uses a fast delay.
static void evaluateDpFaultPaths(float dpPsiNow, bool dpArmCondition, uint32_t nowMs, bool &latchedByCritical){
  latchedByCritical = false; // Default to normal path unless the critical low-pressure path latches first.

  if(dpArmedNow){
    // Track minimum observed differential pressure during this armed window for diagnostics.
    if(dpMinSeenPsiNow < -0.5f || dpPsiNow < dpMinSeenPsiNow){
      dpMinSeenPsiNow = dpPsiNow;
    }

    // ---------------- Critical path ----------------
    const bool criticalEnabled = dp_critical_psi > 0.0f;                      // Zero disables the critical low-pressure path.
    const bool criticalLow = criticalEnabled && (dpPsiNow < dp_critical_psi); // True when differential pressure is below critical threshold.

    if(criticalLow){
      if(railCriticalSince == 0){
        railCriticalSince = nowMs; // Start critical timer on first low sample.
      }
      if(!faultLatched && (nowMs - railCriticalSince) >= dp_critical_ms){
        faultLatched = true;        // Latch real differential pressure fault.
        latchedByCritical = true;   // Record which path caused this latch edge.
      }
    }else if(!criticalEnabled || dpPsiNow > (dp_critical_psi + dp_recover_margin_psi)){
      // Reset critical timer when the critical low-pressure path is disabled or pressure recovers with margin.
      railCriticalSince = 0;
    }

    // ---------------- Normal path ----------------
    if(dpPsiNow < min_dp_psi){
      if(railLowSince == 0){
        railLowSince = nowMs; // Start normal low-pressure timer on first low sample.
      }

      dpLowForMsNow = nowMs - railLowSince; // Publish running low-pressure time for UI and serial monitor.

      if(!faultLatched && dpLowForMsNow >= rail_fault_delay_ms){
        faultLatched = true; // Latch real differential pressure fault after configured delay.
      }
    }else if(dpPsiNow > (min_dp_psi + dp_recover_margin_psi)){
      // Pressure recovered with margin, so clear pending timer and elapsed time.
      railLowSince = 0;
      dpLowForMsNow = 0;
    }else if(railLowSince > 0){
      // Pressure is in the hysteresis band, so keep timer running.
      dpLowForMsNow = nowMs - railLowSince;
    }else{
      // No low timer has started, so elapsed time must be zero.
      dpLowForMsNow = 0;
    }
  }else if(dpArmCondition){
    // Arm condition is true but the configured delay is not complete.
    // Keep fault timers clear so we do not accumulate fault time before monitor is armed.
    railLowSince = 0;
    railCriticalSince = 0;
    dpLowForMsNow = 0;
    dpMinSeenPsiNow = -1.0f;
  }
}

// Execute one complete differential pressure monitor update.
// This is called once per control loop pass.
static void updateDpMonitor(float dpPsiNow, bool dpArmCondition, uint32_t nowMs, bool &faultBefore, bool &latchedByCritical){
  faultBefore = faultLatched;   // Snapshot previous real fault state to detect new latch edges.
  latchedByCritical = false;    // Initialize path marker before evaluating fault logic.

  // Test override can pause automatic differential pressure monitoring.
  // This is useful on the bench when you need spray behavior without automatic differential pressure fault evaluation.
  if(force_dp_monitor_override){
    resetDpMonitorWindow(); // Keep monitor timers and telemetry in a clean disarmed state while override is enabled.
    return; // Skip arm and fault path evaluation until override is turned off.
  }

  updateDpArmState(dpArmCondition, nowMs);                               // Step one: update arm state and delay countdown.
  evaluateDpFaultPaths(dpPsiNow, dpArmCondition, nowMs, latchedByCritical); // Step two: run critical and normal fault paths.

  if(!faultBefore && faultLatched){
    // Capture which real path caused this latch so status can identify normal-low versus critical-low behavior.
    railDpLastRealFaultPath = latchedByCritical ? RAIL_DP_PATH_CRITICAL_LOW : RAIL_DP_PATH_NORMAL_LOW;
    faultLatchedAtMs = nowMs; // Start timing-cut auto-clear timer at the exact latch edge.
    dpBoostCutHold = true;    // Retain boost cut until power cycle for real differential pressure faults.
  }
}

// Publish one simplified monitor state for user interface and serial status consumers.
static void updateDpStateMachine(bool dpFaultNow, bool dpBoostHoldNow){
  if(dpFaultNow){
    dpStateNow = DP_STATE_TIMING_CUT; // Differential pressure fault currently forces timing cut.
  }else if(dpBoostHoldNow){
    dpStateNow = DP_STATE_BOOST_HOLD; // Timing cut cleared, but retained boost hold is still active.
  }else if(force_dp_monitor_override){
    dpStateNow = DP_STATE_OVERRIDE; // Automatic monitor logic is intentionally paused by the test override.
  }else if(dpPendingActive(dpFaultNow)){
    dpStateNow = DP_STATE_PENDING;    // Low-pressure timer is active but fault is not latched yet.
  }else if(dpArmedNow){
    dpStateNow = DP_STATE_ARMED;      // Monitor is armed and healthy.
  }else{
    dpStateNow = DP_STATE_IDLE;       // Monitor is idle because arm conditions are not met.
  }
}

// Build the timing-cut reason string used by serial transition logging.
// Centralizing these strings keeps logging behavior consistent across files.
static const char* timingCutReasonText(bool levelLowEmergency){
  if(force_timing_cut) return "manual_timing";      // Manual timing latch button was active.
  if(force_pressure_ready_fault) return "pressure_ready_manual"; // Manual pressure-ready timeout fault latch was active.
  if(pressureReadyFaultLatched) return "pressure_ready_timeout"; // Real pressure-ready timeout latch was active.
  if(force_dp_fault) return "diff_pressure_manual"; // Manual differential pressure fault button was active.
  if(faultLatched){
    // Differentiate normal-low and critical-low rail differential pressure paths in timing-cut reason reporting.
    if(railDpLastRealFaultPath == RAIL_DP_PATH_CRITICAL_LOW) return "diff_pressure_critical";
    return "diff_pressure_low";
  }
  if(levelLowEmergency) return "level_spray";       // Low level occurred while spray was active.
  return "clear";                                   // No active timing-cut reason.
}
