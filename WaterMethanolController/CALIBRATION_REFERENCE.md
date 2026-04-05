# Calibration Reference (Locked)

Purpose:
- This is the dedicated source of truth for sensor calibration values, input conversion behavior, and calibration-related code additions.
- Keep this document updated in the same pass whenever calibration logic or constants change.

## 1) Calibration Model

### 1.1) MAP (manifold absolute pressure)
- Runtime formula:
  - `mapAdcVoltsLinear = (mapAdcVoltsRaw * adc_input_gain) + adc_input_offset_v`
  - `mapAdcVoltsCorrected = adcCurve(mapAdcVoltsLinear)` (shared piecewise correction)
  - `mapSensorVolts = mapAdcVoltsCorrected / map_div_ratio`
  - `mapKpa = (map_kpa_linear_hpt / 5.0) * mapSensorVolts + map_kpa_offset_hpt`
- Default values:
  - `map_div_ratio = 0.66`
  - `adc_input_gain = 1.0000` (shared with rail pressure sensor input)
  - `adc_input_offset_v = 0.000` (shared with rail pressure sensor input)
  - `adcCurveY` default at breakpoints `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]` is identity `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`
  - `map_kpa_linear_hpt = 312.5`
  - `map_kpa_offset_hpt = -11.25`

### 1.2) Rail pressure sensor
- Runtime formula:
  - `railAdcVoltsLinear = (railAdcVoltsRaw * adc_input_gain) + adc_input_offset_v`
  - `railAdcVoltsCorrected = adcCurve(railAdcVoltsLinear)` (shared piecewise correction)
  - `railSensorVolts = railAdcVoltsCorrected / rail_div_ratio`
  - `railPsi = rail_psi_per_v * railSensorVolts + rail_psi_offset`
- Default values:
  - `rail_div_ratio = 0.66`
  - `adc_input_gain = 1.0000` (shared with MAP input)
  - `adc_input_offset_v = 0.000` (shared with MAP input)
  - `adcCurveY` default at breakpoints `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]` is identity `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`
  - `rail_psi_per_v = 32.5`
  - `rail_psi_offset = -16.25`

## 2) Input Voltage Conversion Path

Code location:
- `Helpers.ino`, function `adcVolts(int pin)`.

Current behavior:
- Eight calibrated millivolt samples are averaged per read cycle.
- The averaged millivolt value is converted to volts with:
  - `adcVolts = millivolts * 0.001`
- Calibrated millivolt reads are sourced from `analogReadMilliVolts(...)`, which uses the ESP32 calibration path.
- Shared ADC correction is then applied in `ControlLoop.ino`:
  - `linear = (rawVolts * adc_input_gain) + adc_input_offset_v`
  - `corrected = adcCurve(linear)` using shared piecewise linear interpolation.
- Output from `adcVolts(...)` is clamped to `0.0 V` through `ADC_VREF`.
- Post-trim corrected voltage in `ControlLoop.ino` is intentionally not clamped so endpoint calibration remains meaningful.
- No fixed board-specific lookup table is applied.
- Live control filtering in `ControlLoop.ino` uses one simple exponential moving average:
  - `alpha = 0.15` for both MAP and rail pressure.

## 3) Calibration-Related Code Additions

### 2026-02-23 - Input conversion update
- File: `Helpers.ino`
  - `adcVolts(...)` was moved from single-sample raw scaling to a short multi-sample path.
- File: `Helpers.ino`
  - Board-specific lookup correction was removed so ADC conversion behavior is consistent across same-hardware boards for flash-and-go deployment.
- Files updated with documentation alignment:
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `AGENTS.md`

### 2026-02-23 - Rail calibration fix pass
- File: `Helpers.ino`
  - `adcVolts(...)` was updated to use averaged raw-count conversion as the only path.
  - Reason: active rail bench sweep showed top-end compression and offset drift with the calibrated millivolt path on current hardware.
  - Goal: restore linear full-range input reporting for rail and MAP direct-input bench calibration.
