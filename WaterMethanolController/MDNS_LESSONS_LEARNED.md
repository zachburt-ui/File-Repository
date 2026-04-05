# mDNS Lessons Learned (Regression Prevention)

## Incident Context
- Repeated reports of `.local` failures while controller was reachable by direct STA IP.
- This pattern isolates the failure to name resolution path, not core HTTP server reachability.
- Latest diagnostic run showed first-hit `.local` access can be slow while subsequent hits are fast, consistent with resolver warm-up behavior.
- A reconnect lifecycle guard that stopped and restarted mDNS on link transitions caused a regression and was removed.
- Additional review found reconnect-mode churn risk in the STA retry path (`WiFi.mode(...)` during retry loop), which can destabilize resolver behavior.

## Key Lessons
1. Separate transport from resolution early.
   - If `http://<ip>/` works, web stack is healthy.
   - Do not diagnose this as a web server outage.
2. Never assume the host is `watermeth`.
   - Host is user-configurable and persisted.
   - Always read `mdnsHost` from `GET /api/config`.
3. Serial evidence is required.
   - mDNS troubleshooting starts with exact boot lines:
     - STA IP line
     - `[MDNS] Started: <host>.local`
4. Cached UI can mimic firmware regressions.
   - A stale browser page can hide current UI/state.
   - Use cache-buster URL or hard refresh during validation.
5. Bench testing and network refactors do not mix.
   - No code churn during active bench unless explicitly approved.
6. mDNS/STA flow is locked.
   - Do not rework reconnect/startup ordering without explicit consent.
7. Measure before modifying firmware.
   - Use timed HTTP checks for both `http://<ip>/` and `http://<host>.local/`.
   - If only first-hit `.local` is slow and retries are normal, classify as client resolver warm-up (not firmware regression).
8. Captive redirect should follow the actual ingress interface.
   - Unknown-path redirects should use the local IP bound to the incoming socket to avoid AP-address redirects while serving STA requests.

## Process Changes (Mandatory)
1. Any Wi-Fi/mDNS edit must include:
   - `MDNS_FAILURE_MODES.md` review/update
   - `MDNS_LESSONS_LEARNED.md` review/update
   - Full mDNS no-regression gate execution
2. Any report of `.local` failure must be triaged in fixed order:
   - IP access test
   - `/api/config` host/mode check
   - Serial mDNS start line check
   - `.local` test using exact configured host
3. If IP works but `.local` fails:
   - Do not modify unrelated firmware logic first.
   - Classify as resolver-path issue until proven otherwise.
4. If `.local` is intermittent in browsers:
   - Clear browser DNS host cache and OS DNS cache first.
   - Re-test before any firmware/network-flow code edits.

## Locked Recovery Action
- Reverted mDNS reconnect lifecycle churn and restored locked behavior:
  - Keep STA reconnect logic.
  - Start mDNS when STA is connected and `mdnsRunning` is false.
  - Do not tear down mDNS on transient STA status drops unless explicitly approved and fully re-validated.
- Hardened runtime behavior without changing locked Wi-Fi mode flow:
  - STA retry now queues `WiFi.reconnect()` and always reapplies `WiFi.begin(sta_ssid, sta_pass)` so retries always use the currently configured credentials.
  - Added mDNS health check (`mdns_hostname_get`) to detect stale responder state.
  - Added explicit mDNS IPv4 re-announcement on responder start, STA link-up, STA IP changes, and a periodic interval to improve first-hit resolution consistency.

## Anti-Regression Rules
- Do not change mDNS or STA behavior during unrelated fixes.
- Do not introduce one-off UI self-healing patches for single missing tiles.
- Keep UI parity between `WebUI.ino` and `Preview UI/ui_preview.html`.
- Preserve user-updatable Wi-Fi/mDNS settings and persistence behavior.

## Definition of Done for mDNS Stability
- Direct IP access works after flash and after reboot.
- `<mdnsHost>.local` access works after flash and after reboot.
- `LOCKED_BEHAVIOR_CHECKLIST.md` updated with test evidence.
- No unapproved network-flow code changes were made.
