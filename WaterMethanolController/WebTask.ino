// ========================= WEB TASK (CORE 0) =====================
// ================================================================
//
// This task handles web server requests, WebSocket updates, and captive portal domain name system requests.
// It runs frequently so the user interface feels responsive.

void startMdnsOnce();
void sanitizeMdnsHost();
extern bool mdnsRunning;

static void webTask(void*){
  // The web task is pinned to core 0 for wireless network stack stability.
  uint32_t lastBeat = 0; // Last serial heartbeat timestamp.
  uint32_t lastStaRetry = 0; // Last station reconnect attempt timestamp.
  uint32_t lastMdnsHealthCheckMs = 0; // Last multicast domain name system health-check timestamp.
  uint32_t lastMdnsAnnounceMs = 0; // Last multicast domain name system IPv4 announcement timestamp.
  bool staWasConnected = false; // Tracks station connection transitions so link-up events can trigger a fresh multicast announcement.
  IPAddress lastStaIp(0,0,0,0); // Tracks the last station address so address changes can trigger a fresh multicast announcement.
  for(;;){
    // Minimal station reconnect that does not change wireless mode.
    // Avoiding repeated Wi-Fi mode resets prevents unnecessary network stack churn during transient disconnects.
    if((wifi_mode == 1 || wifi_mode == 2) && sta_ssid.length() > 0 && !WiFi.isConnected()){
      uint32_t now = millis(); // Current timestamp for retry interval check.
      if((now - lastStaRetry) > 5000){
        lastStaRetry = now; // Record retry timestamp before attempting reconnect.
        Serial.printf("[WIFI] STA reconnect (status=%d ssid=%s)\n", (int)WiFi.status(), sta_ssid.c_str());
        // Queue a reconnect attempt first so the wireless stack can continue any in-progress association work.
        bool reconnectQueued = WiFi.reconnect();
        // Always follow with an explicit begin using configured credentials.
        // This guarantees retries use the currently configured station network name and password,
        // including first-boot firmware defaults and recently updated saved preferences.
        sanitizeMdnsHost();
        WiFi.setHostname(mdnsHost.c_str());
        WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
        if(serial_debug_enable){
          Serial.printf("[WIFI] STA explicit begin with configured credentials (reconnectQueued=%d)\n", reconnectQueued ? 1 : 0);
        }
      }
    }
    // Consider station online only when link status is connected and an IPv4 address is assigned.
    const bool staConnected = (WiFi.status() == WL_CONNECTED) && (WiFi.localIP()[0] != 0); // Treat station as connected only when link is up and address is valid.
    const IPAddress staIp = staConnected ? WiFi.localIP() : IPAddress(0,0,0,0); // Capture the current station address for state tracking and logs.
    bool mdnsJustStarted = false; // Tracks whether the responder started in this loop so we can announce immediately.
    // Keep the locked-in multicast domain name system behavior:
    // start once when station mode has a valid address, and do not tear the responder down during brief status churn.
    if(staConnected && !mdnsRunning){
      startMdnsOnce(); // Register multicast domain name system hostname and Hypertext Transfer Protocol service when station mode is up.
      mdnsJustStarted = mdnsRunning; // Record successful responder startup so we can issue an immediate announce below.
    }
    // Periodically verify that the multicast domain name system responder is still initialized.
    // If the responder is no longer healthy, clear the local running flag and restart once.
    if(staConnected && mdnsRunning){
      uint32_t now = millis(); // Current timestamp for multicast domain name system health-check interval.
      if((now - lastMdnsHealthCheckMs) > 2000){
        lastMdnsHealthCheckMs = now; // Record health-check timestamp before probing responder state.
        char runtimeHost[MDNS_NAME_BUF_LEN] = {0}; // Buffer for the currently registered responder host name.
        esp_err_t hostReadErr = mdns_hostname_get(runtimeHost); // Read responder host name from the multicast domain name system stack.
        if(hostReadErr != ESP_OK){
          if(serial_debug_enable){
            Serial.printf("[MDNS] Health check failed (err=%d). Restarting responder.\n", (int)hostReadErr);
          }
          mdnsRunning = false; // Mark responder as not running so the standard start path can re-initialize it.
          startMdnsOnce(); // Restart responder with the configured host and Hypertext Transfer Protocol service.
        }else if(String(runtimeHost) != mdnsHost){
          if(serial_debug_enable){
            Serial.printf("[MDNS] Host mismatch runtime=%s configured=%s. Restarting responder.\n", runtimeHost, mdnsHost.c_str());
          }
          MDNS.end(); // Stop responder cleanly before re-initializing with the configured host name.
          mdnsRunning = false; // Clear running flag so restart is deterministic.
          startMdnsOnce(); // Restart responder with the saved and sanitized host name.
        }
      }
    }
    // When station connectivity returns, station address changes, or responder restarts, send an explicit multicast announcement.
    // This improves first-hit host-name resolution after reconnects and after responder recovery.
    bool shouldAnnounce = staConnected && mdnsRunning && (mdnsJustStarted || !staWasConnected || (staIp != lastStaIp));
    // While station remains connected, periodically refresh the IPv4 multicast announcement.
    // This keeps host discovery stable across client resolver cache expiry and network multicast jitter.
    if(staConnected && mdnsRunning && !shouldAnnounce){
      uint32_t now = millis(); // Current timestamp for periodic multicast domain name system announcement interval.
      if((now - lastMdnsAnnounceMs) > 15000){
        shouldAnnounce = true; // Trigger periodic announcement every fifteen seconds while connected.
      }
    }
    if(shouldAnnounce){
      esp_netif_t* staNetif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); // Get the default station network interface used by the responder.
      if(staNetif){
        esp_err_t announceErr = mdns_netif_action(staNetif, MDNS_EVENT_ANNOUNCE_IP4); // Send a fresh IPv4 multicast domain name system announcement on station interface.
        lastMdnsAnnounceMs = millis(); // Record announcement attempt time to rate-limit periodic announces.
        if(serial_debug_enable){
          if(announceErr == ESP_OK){
            Serial.printf("[MDNS] Announced %s.local at %s\n", mdnsHost.c_str(), staIp.toString().c_str());
          }else{
            Serial.printf("[MDNS] Announce failed (err=%d)\n", (int)announceErr);
          }
        }
      }else if(serial_debug_enable){
        Serial.println("[MDNS] Station network interface handle unavailable for announcement.");
      }
    }
    staWasConnected = staConnected; // Save station connectivity state for next loop transition checks.
    lastStaIp = staIp; // Save station address for next loop address-change detection.
    bool apClientConnected = false;
    if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA){
      apClientConnected = (WiFi.softAPgetStationNum() > 0);
    }
    const bool wifiLinkActive = staConnected || apClientConnected; // Light turns on when either STA link is up or an AP client is connected.
    gpioFast(PIN_WIFI_LED, wifi_led_active_high ? wifiLinkActive : !wifiLinkActive); // Onboard status light mirrors active controller connectivity.
    if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA){
      dnsServer.processNextRequest(); // Service captive portal domain name requests while access point is enabled.
    }
    server.handleClient(); // Service incoming Hypertext Transfer Protocol requests.
    wsLoopTick(); // Service WebSocket events and periodic live status broadcasts.
    uint32_t now = millis(); // Current timestamp for heartbeat interval check.
    if(serial_debug_enable && (now - lastBeat) > serial_print_heartbeat_ms){
      lastBeat = now; // Record heartbeat timestamp.
      const String ip = WiFi.isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); // Report station address when connected, otherwise report access point address.
      Serial.printf("[WEB] alive heap=%u ip=%s mdns=%s\n", (unsigned)ESP.getFreeHeap(), ip.c_str(), mdnsRunning ? "ON" : "OFF");
    }
    vTaskDelay(pdMS_TO_TICKS(2)); // Short cooperative delay keeps task responsive without starving other tasks.
  }
}
// ================================================================
