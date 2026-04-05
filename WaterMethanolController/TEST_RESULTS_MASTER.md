# Test Results Master (Running)

Purpose:
- This file is the consolidated record of all bench testing, self-testing, and host-side validation.
- It is a historical log, not a behavior contract. Locked behavior remains in `LOCKED_BEHAVIOR_CHECKLIST.md`.

Date context:
- Current consolidated test record date: 2026-04-02
- Latest incremental bench validation update: 2026-04-02
- Latest code-and-compile calibration-path update: 2026-04-02

## 1) Test Sources

### 1.1) Assistant Self-Testing
- Local compile checks with Arduino CLI (`esp32:esp32:esp32da`).
- Flash/upload to COM3.
- Serial monitor verification on COM3 (`[STAT]`, `[EVENT]`, `[FAULT]`).
- Automated API test scripts using:
  - `POST /api/config` for force-state sequencing.
  - `GET /api/status` and `GET /api/config` assertions.
- Host network checks for mDNS and HTTP reachability.

### 1.2) User-Assisted Bench Testing
- Physical voltage injection using a bench supply and voltmeter:
  - MAP input direct.
  - Rail pressure sensor input direct.
- Manual level switch wiring/open-ground checks.
- UI interaction checks (cards, save/reboot, settings persistence).
- Session runbook added: `ASSISTANT_CONNECTIVITY_AND_TESTING_PROCEDURE.md` for repeatable assistant reconnect and bench execution flow.

## 2) Consolidated Pass Summary

