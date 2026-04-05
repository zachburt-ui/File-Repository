# UI Theme Guidelines

This file captures the current UI theme so future edits stay consistent and can be recreated by another AI without guessing.

## Colors
- Core CSS variables (in `WebUI.ino` and `Preview UI/ui_preview.html`):
  - `--bg`, `--bg2`, `--panel`, `--panel2`, `--border`, `--accent`, `--accent2`, `--text`, `--dim`, `--danger`, `--warn`
- These drive the overall dark theme, borders, and accent colors.
- Current values (keep exact unless a deliberate theme change):
  - `--bg:#000`
  - `--bg2:#050608`
  - `--panel:#101216`
  - `--panel2:#151920`
  - `--border:#262b33`
  - `--accent:#00c853`
  - `--accent2:#00e676`
  - `--text:#f5f5f5`
  - `--dim:#9aa1ae`
  - `--danger:#ff5252`
  - `--warn:#ffb300`

## Button Styles
- Base button style: neutral dark background with border.
- `primary`: green-accented Save actions.
- `btn-info`: blue Refresh actions.
- `btn-success`: green success-state action (use sparingly; not for normal Save).
- `btn-danger`: red Safety/reset actions.
- `btn-warn`: yellow actions (Apply / Spread X).
- `ghost`: muted secondary action.
- `btn-latch`: large latch buttons (use with `btn-danger` for manual latch buttons).
- Latched buttons flash full background via `.btn-danger.active` animation.
- `btn-info` must be used for all Refresh actions (including Wi-Fi refresh).
- `btn-warn` is used for Apply/Spread X buttons.
- `btn-danger` must be used for Unforce All and manual latch buttons.

## Usage Conventions
- Save actions: `primary`.
- Refresh actions: `btn-info`.
- Reset/Fault actions: `btn-danger`.
- Keep button color usage consistent across both files:
  - `WebUI.ino`
  - `Preview UI/ui_preview.html`

## Sync Requirement
- Any UI theme or button changes must be applied to both `WebUI.ino` and `Preview UI/ui_preview.html`.

## Layout Elements
- Cards are dark gradient panels with rounded corners; click on empty card space to expand/collapse.
- `status-grid` uses compact labeled tiles for dense status readouts.
- Status tiles now use one neutral gray strip and panel tone across all status categories for consistent at-a-glance state reading.
- `net-pill` wraps network + WS status in a rounded capsule.
- `badge` variants: `ok`, `warn`, `bad` for state emphasis.
- Settings card uses `settings-intro` color pills and `settings-group` blocks with a left accent strip by category.
- Standard section color sequence for sectioned cards is locked:
  - Section 1 `hardware` (blue), Section 2 `flow` (green), Section 3 `safety` (red).
  - Additional sections use non-repeating standard colors: Section 4 `tune` (amber), Section 5 `diag` (cyan).
- Sensor Calibration card uses matching `settings-intro` pills and `cal-group` blocks with the same left-strip visual language and panel depth as Settings.
- Spray Curve card uses matching `settings-intro` pills and `curve-group` section blocks so it visually aligns with Settings and Sensor Calibration.
- Test Section manual fault control groups use matching left-strip section colors (`manual` red, `override` blue, `bypass` green) to align with Settings/Sensor section styling.
- Shared ADC sliders use a larger thumb, visible tick marks, and numeric scale labels for quick read and repeatable adjustments.
- Sensor Calibration table heading lines now use dedicated `.cal-table-title` styling that matches `.cal-title` sizing and emphasis (`13px`, `900`, `.3px` letter spacing).
- Sensor Calibration table blocks use larger title spacing (`.cal-table-title` top margin) to keep clear separation between consecutive tables.
- Sensor Calibration quick-check presentation is split into two tables with section-matched titles: `Custom Sensor Calibration MAP Quick Check` and `Custom Sensor Calibration Rail Quick Check`.
- Shared ADC fit table heading uses the same `.cal-table-title` style and the label `Analog Input Curve Table`.

## Typography & Spacing
- Base font stack: `system-ui,-apple-system,Segoe UI,Roboto,Arial`.
- Headers:
  - `.h1` uses `font-size:20px`, `font-weight:800`, `letter-spacing:.4px`.
  - Card headers use `.h1` with `font-size:16px`.