- Files updated with documentation alignment:
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`

### 2026-02-23 - Active COM5 rail runtime constants update (Legacy Interim)
- Update type:
  - Runtime rail calibration constants were updated through `/api/config` under explicit user approval (`update rail constants`).
- Applied values:
  - `pLin = 31.969`
  - `pOff = -10.517`
  - `railDiv = 0.66` (unchanged)
- Scope:
  - Active bench session on replacement COM5 controller.
  - This update does not change firmware default constants.
- Follow-up:
  - A rail 3-point confirmation run is required immediately after this update.
  - Confirmation result: pass (`0.50 V -> 8.442 psi`, `1.50 V -> 57.604 psi`, `2.50 V -> 106.978 psi`).
  - This interim sensor-constant update is superseded by the ADC trim architecture below.

### 2026-02-23 - Active COM5 MAP runtime constants update (Legacy Interim)
- Update type:
  - Runtime MAP calibration constants were updated through `/api/config` after a full direct-input sweep.
- Applied values:
  - `mapLin = 307.309`
  - `mapOff = -0.180`
  - `mapDiv = 0.66` (unchanged)
- Fit basis:
  - Mid-range least-squares fit on measured `0.50 V` through `2.50 V` points from the active session sweep.
- Scope:
  - Active bench session on replacement COM5 controller.
  - This update does not change firmware default constants.
- Follow-up:
  - A MAP 3-point confirmation run is required immediately after this update.
  - This interim sensor-constant update is superseded by the ADC trim architecture below.

### 2026-02-23 - Manufacturer-calibration lock plus ADC trim layer
- Files:
  - `WaterMethanolController.ino`
  - `ControlLoop.ino`
  - `Helpers.ino`
  - `Prefs.ino`
  - `Api.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
- Behavior:
  - MAP calibration fields (`mapLin/mapOff`) are plain 5V span/offset (`kPa over 0-5V`, `kPa at 0V`).
  - Rail pressure sensor calibration fields (`pLin/pOff`) are plain slope/offset (`psi/V`, `psi`).
  - Shared ADC trim fields are exposed for board-level correction:
    - `adcGain`, `adcOffset`
  - Historical note: this pass locked shared ADC trim defaults to bench baseline (`gain=0.9835`, `offset=0.117 V`) for that session.
  - This historical default lock was superseded by the 2026-02-28 cross-board consistency update, which restored neutral startup defaults.
  - A single shared correction is intentional because the observed ADC correction was a common front-end error across both input paths.
  - ADC trim values persist in preferences and are sanitized before use.
  - COM5 post-flash sanity check confirmed API round-trip and persistence for shared ADC trim update and restore.

### 2026-02-23 - Shared slider and keyboard adjustment consolidation
- Files:
  - `WaterMethanolController.ino`
  - `ControlLoop.ino`
  - `Helpers.ino`
  - `Prefs.ino`
  - `Api.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
- Behavior:
  - UI now uses one shared ADC gain slider and one shared ADC offset slider for both MAP and rail.
  - Sliders support click, drag, and keyboard adjustment (`Arrow`, `Page Up/Down`, `Home`, `End`).
  - Shared ADC trim slider span is intentionally broad for flash-and-go cross-board coverage:
    - `adcGain`: `0.8000` to `1.2000`
    - `adcOffset`: `-0.500 V` to `+0.500 V`
  - Live UI and preview include a quick endpoint workflow note for end users:
    - apply `0.0 V` and `3.3 V` at the sensor input and adjust shared gain/offset until low and high readings match expected endpoints.
  - API keeps legacy per-input keys for compatibility, but internally applies one shared ADC correction pair.

### 2026-02-24 - Calibration evidence consolidation update
- Files:
  - `CALIBRATION_REFERENCE.md`
  - `TEST_RESULTS_MASTER.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
- Documentation behavior:
  - Historical unity-trim sweep records are now explicitly labeled as historical references.
  - Active shared-trim default expected curves are centralized in Section 4 for both MAP and rail direct-input injection.
  - Remaining calibration evidence gap is explicitly tracked in `TEST_RESULTS_MASTER.md` Section 7.6.

### 2026-02-26 - Sensor Calibration live gauge precision update
- Files:
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Documentation behavior:
  - Sensor Calibration now shows a dedicated two-decimal live gauge pair for MAP (kPa abs) and rail pressure (psig).
  - The new calibration gauges are wired to the same live status signals used by the main Live Data card.

### 2026-02-27 - Sensor Calibration display smoothing update
- Files:
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Behavior:
  - Sensor Calibration two-decimal MAP and rail gauges now apply a short display-only exponential smoothing step to reduce visual flicker during bench calibration.
  - Smoothing state is local to calibration gauge rendering and uses the same live status feed.
  - Control-loop math, ADC conversion, sensor equations, and safety logic are unchanged.