- Firmware compile and flash: pass.
- Wi-Fi STA and web UI reachability: pass.
- mDNS stability after hardening: pass (host-side stability runs passed).
- Settings persistence and save/reboot behavior: pass.
- Flash-default behavior (new build loads defaults once): pass.
- dP monitor and safety behavior (bench and self-test): pass.
- MAP input calibration sweep: pass (within about +/-2.2 kPa).
- Rail pressure sensor input calibration sweep: pass on historical full sweep and pass on COM5 locked 3-point validation.
- One-supply combined input-to-logic checks: pass.
- UI theming sync: Sensor Calibration and Wi-Fi Settings cards now match Settings-card visual language in live UI and preview.
- UI theming sync: Methanol Spray Curve card now matches Settings/Sensor Calibration visual language with sectioned groups.
- UI cleanup sync: Test Section manual fault controls are now compact grouped buttons (same IDs and logic, cleaner layout).
- UI cleanup sync: Test Section grouped controls now include Settings-style colored section strips for visual separation.
- UI theming sync: Test Section now uses three full themed section groups (sensor overrides, output overrides, manual fault controls) in live UI and preview.
- UI polish sync: Test Section Section 1 and Section 2 field rows now use aligned control baselines for cleaner textbox/select alignment.
- UI theming sync: Status card tiles and Live Data gauges now use neutral gray styling across categories; IDs and behavior are unchanged.
- Fault wording sync: live UI and preview now consistently use `Fault Status` and `Rail dP Fault` wording.
- Shared ADC sliders now include larger control styling plus visible tick and number scales in live UI and preview.
- Shared ADC slider span is now widened in live UI and preview for cross-board tolerance (`adcGain 0.8000..1.2000`, `adcOffset -0.500..0.500 V`).
- Shared ADC Trim now includes a quick in-card calibration instruction for end users (`0.0 V` and `3.3 V` endpoint trim workflow).
- Firmware comment pass completed across core `.ino` files plus mirrored UI script comments in `WebUI.ino` and `Preview UI/ui_preview.html` (comment-only maintenance, no logic change).
- User preference update applied: UI script comments were removed from `WebUI.ino` and `Preview UI/ui_preview.html` (comment-only formatting change, no logic change).
- Final-sweep fix: UI save path now preserves valid numeric zero values for affected controls in both live UI and preview.
- Final-sweep fix: serial debug preference now persists across reboot while firmware default remains ON for first-boot behavior.
- Final-sweep fix: curve sanitation now enforces strictly increasing kPa points and interpolation is guarded against zero-width segments.
- Final-sweep fix: captive redirect now targets the request-interface IP with STA/AP fallback behavior.
- Final-sweep update: GPIO2 Wi-Fi LED now represents active controller connectivity (STA link-up or AP client connected).
- 2026-02-24 STA reconnect hardening update: retry path now always reapplies explicit `WiFi.begin(sta_ssid, sta_pass)` after `WiFi.reconnect()` so first-connection firmware credentials are always retried deterministically.
- 2026-02-24 compile validation after STA reconnect hardening: pass for normal and warnings builds.
- 2026-02-26 Sensor Calibration UI update: added two-decimal live gauges for MAP (kPa abs) and rail pressure (psig) in Sensor Calibration for calibration workflow readability.
- 2026-02-26 Sensor Calibration UI update: dP and spray command were intentionally not included in the new calibration gauge pair.
- 2026-02-26 compile validation after Sensor Calibration UI update: pass for normal and warnings builds.
- 2026-02-27 Sensor Calibration readability update: two-decimal MAP and rail gauges now use short display-only smoothing in live UI and preview.
- 2026-02-27 compile validation after Sensor Calibration readability update: pass for normal build.
- 2026-02-27 Status history update: Status tile `stReason` is now labeled `Fault History` and displays persistent last-active safety/fault reason.
- 2026-02-27 Status history update: live API now publishes `faultHistory` and firmware persists it under preferences key `fault_hist`.
- 2026-02-27 compile validation after Status history update: pass for normal build.
- 2026-02-27 ADC endpoint-calibration validity update: removed post-trim ADC clamping in control-loop conversion so endpoint calibration remains meaningful.
- 2026-02-27 compile validation after ADC endpoint-calibration validity update: pass for normal and warnings builds.
- 2026-02-28 shared multi-point ADC curve update: Sensor Calibration now includes a breakpoint table and one-click fit for shared ADC curve midpoints in live UI and preview.
- 2026-02-28 compile validation after shared multi-point ADC curve update: pass for normal build; warnings build passed after retry following a host file-lock race during parallel compile.
- 2026-02-28 Sensor Equation table-entry fit update: Sensor Calibration now includes MAP (`V` + `kPa`) and rail (`V` + `psi`) entry tables for direct fit into `mapLin/mapOff` and `pLin/pOff`.
- 2026-02-28 compile validation after Sensor Equation table-entry fit update: pass for normal and warnings builds.
- 2026-02-28 wording update: user-facing UI text renamed from `Desired Rail dP` to `Target Injector dP` with no behavior change.
- 2026-02-28 naming-alignment update: code/UI config field renamed to `targetInjectorDp`; API GET now returns `targetInjectorDp` and POST still accepts legacy `desiredRailDp` for compatibility.
- 2026-02-28 compile validation after naming-alignment update: pass for normal and warnings builds.
- 2026-03-01 flash baseline ADC curve update: firmware default and live/preview UI default ADC curve points now match Barry reflash baseline `[0.0, 0.4850, 0.9820, 1.4800, 1.9770, 2.4610, 2.9730, 3.3]`.
- 2026-03-02 ADC preset revert update: firmware default and live/preview UI default ADC curve points were restored to identity `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`; Barry baseline remains historical.
- 2026-03-01 compile validation after flash-baseline ADC curve default sync: pass for normal and warnings builds (warnings build required one retry after host file-lock race).
- 2026-03-02 Sensor Equation table update: MAP and rail point-entry table presets now both use `0.5-4.5 V`, and both tables now include a third expected-value reference column.
- 2026-03-02 Sensor Equation table note update: UI/preview now state voltage measurement point is divider input (sensor side) and keep explicit `3.3 V` controller-input protection note.
- 2026-03-02 Sensor quick-check table update: Sensor Equation Values now includes measured-entry quick-check rows that compute expected MAP/rail and `% error`.
- 2026-03-02 Sensor table capture update: MAP fit, rail fit, and quick-check tables now include per-row `Capture` buttons that pull current live MAP/rail values.
- 2026-03-02 Sensor Equation UI cleanup: legacy rail two-point quick-calc controls (`P1/V1`, `P2/V2`) were removed from live UI and preview.
- 2026-03-02 Sensor Calibration section-order and naming update: shared ADC section is now labeled `Analog Input Calibration`, and required order is explicit (`Analog Input Calibration`, then `Preset Selection`, then `Sensor Equation Values`) in live UI and preview.
- 2026-03-02 compile validation after Sensor Equation table range/expected-column/note update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 compile validation after ADC preset revert to identity defaults: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 compile validation after Sensor Equation quick-calc control removal: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-03 Test Section force-model update: Force Pump and Force Injectors now behave as absolute bench overrides in firmware and preview simulation, and Test Section copy now documents the locked absolute-force behavior.
- 2026-03-03 compile validation after Test Section absolute-force update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1163283` bytes, RAM `51392` bytes).
- 2026-03-03 Status visibility update: added full-width `Current Fault Status` tile directly below `Fault History` in both live UI and preview; tile shows active fault reason or `None`.
- 2026-03-03 Live Data unit expansion update: added `MAP (psia)` gauge (while keeping `MAP (kPa abs)`), and added `Rail Pressure (kPag)` gauge (while keeping `Rail Pressure (psig)`) in both live UI and preview.
- 2026-03-03 Live Data layout update: main Live Data now uses a clean four-across baseline in both live UI and preview and still fills row width when fewer gauges are on a row.
- 2026-03-03 barometric differential pressure update: firmware now captures barometric reference early at boot, publishes `baroKpa` and `baroPsi` in `/api/status`, adds `Baro (kPa abs)` gauge in live UI/preview, and computes injector dP as `rail gauge + boot barometric reference - manifold absolute pressure`.
- 2026-03-03 compile validation after barometric differential pressure and Live Data layout updates: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1164191` bytes, RAM `51408` bytes).
- 2026-03-03 Sensor Calibration capture cleanup: removed capture buttons from MAP/Rail fit and MAP/Rail quick-check tables in live UI and preview while keeping ADC curve table capture intact.
- 2026-03-03 final compile validation after barometric update, 4-wide Live Data layout fix, and sensor-table capture cleanup: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1163231` bytes, RAM `51408` bytes).
- 2026-03-03 fault-path split and status-uniformity update:
  - Rail dP and pressure-ready timeout now use separate real latches and separate retained boost-hold latches.
  - Manual and bypass controls were split per path (`Rail dP` vs `Pressure-Ready`) in live UI and preview.
  - Status now shows separate `Rail dP Fault` and `Pressure-Ready Fault` tiles and uses one shared `Current Fault Status` text source for Status + Live Data.
  - Timing-cut auto-clear is now one shared path for all real timing-cut latches.
- 2026-03-03 compile validation after fault-path split and shared timing-cut auto-clear update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1168895` bytes, RAM `51416` bytes).
- 2026-03-03 terminology alignment update: user-facing `dP Arm Settle` text was renamed to `dP Delay` in live UI, preview UI, and guide docs while preserving persisted key compatibility (`dpArmSettleMs`); no logic change.
- 2026-03-03 default dP profile update: firmware/live UI/preview defaults were tuned for normal spray-related dP drop (`dP min 50`, `dP fault delay 3000 ms`, `dP recover 1.5 psi`, `critical dP 25 psi for 150 ms`, `dP delay 500 ms`, `pressure-ready timeout 3000 ms`; target injector dP default remains `60 psi`).
- 2026-03-03 compile validation after default dP profile update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1168895` bytes, RAM `51416` bytes).
- 2026-03-03 rail dP state-identification update: live API now publishes `railDpStateDetail`, Status now shows `Rail dP State Detail`, and current-fault wording now distinguishes rail dP normal-low versus critical-low timing and boost-hold states in firmware, live UI, and preview.
- 2026-03-03 compile validation after rail dP state-identification update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1169879` bytes, RAM `51424` bytes).
- 2026-03-03 Live Data label update: fault gauge title is now `Fault Status` in live UI and preview, with no logic change to the underlying unified current-fault signal.
- 2026-03-03 Live Data boost gauge update: added `Boost (psi)` gauge in live UI and preview, calculated from live status as `MAP psia - Baro psia`.
- 2026-03-03 Live Data cleanup update: removed `Rail Pressure (kPag)` gauge from live UI and preview; `Rail Pressure (psig)` remains as the single rail pressure gauge in Live Data.
- 2026-03-03 compile validation after Live Data rail-gauge cleanup: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1169895` bytes, RAM `51424` bytes).
- 2026-03-03 Test Section barometric override update: added `Force Baro (kPa abs)` to live UI and preview test controls; differential pressure math, forced dP-to-rail reconstruction, and `/api/status` barometric outputs now use active barometric reference (boot capture unless `Force Baro` is set).
- 2026-03-03 compile validation after Test Section barometric override update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1170643` bytes, RAM `51424` bytes).
- 2026-03-03 Live Data barometric and fault-visibility update: added `Baro (psia)` gauge in live UI and preview, and set `Fault Status` gauge to a dedicated full-width bottom row tile in both live UI and preview.
- 2026-03-03 Preview UI default gauge-value update: preview live-gauge placeholder values and mock runtime defaults were set to a realistic healthy dP-monitor condition (`MAP 178 kPa`, `Rail 71.3 psig`, `Injector dP ~60.2 psi`, no fault) for better at-a-glance startup context.
- 2026-03-03 card-toggle hitbox update: removed card-body click-toggle handlers from live UI and preview so only title-strip clicks toggle card collapse/expand.
- 2026-03-29 timing-cut output failsafe update: timing-cut output on GPIO32 was flipped to failsafe polarity so the healthy state is `HIGH` and active timing cut drives the output `LOW` to ground IAT, matching the boost-cut output philosophy.
- 2026-03-29 compile validation after timing-cut output failsafe update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1170583` bytes, RAM `51424` bytes).
- 2026-04-02 LED terminology alignment update: red/blue indicator naming was standardized to `LED` across firmware, API, live UI, preview UI, and docs.
- 2026-04-02 spray-status output update: added green spray LED output on GPIO25, driven only when pump output is on and injector on-time is active.
- 2026-04-02 compile validation after LED terminology alignment and green spray LED update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .` (program `1170983` bytes, RAM `51424` bytes).
- 2026-03-02 compile validation after Analog Input Calibration section rename/order update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 compile validation after Sensor quick-check `% error` table update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 compile validation after Sensor table capture-button update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Sensor Equation input-init update: `mapOff`, `pLin`, and `pOff` now initialize with defaults in live UI and preview (`-11.25`, `32.5`, `-16.25`).
- 2026-03-02 compile validation after Sensor Equation input-init update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 runtime regression fix: Sensor quick-check `% error` formatting now uses strict finite guards in live UI and preview so blank measured cells show `-` and do not stop script execution.
- 2026-03-02 compile validation after Sensor quick-check runtime guard fix: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Sensor Calibration readability update: MAP fit, rail fit, and quick-check table heading lines now use a larger dedicated title style in live UI and preview.
- 2026-03-02 compile validation after Sensor Calibration table-title readability update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Sensor Calibration quick-check split update: quick-check is now two tables (`MAP Quick Check` and `Rail Quick Check`) with separate sensor-specific capture buttons.
- 2026-03-02 Sensor Calibration naming cleanup: table names simplified to `MAP Fit Table`, `Rail Fit Table`, `MAP Quick Check`, and `Rail Quick Check` with full descriptions moved under each title.
- 2026-03-02 compile validation after quick-check table split and naming cleanup: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Sensor Calibration ADC heading update: shared ADC fit table now uses a larger title line named `Shared ADC Curve Table` in live UI and preview.
- 2026-03-02 compile validation after ADC table-title update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 section color standard update: sectioned cards now use locked non-repeating colors in live UI and preview (`Section 1=hardware`, `Section 2=flow`, `Section 3=safety`, `Section 4=tune`, `Section 5=diag`).
- 2026-03-02 Wi-Fi section visual update: Wi-Fi Settings now follows the same standard with `Section 1=hardware`, `Section 2=flow`, `Section 3=safety`.
- 2026-03-02 compile validation after section color standard update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Status visual update: all Status card tiles now use one neutral gray style in live UI and preview.
- 2026-03-02 compile validation after Status visual update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Live Data visual update: all Live Data gauges now use one neutral gray style in live UI and preview.
- 2026-03-02 compile validation after Live Data visual update: pass for `arduino-cli compile --fqbn esp32:esp32:esp32da .`.
- 2026-03-02 Sensor Calibration naming alignment update: Section 2 renamed to `Sensor Preset Selection`, Section 3 renamed to `Custom Sensor Calibration`, and Section 3 table headings now use section-matched `Custom Sensor Calibration ...` wording in live UI and preview.
- 2026-03-02 Sensor Calibration helper-format update: shared ADC helper copy now uses a spaced bullet list in live UI and preview.
- 2026-03-02 Sensor Calibration typography update: table heading text now matches `Analog Input Calibration` title sizing/weight (`13px`, `900`, `.3px`) in live UI and preview.
- 2026-03-02 Sensor Calibration live-gauge duplication update: two-decimal MAP and rail calibration gauges are now duplicated above each Section 3 table in live UI and preview, wired to the same live smoothing path as the existing calibration gauge pair.
- 2026-03-02 Sensor Calibration table spacing and heading rename update: shared ADC heading is now `Analog Input Curve Table`, and table blocks now have additional vertical separation while gauges stay tight to their associated tables.
- 2026-03-02 Sensor Calibration gauge density alignment update: Section 3 now uses one sensor-relevant gauge per table (MAP on MAP tables, rail on rail tables) instead of both gauges on every table, while keeping gauge placement tight to each table.
- 2026-03-02 Sensor Calibration gauge width update: per-table calibration gauge rows now use full-width layout so each gauge box fills across the table area.
- 2026-03-02 Sensor Calibration spacing increase update: inter-table spacing was increased again by expanding `.cal-table-title` margins so tables are less visually crowded.
- 2026-03-02 Settings label terminology alignment update: Methanol Controller Settings and Wi-Fi Settings now display `Section` labels instead of `Step` labels in live UI and preview.
- 2026-03-02 Settings fault-state sectioning update: pressure-ready controls now have their own dedicated settings section, and Settings fault controls are grouped by fault state (`Low Level`, `Rail dP`, `Pressure-Ready`) before flow model and signal/diagnostics sections.
- 2026-03-02 Sensor Calibration visual consistency alignment update: calibration table titles, descriptions, and gauge rows were normalized to consistent spacing and layout; shared gauge-row classes now drive one-gauge and two-gauge row spacing in live UI and preview.
- Real vehicle/load validation: not yet executed.

