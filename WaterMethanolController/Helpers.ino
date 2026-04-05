// ===================== LOW-LEVEL HELPERS =========================
// ================================================================
//
// This file contains small helper functions that are used in many places.
// They keep the main control code readable by hiding tiny, repeated details.

// Write a general purpose input and output pin quickly using the ESP32 microcontroller low level driver.
// This avoids some overhead compared to digitalWrite.
static inline void gpioFast(int pin, bool v){
  gpio_set_level((gpio_num_t)pin, v ? 1 : 0);
}
// Clamp a floating-point value between a minimum and a maximum.
// This is used to keep duty cycles and settings in safe ranges.
static inline float clampf(float x, float lo, float hi){
  if(x < lo) return lo;
  if(x > hi) return hi;
  return x;
}
// Return the active barometric reference in kilopascal absolute.
// Normal operation uses the boot-captured reference, and bench testing can force a temporary override.
static float effectiveBaroKpa(){
  if(isfinite(force_baro_kpa) && force_baro_kpa >= 0.0f){
    return force_baro_kpa;
  }
  return baroKpa;
}
// Return the active barometric reference in pounds per square inch absolute.
static float effectiveBaroPsi(){
  return effectiveBaroKpa() * 0.1450377377f;
}
// Read an analog to digital converter pin and convert the averaged calibrated millivolt values into volts.
// This path uses the ESP32 core calibration stack (including per-chip eFuse data when available) for better cross-board consistency.
static float adcVolts(int pin){
  // Sample a few times to reduce single-sample jitter from the analog front end.
  // The short average keeps control response fast while improving repeatability.
  const uint8_t sampleCount = 8;
  uint32_t mvSum = 0;
  for(uint8_t i = 0; i < sampleCount; ++i){
    // Collect calibrated millivolt samples so each board benefits from the microcontroller's factory ADC trim data.
    mvSum += (uint32_t)analogReadMilliVolts(pin);
  }
  const float mvAvg = (float)mvSum / (float)sampleCount;
  // Convert averaged calibrated millivolts to volts.
  // Clamp to the valid analog input range so downstream pressure math stays bounded.
  const float volts = mvAvg * 0.001f;
  return clampf(volts, 0.0f, ADC_VREF);
}
// Guard against invalid manifold absolute pressure scaling values.
// This prevents corrupted settings from forcing the MAP reading to zero.
void sanitizeMapCalibration(){
  if(!isfinite(map_div_ratio) || map_div_ratio < 0.10f || map_div_ratio > 1.10f){
    map_div_ratio = 0.66f;
  }
  if(!isfinite(map_kpa_linear_hpt) || map_kpa_linear_hpt < 50.0f || map_kpa_linear_hpt > 800.0f){
    map_kpa_linear_hpt = 312.5f;
  }
  if(!isfinite(map_kpa_offset_hpt) || map_kpa_offset_hpt < -200.0f || map_kpa_offset_hpt > 200.0f){
    map_kpa_offset_hpt = -11.25f;
  }
  map_kpa_per_v = map_kpa_linear_hpt / 5.0f;
}
// Guard against invalid rail pressure scaling values.
void sanitizeRailCalibration(){
  if(!isfinite(rail_div_ratio) || rail_div_ratio < 0.10f || rail_div_ratio > 1.10f){
    rail_div_ratio = 0.66f;
  }
  if(!isfinite(rail_psi_per_v) || rail_psi_per_v < 1.0f || rail_psi_per_v > 200.0f){
    rail_psi_per_v = 32.5f;
  }
  if(!isfinite(rail_psi_offset) || rail_psi_offset < -200.0f || rail_psi_offset > 200.0f){
    rail_psi_offset = -16.25f;
  }
}
// Guard shared ADC input calibration values that correct board-level analog voltage measurement error.
// This keeps sensor manufacturer calibration values separate while allowing one common ADC trim for both MAP and rail.
void sanitizeAdcInputCalibration(){
  if(!isfinite(adc_input_gain) || adc_input_gain < 0.80f || adc_input_gain > 1.20f){
    adc_input_gain = 1.0000f;
  }
  if(!isfinite(adc_input_offset_v) || adc_input_offset_v < -0.50f || adc_input_offset_v > 0.50f){
    adc_input_offset_v = 0.000f;
  }
}
// Reset the shared analog to digital converter correction curve to identity mapping.
// Identity means output voltage equals input voltage at each fixed breakpoint.
void resetAdcCurveCalibration(){
  for(uint8_t i = 0; i < ADC_CURVE_PTS; ++i){
    adc_curve_out_v[i] = adc_curve_in_v[i];
  }
}
// Guard the shared analog to digital converter correction curve.
// Endpoints are locked to zero and full-scale so endpoint anchors remain meaningful.
// Midpoints are constrained to finite monotonic values in range to avoid invalid piecewise mapping.
void sanitizeAdcCurveCalibration(){
  // Hard-lock endpoints so shared gain and offset remain the only endpoint correction controls.
  adc_curve_out_v[0] = 0.0f;
  adc_curve_out_v[ADC_CURVE_PTS - 1] = ADC_VREF;
  // Validate and clamp midpoint entries first.
  for(uint8_t i = 1; i < (ADC_CURVE_PTS - 1); ++i){
    if(!isfinite(adc_curve_out_v[i])){
      adc_curve_out_v[i] = adc_curve_in_v[i];
    }
    adc_curve_out_v[i] = clampf(adc_curve_out_v[i], 0.0f, ADC_VREF);
  }
  // Enforce monotonic nondecreasing output values to preserve monotonic pressure conversion.
  for(uint8_t i = 1; i < ADC_CURVE_PTS; ++i){
    if(adc_curve_out_v[i] < adc_curve_out_v[i - 1]){
      adc_curve_out_v[i] = adc_curve_out_v[i - 1];
    }
  }
}
// Apply shared piecewise linear analog to digital converter curve correction.
// Input is corrected linear pin voltage after shared gain and offset trim.
// Output is curve-corrected pin voltage used by both MAP and rail conversion paths.
float applyAdcCurveCorrection(float pinVolts){
  if(!isfinite(pinVolts)) return 0.0f;
  const float v = clampf(pinVolts, 0.0f, ADC_VREF);
  if(v <= adc_curve_in_v[0]) return adc_curve_out_v[0];
  if(v >= adc_curve_in_v[ADC_CURVE_PTS - 1]) return adc_curve_out_v[ADC_CURVE_PTS - 1];
  for(uint8_t i = 0; i < (ADC_CURVE_PTS - 1); ++i){
    const float x0 = adc_curve_in_v[i];
    const float x1 = adc_curve_in_v[i + 1];
    if(v <= x1){
      const float y0 = adc_curve_out_v[i];
      const float y1 = adc_curve_out_v[i + 1];
      if((x1 - x0) <= 0.0f) return y1;
      const float t = (v - x0) / (x1 - x0);
      return y0 + ((y1 - y0) * t);
    }
  }
  return adc_curve_out_v[ADC_CURVE_PTS - 1];
}
// Linearly interpolate the spray duty curve (manifold absolute pressure in kilopascal to duty percent).
// The curve points are ordered by manifold absolute pressure, and we interpolate between neighbors.
static float curveLookupDuty(float kpa){
  // Keep duty at zero below the first curve point so low or missing MAP voltage cannot request spray.
  // This preserves the first point duty exactly at Curve Start kPa and above.
  if(kpa < curve_kpa[0]) return 0.0f;
  for(int i=0;i<CURVE_PTS-1;i++){
    float x0=curve_kpa[i], x1=curve_kpa[i+1];
    if(kpa <= x1){
      float y0=curve_duty[i], y1=curve_duty[i+1];
      // Guard against a zero-width segment so malformed curve input cannot divide by zero.
      if((x1 - x0) <= 0.0f) return y1;
      float t=(kpa-x0)/(x1-x0);
      return y0 + t*(y1-y0);
    }
  }
  return curve_duty[CURVE_PTS-1];
}
// Convert the current safety mode into a human-readable string.
static const char* modeName(Mode m){
  switch(m){
    case MODE_IAT_CUT: return "Timing Cut";
    default: return "Normal";
  }
}
// Convert the current differential pressure monitor state into a human-readable string.
static const char* dpStateName(uint8_t s){
  switch((DpMonitorState)s){
    case DP_STATE_ARMED: return "ARMED";
    case DP_STATE_PENDING: return "PENDING";
    case DP_STATE_TIMING_CUT: return "TIMING_CUT";
    case DP_STATE_BOOST_HOLD: return "BOOST_HOLD";
    case DP_STATE_OVERRIDE: return "OVERRIDE";
    default: return "IDLE";
  }
}
// ================================================================
