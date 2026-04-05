// ===================== PULSE WIDTH TIMERS ========================
// ================================================================
//
// This file implements software pulse width modulation using esp_timer edge scheduling.
// Instead of a high-frequency interrupt, we schedule each edge once.
// That keeps the web server responsive and avoids starving the wireless network stack.

// This is the injector pulse width modulation callback.
// The callback toggles the injector solid state relay at the correct on and off times.
static void IRAM_ATTR injCb(void*){
  uint32_t per = injPeriodUs;
  uint32_t on  = injOnUs;
  // Enforce a minimum period so timer rescheduling always stays valid.
  if(per < 50) per = 50;

  // A duty of zero percent means the injector output stays off.
  if(on == 0){
    injHigh = false;
    gpioFast(PIN_INJ_SSR, false);
    esp_timer_start_once(injTimer, per);
    return;
  }

  // A duty of one hundred percent means the injector output stays on.
  if(on >= per){
    injHigh = true;
    gpioFast(PIN_INJ_SSR, true);
    esp_timer_start_once(injTimer, per);
    return;
  }

  // Toggle between the on and off edges.
  bool high = injHigh;
  if(!high){
    injHigh = true;
    gpioFast(PIN_INJ_SSR, true);

    uint32_t t = on;
    // Keep minimum re-arm window to avoid zero-delay timer churn.
    if(t < 50) t = 50;
    esp_timer_start_once(injTimer, t);
  }else{
    injHigh = false;
    gpioFast(PIN_INJ_SSR, false);

    uint32_t t = (per > on) ? (per - on) : 50;
    // Keep minimum re-arm window to avoid zero-delay timer churn.
    if(t < 50) t = 50;
    esp_timer_start_once(injTimer, t);
  }
}

// This creates the pulse width modulation timers and starts the first edge for each output.
static void pwmInit(){
  esp_timer_create_args_t a1 = {};
  a1.callback = &injCb;
  a1.arg = nullptr;
  // Use task dispatch so the wireless network stack stays stable.
  a1.dispatch_method = ESP_TIMER_TASK;
  a1.name = "injPWM";
  ESP_ERROR_CHECK(esp_timer_create(&a1, &injTimer));

  // Initialize outputs to safe states, then start the edge schedulers.
  injHigh = false;
  gpioFast(PIN_INJ_SSR, false);
  // Set the timing cut output to the inactive state based on polarity.
  gpioFast(PIN_IAT_SSR, iat_ssr_active_high ? false : true);
  // Set the boost cut relay to the allow state based on polarity.
  gpioFast(PIN_BOOST_SSR, boost_ssr_active_high ? true : false);

  esp_timer_start_once(injTimer, 20000);
}

// ================================================================