### 2.1) 2026-02-24 Forced-State and Bypass Sweep (COM5, live API)
- Scope: exercised logic states using forced values plus per-state overrides and bypasses while user held physical inputs at MAP `0 V` and rail `0 V`.
- Method: assistant-driven `POST /api/config` force sequencing with `GET /api/status` assertions and assistant-managed board reboots between hold-latched scenarios.
- Verified pass states:
  - Baseline idle with no faults and no cuts.
  - Pressure-ready BUILDING and READY transitions from forced MAP/rail states.
  - Low level before spray: boost cut only (no timing cut).
  - Low level during spray: timing cut + boost cut, then clear when level returns OK.
  - dP monitor override state (`OVERRIDE`) with no dP fault assertion.
  - Manual triggers: boost cut, timing cut, rail dP fault, pressure-ready fault, dP boost hold.
  - dP fault bypass masks dP timing-cut behavior while low dP is forced.
  - Real dP fault path: timing cut then auto-clear to retained boost hold.
  - dP hold bypass masks retained boost hold without reboot.
- Clarification confirmed:
  - Pressure-ready override alone does not disable dP monitor safety paths; with low dP it can still transition into dP fault.
  - For map-only spray bench testing without dP fault intervention, use pressure-ready override together with dP monitor override (or dP fault bypass) as intended.