### 2026-02-27 - ADC endpoint-calibration validity fix
- Files:
  - `ControlLoop.ino`
  - `WaterMethanolController.ino`
  - `AGENTS.md`
  - `CALIBRATION_REFERENCE.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
  - `MethController_User_Guide_V4.7.md`
- Behavior:
  - Post-trim ADC clamping in `ControlLoop.ino` was removed so corrected ADC voltage remains linear after shared gain and offset correction.
  - This makes `0.0 V` and `3.3 V` usable endpoint anchors for ADC calibration on the current firmware path.
  - Raw ADC conversion remains bounded inside `adcVolts(...)`.

### 2026-02-28 - Cross-board ADC conversion consistency update
- Files:
  - `Helpers.ino`
  - `WaterMethanolController.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `AGENTS.md`
  - `CALIBRATION_REFERENCE.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
  - `MethController_User_Guide_V4.7.md`
- Behavior:
  - `adcVolts(...)` now averages calibrated millivolt reads from `analogReadMilliVolts(...)` instead of scaling raw ADC counts with a fixed reference.
  - Shared ADC trim defaults were reset to neutral startup values (`adcGain=1.0000`, `adcOffset=0.000 V`) for portable flash-and-go behavior on random ESP32 boards.
  - Existing shared ADC gain and offset trim workflow remains in place for residual per-board correction.
  - No per-board lookup table was introduced.

### 2026-02-28 - Shared multi-point ADC curve calibration update
- Files:
  - `WaterMethanolController.ino`
  - `Helpers.ino`
  - `ControlLoop.ino`
  - `Prefs.ino`
  - `Api.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `CALIBRATION_REFERENCE.md`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Behavior:
  - Added shared piecewise ADC correction curve for both MAP and rail input paths.
  - Fixed breakpoints are `0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3` volts.
  - Curve endpoints are hard-locked to `0.0 V` and `3.3 V` so endpoint anchors stay meaningful.
  - Midpoints are persisted and sanitized as monotonic nondecreasing values.
  - Sensor Calibration card now includes a multi-point table that accepts per-breakpoint MAP/rail readings and computes fitted curve outputs for shared ADC correction.
  - Curve fit updates runtime config and applies on Save.

### 2026-03-01 - Flash baseline ADC curve update for Barry reflash
- Files:
  - `WaterMethanolController.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `CALIBRATION_REFERENCE.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
  - `MethController_User_Guide_V4.7.md`
- Behavior:
  - Firmware default shared ADC curve output points were set to:
    - `[0.0, 0.4850, 0.9820, 1.4800, 1.9770, 2.4610, 2.9730, 3.3]`
  - Live UI and preview UI defaults were synchronized to the same curve so first-boot defaults and UI defaults match.
  - Shared ADC gain/offset startup defaults remain neutral (`adcGain=1.0000`, `adcOffset=0.000 V`).
  - This entry is historical and was later superseded by restoring identity startup defaults.

### 2026-03-02 - ADC preset revert from Barry-specific baseline to identity
- Files:
  - `WaterMethanolController.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `CALIBRATION_REFERENCE.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
  - `MethController_User_Guide_V4.7.md`
- Behavior:
  - Shared ADC curve startup defaults were restored to identity:
    - `[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`
  - Barry-specific startup curve values remain documented as historical only.
  - Shared ADC gain/offset startup defaults remain neutral (`adcGain=1.0000`, `adcOffset=0.000 V`).

### 2026-02-28 - MAP and rail sensor equation fit tables update
- Files:
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Behavior:
  - Added MAP point-entry table in Sensor Equation Values using sensor-side `0-5 V` plus MAP `kPa` pairs.
  - Added rail point-entry table in Sensor Equation Values using sensor-side `0-5 V` plus rail `psi` pairs.
  - Each table applies linear best-fit directly to calibration fields:
    - MAP table fits `mapLin` and `mapOff`.
    - Rail table fits `pLin` and `pOff`.
  - Later updates (2026-03-02) set both MAP and rail table voltage presets to `0.5-4.5 V` and added expected-value reference columns.

