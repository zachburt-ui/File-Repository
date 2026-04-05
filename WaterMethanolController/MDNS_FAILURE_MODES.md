# mDNS Failure Modes (Locked Runbook)

## Purpose
This document defines known mDNS failure modes for this controller and the required triage path.
Use this before changing any Wi-Fi or mDNS code.

## Locked Baseline
- mDNS + STA flow is locked.
- No regressions on mDNS or Wi-Fi are allowed.
- Host name is user-configurable in UI (`mdnsHost`) and persists in preferences.
- mDNS is expected only when STA has a valid IP address.

## Current Implementation Snapshot
- Host sanitize/start: `WaterMethanolController.ino` (`sanitizeMdnsHost`, `startMdnsOnce`).
- STA setup and hostname apply: `WaterMethanolController.ino` (setup `startSTA` path).
- STA reconnect handling keeps the selected mode and credentials; each retry queues `WiFi.reconnect()` and then always reapplies explicit `WiFi.begin(sta_ssid, sta_pass)` without mode churn so first-boot firmware credentials and saved credentials are both honored deterministically: `WebTask.ino`.
- mDNS runtime guard in web task:
  - starts responder when STA has valid IP and `mdnsRunning` is false
  - validates responder health with `mdns_hostname_get(...)`
  - re-announces on responder start, STA link-up, STA IP change, and periodic interval with `mdns_netif_action(..., MDNS_EVENT_ANNOUNCE_IP4)`
  (`WebTask.ino`)
- Persist/load `mdnsHost`, `wifi_mode`, STA credentials: `Prefs.ino`.
- UI/API host fields: `Api.ino`, `WebUI.ino`.
- Captive redirect target now uses the interface-local IP of the incoming request (`server.client().localIP()`), with STA and AP fallback order when needed: `Api.ino`.
- GPIO2 Wi-Fi LED now indicates active controller connectivity: STA link-up or at least one AP client connected: `WebTask.ino`.

## Failure Mode Matrix
| ID | Failure mode | Observable symptom | High-confidence cause | Fast check | Required recovery |
|---|---|---|---|---|---|
| FM-01 | Hostname mismatch | `watermeth.local` fails, IP works | Saved `mdnsHost` is not `watermeth` | `GET /api/config` -> `mdnsHost` | Open `http://<mdnsHost>.local/` or set host back to `watermeth` and save |
| FM-02 | STA not connected | `.local` fails and IP unavailable | STA not associated or no lease | Serial has no STA IP line | Fix STA SSID/pass, then reconnect |
| FM-03 | AP-only mode | `.local` fails | `wifiMode=0` (AP only) | `GET /api/config` -> `wifiMode` | Use AP IP (`192.168.4.1`) or enable STA/AP+STA |
| FM-04 | Client not on same LAN | `.local` fails, IP may fail | Browser device is on different network | Compare client network vs STA SSID | Put browser client on same network as controller STA |
| FM-05 | Local mDNS resolver issue | `.local` fails intermittently, IP works | Client resolver stack issue | `ping <host>.local` from same client | Restart client mDNS service / reconnect network |
| FM-06 | Browser cache confusion | UI seems old or tile missing | Cached old HTML from prior firmware | `Ctrl+F5` or query-string load | Open `http://<ip>/?v=<timestamp>` |
| FM-07 | VPN/firewall interference | `.local` and sometimes WS unstable | Multicast blocked by client firewall/VPN | Temporarily disable VPN/firewall | Exclude local subnet or keep VPN off for tuning |
| FM-08 | Network multicast suppression | `.local` fails, serial says mDNS started | Upstream network suppresses mDNS multicast | IP always works, `.local` never works | Use direct IP on that network; do not alter controller flow |
| FM-09 | Stale expectation after host sanitize | Entered host differs from expected | Invalid chars/spaces are normalized | Compare requested vs `api/config mdnsHost` | Use sanitized host value shown by API |
| FM-10 | Unapproved code change regression | `.local` stopped after code pass | mDNS/STA flow altered during unrelated change | Compare changed files with locked baseline | Revert unapproved network changes immediately |
| FM-11 | DNS captive redirection confusion | Browser opens wrong page | Portal/DNS assumptions while in mixed AP/STA context | Check current URL and network mode | Use explicit `http://<ip>/` or `http://<host>.local/` |
| FM-12 | Name resolution only failure | IP works, `.local` fails | Resolver path only; HTTP service healthy | `GET http://<ip>/api/config` succeeds | Treat as resolver issue, keep firmware unchanged |
| FM-13 | Cold-start resolver delay | `.local` fails first try, works after delay/retry | Client mDNS cache/discovery not warmed yet | Repeat `.local` request after 3-10 seconds | Use one retry rule and browser DNS cache clear before firmware changes |
| FM-14 | Responder lifecycle churn regression | `.local` becomes less stable after reconnect-flow edits | mDNS stop/start on transient STA status churn introduces responder instability | Compare behavior before/after reconnect lifecycle edits | Revert to locked one-shot mDNS start behavior and re-test |
| FM-15 | Browser HTTPS auto-upgrade | Browser says site not reachable while mDNS resolves and HTTP works | Browser forces `https://<host>.local` but controller serves HTTP on port 80 only | Check address bar protocol and test `http://<host>.local/` explicitly | Use explicit `http://` URL and clear browser HSTS policy for host if present |

## Required Triage Sequence (No Guessing)
1. Confirm HTTP by direct IP: `http://<sta-ip>/`.
2. Read live config: `http://<sta-ip>/api/config` and record `wifiMode`, `mdnsHost`, `staSsid`.
3. Confirm serial lines include STA IP and `[MDNS] Started: <host>.local`.
4. Resolve exact host from step 2, not assumed `watermeth`.
5. Test `http://<host>.local/`.
6. If step 1 works and step 5 fails, classify as resolver-path issue (FM-05/07/08/12), not controller HTTP failure.
7. If `.local` succeeds on retry but not first hit, classify as FM-13 and treat as client resolver warm-up behavior.

## Browser-Side Resolver Recovery (Windows)
Use these in order before touching firmware:
1. Open exact URL with protocol: `http://<host>.local/` (not search bar text without scheme).
2. Hard refresh active tab (`Ctrl+F5`).
3. Clear OS DNS cache: `ipconfig /flushdns`.
4. Clear browser DNS host cache:
   - Edge: `edge://net-internals/#dns` -> `Clear host cache`
   - Chrome: `chrome://net-internals/#dns` -> `Clear host cache`
5. Close and reopen browser, then retry `.local`.
6. Confirm fallback path remains valid: `http://<sta-ip>/`.
7. If browser still fails, verify the address bar is not forcing `https://<host>.local/` and clear host HSTS policy in browser net internals.

## mDNS No-Regression Gate (Required After Any Network-Touching Change)
1. Flash firmware.
2. Verify serial prints STA IP and mDNS start line.
3. Verify `GET /api/config` returns expected `mdnsHost`.
4. Verify `http://<sta-ip>/` loads.
5. Verify `http://<mdnsHost>.local/` loads.
6. Reboot once and repeat steps 2 through 5.
7. Update `LOCKED_BEHAVIOR_CHECKLIST.md` with result.