### 2.2) 2026-02-24 Assistant Full Self-Test Sweep (No User Inputs)
- Scope:
  - End-to-end assistant-driven API and runtime verification on the flashed controller over `watermeth.local`.
- Method:
  - Automated `POST /api/config` force and bypass sequencing.
  - Automated `GET /api/status` assertions per state.
  - ADC trim write/read/restore validation.
  - HTTP reachability checks on mDNS host and active host endpoint.
  - Assistant-managed reboot on COM5 and persistence verification for `sper`.
  - WebSocket status stream smoke test using host WebSocket client.
- Result summary:
  - 18 total checks.
  - 18 pass.
  - 0 fail.
- Verified pass checks:
  - Baseline zero-MAP no-spray behavior.
  - Baseline no-cut behavior.
  - Pressure-ready BUILDING state.
  - Pressure-ready READY state with injector output.
  - Manual Boost Cut behavior.
  - Manual Timing Cut behavior.
  - Manual Rail dP Fault behavior.
  - Manual Pressure-Ready Fault behavior.
  - Manual dP Boost Hold behavior.
  - dP Monitor Override state behavior.
  - Map-only spray path with required overrides (`Pressure-Ready Override` + `dP Monitor Override`).
  - Level Fault Bypass behavior.
  - dP Fault Bypass behavior.
  - dP Hold Bypass behavior.
  - ADC trim round-trip behavior (`adcGain`, `adcOffset`).
  - HTTP reachability via mDNS and host endpoint.
  - WebSocket status stream message receipt and key validation.
  - Persistence across assistant-managed reboot for `sper`, then restore to original value.