### 2026-03-02 - Sensor equation table range and expected-reference update
- Files:
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `CALIBRATION_REFERENCE.md`
  - `MethController_User_Guide_V4.7.md`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Behavior:
  - MAP point-entry table now uses sensor-side `0.5-4.5 V` presets.
  - Rail point-entry table uses sensor-side `0.5-4.5 V` presets.
  - Both tables now include a third expected-value reference column:
    - MAP expected `kPa` from current `mapLin/mapOff`.
    - Rail expected `psi` from current `pLin/pOff`.
  - Sensor Equation Values quick check is now split into two tables:
    - `MAP Quick Check` for MAP measured/expected/error.
    - `Rail Quick Check` for rail measured/expected/error.
  - MAP fit rows, rail fit rows, MAP quick-check rows, and rail quick-check rows include per-row `Capture` buttons.
  - Quick-check capture buttons are sensor-specific and update only the selected sensor table.
  - Legacy rail two-point quick-calc UI controls (`P1/V1`, `P2/V2`) were removed from live UI and preview.
  - Shared ADC section label is now `Analog Input Calibration` and is explicitly ordered first in Sensor Calibration.
  - UI note now states table voltages are measured at the divider input (sensor side), with explicit `3.3 V` controller-input protection note (default divider `0.66`: `0.5-4.5 V` divider input corresponds to `0.33-2.97 V` controller input).
  - Quick-check `% error` display now uses strict finite-number guards so blank measured cells display `-` instead of throwing runtime exceptions.
  - MAP fit, rail fit, MAP quick check, and rail quick check heading lines now use a dedicated larger title style with simplified names for calibration readability.
  - Shared ADC fit table heading is now `Shared ADC Curve Table` and uses the same larger title style for consistency.
  - Fit logic is unchanged and still applies linear best-fit into `mapLin/mapOff` and `pLin/pOff`.