- Body background is a radial gradient: `radial-gradient(circle at top,#1b2833 0%,#000 55%)`.
- Cards: `border-radius:14px`, `padding:14px`, `margin:10px 0`.
- Inputs: `border-radius:10px`, dark background `#0b0d10`.
- Status tiles: `border-radius:10px`, compact 6px 8px padding.
- Buttons: `border-radius:12px` (14px for accent button variants).

## Charts & Tables
- Curve canvas uses 2x pixel scaling, dark gradient background, and thick green curve.
- Clamp line uses yellow dashed line with label `Clamp XX%`.
- Hover marker uses bright green stroke + tooltip box.
- Curve table is compact with border lines (`border-bottom:1px solid var(--border)`).
- Clamped rows show a `CLAMP` badge in the Duty % column.

## Interaction Rules
- Cards default to collapsed; user expands by clicking the card title or empty card space.
- Hover line only appears when pointer is over the curve canvas; clears on leave.
- Manual fault controls use compact grouped buttons with clear section headers and solid active-state coloring (no flashing animation).

## Structure Rules
- Buttons live at the bottom of each card.
- Save applies all settings on the page.
- Refresh pulls latest settings on the page.
- Preview must mirror live UI and remain fully interactive (test section works without ESP32).

## Card Order (Top to Bottom)
1. Setup Quickstart
2. Status
3. Methanol Controller Live Data
4. Methanol Spray Curve
5. Methanol Controller Settings
6. Sensor Calibration
7. Wi-Fi Settings
8. Test Section

## Card Contents (What Must Exist)
- Setup Quickstart: numbered steps for first-time setup, includes mention of manual latches and power-cycle settings persistence.
- Status: `status-grid` tiles for pump/inj/spray/boost cut/timing cut/IAT SSR/safety/level (raw)/blue-green-red LEDs/pressure-ready/level fault/rail dP fault pending/dP monitor state/dP monitor override/pressure-ready override/level fault bypass/dP fault bypass/dP hold bypass/dP low timer/dP min seen/dP armed settle/fault reason.
- Methanol Controller Live Data: large gauges for MAP, Spray Command, Rail Pressure, Injector dP, Fault Reason using one neutral gray tile style; Save/Refresh at bottom.
- Methanol Spray Curve: themed sections in this order:
  - Section 1 `Curve Axis and Ceiling`
  - Section 2 `Curve Preview and Table`
  - Section 3 `Live Curve Summary`
  - Includes curve start/max, spread action, duty clamp, curve canvas with hover and clamp line, curve table (kPa/Duty/Flow GPH), and summary KPIs (Current MAP, Curve Duty @ MAP, Duty Clamp).
  - Save/Refresh remains at the bottom of the card.
- Methanol Controller Settings: tune-order helper line, category pills, and five labeled sections in this order:
  - Section 1 `Low Level Fault Inputs`
  - Section 2 `Rail dP Fault Settings`
  - Section 3 `Pressure-Ready Fault Settings`
  - Section 4 `Flow and Spray Model`
  - Section 5 `Signal Conditioning and Diagnostics`
  - Section color mapping follows the locked sequence:
    - Section 1 `hardware`, Section 2 `flow`, Section 3 `safety`, Section 4 `tune`, Section 5 `diag`.
  - Fault-state grouping is explicit in Settings:
    - low level inputs in Section 1, rail dP fault controls in Section 2, pressure-ready fault controls in Section 3.
  - Include `Apply Default Settings` button in the card action row before Save/Refresh.
  - Default settings include `dpMinPsi=60`, `targetInjectorDp=60`, and the baseline curve/calibration values used for first startup.
  - Keep all existing setting IDs and Save/Refresh buttons intact.