### 2.3) 2026-02-24 Final-Sweep Targeted Verification (Post-Fix)
- Scope:
  - Verified each approved final-sweep fix after flashing updated firmware.
- Method:
  - Static source verification for UI zero-value save logic in live UI and preview.
  - Live API readback tests for zero-value fields (`pressureReadyTimeoutMs`, `mapOff`, `pOff`, `sdbg`, `dpArmPct`, `dpRecover`).
  - Live reboot persistence test for `sdbg`.
  - Live curve duplicate-point post to verify strict-increase sanitation and stable interpolation output.
  - Live unknown-path HTTP request to verify captive redirect target behavior.
  - mDNS no-regression gate re-run before and after assistant-managed reboot, including serial boot capture for STA and mDNS lines.
- Result summary:
  - Final-sweep targeted checks: pass.
  - mDNS no-regression gate after Wi-Fi-touching updates: pass.

### 2.4) 2026-02-24 Targeted Re-Test of Prior Failing Logic Checks (No Full Suite Re-Run)
- Scope:
  - Re-tested only the three previously failing logic checks.
  - Did not re-run previously passing checks.
- Root cause of prior failures:
  - Test sequencing issue in the harness, not a firmware logic regression.
  - The pressure-ready-timeout test was executed after a retained `dpBoostHold` scenario, which intentionally blocks spray/pressure-build paths until reboot.
  - The critical fast-fault check used a narrow timing window against an intentionally short auto-clear setting, causing intermittent miss risk.
  - Serial-only reboot control was less deterministic than API-triggered restart in this setup.
- Harness updates:
  - Added API reboot path (`POST /api/config` with current `wifiMode`) with serial reboot fallback.
  - Added explicit reboot before pressure-ready-timeout fault check.
  - Widened critical fast-fault assertion to validate effective rail dP fault state (`boost cut + fault reason`) instead of only a brief `dpFault=true` sample.
- Targeted results:
  - `logic_dp_critical_fast_fault`: pass.
  - `logic_pressure_ready_timeout_fault`: pass.
  - `logic_map_only_spray_with_required_overrides`: pass.
- Result summary:
  - Targeted total: `3`.
  - Targeted pass: `3`.
  - Targeted fail: `0`.

### 2.5) 2026-02-24 Post-Closeout Assistant Spot Checks
- Scope:
  - Quick assistant-only post-closeout checks without re-running the full passing matrix.
- Checks and results:
  - Direct-IP status endpoint stability (`http://172.20.10.7/api/status`): `20/20` pass.
  - mDNS quick resolve sample (`watermeth.local`): `9/10` resolve pass during host-side sampling.
  - User confirmation in the same session: mDNS functional behavior accepted (`"mdns is fine"`).
  - Prior targeted failing logic checks remain resolved (`3/3` pass from Section 2.4).
- Notes:
  - A longer unattended host-side soak command was attempted and hit host command-time limits before completion; this is recorded as inconclusive and non-blocking for this release decision.

## 3) Firmware and Build Validation

- `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass.
- `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass.
- `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32da .`: pass.
- `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32da .`: pass (ADC trim slider build).
- `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32da .`: pass (theme-match and fault-controls cleanup build).
- Multiple post-flash runtime checks via `/api/status`, `/api/config`, and COM3 serial: pass.
- `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass after comment-only maintenance pass (`WebUI.ino`, `Preview UI/ui_preview.html`, and firmware helper/API/task comment updates).
- `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass after the same comment-only maintenance pass.
- `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass after final-sweep fixes.
- `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass after final-sweep fixes.
- `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32da .`: pass (final-sweep fix build).
- `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass after shared multi-point ADC curve calibration update.
- `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass after shared multi-point ADC curve calibration update (one parallel attempt failed due host file lock; retry succeeded).
- `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass after `targetInjectorDp` naming alignment update.
- `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass after `targetInjectorDp` naming alignment update.

## 4) Wi-Fi and mDNS Validation

- STA connection, UI load, serial operation, card behavior: user-confirmed pass.
- mDNS regression debugging and hardening completed.
- Host-side checks recorded:
  - DNS resolve stability run: `40/40` success.
  - HTTP `.local` reachability run: `30/30` success.
- Browser confirmation: `.local` working in Edge and Chrome after fixes.
- Post-fix mDNS no-regression gate rerun passed:
  - Pre-reboot: `mdnsHost` readback valid, direct STA IP HTTP load pass, `.local` HTTP load pass.
  - Post-reboot: same checks pass.
  - Serial boot capture confirms STA IP line and `[MDNS] Started` line.
- 2026-02-24 STA reconnect hardening follow-up:
  - Local host reachability checks during this edit session did not find a live controller at `watermeth.local`, `172.20.10.7`, or `172.20.10.8`, so the hardware mDNS no-regression gate for this specific reconnect change is pending the next live flash-and-check session.

## 5) Persistence and Defaults Validation

- Save persistence across reboot: pass (user-confirmed and API-confirmed).
- Save-triggered reboot behavior for Wi-Fi settings: pass.
- Defaults-on-flash behavior:
  - New firmware build detects build tag change.
  - Loads firmware defaults once on first boot of that build.
  - Persists user changes across normal reboots after that.
- Pressure-ready timeout default update to `2500 ms`:
  - Firmware/UI/preview/docs synchronized and verified.
- Serial debug preference (`sdbg`) persistence:
  - Verified OFF value persists across assistant-managed reboot.
  - Restored to ON after validation.

## 6) Safety and Control Logic Validation

