# Assistant Connectivity and Bench Testing Procedure

## 1) Purpose
- Provide one repeatable procedure for reconnecting the assistant to the live controller and running bench tests quickly.
- Reduce repeated troubleshooting when IP, mDNS, COM port, or browser behavior changes between sessions.

## 2) Session Rules
- During active bench testing, no firmware code changes are made unless explicitly approved.
- The assistant owns reboot actions during bench testing.
- The assistant must show the full current test block verbatim before each next single check.
- One voltage point at a time: user sets voltage, then replies `set`.
- If any chat leak of commands/tool payload/internal text occurs, stop tool use immediately and resume only after explicit `continue`.

## 3) Fast Connectivity Bring-Up
1. Confirm controller is powered and connected to the same network as the host PC.
2. Check direct IP first (fastest path).
3. Check `watermeth.local` second (convenience path).
4. Confirm API response from `/api/status`.
5. Confirm UI load in browser.

### Host checks (PowerShell)
```powershell
ping -n 1 172.20.10.7
Invoke-WebRequest -UseBasicParsing -Uri "http://172.20.10.7/api/status" -TimeoutSec 8
ping -n 1 watermeth.local
Invoke-WebRequest -UseBasicParsing -Uri "http://watermeth.local/api/status" -TimeoutSec 8
```

## 4) Browser Procedure
1. Use `http://watermeth.local` first.
2. If `.local` is slow or fails, use direct IP (`http://<ip>`).
3. Keep both Edge and Chrome available for A/B checks.
4. Do not use `https://` for this device.

## 5) Serial/Port Procedure
1. Detect available COM ports before flashing or serial capture.
2. If COM number changes after reconnect/reset, update all tool commands to the new COM port.
3. If no ESP32 COM port appears, verify cable/power and re-enumerate ports.

### Reboot procedure during assistant-led testing
1. Use API reboot first for deterministic remote restart:
2. Read current `wifiMode` from `/api/config`.
3. Post the same `wifiMode` value back to `/api/config` to trigger controlled restart.
4. Wait for `/api/status` to return again before resuming tests.
5. Use COM DTR/RTS reboot only as fallback if API reboot is unavailable.

### Host check (PowerShell)
```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name
```

## 6) Standard Bench Test Flow
1. Assistant posts current test block.
2. Assistant requests one explicit next point.
3. User sets voltage and replies `set`.
4. Assistant samples and reports measured value plus pass/fail.
5. Assistant logs results into `TEST_RESULTS_MASTER.md`.
6. Repeat until block completion, then move to next block.

## 7) Rail and MAP Sweep Order
- Rail full sweep: `0.00, 0.50, 1.00, 1.50, 2.00, 2.50, 3.00, 3.30 V`
- MAP full sweep: `0.00, 0.50, 1.00, 1.50, 2.00, 2.50, 3.00, 3.30 V`
- Record measured, expected, and pass/fail for each point.

## 8) Connectivity Failure Handling
### A) API timeouts or intermittent `no_samples`
1. Retry using direct IP only.
2. Increase request timeout before declaring failure.
3. Run short stability sample loop before resuming tests.

### B) mDNS failures (`watermeth.local` not resolving)
1. Keep testing on direct IP to avoid blocking bench progress.
2. Run name-resolution and HTTP checks separately.
3. Log result to mDNS docs only if firmware behavior changed.

### C) Brownout or reset loop events
1. Stabilize power source and cabling.
2. Re-check COM port.
3. Reflash firmware only if boot loop persists after power stabilization.

## 9) Documentation Update Requirements Per Change
- Calibration or ADC conversion changes:
  - `CALIBRATION_REFERENCE.md`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Wi-Fi or mDNS changes:
  - `MDNS_FAILURE_MODES.md`
  - `MDNS_LESSONS_LEARNED.md`

## 10) Completion Criteria for a Session
- Connectivity stable by direct IP and `.local`.
- Requested rail/MAP sweeps completed and logged.
- Input-to-logic checks completed for available bench hardware.
- All required docs updated in the same pass.