- Sensor Calibration: themed section groups in this order:
  - Section 1 `Analog Input Calibration`
  - Section 2 `Sensor Preset Selection`
  - Section 3 `Custom Sensor Calibration`
  - Section color mapping follows the locked sequence:
    - Section 1 `hardware`, Section 2 `flow`, Section 3 `safety`.
  - Custom Sensor Calibration table labels use section-matched titles:
    - `Custom Sensor Calibration MAP Fit`
    - `Custom Sensor Calibration Rail Fit`
    - `Custom Sensor Calibration MAP Quick Check`
    - `Custom Sensor Calibration Rail Quick Check`
  - Each Custom Sensor Calibration table includes one sensor-relevant two-decimal live gauge directly above the table (MAP gauge on MAP tables, rail gauge on rail tables), and that gauge row spans full table width.
  - Shared ADC Trim table label uses simplified title:
    - `Analog Input Curve Table`
  - Include shared ADC gain/offset sliders with click/drag and keyboard guidance.
  - Calibration table gauge rows use shared classes for consistent spacing and layout (`.cal-gauge-row.single` for one-gauge rows and `.cal-gauge-row.dual` for the analog input curve table row).
  - Include a quick trim helper line under the sliders describing the `0.0 V` and `3.3 V` endpoint adjustment workflow.
  - Shared ADC slider ranges are:
    - Gain `0.8000` to `1.2000`
    - Offset `-0.500 V` to `+0.500 V`
- Wi-Fi Settings: themed section groups in this order:
  - Section 1 `Wi-Fi Mode`
  - Section 2 `AP and mDNS Identity`
  - Section 3 `Station Credentials`
  - Wi-Fi section color mapping:
    - Section 1 uses `hardware` (blue accent)
    - Section 2 uses `flow` (green accent)
    - Section 3 uses `safety` (red accent)
  - Keep the same settings IDs and Save/Refresh actions.
- Test Section: sensor overrides (MAP/rail/dP), output overrides (level/pump/injectors/duty), manual fault controls and per-error overrides (boost cut, timing cut, rail dP fault, pressure-ready fault, dP boost hold, dP monitor override, pressure-ready override, level fault bypass, dP fault bypass, dP hold bypass), Save + Unforce All.
  - Themed sections in order:
    - Section 1 `Sensor Overrides`
    - Section 2 `Output Overrides`
    - Section 3 `Manual Fault Controls and Overrides`

## Layout Map (IDs & Key Classes)
- Network pill: `.net-pill` containing `#netLine` and `#wsBadge`.
- Cards use `.card`; collapse/expand via `card.collapsed`.
- Live Data card title: `Methanol Controller Live Data` (used by auto-expand logic).
- Gauges: `.kv` tiles; values in `.v`.
- Status grid: `.status-grid` with `.status-item` and `.status-item.full`.
- Status tile category classes: `.status-item.flow`, `.status-item.safety`, `.status-item.dp`, `.status-item.system`.
- Curve: `#curve` canvas; table `#curveTable`.
- Live Data gauge grid: `.live-grid` with `.kv.live-kv` and metric classes `.live-map`, `.live-spray`, `.live-rail`, `.live-dp`, `.live-fault`.
- Spray Curve card: `.curve-layout`, `.curve-group`, `.curve-step`, `.curve-title`, `.curve-help`, `.curve-grid`.
- Settings card: `.settings-intro`, `.settings-pill`, `.settings-layout`, `.settings-group`, `.settings-step`, `.settings-grid`.
- Shared section color variants available for settings cards: `hardware`, `flow`, `safety`, `tune`, `diag`.
- Sensor Calibration card: `.cal-layout`, `.cal-group`, `.cal-step`, `.cal-title`, `.cal-help`.
- Sensor Calibration table heading class: `.cal-table-title`.
- Shared ADC slider scale elements: `.cal-slider`, `.cal-slider-scale`, `.cal-slider-labels`.
- Test Section card groups: `.test-layout`, `.test-group`, `.test-step`, `.test-title`, `.test-help`, `.test-grid`.
- Test Section fields use bottom-aligned `.test-field` columns and fixed label height so Section 1 and Section 2 controls line up cleanly.
- Test Section manual controls: `.fault-controls`, `.fault-group`, `.fault-group-title`, `.fault-grid`.
- Fault and override buttons: `#btnBoostCut`, `#btnTimingCut`, `#btnDpFault`, `#btnPressureReadyFault`, `#btnDpBoostHold`, `#btnDpMonOverride`, `#btnPressureReadyOverride`, `#btnLevelBypass`, `#btnDpFaultBypass`, `#btnDpHoldBypass` with `btn-danger btn-latch`.
- Fault gauge: `#faultKv` shows specific fault reason.

## Preview Requirements
- `Preview UI/ui_preview.html` must visually and structurally match live UI.
- All buttons, toggles, and test inputs must be interactive in preview.
- Preview Save applies in-page state; Refresh resets to initial defaults.
- Latch buttons update fault reason text and visual state.