Validated by direct automation and bench checks:
- Pressure-ready building and ready transitions: pass.
- Pressure-ready timeout fault path: pass.
- dP Monitor Override path: pass (override ON keeps `dpState=OVERRIDE` and pauses only automatic dP monitor fault logic).
- Pressure-Ready Override path: pass (override ON bypasses pressure-ready injector blocking and pressure-ready timeout fault latching, while dP monitor safety remains active unless separately bypassed/overridden).
- Injector dP fault before spray: pass.
- Injector dP fault during spray: pass.
- Critical low-pressure fast fault path: pass.
- Timing-cut auto-clear with retained boost hold until reboot: pass.
- Low level before spray (boost cut only): pass.
- Low level during spray (timing + boost cut): pass.
- Manual latches and dP monitor override behavior: pass.

Important bench sequencing note:
- For force-based pressure-ready and timeout tests, first force injectors OFF briefly and keep forced rail low until filtered rail settles low, then start the scenario. This avoids stale pressure-ready state from prior high-rail force conditions.

## 7) Physical Input Calibration Results

Reference basis:
- Full calibration model and conversion path are documented in `CALIBRATION_REFERENCE.md`.

### 7.1) MAP Input Direct Sweep (Passed)
- Basis: `mapLin=312.5`, `mapOff=-11.25`, `mapDiv=0.66`.
- Results:
  - `0.00 V`: measured `-11.2`, expected `-11.3`, error `+0.1 kPa`
  - `0.50 V`: measured `36.0`, expected `36.1`, error `-0.1 kPa`
  - `1.00 V`: measured `83.7`, expected `83.4`, error `+0.3 kPa`
  - `1.50 V`: measured `131.6`, expected `130.8`, error `+0.8 kPa`
  - `2.00 V`: measured `178.3`, expected `178.1`, error `+0.2 kPa`
  - `2.50 V`: measured `226.6`, expected `225.5`, error `+1.1 kPa`
  - `3.00 V`: measured `275.0`, expected `272.8`, error `+2.2 kPa`
  - `3.30 V`: measured `301.2`, expected `301.3`, error `-0.1 kPa`

### 7.2) Rail Pressure Sensor Input Direct Sweep (Passed)
- Basis: `pLin=32.5`, `pOff=-16.25`, `railDiv=0.66`.
- Results:
  - `0.00 V`: measured `-16.2`, expected `-16.3`, error `+0.1 psi`
  - `0.50 V`: measured `8.7`, expected `8.4`, error `+0.3 psi`
  - `1.00 V`: measured `33.1`, expected `33.0`, error `+0.1 psi`
  - `1.50 V`: measured `58.0`, expected `57.6`, error `+0.4 psi`
  - `2.00 V`: measured `82.9`, expected `82.2`, error `+0.7 psi`
  - `2.50 V`: measured `107.6`, expected `106.9`, error `+0.7 psi`
  - `3.00 V`: measured `132.5`, expected `131.5`, error `+1.0 psi`
  - `3.30 V`: measured `146.2`, expected `146.3`, error `-0.1 psi`

### 7.3) Replacement Board COM5 Rail 3-Point Check (Locked for Session)
- Context:
  - Controller was moved to replacement hardware on COM5 after flash instability on prior board.
  - Rail pressure sensor input was rechecked with direct input injection and reboot repeatability.
- Results:
  - `0.00 V`: measured `-16.25 psi`
  - `1.50 V`: measured `57.0 to 57.4 psi`
  - `3.30 V`: measured `140.56 psi`
- Repeatability:
  - Assistant-triggered reboot was run between high-point checks.
  - `3.30 V` rail reading returned to the same `140.56 psi`.
- Session lock:
  - These COM5 three-point rail results are locked for this active bench session and used as the reference baseline moving forward.

### 7.4) Replacement Board COM5 MAP 3-Point Check (Locked for Session)
- Context:
  - After correcting MAP wiring on the replacement COM5 controller, MAP direct-input checks were rerun as a fast three-point set.
- Results:
  - `0.00 V`: measured `-11.25 kPa`
  - `2.20 V`: measured `194.7 to 196.2 kPa`
  - `3.30 V`: measured `290.31 kPa`
- Session lock:
  - These COM5 three-point MAP results are locked for this active bench session and used as the reference baseline moving forward.

### 7.5) Active Rail Full Sweep Re-Validation (Historical Pre-Fix Record)
- Date:
  - 2026-02-23
- Context:
  - User requested a fresh full rail pressure sensor sweep because live values were not tracking expected points.
  - Sweep was run at `0.00 / 0.50 / 1.00 / 1.50 / 2.00 / 2.50 / 3.00 / 3.30 V` on live hardware.
- Results before fix:
  - `0.00 V`: `-9.258 psi` (fail vs expected `-16.250`)
  - `0.50 V`: `9.926 psi` (fail vs expected `8.371`)
  - `1.00 V`: `34.733 psi` (fail vs expected `33.000`)
  - `1.50 V`: `59.625 psi` (fail vs expected `57.629`)
  - `2.00 V`: `84.182 psi` (fail vs expected `82.258`)
  - `2.50 V`: `108.681 psi` (fail vs expected `106.886`)
  - `3.00 V`: `132.422 psi` (pass vs expected `131.515`)
  - `3.30 V`: `137.830 psi` (fail vs expected `146.250`)