### 2026-02-28 - Target injector dP naming alignment update
- Files:
  - `Api.ino`
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`
  - `LOCKED_BEHAVIOR_CHECKLIST.md`
  - `TEST_RESULTS_MASTER.md`
- Behavior:
  - Primary configuration field for spray-enable/flow reference dP is now `targetInjectorDp`.
  - Legacy write key `desiredRailDp` is still accepted in API POST handling for compatibility.

## 4) Active Default Bench Reference (MAP and Rail Input Direct)

With active firmware/UI defaults (`mapLin=312.5`, `mapOff=-11.25`, `mapDiv=0.66`, `pLin=32.5`, `pOff=-16.25`, `railDiv=0.66`, `adc_input_gain=1.0000`, `adc_input_offset_v=0.000`, `adcCurveY=[0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3]`), expected direct-input values are:

Expected MAP for direct MAP-input injection:
- `0.00 V -> -11.3 kPa`
- `0.50 V -> 36.1 kPa`
- `1.00 V -> 83.4 kPa`
- `1.50 V -> 130.8 kPa`
- `2.00 V -> 178.1 kPa`
- `2.50 V -> 225.5 kPa`
- `3.00 V -> 272.8 kPa`
- `3.30 V -> 301.3 kPa`

Expected rail pressure sensor value for direct rail-input injection:
- `0.00 V -> -16.3 psi`
- `0.50 V -> 8.4 psi`
- `1.00 V -> 33.0 psi`
- `1.50 V -> 57.6 psi`
- `2.00 V -> 82.3 psi`
- `2.50 V -> 106.9 psi`
- `3.00 V -> 131.5 psi`
- `3.30 V -> 146.3 psi`

## 5) Guardrails

- Any change to calibration formulas, constants, or defaults must update this file in the same pass.
- Keep wording consistent across:
  - firmware comments,
  - live UI text,
  - preview UI text,
  - `MethController_User_Guide_V4.7.md`,
  - `LOCKED_BEHAVIOR_CHECKLIST.md`.

## 6) Latest MAP Bench Validation (Input Direct)

Date:
- 2026-02-23

Test basis:
- `mapLin=312.5`, `mapOff=-11.25`, `mapDiv=0.66`
- Historical sweep basis used legacy unity trim (`adcGain=1.0000`, `adcOffset=0.000`) for expected-value comparison.
- MAP injected directly at controller MAP input, controller ground as return.
- Serial source: COM3 `[STAT] map=...kPa`

Results:
- `0.00 V`: measured `-11.2`, expected `-11.3`, error `+0.1 kPa`
- `0.50 V`: measured `36.0`, expected `36.1`, error `-0.1 kPa`
- `1.00 V`: measured `83.7`, expected `83.4`, error `+0.3 kPa`
- `1.50 V`: measured `131.6`, expected `130.8`, error `+0.8 kPa`
- `2.00 V`: measured `178.3`, expected `178.1`, error `+0.2 kPa`
- `2.50 V`: measured `226.6`, expected `225.5`, error `+1.1 kPa`
- `3.00 V`: measured `275.0`, expected `272.8`, error `+2.2 kPa`
- `3.30 V`: measured `301.2`, expected `301.3`, error `-0.1 kPa`

Status:
- MAP input calibration is considered dialed for this bench setup.
 - This section is a historical sweep record; active firmware defaults now use shared ADC trims and therefore a different expected-value curve (Section 4).

## 7) Latest Rail Pressure Sensor Bench Validation (Input Direct)

Date:
- 2026-02-23

Test basis:
- `pLin=32.5`, `pOff=-16.25`, `railDiv=0.66`
- Historical sweep basis used legacy unity trim (`adcGain=1.0000`, `adcOffset=0.000`) for expected-value comparison.
- Rail input injected directly at controller rail input, controller ground as return.
- Serial source: COM3 `[STAT] rail=...psi`

Results:
- `0.00 V`: measured `-16.2`, expected `-16.3`, error `+0.1 psi`
- `0.50 V`: measured `8.7`, expected `8.4`, error `+0.3 psi`
- `1.00 V`: measured `33.1`, expected `33.0`, error `+0.1 psi`
- `1.50 V`: measured `58.0`, expected `57.6`, error `+0.4 psi`
- `2.00 V`: measured `82.9`, expected `82.2`, error `+0.7 psi`
- `2.50 V`: measured `107.6`, expected `106.9`, error `+0.7 psi`
- `3.00 V`: measured `132.5`, expected `131.5`, error `+1.0 psi`
- `3.30 V`: measured `146.2`, expected `146.3`, error `-0.1 psi`

Status:
- Rail pressure sensor input calibration is considered dialed for this bench setup.
 - This section is a historical sweep record; active firmware defaults now use shared ADC trims and therefore a different expected-value curve (Section 4).

## 8) Replacement Board COM5 Rail 3-Point Validation (Session Lock)

Date:
- 2026-02-23

Test basis:
- `pLin=32.5`, `pOff=-16.25`, `railDiv=0.66`
- Rail pressure sensor input injected directly at the controller rail input, controller ground as return.
- Serial and API source: COM5 `/api/status` and `[STAT] rail=...psi`.

Results:
- `0.00 V`: measured `-16.25 psi`
- `1.50 V`: measured `57.0 to 57.4 psi`
- `3.30 V`: measured `140.56 psi`

Repeatability check:
- Assistant-triggered reboot was performed, then `3.30 V` was rechecked.
- High-point reading remained `140.56 psi`.

Status:
- These COM5 rail three-point values are locked for the current session baseline and used for ongoing bench logic validation.
- This three-point lock was captured in the COM5 session timeline and is retained as historical evidence; current default expected values for shared-trim builds are listed in Section 4.

## 9) Replacement Board COM5 MAP 3-Point Validation (Session Lock)

Date:
- 2026-02-23

Test basis:
- `mapLin=312.5`, `mapOff=-11.25`, `mapDiv=0.66`
- MAP input injected directly at the controller MAP input, controller ground as return.
- Serial and API source: COM5 `/api/status` and `[STAT] map=...kPa`.

Results:
- `0.00 V`: measured `-11.25 kPa`
- `2.20 V`: measured `194.7 to 196.2 kPa`
- `3.30 V`: measured `290.31 kPa`

Status:
- These COM5 MAP three-point values are locked for the current session baseline and used for ongoing bench logic validation.
- This three-point lock was captured in the COM5 session timeline and is retained as historical evidence; current default expected values for shared-trim builds are listed in Section 4.

## 10) Historical Calibration Record Preservation

- Historical calibration records in this document and in `TEST_RESULTS_MASTER.md` are intentionally preserved and must not be removed even when superseded by newer defaults.
- If a calibration set is superseded, keep the original measured values and mark them as historical context.
- Current pass note (2026-02-24):
  - No new physical calibration sweep was executed in this pass.
  - Prior calibration records were retained and re-indexed in `TEST_RESULTS_MASTER.md` Section 7.7 to keep coverage complete.
- Current pass note (2026-03-03):
  - Sensor Calibration table capture workflow was simplified: MAP/Rail fit and MAP/Rail quick-check capture buttons were removed.
  - Shared ADC curve table capture remains active.
  - Rail pressure labeling is now explicit as gauge units (`psig`, `kPag`) and MAP absolute labeling uses `psia` where shown.