- Action taken under explicit user approval (`fix rail cal`):
  - `Helpers.ino` `adcVolts(...)` was updated to averaged raw ADC scaling only.
  - Historical note: this was later superseded by the 2026-02-28 cross-board calibration-path update to calibrated millivolt reads.
- Legacy interim actions:
  - Runtime rail and MAP sensor constants were temporarily adjusted during bench iteration to confirm fit behavior.
  - Those interim sensor-constant overrides are superseded by the ADC-trim architecture below.
- Final architecture action under user direction:
  - MAP constants remain in plain 5V span/offset form (`mapLin` kPa over 0-5V, `mapOff` kPa at 0V).
  - Rail pressure sensor constants remain in plain slope/offset form (`pLin` psi/V, `pOff` psi).
  - Board-level correction now uses shared ADC trims:
    - `adcGain`, `adcOffset` (applied to both MAP and rail)
  - ADC trim controls were added to firmware API, preference persistence, live UI, and preview UI.
- Status:
  - This section is retained as historical context for the pre-fix mismatch and the correction path.
  - ADC trim slider round-trip sanity check on COM5 passed (shared `adcGain` update and restore persisted).
  - Firmware and UI preset defaults were then updated to a shared ADC baseline for both inputs:
    - historical session baseline: `adcGain=0.9835`, `adcOffset=0.117`
  - Follow-up stability action:
    - After confirming the instability source was wiring, advanced filter experiments were removed.
    - Control loop returned to baseline smoothing (`alpha=0.15`) with 8-sample ADC averaging.

### 7.6) Calibration Coverage Matrix (Current)
- Covered and logged:
  - Full 8-point MAP sweep (`0.00` to `3.30 V`) with measured vs expected values (Section 7.1).
  - Full 8-point rail pressure sensor sweep (`0.00` to `3.30 V`) with measured vs expected values (Section 7.2).
  - Replacement COM5 locked 3-point rail validation (Section 7.3).
  - Replacement COM5 locked 3-point MAP validation (Section 7.4).
- Active default shared-trim reference (firmware/UI defaults):
  - `adcGain=1.0000`, `adcOffset=0.000`, `adcCurveY=[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`, `mapLin=312.5`, `mapOff=-11.25`, `mapDiv=0.66`, `pLin=32.5`, `pOff=-16.25`, `railDiv=0.66`.
  - Expected MAP at controller input:
    - `0.00 V -> -11.250 kPa`
    - `0.50 V -> 36.098 kPa`
    - `1.00 V -> 83.447 kPa`
    - `1.50 V -> 130.795 kPa`
    - `2.00 V -> 178.144 kPa`
    - `2.50 V -> 225.492 kPa`
    - `3.00 V -> 272.841 kPa`
    - `3.30 V -> 301.250 kPa`
  - Expected rail pressure sensor at controller input:
    - `0.00 V -> -16.250 psi`
    - `0.50 V -> 8.371 psi`
    - `1.00 V -> 32.992 psi`
    - `1.50 V -> 57.614 psi`
    - `2.00 V -> 82.235 psi`
    - `2.50 V -> 106.856 psi`
    - `3.00 V -> 131.477 psi`
    - `3.30 V -> 146.250 psi`
- Remaining calibration evidence gap:
  - A fresh physical 8-point sweep on replacement hardware using current defaults (neutral gain/offset plus active shared ADC curve) on the calibrated-millivolt ADC path is not yet logged in this document.

### 7.7) Recovered Historical Calibration Records (No New Bench Run)
- Source:
  - Recovered from locked checklist and calibration reference records so previous calibration work stays documented.
- Recovered rail pressure sensor confirmation set (interim constants validation record):
  - `0.50 V -> 8.442 psi`
  - `1.50 V -> 57.604 psi`
  - `2.50 V -> 106.978 psi`
- Recovered replacement COM5 rail lock set:
  - `0.00 V -> -16.25 psi`
  - `1.50 V -> 57.0 to 57.4 psi`
  - `3.30 V -> 140.56 psi`
- Recovered replacement COM5 MAP lock set:
  - `0.00 V -> -11.25 kPa`
  - `2.20 V -> 194.7 to 196.2 kPa`
  - `3.30 V -> 290.31 kPa`
- Status:
  - These records are intentionally retained as historical calibration evidence and were not regenerated in this pass.

### 7.8) Cross-Board ADC Conversion Consistency Update (No New Bench Sweep Yet)
- Date:
  - 2026-02-28
- Firmware and UI changes under explicit user approval:
  - `adcVolts(...)` now averages `analogReadMilliVolts(...)` results for MAP and rail input conversion.
  - Shared ADC startup defaults are now neutral in firmware, live UI, and preview UI (`adcGain=1.0000`, `adcOffset=0.000`).
  - Shared ADC sanitize fallback defaults are also neutralized to match startup behavior.
- Validation completed this pass:
  - `arduino-cli compile --fqbn esp32:esp32:esp32da .`: pass.
  - `arduino-cli compile --warnings all --fqbn esp32:esp32:esp32da .`: pass.
- Validation still pending:
  - Fresh physical sweep evidence on the replacement board with frozen settings after endpoint trim on the updated ADC conversion path.

## 8) Input-to-Logic Cross-Checks (One Supply)

Method:
- One input physical, other input forced in API, then verify live logic state.

Verified examples:
- Forced MAP (`178 kPa`) with physical rail:
  - Low rail input produced not-ready behavior (`pressureReady=false`, `injOn=false`).
  - High rail input produced ready/injection behavior (`pressureReady=true`, `injOn=true`).
- Physical level switch checks:
  - Open level switch held low-level boost cut.
  - Grounded level switch cleared low-level boost cut.

### 8.1) COM5 Inputs-to-Logic Validation (MAP Live, Rail Forced)
- Date:
  - 2026-02-23
- Setup:
  - `forceRailPsi=120` used to simulate ready rail while MAP stayed on live physical input.
  - All manual latches and overrides OFF.
- Checks:
  - MAP `0.0 V`:
    - `map=-11.25`, `duty=0`, `pump=0`, `inj=0`, `pressureReady=0` (pass).
  - MAP `2.2 V`:
    - `map=195.4 to 196.9`, `duty~49 to 51%`, `pump=1`, `inj=1`, `pressureReady=1`, no cuts (pass).
  - Level switch opened while MAP stayed high:
    - `timingCut=1`, `boostCut=1`, `red=1`, `blue=1`, `duty=0`, `pump=0`, `inj=0` (pass).
  - Level switch grounded again:
    - `timingCut=0`, `boostCut=0`, `red=0`, `blue=0`, `pump=1`, `inj=1` resumed with MAP high (pass).
- Result:
  - Live MAP and live level input transitions drove the expected safety and output state transitions.

### 8.2) COM5 Inputs-to-Logic Validation (Rail Live, MAP Forced)
- Date:
  - 2026-02-23
- Setup:
  - `forceMapKpa=178` used to hold fixed spray demand while rail stayed on live physical input.
  - All manual latches and overrides OFF.
- Checks:
  - Rail `0.0 V`:
    - `rail=-16.25`, `pressureReady=0`, `inj=0`, `duty=0`, `pump=1` (building), no cuts (pass).
  - Rail `3.3 V`:
    - `rail=140.56`, `pressureReady=1`, `inj=1`, `duty=28`, `pump=1`, no cuts (pass).
  - Level switch opened while rail stayed high:
    - `timingCut=1`, `boostCut=1`, `red=1`, `blue=1`, `duty=0`, `pump=0`, `inj=0` (pass).
  - Level switch grounded again:
    - `timingCut=0`, `boostCut=0`, `red=0`, `blue=0`, `pump=1`, `inj=1` resumed with forced MAP demand (pass).
- Result:
  - Live rail and live level input transitions drove the expected pressure-ready gating, safety cut behavior, and recovery behavior.

### 8.3) COM5 dP Monitor Override Regression Check (Automated API)
- Date:
  - 2026-02-23
- Setup:
  - Controller rebooted after flash on COM5.
  - Forced test scenario: `forceInj=2`, `forceDuty=20`, `forceLevel=2`, `forceMapKpa=220`, `forceRailPsi=0`, `pressureReadyTimeoutMs=400`.
- Checks:
  - Override ON (`forceDpMonitorOverride=true`):
    - `dpState=OVERRIDE`, `dpFault=false`, and no automatic dP fault latch while override remained ON (pass).
  - Override OFF (`forceDpMonitorOverride=false`) with same forced MAP/rail and timeout:
    - `dpState=TIMING_CUT`, `dpFault=true`, `pressureReady=false`, `sprayPct=0`, `injOn=false` (pass).
- Result:
  - dP Monitor Override is isolated to dP monitor pause behavior.

### 8.4) COM5 Pressure-Ready Override Validation (Automated API)
- Date:
  - 2026-02-23
- Setup:
  - Forced test scenario: `forceInj=2`, `forceDuty=20`, `forceLevel=2`, `forceMapKpa=220`, `forceRailPsi=0`, `pressureReadyTimeoutMs=400`.
- Checks:
  - Override ON (`forcePressureReadyOverride=true`):
    - `pressureReady=true`, `sprayPct=20`, `injOn=true`, `dpState=IDLE` (no dP monitor override active) (pass).
  - Override OFF (`forcePressureReadyOverride=false`) with same forced MAP/rail and timeout:
    - `dpState=TIMING_CUT`, `dpFault=true`, `pressureReady=false`, `sprayPct=0`, `injOn=false` (pass).
- Result:
  - Pressure-Ready Override now provides separate bench spray gating bypass without changing dP monitor override semantics.

### 8.5) COM5 Timing-Cut and Boost-Cut Synchronization Check (Automated API)
- Date:
  - 2026-02-23
- Setup:
  - Controller reset to clear retained latches before run.
  - Forced scenario: `forceInj=2`, `forceDuty=20`, `forceLevel=2`, `forceMapKpa=220`, `forceRailPsi=0`, `pressureReadyTimeoutMs=400`, all overrides OFF.
- Observed transition:
  - Baseline: `dpState=IDLE`, `dpFault=false`, `dpHold=false`, `timing=false`, `boostCut=false`.
  - Fault edge sample: `dpState=TIMING_CUT`, `dpFault=true`, `dpHold=true`, `timing=true`, `boostCut=true` (same sampled state).
  - After timing auto-clear delay: `dpState=BOOST_HOLD`, `dpFault=false`, `dpHold=true`, `timing=false`, `boostCut=true`.
- Result:
  - Timing cut and boost cut now assert together on the real fault edge, and boost cut retention remains active after timing cut auto-clear until power cycle.

## 9) Known Limits of Test Coverage

- No persistent file logging is currently implemented in firmware.
  - Runtime visibility is via serial stream and live API/UI only.
- No full in-vehicle dynamic/load validation recorded yet.
- Bench validation with one power supply cannot drive physical MAP and rail inputs simultaneously without force-mode substitution.
