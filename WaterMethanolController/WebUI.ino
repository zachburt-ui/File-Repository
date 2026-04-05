// ============================= UI ===============================
// ================================================================

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>WaterMethanol Controller</title>
<style>
:root{--bg:#000;--bg2:#050608;--panel:#101216;--panel2:#151920;--border:#262b33;--accent:#00c853;--accent2:#00e676;--text:#f5f5f5;--dim:#9aa1ae;--danger:#ff5252;--warn:#ffb300;}
*{box-sizing:border-box;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial}
body{margin:0;color:var(--text);background:radial-gradient(circle at top,#1b2833 0%,#000 55%);}
a{color:var(--accent2)}
.wrap{max-width:980px;margin:0 auto;padding:16px}
.top{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:10px}
.h1{font-size:20px;font-weight:800;letter-spacing:.4px}
.card .h1{cursor:pointer}
.sub{color:var(--dim);font-size:12px}
.status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:8px 12px;margin-top:6px}
.status-item{position:relative;overflow:hidden;display:flex;align-items:center;justify-content:space-between;gap:10px;background:#0b0d10;border:1px solid var(--border);border-radius:10px;padding:6px 8px 6px 12px}
.status-item::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--status-strip,#4a5564)}
.status-item.flow{--status-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.status-item.safety{--status-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.status-item.dp{--status-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.status-item.system{--status-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.status-item.full{grid-column:1/-1}
.status-k{color:var(--dim);font-size:12px}
.status-v{font-weight:800;font-size:12px}
.card{background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--border);border-radius:14px;padding:14px;margin:10px 0}
.card{position:relative}
.card.collapsed > *{display:none}
.card.collapsed > .h1{display:block}
.card.collapsed::before{display:none}
.row{display:flex;flex-wrap:wrap;gap:10px;align-items:flex-end}
.settings-intro{display:flex;flex-wrap:wrap;gap:8px;margin-top:8px}
.settings-pill{display:inline-flex;align-items:center;gap:6px;padding:4px 10px;border-radius:999px;border:1px solid var(--border);font-size:11px;font-weight:800;letter-spacing:.2px;background:#0b0d10;color:var(--dim)}
.settings-pill.safety{border-color:#7a3a3a;color:#ffd6d6;background:#170f10}
.settings-pill.flow{border-color:#37523d;color:#c9f5cf;background:#0f1410}
.settings-pill.hardware{border-color:#35546a;color:#cde8ff;background:#0d1217}
.settings-pill.tune{border-color:#8b5a2b;color:#ffe0bf;background:#17120d}
.settings-pill.diag{border-color:#2f6e73;color:#c7f4f8;background:#0b1416}
.settings-layout{display:grid;gap:12px;margin-top:10px}
.settings-group{position:relative;overflow:hidden;border:1px solid var(--border);border-radius:12px;padding:12px;background:#0b0d10;box-shadow:0 0 0 1px rgba(255,255,255,0.02) inset}
.settings-group::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--group-strip,#4a5564)}
.settings-group.safety{--group-strip:#b24a4a;border-color:#4c3030;background:linear-gradient(180deg,#171012,#0d0a0b)}
.settings-group.flow{--group-strip:#4f8a5a;border-color:#2f4734;background:linear-gradient(180deg,#0f1510,#090d09)}
.settings-group.hardware{--group-strip:#4d85ad;border-color:#2a3f50;background:linear-gradient(180deg,#0d1418,#090d10)}
.settings-group.tune{--group-strip:#d08a46;border-color:#5a4028;background:linear-gradient(180deg,#17140f,#0d0b09)}
.settings-group.diag{--group-strip:#4aa7b0;border-color:#2f4f55;background:linear-gradient(180deg,#0d1719,#091011)}
.settings-head{display:flex;align-items:flex-start;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:8px;padding-left:6px}
.settings-step{font-size:10px;color:var(--dim);font-weight:800;letter-spacing:.9px;text-transform:uppercase}
.settings-title{font-size:13px;font-weight:900;letter-spacing:.3px}
.settings-help{font-size:11px;color:var(--dim);max-width:380px}
.settings-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:10px}
.settings-field{min-width:0}
.settings-group label{min-height:0;margin:0 0 4px}
.settings-group .sub{line-height:1.25}
@media(min-width:930px){.settings-layout{grid-template-columns:repeat(2,minmax(0,1fr))}.settings-group.full{grid-column:1/-1}}
.cal-layout{display:grid;gap:12px;margin-top:10px}
.cal-group{position:relative;overflow:hidden;border:1px solid var(--border);border-radius:12px;padding:12px;background:#0b0d10;box-shadow:0 0 0 1px rgba(255,255,255,0.02) inset}
.cal-group::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--cal-strip,#4a5564)}
.cal-group.presets{--cal-strip:#4f8a5a;border-color:#2f4734;background:linear-gradient(180deg,#0f1510,#090d09);order:2}
.cal-group.sensor{--cal-strip:#b24a4a;border-color:#4c3030;background:linear-gradient(180deg,#171012,#0d0a0b);order:3}
.cal-group.shared{--cal-strip:#4d85ad;border-color:#2a3f50;background:linear-gradient(180deg,#0d1418,#090d10);order:1}
.cal-head{display:flex;align-items:flex-start;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:8px;padding-left:6px}
.cal-step{font-size:10px;color:var(--dim);font-weight:800;letter-spacing:.9px;text-transform:uppercase}
.cal-title{font-size:13px;font-weight:900;letter-spacing:.3px}
.cal-help{font-size:11px;color:var(--dim);max-width:420px}
.cal-group .row{margin-top:8px}
.cal-group .row:first-of-type{margin-top:0}
.cal-group .row > div{min-width:0}
.cal-group input + input{margin-top:8px}
.adc-cal-wrap{margin-top:8px;border:1px solid var(--border);border-radius:10px;overflow:auto;background:#0b0d10}
.adc-cal-table{width:100%;border-collapse:collapse;min-width:780px}
.adc-cal-table th,.adc-cal-table td{border-bottom:1px solid var(--border);padding:6px 8px;text-align:left;font-size:12px}
.adc-cal-table th{color:var(--dim);font-weight:800;background:#0f1319}
.adc-cal-table tr:last-child td{border-bottom:none}
.adc-cal-table input{padding:7px 8px;border-radius:8px}
.cal-table-title{margin-top:28px;margin-bottom:6px;font-size:13px;font-weight:900;letter-spacing:.3px;color:#d8e2ee;line-height:1.3}
.curve-layout{display:grid;gap:12px;margin-top:10px}
.curve-group{position:relative;overflow:hidden;border:1px solid var(--border);border-radius:12px;padding:12px;background:#0b0d10;box-shadow:0 0 0 1px rgba(255,255,255,0.02) inset}
.curve-group::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--curve-strip,#4a5564)}
.curve-group.hardware{--curve-strip:#4d85ad;border-color:#2a3f50;background:linear-gradient(180deg,#0d1418,#090d10)}
.curve-group.flow{--curve-strip:#4f8a5a;border-color:#2f4734;background:linear-gradient(180deg,#0f1510,#090d09)}
.curve-group.safety{--curve-strip:#b24a4a;border-color:#4c3030;background:linear-gradient(180deg,#171012,#0d0a0b)}
.curve-head{display:flex;align-items:flex-start;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:8px;padding-left:6px}
.curve-step{font-size:10px;color:var(--dim);font-weight:800;letter-spacing:.9px;text-transform:uppercase}
.curve-title{font-size:13px;font-weight:900;letter-spacing:.3px}
.curve-help{font-size:11px;color:var(--dim);max-width:420px}
.curve-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:10px}
.curve-field{min-width:0}
.curve-group label{min-height:0;margin:0 0 4px}
.curve-group .sub{line-height:1.25}
.curve-group .row{margin-top:8px}
.test-layout{display:grid;gap:12px;margin-top:10px}
.test-group{position:relative;overflow:hidden;border:1px solid var(--border);border-radius:12px;padding:12px;background:#0b0d10;box-shadow:0 0 0 1px rgba(255,255,255,0.02) inset}
.test-group::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--test-strip,#4a5564)}
.test-group.sensor{--test-strip:#4d85ad;border-color:#2a3f50;background:linear-gradient(180deg,#0d1418,#090d10)}
.test-group.output{--test-strip:#4f8a5a;border-color:#2f4734;background:linear-gradient(180deg,#0f1510,#090d09)}
.test-group.manual{--test-strip:#b24a4a;border-color:#4c3030;background:linear-gradient(180deg,#171012,#0d0a0b)}
.test-head{display:flex;align-items:flex-start;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:8px;padding-left:6px}
.test-step{font-size:10px;color:var(--dim);font-weight:800;letter-spacing:.9px;text-transform:uppercase}
.test-title{font-size:13px;font-weight:900;letter-spacing:.3px}
.test-help{font-size:11px;color:var(--dim);max-width:420px}
.test-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:10px}
.test-field{min-width:0;display:flex;flex-direction:column;justify-content:flex-end}
.test-group label{min-height:38px;margin:0 0 4px}
.test-group .sub{line-height:1.25}
.kv{flex:1 1 180px;background:#0b0d10;border:1px solid var(--border);border-radius:12px;padding:10px}
.kv .k{color:var(--dim);font-size:12px}
.kv .v{font-size:18px;font-weight:800;margin-top:4px}
.live-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:10px}
.live-grid-main{display:flex;flex-wrap:wrap;gap:10px}
.live-grid-main .kv.live-kv{flex:1 1 calc(25% - 10px);min-width:170px}
.live-grid-main .kv.live-fault{flex:1 1 100%;min-width:100%}
.cal-gauge-row{margin-top:8px}
.cal-gauge-row.single{grid-template-columns:1fr}
.cal-gauge-row.dual{grid-template-columns:repeat(2,minmax(170px,1fr))}
.kv.live-kv{position:relative;overflow:hidden;padding-left:12px}
.kv.live-kv::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--live-strip,#4a5564)}
.kv.live-map{--live-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.kv.live-spray{--live-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.kv.live-rail{--live-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.kv.live-dp{--live-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.kv.live-fault{--live-strip:#6d6f76;border-color:#343841;background:linear-gradient(180deg,#11141b,#0c0e13)}
.hr{height:1px;background:var(--border);margin:12px 0}
label{display:block;color:var(--dim);font-size:12px;margin:10px 0 4px;min-height:30px}
input,select{width:100%;padding:10px 10px;border-radius:10px;border:1px solid var(--border);background:#0b0d10;color:var(--text);outline:none}
small{display:block;color:var(--dim);margin-top:4px;line-height:1.2}
summary{cursor:pointer}
details summary{list-style:none}
details summary::-webkit-details-marker{display:none}
details summary{display:inline-flex;align-items:center;gap:8px;padding:8px 12px;border:1px solid var(--border);border-radius:999px;background:#0b0d10;color:var(--dim);font-weight:800;font-size:12px}
details[open] summary{color:var(--accent2);border-color:var(--accent)}
details summary::after{content:"-";font-size:10px;color:var(--dim)}
details[open] summary::after{content:"-";color:var(--accent2)}
.cal-slider{width:100%;padding:0;height:30px;accent-color:var(--accent2);cursor:pointer}
.cal-slider::-webkit-slider-runnable-track{height:8px;border-radius:999px;background:#1b2430;border:1px solid #2c3748}
.cal-slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:20px;height:20px;border-radius:50%;background:var(--accent2);border:2px solid #0b0d10;box-shadow:0 0 0 1px #0b0d10;margin-top:-7px}
.cal-slider::-moz-range-track{height:8px;border-radius:999px;background:#1b2430;border:1px solid #2c3748}
.cal-slider::-moz-range-thumb{width:20px;height:20px;border-radius:50%;background:var(--accent2);border:2px solid #0b0d10;box-shadow:0 0 0 1px #0b0d10}
.cal-slider-scale{position:relative;height:12px;margin-top:2px}
.cal-slider-scale::before{content:"";position:absolute;left:0;right:0;top:5px;height:2px;background:#273243}
.cal-slider-scale .tick{position:absolute;top:1px;width:2px;height:10px;background:#5f6f85;transform:translateX(-1px)}
.cal-slider-labels{display:flex;justify-content:space-between;gap:8px;color:var(--dim);font-size:11px;font-weight:700;letter-spacing:.2px}
.btns{display:flex;gap:10px;flex-wrap:wrap;margin-top:10px}
button{border:1px solid var(--border);background:#0b0d10;color:var(--text);padding:10px 12px;border-radius:12px;font-weight:800;cursor:pointer}
button.primary{border-color:var(--accent);box-shadow:0 0 0 1px var(--accent) inset;background:#07140d}
.btn-success{border-color:#00c853;background:#0b140e;color:#e6ffe9;box-shadow:0 0 0 1px #00c853 inset;border-radius:14px}
.btn-danger{border-color:#ff5252;background:#1a0b0b;color:#ffeaea;box-shadow:0 0 0 1px #ff5252 inset;border-radius:14px}
.btn-info{border-color:#42a5f5;background:#0d1a29;color:#e8f3ff;box-shadow:0 0 0 1px #42a5f5 inset,0 0 6px rgba(66,165,245,0.45);border-radius:14px}
.btn-warn{border-color:var(--warn);background:#1b1405;color:#fff3d6;box-shadow:0 0 0 1px var(--warn) inset;border-radius:14px}
.ghost{border-color:var(--border);color:var(--dim);box-shadow:none}
.btn-latch{min-width:0;padding:9px 11px;border-radius:10px;font-size:12px;letter-spacing:.1px}
.btn-danger.btn-latch{background:#0f1218;border-color:#2f3b4e;color:#d6e3f5;box-shadow:0 0 0 1px #2f3b4e inset}
.btn-danger.btn-latch.active{background:#2a1010;border-color:#ff5252;color:#ffeaea;box-shadow:0 0 0 1px #ff5252 inset}
.fault-controls{display:grid;gap:10px;margin-top:12px}
.fault-group{position:relative;overflow:hidden;border:1px solid var(--border);border-radius:12px;padding:10px 10px 10px 12px;background:#0b0d10;box-shadow:0 0 0 1px rgba(255,255,255,0.02) inset}
.fault-group::before{content:"";position:absolute;left:0;top:0;bottom:0;width:4px;background:var(--fault-strip,#4a5564)}
.fault-group.manual{--fault-strip:#b24a4a;border-color:#4c3030;background:linear-gradient(180deg,#171012,#0d0a0b)}
.fault-group.override{--fault-strip:#4d85ad;border-color:#2a3f50;background:linear-gradient(180deg,#0d1418,#090d10)}
.fault-group.bypass{--fault-strip:#4f8a5a;border-color:#2f4734;background:linear-gradient(180deg,#0f1510,#090d09)}
.fault-group-title{font-size:11px;font-weight:900;letter-spacing:.35px;color:#dfe7f2;text-transform:uppercase;margin-bottom:8px;padding-left:6px}
.fault-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:8px}
.badge{display:inline-block;padding:4px 8px;border-radius:999px;border:1px solid var(--border);font-size:12px;color:var(--dim)}
.badge.ok{border-color:var(--accent);color:var(--accent2)}
.badge.warn{border-color:var(--warn);color:var(--warn)}
.badge.bad{border-color:var(--danger);color:var(--danger)}
.net-pill{display:inline-flex;align-items:center;gap:8px;border:1px solid var(--border);border-radius:999px;padding:6px 10px;background:#0b0d10}
pre{white-space:pre-wrap;margin:0}
canvas{width:100%;height:240px;border:1px solid var(--border);border-radius:12px;background:#07090c}

canvas#curve{width:100%;height:360px;border:1px solid var(--border);border-radius:14px;background:linear-gradient(180deg,#0b0f14,#06080b);touch-action:none;}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div>
      <div class="h1">WaterMethanol Controller</div>
      <div class="sub">AP/STA web UI</div>
    </div>
    <div class="sub"><span class="net-pill"><span id="netLine">...</span> <span class="badge" id="wsBadge">WS: OFF</span></span></div>
  </div>

  <div class="card collapsed" data-title="Setup Quickstart">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Setup Quickstart</div>
    <div class="sub">Defaults: See each card below for the shipping values.</div>
    <div class="sub">
      1. Power up and connect to the controller Wi-Fi or your configured STA network.<br>
      2. Set MAP preset (or Linear/Offset + divider ratio) and verify idle MAP reads reasonable for your engine and altitude.<br>
      3. Set analog input calibration; verify rail psig and injector dP build correctly when spray is requested.<br>
      4. Set curve start/max and shape; verify Spray Command follows MAP in the curve preview.<br>
      5. Set duty clamp, pressure fault threshold, pressure fault delay, target injector dP, and pressure-ready timeout.<br>
      6. Set injector size and methanol mix for flow reference math.<br>
      7. Use the manual fault buttons to test safety behavior.<br>
      8. Save, then power cycle to confirm settings persist.
    </div>
  </div>

  <div class="card collapsed" data-title="Status">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Status</div>
    <div class="sub">Defaults: Live status values (no defaults).</div>
    <div class="status-grid" style="margin-top:8px">
      <div class="status-item flow"><span class="status-k">Pump</span><span class="status-v" id="stPump">?</span></div>
      <div class="status-item flow"><span class="status-k">Injectors</span><span class="status-v" id="stInj">?</span></div>
      <div class="status-item flow"><span class="status-k">Spray</span><span class="status-v" id="stSpray">?</span></div>
      <div class="status-item safety"><span class="status-k">Boost Cut</span><span class="status-v" id="stBoost">?</span></div>
      <div class="status-item safety"><span class="status-k">Timing Cut</span><span class="status-v" id="stIat">?</span></div>
      <div class="status-item safety"><span class="status-k">Timing Cut Output</span><span class="status-v" id="stIatSsr">?</span></div>
      <div class="status-item safety"><span class="status-k">Safety State</span><span class="status-v" id="stSafety">?</span></div>
      <div class="status-item system"><span class="status-k">Level Switch</span><span class="status-v"><span id="stLevel">?</span> <span class="status-k" style="margin-left:6px">raw</span> <span id="stLvlRaw">?</span></span></div>
      <div class="status-item system"><span class="status-k">Blue LED</span><span class="status-v" id="stBlueLed">?</span></div>
      <div class="status-item flow"><span class="status-k">Green LED</span><span class="status-v" id="stGreenLed">?</span></div>
      <div class="status-item safety"><span class="status-k">Red LED</span><span class="status-v" id="stRedLed">?</span></div>
      <div class="status-item flow"><span class="status-k">Pressure Ready</span><span class="status-v" id="stPressureReady">?</span></div>
      <div class="status-item safety"><span class="status-k">Level Fault</span><span class="status-v" id="stLevelFault">?</span></div>
      <div class="status-item dp"><span class="status-k">Rail dP Fault</span><span class="status-v"><span id="stDpFault">?</span> <span class="status-k" style="margin-left:6px">pending</span> <span id="stDpPending">?</span></span></div>
      <div class="status-item dp"><span class="status-k">Rail dP State Detail</span><span class="status-v" id="stDpFaultType">NONE</span></div>
      <div class="status-item dp"><span class="status-k">Pressure-Ready Fault</span><span class="status-v" id="stPressureReadyFault">?</span></div>
      <div class="status-item dp"><span class="status-k">dP Monitor State</span><span class="status-v" id="stDpState">?</span></div>
      <div class="status-item dp"><span class="status-k">dP Monitor Override</span><span class="status-v" id="stDpOverride">OFF</span></div>
      <div class="status-item dp"><span class="status-k">Pressure-Ready Override</span><span class="status-v" id="stPressureReadyOverride">OFF</span></div>
      <div class="status-item dp"><span class="status-k">Level Fault Bypass</span><span class="status-v" id="stLevelBypass">OFF</span></div>
      <div class="status-item dp"><span class="status-k">dP Fault Bypass</span><span class="status-v" id="stDpFaultBypass">OFF</span></div>
      <div class="status-item dp"><span class="status-k">Pressure-Ready Fault Bypass</span><span class="status-v" id="stPressureReadyFaultBypass">OFF</span></div>
      <div class="status-item dp"><span class="status-k">dP Hold Bypass</span><span class="status-v" id="stDpHoldBypass">OFF</span></div>
      <div class="status-item dp"><span class="status-k">Pressure-Ready Hold Bypass</span><span class="status-v" id="stPressureReadyHoldBypass">OFF</span></div>
      <div class="status-item dp"><span class="status-k">dP Low Timer</span><span class="status-v"><span id="stDpLowMs">0</span> ms</span></div>
      <div class="status-item dp"><span class="status-k">dP Min Seen</span><span class="status-v"><span id="stDpMinSeen">-</span> psi</span></div>
      <div class="status-item dp"><span class="status-k">dP Armed</span><span class="status-v"><span id="stDpArmed">NO</span> <span class="status-k" style="margin-left:6px">delay</span> <span id="stDpSettle">0 ms</span></span></div>
      <div class="status-item full safety"><span class="status-k">Fault History</span><span class="status-v" id="stReason">?</span></div>
      <div class="status-item full safety"><span class="status-k">Current Fault Status</span><span class="status-v" id="stCurrentFault">?</span></div>
    </div>
  </div>

  <div class="card collapsed" data-title="Methanol Controller Live Data">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Methanol Controller Live Data</div>
    <div class="sub">Defaults: Live sensor values (no defaults).</div>
    <div class="live-grid live-grid-main" style="margin-top:8px">
      <div class="kv live-kv live-map"><div class="k">MAP (kPa abs)</div><div class="v" id="mapKpa">0</div></div>
      <div class="kv live-kv live-map"><div class="k">MAP (psia)</div><div class="v" id="mapPsi">0.0</div></div>
      <div class="kv live-kv live-map"><div class="k">Baro (kPa abs)</div><div class="v" id="baroKpa">101.3</div></div>
      <div class="kv live-kv live-map"><div class="k">Baro (psia)</div><div class="v" id="baroPsi">14.70</div></div>
      <div class="kv live-kv live-map"><div class="k">Boost (psi)</div><div class="v" id="boostPsiKv">0.0</div></div>
      <div class="kv live-kv live-rail"><div class="k">Rail Pressure (psig)</div><div class="v" id="railPsi">0.0</div></div>
      <div class="kv live-kv live-spray"><div class="k">Spray Command (%)</div><div class="v" id="sprayPct">0</div></div>
      <div class="kv live-kv live-dp"><div class="k">Injector dP (psi)</div><div class="v" id="dpPsi">0.0</div></div>
      <div class="kv live-kv live-fault"><div class="k">Fault Status</div><div class="v" id="faultKv">OK</div></div>
    </div>
    <div class="btns" style="margin-top:6px">
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-info" onclick="loadCfg()">Refresh</button>
    </div>
  </div>

  <div class="card collapsed" data-title="Methanol Spray Curve">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Methanol Spray Curve</div>
    <div class="sub">Defaults: Curve spans ~145-245 kPa; Duty Clamp 60%; Target Injector dP 60 psi; Injector 36 lb/hr @58; Mix 60%.</div>
    <div class="sub">10 points. The controller interpolates between points to compute <b>Spray Command %</b>. Below Curve Start, Spray Command is 0%. Use start/max to set X-axis range like AEM.</div>
    <div class="settings-intro">
      <span class="settings-pill hardware">Axis and Clamp</span>
      <span class="settings-pill flow">Curve Preview and Table</span>
      <span class="settings-pill safety">Live Summary</span>
    </div>
    <div class="curve-layout">
      <div class="curve-group hardware">
        <div class="curve-head">
          <div>
            <div class="curve-step">Section 1</div>
            <div class="curve-title">Curve Axis and Ceiling</div>
          </div>
          <div class="curve-help">Set the start and max kilopascal range first, spread points, then set the duty clamp ceiling.</div>
        </div>
        <div class="curve-grid">
          <div class="curve-field">
            <label>Curve Start kPa</label>
            <input id="curveStart" type="number" step="1">
          </div>
          <div class="curve-field">
            <label>Curve Max kPa</label>
            <input id="curveMax" type="number" step="1">
          </div>
          <div class="curve-field" style="display:flex;align-items:flex-end;gap:10px">
            <button class="btn-warn" onclick="spreadCurve()">Spread X</button>
          </div>
          <div class="curve-field">
            <label>Duty Clamp (%) <span id="dutyClampFlow">...</span> <span class="sub" style="display:inline">Hard ceiling on commanded spray.</span></label>
            <input id="dutyClamp" type="number" min="0" max="100" step="1">
          </div>
        </div>
      </div>
      <div class="curve-group flow">
        <div class="curve-head">
          <div>
            <div class="curve-step">Section 2</div>
            <div class="curve-title">Curve Preview and Table</div>
          </div>
          <div class="curve-help">Drag points on the graph or edit table values. The curve interpolation drives spray command from live MAP.</div>
        </div>
        <div class="sub" id="hoverLine" style="display:none"></div>
        <canvas id="curve"></canvas>
        <table id="curveTable" style="width:100%;border-collapse:collapse;font-size:12px">
          <thead><tr><th style="text-align:left;padding:6px;border-bottom:1px solid var(--border)">kPa</th><th style="text-align:left;padding:6px;border-bottom:1px solid var(--border)">Duty %</th><th style="text-align:left;padding:6px;border-bottom:1px solid var(--border)">Flow (GPH)</th></tr></thead>
          <tbody></tbody>
        </table>
      </div>
      <div class="curve-group safety">
        <div class="curve-head">
          <div>
            <div class="curve-step">Section 3</div>
            <div class="curve-title">Live Curve Summary</div>
          </div>
          <div class="curve-help">Use these values to confirm the live MAP, curve output, and duty clamp are aligned during runtime.</div>
        </div>
        <div class="row">
          <div class="kv"><div class="k">Current MAP</div><div class="v"><span id="mapKpa2">0</span> kPa</div></div>
          <div class="kv"><div class="k">Curve Duty @ MAP</div><div class="v"><span id="curvePct">0</span> %</div></div>
          <div class="kv"><div class="k">Duty Clamp (max)</div><div class="v"><span id="clampPct">0</span> %</div></div>
        </div>
      </div>
    </div>
    <div class="btns">
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-info" onclick="loadCfg()">Refresh</button>
    </div>
  </div>

  <div class="card collapsed" data-title="Methanol Controller Settings">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Methanol Controller Settings</div>
    <div class="sub">Defaults: dP min 50 psi; Critical dP 25 psi for 150 ms; Fault delay 3000 ms; dP arm 5%; dP delay 500 ms; dP recover 1.5 psi; Auto-clear 4000 ms; Target Injector dP 60; Pressure-Ready Timeout 3000 ms; Injector 36 lb/hr @58; PWM 50 Hz; Mix 60%; Level debounce 300 ms; MAP/Rail divider 0.66; Serial debug ON; Status period 250 ms.</div>
    <div class="sub">New setup shortcut: apply Default Settings, then review analog input calibration and hit Save.</div>
    <div class="sub">Tune in this order: low level fault inputs, rail dP fault settings, pressure-ready fault settings, flow model, then signal and diagnostics.</div>
    <div class="settings-intro">
      <span class="settings-pill hardware">Low Level Fault</span>
      <span class="settings-pill flow">Rail dP Fault</span>
      <span class="settings-pill safety">Pressure-Ready Fault</span>
    </div>
    <div class="settings-layout">
      <div class="settings-group hardware full">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 1</div>
            <div class="settings-title">Low Level Fault Inputs</div>
          </div>
          <div class="settings-help">This section controls low-level input filtering before low-level safety behavior is applied.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>Level Debounce (ms)</label>
            <input id="lvlDebounce" type="number" min="0" step="1">
            <small>Level switch state must remain stable for this long before the controller accepts the change.</small>
          </div>
        </div>
      </div>

      <div class="settings-group flow">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 2</div>
            <div class="settings-title">Rail dP Fault Settings</div>
          </div>
          <div class="settings-help">This section controls rail dP fault thresholds, arming behavior, recovery behavior, and timing-cut handling.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>Pressure Fault Threshold (psi)</label>
            <input id="dpMinPsi" type="number" min="0" step="0.5">
            <small>If injector dP stays below this while spraying, the normal fault delay starts running.</small>
          </div>
          <div class="settings-field">
            <label>Pressure Fault Delay (ms)</label>
            <input id="dpFaultMs" type="number" min="0" step="100">
            <small>Injector dP must stay below the threshold for this long before a normal fault latches.</small>
          </div>
          <div class="settings-field">
            <label>Critical dP Threshold (psi)</label>
            <input id="dpCriticalPsi" type="number" min="0" step="0.5">
            <small>If injector dP falls below this, the critical low-pressure fast fault path starts immediately.</small>
          </div>
          <div class="settings-field">
            <label>Critical dP Delay (ms)</label>
            <input id="dpCriticalMs" type="number" min="0" step="10">
            <small>This is the fast-path delay for severe pressure loss before timing cut is forced.</small>
          </div>
          <div class="settings-field">
            <label>dP Arm Duty (%)</label>
            <input id="dpArmPct" type="number" min="0" step="0.5">
            <small>dP monitoring only starts once spray command exceeds this duty and spray-enable pressure is reached.</small>
          </div>
          <div class="settings-field">
            <label>dP Delay (ms)</label>
            <input id="dpArmSettleMs" type="number" min="0" step="10">
            <small>After arm conditions are true, dP monitoring waits this delay before fault timers can run.</small>
          </div>
          <div class="settings-field">
            <label>dP Recover Margin (psi)</label>
            <input id="dpRecover" type="number" min="0" step="0.5">
            <small>Pending low-pressure timers clear only after dP rises this far above the threshold.</small>
          </div>
          <div class="settings-field">
            <label>Timing Cut Auto-Clear (ms)</label>
            <input id="timingCutAutoMs" type="number" min="0" step="100">
            <small>Set to zero to disable auto-clear and require manual timing cut reset.</small>
          </div>
        </div>
      </div>

      <div class="settings-group safety">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 3</div>
            <div class="settings-title">Pressure-Ready Fault Settings</div>
          </div>
          <div class="settings-help">This section controls pressure-ready targeting and timeout fault behavior before spray is allowed.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>Target Injector dP / Spray Enable Target (psi)</label>
            <input id="targetInjectorDp" type="number" step="0.5" value="60">
            <small>Spray will not open injectors until live injector dP reaches this target. This value is also used for flow reference math.</small>
          </div>
          <div class="settings-field">
            <label>Pressure-Ready Timeout (ms)</label>
            <input id="pressureReadyTimeoutMs" type="number" min="0" step="10">
            <small>If injector dP does not reach the target in this time, the controller latches a pressure-ready fault.</small>
          </div>
        </div>
      </div>

      <div class="settings-group tune">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 4</div>
            <div class="settings-title">Flow and Spray Model</div>
          </div>
          <div class="settings-help">This section sets injector model assumptions and base flow math inputs.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>Injector Size (lb/hr @58 psi)</label>
            <input id="injLb" type="number" step="0.1">
            <small>Use the per-injector rated flow value at 58 psi base pressure.</small>
          </div>
          <div class="settings-field">
            <label>Injector PWM Frequency (Hz)</label>
            <input id="injHz" type="number" min="1" step="1">
            <small>How fast the injector SSR is pulsed. Most systems start in the 20 to 50 Hz range.</small>
          </div>
          <div class="settings-field">
            <label>Methanol Mix (%)</label>
            <input id="mixMethPct" type="number" min="0" max="100" step="1" value="60">
            <small>Methanol percentage in the blend. Water percentage is one hundred minus this value.</small>
          </div>
        </div>
      </div>

      <div class="settings-group diag">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 5</div>
            <div class="settings-title">Signal Conditioning and Diagnostics</div>
          </div>
          <div class="settings-help">This section controls analog conversion inputs and serial troubleshooting output.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>MAP Divider Ratio</label>
            <input id="mapDivRatio" type="number" step="0.001">
            <small>ADC volts equals sensor volts multiplied by this ratio. Common harnesses are around 0.66.</small>
          </div>
          <div class="settings-field">
            <label>Rail Divider Ratio</label>
            <input id="railDivRatio" type="number" step="0.001">
            <small>ADC volts equals sensor volts multiplied by this ratio. Common harnesses are around 0.66.</small>
          </div>
          <div class="settings-field">
            <label>Serial Debug</label>
            <select id="serialDebug">
              <option value="1">Enabled</option>
              <option value="0">Disabled</option>
            </select>
            <small>Enable to print detailed runtime diagnostics to the serial monitor.</small>
          </div>
          <div class="settings-field">
            <label>Serial Status Period (ms)</label>
            <input id="serialPeriod" type="number" min="0" step="10">
            <small>How often recurring status lines are printed when serial debug is enabled.</small>
          </div>
        </div>
      </div>
    </div>
    <div class="btns">
      <button class="btn-warn" onclick="applyDefaultSettings()">Apply Default Settings</button>
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-info" onclick="loadCfg()">Refresh</button>
    </div>
  </div>

  <div class="card collapsed" data-title="Sensor Calibration">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Sensor Calibration</div>
    <div class="sub">Defaults: MAP preset GM 3-bar (312.5 / -11.25); Rail preset GM 0-130 psi (32.5 / -16.25); MAP/Rail divider 0.66; Shared ADC trim gain 1.0000 and offset 0.000 V.</div>
    <div class="sub">Required order: complete Analog Input Calibration first, then set sensor presets and custom sensor calibration values.</div>
    <div class="settings-intro">
      <span class="settings-pill hardware">Analog Input Calibration</span>
      <span class="settings-pill flow">Sensor Preset Selection</span>
      <span class="settings-pill safety">Custom Sensor Calibration</span>
    </div>
    <div class="cal-layout">
      <div class="cal-group presets">
        <div class="cal-head">
          <div>
            <div class="cal-step">Section 2</div>
            <div class="cal-title">Sensor Preset Selection</div>
          </div>
          <div class="cal-help">After Analog Input Calibration is complete, start from known MAP and rail pressure sensor presets, then use custom values only if your sensor requires it.</div>
        </div>
        <div class="row">
          <div style="flex:1 1 260px">
            <label>MAP Preset</label>
            <select id="mapPreset" onchange="presetChange(this.value)">
              <option value="">Custom / Manual</option>
              <option value="200,-10.33">GM 2-bar - LNF/LHU Cobalt/HHR - 200 / -10.33</option>
              <option value="256,-3.2">GM 2.5-bar - 05-07 LS2/LS7 - 256 / -3.2</option>
              <option value="305,10.33">EFIsource 3-bar - Aftermarket - 305 / 10.33</option>
              <option value="312.5,-11.25" selected>GM 3-bar - LSA/LS9/TBSS - 312.5 / -11.25</option>
              <option value="350,-22">GM 3.5-bar - ZL1/CTS-V - 350 / -22</option>
              <option value="400,-20">GM 4-bar - LT4/Late Gen5 - 400 / -20</option>
            </select>
          </div>
          <div style="flex:1 1 260px">
            <label>Rail Pressure Sensor Preset</label>
            <select id="railPreset" onchange="presetRail(this.value)">
              <option value="">Custom / Manual</option>
              <option value="32.5,-16.25" selected>GM 0-130 psi oil/rail - 32.5 / -16.25</option>
              <option value="25.0,0.0">100 psi stainless Amazon/Aftermarket (0.5-4.5V) - 25.0 / 0.0</option>
            </select>
          </div>
        </div>
        <div class="sub">MAP preset ranges (kPa abs): 2-bar ~0-200 (~0-15 psig), 2.5-bar ~0-255 (~0-22 psig), 3-bar ~0-312 (~0-29 psig), 3.5-bar ~0-350 (~0-36 psig), 4-bar ~0-400 (~0-43 psig). Rail preset matches common GM 0-130 psi 0.5-4.5V.</div>
      </div>
      <div class="cal-group sensor">
        <div class="cal-head">
          <div>
            <div class="cal-step">Section 3</div>
            <div class="cal-title">Custom Sensor Calibration</div>
          </div>
          <div class="cal-help">Complete this after Analog Input Calibration. Use plain 5V sensor data. MAP uses span plus offset. Rail pressure sensor uses slope plus offset.</div>
        </div>
        <div class="sub">Setup order: complete Analog Input Calibration first, then fit and verify custom sensor calibration values.</div>
        <div class="row">
          <div style="flex:1 1 260px">
            <label>MAP Span (kPa over 0-5V)</label>
            <input id="mapLin" type="number" step="0.001" value="312.5">
            <small>MAP equation: kPa = (Span * (Volts/5)) + Offset.</small>
          </div>
          <div style="flex:1 1 260px">
            <label>MAP Offset (kPa)</label>
            <input id="mapOff" type="number" step="0.001" value="-11.25">
            <small>MAP offset at 0 V sensor output.</small>
          </div>
        </div>
        <div class="row">
          <div style="flex:1 1 260px">
            <label>Rail Slope (psig/V)</label>
            <input id="pLin" type="number" step="0.001" value="32.5">
            <small>Rail equation: psig = (Slope * Volts) + Offset.</small>
          </div>
          <div style="flex:1 1 260px">
            <label>Rail Offset (psig)</label>
            <input id="pOff" type="number" step="0.001" value="-16.25">
            <small>Rail offset at 0 V sensor output (psig).</small>
          </div>
        </div>
        <div class="sub" style="margin-top:8px">Note: measure table voltages at the divider input (sensor side). Do not apply more than 3.3V at the controller ADC input. For a 0.66 divider, 0.5-4.5V at the divider input corresponds to 0.33-2.97V at the controller input.</div>
        <div class="cal-table-title">Custom Sensor Calibration MAP Fit</div>
        <div class="sub">Sensor-side volts (0.5-4.5V). Enter voltage and MAP kPa points, then click Fit MAP Span/Offset. Expected kPa uses the current MAP equation values.</div>
        <div class="live-grid cal-gauge-row single">
          <div class="kv live-kv live-map"><div class="k">MAP (kPa abs)</div><div class="v"><span class="calMapKpa2dCopy">0.00</span></div></div>
        </div>
        <div class="adc-cal-wrap">
          <table class="adc-cal-table">
            <thead>
              <tr>
                <th>MAP Sensor V</th>
                <th>MAP kPa</th>
                <th>Expected kPa</th>
              </tr>
            </thead>
            <tbody id="mapFitBody"></tbody>
          </table>
        </div>
        <div class="btns" style="margin-top:8px">
          <button class="btn-success" onclick="fitMapFromTable()">Fit MAP Span/Offset</button>
          <button class="btn-warn" onclick="clearMapFitTable()">Clear MAP Table</button>
        </div>
        <div class="sub" id="mapFitMsg">Enter at least two MAP points to fit.</div>
        <div class="cal-table-title">Custom Sensor Calibration Rail Fit</div>
        <div class="sub">Sensor-side volts (0.5-4.5V). Enter voltage and rail psig points, then click Fit Rail Slope/Offset. Expected psig uses the current rail equation values.</div>
        <div class="live-grid cal-gauge-row single">
          <div class="kv live-kv live-rail"><div class="k">Rail Pressure (psig)</div><div class="v"><span class="calRailPsi2dCopy">0.00</span></div></div>
        </div>
        <div class="adc-cal-wrap">
          <table class="adc-cal-table">
            <thead>
              <tr>
                <th>Rail Sensor V</th>
                <th>Rail psig</th>
                <th>Expected psig</th>
              </tr>
            </thead>
            <tbody id="railFitBody"></tbody>
          </table>
        </div>
        <div class="btns" style="margin-top:8px">
          <button class="btn-success" onclick="fitRailFromTable()">Fit Rail Slope/Offset</button>
          <button class="btn-warn" onclick="clearRailFitTable()">Clear Rail Table</button>
        </div>
        <div class="sub" id="railFitMsg">Enter at least two rail points to fit.</div>
        <div class="cal-table-title">Custom Sensor Calibration MAP Quick Check</div>
        <div class="sub">Sensor-side volts (0.5-4.5V). Enter measured MAP, then compare expected MAP and % error against the current MAP equation.</div>
        <div class="live-grid cal-gauge-row single">
          <div class="kv live-kv live-map"><div class="k">MAP (kPa abs)</div><div class="v"><span class="calMapKpa2dCopy">0.00</span></div></div>
        </div>
        <div class="adc-cal-wrap">
          <table class="adc-cal-table">
            <thead>
              <tr>
                <th>Check V (eq)</th>
                <th>MAP Measured (kPa)</th>
                <th>MAP Expected (kPa)</th>
                <th>MAP Error (%)</th>
              </tr>
            </thead>
            <tbody id="sensorQuickCheckMapBody"></tbody>
          </table>
        </div>
        <div class="cal-table-title">Custom Sensor Calibration Rail Quick Check</div>
        <div class="sub">Sensor-side volts (0.5-4.5V). Enter measured rail, then compare expected rail and % error against the current rail equation.</div>
        <div class="live-grid cal-gauge-row single">
          <div class="kv live-kv live-rail"><div class="k">Rail Pressure (psig)</div><div class="v"><span class="calRailPsi2dCopy">0.00</span></div></div>
        </div>
        <div class="adc-cal-wrap">
          <table class="adc-cal-table">
            <thead>
              <tr>
                <th>Check V (eq)</th>
                <th>Rail Measured (psig)</th>
                <th>Rail Expected (psig)</th>
                <th>Rail Error (%)</th>
              </tr>
            </thead>
            <tbody id="sensorQuickCheckRailBody"></tbody>
          </table>
        </div>
      </div>
      <div class="cal-group shared">
        <div class="cal-head">
          <div>
            <div class="cal-step">Section 1</div>
            <div class="cal-title">Analog Input Calibration</div>
          </div>
          <div class="cal-help">Complete this first. Shared board-level ADC correction must be finished before preset and equation fitting.</div>
        </div>
        <div class="row">
          <div style="flex:1 1 260px">
            <label>Shared ADC Gain Trim (MAP and Rail)</label>
            <input id="adcGain" class="cal-slider" type="range" min="0.8000" max="1.2000" step="0.0005" value="1.0000" oninput="$('adcGainVal').textContent=Number(this.value).toFixed(4)">
            <div class="cal-slider-scale" aria-hidden="true">
              <span class="tick" style="left:0%"></span>
              <span class="tick" style="left:25%"></span>
              <span class="tick" style="left:50%"></span>
              <span class="tick" style="left:75%"></span>
              <span class="tick" style="left:100%"></span>
            </div>
            <div class="cal-slider-labels" aria-hidden="true">
              <span>0.8000</span><span>0.9000</span><span>1.0000</span><span>1.1000</span><span>1.2000</span>
            </div>
            <small>Value: <span id="adcGainVal">1.0000</span>. Multiplies measured MAP and rail ADC voltage before divider math. Click or drag to tune.</small>
          </div>
          <div style="flex:1 1 260px">
            <label>Shared ADC Offset Trim (V) (MAP and Rail)</label>
            <input id="adcOffset" class="cal-slider" type="range" min="-0.500" max="0.500" step="0.001" value="0.000" oninput="$('adcOffsetVal').textContent=Number(this.value).toFixed(3)">
            <div class="cal-slider-scale" aria-hidden="true">
              <span class="tick" style="left:0%"></span>
              <span class="tick" style="left:25%"></span>
              <span class="tick" style="left:50%"></span>
              <span class="tick" style="left:75%"></span>
              <span class="tick" style="left:100%"></span>
            </div>
            <div class="cal-slider-labels" aria-hidden="true">
              <span>-0.500</span><span>-0.250</span><span>0.000</span><span>0.250</span><span>0.500</span>
            </div>
            <small>Value: <span id="adcOffsetVal">0.000</span> volts. Added to measured MAP and rail ADC voltage before divider math. Click or drag to tune.</small>
          </div>
        </div>
        <div class="cal-table-title">Analog Input Curve Table</div>
        <div class="sub">Hold each injected breakpoint voltage, enter or capture MAP and rail readings, then click Fit Shared ADC Curve. Calibration live gauges (2 decimals) use the same live signals as the main Live Data card. Keyboard support: focus a slider, then use Arrow keys for fine steps, Page Up/Down for larger steps, and Home/End for min/max.</div>
        <div class="live-grid cal-gauge-row dual">
          <div class="kv live-kv live-map"><div class="k">MAP (kPa abs)</div><div class="v"><span id="calMapKpa2d" class="calMapKpa2dCopy">0.00</span></div></div>
          <div class="kv live-kv live-rail"><div class="k">Rail Pressure (psig)</div><div class="v"><span id="calRailPsi2d" class="calRailPsi2dCopy">0.00</span></div></div>
        </div>
        <div class="adc-cal-wrap">
          <table class="adc-cal-table">
            <thead>
              <tr>
                <th>Breakpoint (V)</th>
                <th>MAP Read (kPa)</th>
                <th>Rail Read (psi)</th>
                <th>Curve Out (V)</th>
                <th>Capture</th>
              </tr>
            </thead>
            <tbody id="adcBpBody"></tbody>
          </table>
        </div>
        <div class="btns" style="margin-top:8px">
          <button class="btn-success" onclick="fitAdcCurveFromTable()">Fit Shared ADC Curve</button>
          <button class="btn-warn" onclick="resetAdcCurveIdentity()">Reset ADC Curve to Identity</button>
        </div>
        <div class="sub" id="adcCurveFitMsg">Curve is applied in memory. Press Save to persist.</div>
      </div>
    </div>
    <div class="btns">
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-info" onclick="loadCfg()">Refresh</button>
    </div>
  </div>

  <div class="card collapsed" data-title="Wi-Fi Settings">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Wi-Fi Settings</div>
    <div class="sub">Defaults: Mode STA (forced); STA SSID iPhone / Pass 00000000; AP SSID watermeth (open); mDNS watermeth.</div>
    <div class="sub">Set mode first, then access point and multicast domain name system identity, then station credentials.</div>
    <div class="settings-intro">
      <span class="settings-pill hardware">Mode</span>
      <span class="settings-pill flow">AP and mDNS</span>
      <span class="settings-pill safety">STA Credentials</span>
    </div>
    <div class="settings-layout">
      <div class="settings-group hardware">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 1</div>
            <div class="settings-title">Wi-Fi Mode</div>
          </div>
          <div class="settings-help">Choose access point, station, or both. The selected mode persists across reboots.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>Wi-Fi Mode</label>
            <select id="wifiMode">
              <option value="0">AP only (uses AP SSID below)</option>
              <option value="1">STA only (connect to home Wi-Fi)</option>
              <option value="2">AP + STA (both)</option>
            </select>
          </div>
        </div>
      </div>
      <div class="settings-group flow">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 2</div>
            <div class="settings-title">AP and mDNS Identity</div>
          </div>
          <div class="settings-help">Access point name and multicast domain name system host are user editable and saved. Changes apply after reboot.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>AP SSID</label>
            <input id="apSsid" type="text" maxlength="32">
          </div>
          <div class="settings-field">
            <label>mDNS Hostname</label>
            <input id="mdnsHost" type="text" maxlength="32">
          </div>
        </div>
      </div>
      <div class="settings-group safety">
        <div class="settings-head">
          <div>
            <div class="settings-step">Section 3</div>
            <div class="settings-title">Station Credentials</div>
          </div>
          <div class="settings-help">Enter station credentials only when using station mode. Leave station password blank when not required.</div>
        </div>
        <div class="settings-grid">
          <div class="settings-field">
            <label>STA SSID</label>
            <input id="staSsid" type="text" maxlength="32">
          </div>
          <div class="settings-field">
            <label>STA Password <span class="sub" style="display:inline">Leave blank unless using STA mode.</span></label>
            <input id="staPass" type="password" maxlength="63">
          </div>
        </div>
      </div>
    </div>
    <div class="btns">
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-info" onclick="loadCfg()">Refresh</button>
    </div>
  </div>

  <div class="card collapsed" data-title="Test Section">
    <div class="h1" style="font-size:16px" onclick="toggleCard(this)">Test Section</div>
    <div class="sub">Bench testing tools. Test section force commands are absolute overrides for bench use.</div>
    <div class="sub">Defaults: All force values blank/off; all manual fault controls OFF.</div>
    <div class="test-layout">
      <div class="test-group sensor">
        <div class="test-head">
          <div>
            <div class="test-step">Section 1</div>
            <div class="test-title">Sensor Overrides</div>
          </div>
          <div class="test-help">Force individual sensor values for bench testing without physical signal changes.</div>
        </div>
        <div class="test-grid">
          <div class="test-field">
            <label>Force MAP (kPa) <span class="sub" style="display:inline">Leave blank or negative to use the sensor.</span></label>
            <input id="forceMapKpa" type="number" step="0.1" placeholder="leave blank for normal">
          </div>
          <div class="test-field">
            <label>Force Rail (psig) <span class="sub" style="display:inline">Leave blank or negative to use the sensor.</span></label>
            <input id="forceRailPsi" type="number" step="0.1" placeholder="leave blank for normal">
          </div>
          <div class="test-field">
            <label>Force Baro (kPa abs) <span class="sub" style="display:inline">Leave blank or negative to use boot capture.</span></label>
            <input id="forceBaroKpa" type="number" step="0.1" placeholder="leave blank for normal">
          </div>
          <div class="test-field">
            <label>Force Injector dP (psi) <span class="sub" style="display:inline">Overrides rail from MAP + dP.</span></label>
            <input id="forceDpPsi" type="number" step="0.1" placeholder="leave blank for normal">
          </div>
        </div>
      </div>

      <div class="test-group output">
        <div class="test-head">
          <div>
            <div class="test-step">Section 2</div>
            <div class="test-title">Output Overrides</div>
          </div>
          <div class="test-help">Force level, pump, and injector outputs directly. Force Pump and Force Injectors ignore automatic safety gating while active.</div>
        </div>
        <div class="test-grid">
          <div class="test-field">
            <label>Force Level Switch</label>
            <select id="forceLevel">
              <option value="0">Normal</option>
              <option value="1">Force LOW (open)</option>
              <option value="2">Force OK (grounded)</option>
            </select>
          </div>
          <div class="test-field">
            <label>Force Pump <span class="sub" style="display:inline">Force ON and Force OFF are absolute output commands.</span></label>
            <select id="forcePump">
              <option value="0">Normal</option>
              <option value="1">Force OFF</option>
              <option value="2">Force ON</option>
            </select>
          </div>
          <div class="test-field">
            <label>Force Injectors <span class="sub" style="display:inline">Force ON is absolute and uses the duty below.</span></label>
            <select id="forceInj">
              <option value="0">Normal</option>
              <option value="1">Force OFF</option>
              <option value="2">Force ON (duty below)</option>
            </select>
          </div>
          <div class="test-field">
            <label>Forced Duty (%) <span class="sub" style="display:inline">Only used when Force Injectors is ON.</span></label>
            <input id="forceDuty" type="number" min="0" max="100" step="1">
          </div>
        </div>
      </div>

      <div class="test-group manual">
        <div class="test-head">
          <div>
            <div class="test-step">Section 3</div>
            <div class="test-title">Manual Fault Controls and Overrides</div>
          </div>
          <div class="test-help">Use individual controls to force or bypass each safety path during bench testing.</div>
        </div>
        <div class="fault-controls">
          <div class="fault-group manual">
            <div class="fault-group-title">Manual Fault Triggers</div>
            <div class="fault-grid">
              <button class="btn-danger btn-latch" id="btnBoostCut" onclick="toggleBoostCut()">Manual Boost Cut: OFF</button>
              <button class="btn-danger btn-latch" id="btnTimingCut" onclick="toggleTimingCut()">Manual Timing Cut: OFF</button>
              <button class="btn-danger btn-latch" id="btnDpFault" onclick="toggleDpFault()">Rail dP Fault: OFF</button>
              <button class="btn-danger btn-latch" id="btnPressureReadyFault" onclick="togglePressureReadyFault()">Pressure-Ready Fault: OFF</button>
              <button class="btn-danger btn-latch" id="btnDpBoostHold" onclick="toggleDpBoostHold()">Rail dP Boost Hold: OFF</button>
              <button class="btn-danger btn-latch" id="btnPressureReadyBoostHold" onclick="togglePressureReadyBoostHold()">Pressure-Ready Boost Hold: OFF</button>
            </div>
          </div>
          <div class="fault-group override">
            <div class="fault-group-title">Monitor Overrides</div>
            <div class="fault-grid">
              <button class="btn-danger btn-latch" id="btnDpMonOverride" onclick="toggleDpMonOverride()">dP Monitor Override: OFF</button>
              <button class="btn-danger btn-latch" id="btnPressureReadyOverride" onclick="togglePressureReadyOverride()">Pressure-Ready Override: OFF</button>
            </div>
          </div>
          <div class="fault-group bypass">
            <div class="fault-group-title">Safety Bypasses</div>
            <div class="fault-grid">
              <button class="btn-danger btn-latch" id="btnLevelBypass" onclick="toggleLevelBypass()">Level Fault Bypass: OFF</button>
              <button class="btn-danger btn-latch" id="btnDpFaultBypass" onclick="toggleDpFaultBypass()">dP Fault Bypass: OFF</button>
              <button class="btn-danger btn-latch" id="btnPressureReadyFaultBypass" onclick="togglePressureReadyFaultBypass()">Pressure-Ready Fault Bypass: OFF</button>
              <button class="btn-danger btn-latch" id="btnDpHoldBypass" onclick="toggleDpHoldBypass()">dP Hold Bypass: OFF</button>
              <button class="btn-danger btn-latch" id="btnPressureReadyHoldBypass" onclick="togglePressureReadyHoldBypass()">Pressure-Ready Hold Bypass: OFF</button>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="btns" style="margin-top:10px">
      <button class="primary" onclick="saveAll()">Save</button>
      <button class="btn-danger" onclick="unforceAll()">Unforce All</button>
    </div>
  </div>
</div>

<script>
const $=id=>document.getElementById(id);
const setText=(id, v)=>{ const el=$(id); if(el) el.textContent=v; };
const setTextByClass=(cls, v)=>{ document.querySelectorAll(`.${cls}`).forEach(el=>{ el.textContent=v; }); };
const ADC_BREAKPOINTS = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3];
const MAP_SENSOR_CAL_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const RAIL_SENSOR_CAL_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const SENSOR_QUICK_CHECK_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const DEFAULTS = {
  dp_arm_duty_pct: 5.0,
  dp_recover_margin_psi: 1.5,
  dp_critical_psi: 25.0,
  dp_critical_ms: 150,
  dp_arm_settle_ms: 500,
  timing_cut_auto_clear_ms: 4000,
  pressure_ready_timeout_ms: 3000,
  level_debounce_ms: 300,
  inj_lbhr_at_58psi: 36.0,
  mix_meth_pct: 60.0,
  flow_ref_dp_psi: 60.0,
  map_div_ratio: 0.66,
  rail_div_ratio: 0.66,
  adc_input_gain: 1.0000,
  adc_input_offset_v: 0.000,
  serial_status_period_ms: 250,
  serial_debug_enable: 1
};
const DEFAULT_SETTINGS_PRESET = {
  injHz: 50,
  dutyClamp: 60,
  dpMinPsi: 50,
  dpFaultMs: 3000,
  dpArmPct: DEFAULTS.dp_arm_duty_pct,
  dpRecover: DEFAULTS.dp_recover_margin_psi,
  dpCriticalPsi: DEFAULTS.dp_critical_psi,
  dpCriticalMs: DEFAULTS.dp_critical_ms,
  dpArmSettleMs: DEFAULTS.dp_arm_settle_ms,
  timingCutAutoMs: DEFAULTS.timing_cut_auto_clear_ms,
  pressureReadyTimeoutMs: DEFAULTS.pressure_ready_timeout_ms,
  lvlDebounce: DEFAULTS.level_debounce_ms,
  injLb: DEFAULTS.inj_lbhr_at_58psi,
  mixMethPct: DEFAULTS.mix_meth_pct,
  targetInjectorDp: DEFAULTS.flow_ref_dp_psi,
  mapLin: 312.5,
  mapOff: -11.25,
  pLin: 32.5,
  pOff: -16.25,
  mapDiv: DEFAULTS.map_div_ratio,
  railDiv: DEFAULTS.rail_div_ratio,
  adcGain: DEFAULTS.adc_input_gain,
  adcOffset: DEFAULTS.adc_input_offset_v,
  adcCurveY: [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3],
  sdbg: DEFAULTS.serial_debug_enable,
  sper: DEFAULTS.serial_status_period_ms,
  curve: [
    {kpa:145, pct:5},
    {kpa:156, pct:10},
    {kpa:167, pct:18},
    {kpa:178, pct:28},
    {kpa:189, pct:40},
    {kpa:200, pct:55},
    {kpa:211, pct:70},
    {kpa:222, pct:85},
    {kpa:233, pct:95},
    {kpa:245, pct:100}
  ]
};

function fmt(n,d=1){return (Number(n)||0).toFixed(d);}
let adcBpRows = ADC_BREAKPOINTS.map(()=>({map:"", rail:""}));

function adcIdentityCurveY(){
  return ADC_BREAKPOINTS.slice();
}

function clampNum(v, lo, hi){
  return Math.max(lo, Math.min(hi, v));
}

function ensureCfgAdcCurveY(){
  if(!cfg) return adcIdentityCurveY();
  if(!Array.isArray(cfg.adcCurveY) || cfg.adcCurveY.length !== ADC_BREAKPOINTS.length){
    cfg.adcCurveY = adcIdentityCurveY();
  }
  cfg.adcCurveY = cfg.adcCurveY.map((v, i)=>isFinite(Number(v)) ? Number(v) : ADC_BREAKPOINTS[i]);
  return cfg.adcCurveY;
}

function updateAdcCurveFitMsg(msg){
  setText("adcCurveFitMsg", msg);
}

function renderAdcBpTable(){
  const tb = $("adcBpBody");
  if(!tb) return;
  const curveY = ensureCfgAdcCurveY();
  tb.innerHTML = "";
  ADC_BREAKPOINTS.forEach((bp, i)=>{
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${bp.toFixed(2)}</td>
      <td><input id="adcBpMap${i}" type="number" step="0.01" value="${adcBpRows[i].map}"></td>
      <td><input id="adcBpRail${i}" type="number" step="0.01" value="${adcBpRows[i].rail}"></td>
      <td><input id="adcCurveY${i}" type="number" step="0.0001" value="${Number(curveY[i]).toFixed(4)}" ${i===0 || i===(ADC_BREAKPOINTS.length-1) ? "readonly" : ""}></td>
      <td><button class="btn-info" onclick="captureAdcBpRow(${i})">Capture</button></td>`;
    tb.appendChild(tr);
  });
  ADC_BREAKPOINTS.forEach((_, i)=>{
    const mapEl = $(`adcBpMap${i}`);
    const railEl = $(`adcBpRail${i}`);
    const curveEl = $(`adcCurveY${i}`);
    if(mapEl){
      mapEl.addEventListener("input", ()=>{ adcBpRows[i].map = mapEl.value; });
    }
    if(railEl){
      railEl.addEventListener("input", ()=>{ adcBpRows[i].rail = railEl.value; });
    }
    if(curveEl){
      curveEl.addEventListener("input", ()=>{
        const y = Number(curveEl.value);
        if(!isFinite(y) || !cfg) return;
        const out = ensureCfgAdcCurveY();
        out[i] = clampNum(y, 0.0, 3.3);
        out[0] = 0.0;
        out[out.length - 1] = 3.3;
        for(let j = 1; j < out.length; j++) out[j] = Math.max(out[j], out[j - 1]);
        for(let j = out.length - 2; j > 0; j--) out[j] = Math.min(out[j], out[j + 1]);
        cfg.adcCurveY = out.map(v=>Number(v.toFixed(4)));
        renderAdcBpTable();
      });
    }
  });
}

function captureAdcBpRow(i){
  if(!status || i < 0 || i >= adcBpRows.length) return;
  const mapDisp = (status.mapKpa!==undefined && status.mapKpa !== null) ? status.mapKpa : (status.mapKpaRaw||0);
  adcBpRows[i].map = Number(mapDisp).toFixed(2);
  adcBpRows[i].rail = Number(status.railPsi||0).toFixed(2);
  renderAdcBpTable();
}

function mapReadingToPinVolts(mapKpaVal){
  const mapLin = Number($("mapLin").value);
  const mapOff = Number($("mapOff").value);
  const mapDiv = Number($("mapDivRatio").value);
  if(!isFinite(mapLin) || !isFinite(mapOff) || !isFinite(mapDiv) || Math.abs(mapLin) < 1e-6 || mapDiv <= 0.0) return NaN;
  const sensV = (Number(mapKpaVal) - mapOff) / (mapLin / 5.0);
  return sensV * mapDiv;
}

function railReadingToPinVolts(railPsiVal){
  const pLin = Number($("pLin").value);
  const pOff = Number($("pOff").value);
  const railDiv = Number($("railDivRatio").value);
  if(!isFinite(pLin) || !isFinite(pOff) || !isFinite(railDiv) || Math.abs(pLin) < 1e-6 || railDiv <= 0.0) return NaN;
  const sensV = (Number(railPsiVal) - pOff) / pLin;
  return sensV * railDiv;
}

function collectAdcCurveFitSamples(){
  const samples = [];
  ADC_BREAKPOINTS.forEach((targetV, i)=>{
    const mapVal = Number(adcBpRows[i].map);
    if(isFinite(mapVal)){
      const x = mapReadingToPinVolts(mapVal);
      if(isFinite(x)) samples.push({x, y:targetV});
    }
    const railVal = Number(adcBpRows[i].rail);
    if(isFinite(railVal)){
      const x = railReadingToPinVolts(railVal);
      if(isFinite(x)) samples.push({x, y:targetV});
    }
  });
  samples.sort((a,b)=>a.x-b.x);
  return samples;
}

function collapseAdcSamples(samples){
  const out = [];
  samples.forEach(s=>{
    if(!out.length || Math.abs(s.x - out[out.length - 1].x) > 1e-4){
      out.push({x:s.x, y:s.y, n:1});
    }else{
      const p = out[out.length - 1];
      p.y = ((p.y * p.n) + s.y) / (p.n + 1);
      p.n += 1;
    }
  });
  return out.map(p=>({x:p.x, y:p.y}));
}

function fitCurveYFromSamples(samples){
  const pts = collapseAdcSamples(samples);
  if(pts.length < 2) return adcIdentityCurveY();
  const fit = ADC_BREAKPOINTS.map((xk)=>{
    if(xk <= pts[0].x) return pts[0].y;
    if(xk >= pts[pts.length - 1].x) return pts[pts.length - 1].y;
    for(let i = 0; i < (pts.length - 1); i++){
      const a = pts[i];
      const b = pts[i + 1];
      if(xk <= b.x){
        const t = (xk - a.x) / Math.max(1e-6, (b.x - a.x));
        return a.y + ((b.y - a.y) * t);
      }
    }
    return pts[pts.length - 1].y;
  });
  fit[0] = 0.0;
  fit[fit.length - 1] = 3.3;
  for(let i = 1; i < fit.length; i++) fit[i] = Math.max(fit[i], fit[i - 1]);
  for(let i = fit.length - 2; i > 0; i--) fit[i] = Math.min(fit[i], fit[i + 1]);
  return fit.map(v=>Number(clampNum(v, 0.0, 3.3).toFixed(4)));
}

function fitAdcCurveFromTable(){
  if(!cfg) return;
  const samples = collectAdcCurveFitSamples();
  if(samples.length < 4){
    updateAdcCurveFitMsg("Need at least 4 valid MAP/rail sample entries to fit the curve.");
    return;
  }
  cfg.adcCurveY = fitCurveYFromSamples(samples);
  renderAdcBpTable();
  updateAdcCurveFitMsg(`Fit complete from ${samples.length} samples. Press Save to persist.`);
}

function resetAdcCurveIdentity(){
  if(!cfg) return;
  cfg.adcCurveY = adcIdentityCurveY();
  renderAdcBpTable();
  updateAdcCurveFitMsg("Shared ADC curve reset to identity. Press Save to persist.");
}

function renderMapFitTable(){
  const tb = $("mapFitBody");
  if(!tb || tb.dataset.init === "1") return;
  tb.innerHTML = "";
  MAP_SENSOR_CAL_POINTS.forEach((v, i)=>{
    const tr = document.createElement("tr");
    tr.innerHTML = `<td><input id="mapFitV${i}" type="number" step="0.01" value="${v.toFixed(2)}"></td>
      <td><input id="mapFitK${i}" type="number" step="0.01"></td>
      <td><span id="mapFitExp${i}">-</span></td>`;
    tb.appendChild(tr);
    const vEl = $(`mapFitV${i}`);
    if(vEl) vEl.addEventListener("input", ()=>updateMapFitExpectedRow(i));
  });
  tb.dataset.init = "1";
  updateMapFitExpectedTable();
}

function renderRailFitTable(){
  const tb = $("railFitBody");
  if(!tb || tb.dataset.init === "1") return;
  tb.innerHTML = "";
  RAIL_SENSOR_CAL_POINTS.forEach((v, i)=>{
    const tr = document.createElement("tr");
    tr.innerHTML = `<td><input id="railFitV${i}" type="number" step="0.01" value="${v.toFixed(2)}"></td>
      <td><input id="railFitP${i}" type="number" step="0.01"></td>
      <td><span id="railFitExp${i}">-</span></td>`;
    tb.appendChild(tr);
    const vEl = $(`railFitV${i}`);
    if(vEl) vEl.addEventListener("input", ()=>updateRailFitExpectedRow(i));
  });
  tb.dataset.init = "1";
  updateRailFitExpectedTable();
}

function renderSensorQuickCheckTable(){
  const mapTb = $("sensorQuickCheckMapBody");
  if(mapTb && mapTb.dataset.init !== "1"){
    mapTb.innerHTML = "";
    SENSOR_QUICK_CHECK_POINTS.forEach((v, i)=>{
      const tr = document.createElement("tr");
      tr.innerHTML = `<td>${v.toFixed(2)}</td>
        <td><input id="quickMap${i}" type="number" step="0.01"></td>
        <td><span id="quickMapExp${i}">-</span></td>
        <td><span id="quickMapErr${i}">-</span></td>`;
      mapTb.appendChild(tr);
      const mapEl = $(`quickMap${i}`);
      if(mapEl) mapEl.addEventListener("input", ()=>updateSensorQuickCheckMapRow(i));
    });
    mapTb.dataset.init = "1";
  }
  const railTb = $("sensorQuickCheckRailBody");
  if(railTb && railTb.dataset.init !== "1"){
    railTb.innerHTML = "";
    SENSOR_QUICK_CHECK_POINTS.forEach((v, i)=>{
      const tr = document.createElement("tr");
      tr.innerHTML = `<td>${v.toFixed(2)}</td>
        <td><input id="quickRail${i}" type="number" step="0.01"></td>
        <td><span id="quickRailExp${i}">-</span></td>
        <td><span id="quickRailErr${i}">-</span></td>`;
      railTb.appendChild(tr);
      const railEl = $(`quickRail${i}`);
      if(railEl) railEl.addEventListener("input", ()=>updateSensorQuickCheckRailRow(i));
    });
    railTb.dataset.init = "1";
  }
  updateSensorQuickCheckTable();
}

function mapExpectedFromSensorVolts(v){
  const mapLin = Number($("mapLin").value);
  const mapOff = Number($("mapOff").value);
  if(!isFinite(v) || !isFinite(mapLin) || !isFinite(mapOff)) return null;
  return ((mapLin / 5.0) * v) + mapOff;
}

function railExpectedFromSensorVolts(v){
  const pLin = Number($("pLin").value);
  const pOff = Number($("pOff").value);
  if(!isFinite(v) || !isFinite(pLin) || !isFinite(pOff)) return null;
  return (pLin * v) + pOff;
}

function updateMapFitExpectedRow(i){
  const v = Number($(`mapFitV${i}`)?.value);
  const expected = mapExpectedFromSensorVolts(v);
  setText(`mapFitExp${i}`, isFinite(expected) ? expected.toFixed(2) : "-");
}

function updateRailFitExpectedRow(i){
  const v = Number($(`railFitV${i}`)?.value);
  const expected = railExpectedFromSensorVolts(v);
  setText(`railFitExp${i}`, isFinite(expected) ? expected.toFixed(2) : "-");
}

function updateMapFitExpectedTable(){
  for(let i = 0; i < MAP_SENSOR_CAL_POINTS.length; i++) updateMapFitExpectedRow(i);
}

function updateRailFitExpectedTable(){
  for(let i = 0; i < RAIL_SENSOR_CAL_POINTS.length; i++) updateRailFitExpectedRow(i);
}

function updateFitExpectedTables(){
  updateMapFitExpectedTable();
  updateRailFitExpectedTable();
}

function percentError(measured, expected, fallbackDen){
  if(!Number.isFinite(measured) || !Number.isFinite(expected)) return null;
  let denom = Math.abs(expected);
  if(denom < 1e-6 && Number.isFinite(fallbackDen) && Math.abs(fallbackDen) >= 1e-6){
    denom = Math.abs(fallbackDen);
  }
  if(denom < 1e-6){
    return Math.abs(measured - expected) < 1e-6 ? 0.0 : null;
  }
  return ((measured - expected) / denom) * 100.0;
}

function updateSensorQuickCheckMapRow(i){
  const v = SENSOR_QUICK_CHECK_POINTS[i];
  const mapExpected = mapExpectedFromSensorVolts(v);
  const mapFs = (mapExpectedFromSensorVolts(4.5) ?? NaN) - (mapExpectedFromSensorVolts(0.5) ?? NaN);
  const mapMeasured = Number($(`quickMap${i}`)?.value);
  const mapErrPct = percentError(mapMeasured, mapExpected, mapFs);
  setText(`quickMapExp${i}`, Number.isFinite(mapExpected) ? mapExpected.toFixed(2) : "-");
  setText(`quickMapErr${i}`, Number.isFinite(mapErrPct) ? `${mapErrPct.toFixed(2)}%` : "-");
}

function updateSensorQuickCheckRailRow(i){
  const v = SENSOR_QUICK_CHECK_POINTS[i];
  const railExpected = railExpectedFromSensorVolts(v);
  const railFs = (railExpectedFromSensorVolts(4.5) ?? NaN) - (railExpectedFromSensorVolts(0.5) ?? NaN);
  const railMeasured = Number($(`quickRail${i}`)?.value);
  const railErrPct = percentError(railMeasured, railExpected, railFs);
  setText(`quickRailExp${i}`, Number.isFinite(railExpected) ? railExpected.toFixed(2) : "-");
  setText(`quickRailErr${i}`, Number.isFinite(railErrPct) ? `${railErrPct.toFixed(2)}%` : "-");
}

function updateSensorQuickCheckTable(){
  for(let i = 0; i < SENSOR_QUICK_CHECK_POINTS.length; i++){
    updateSensorQuickCheckMapRow(i);
    updateSensorQuickCheckRailRow(i);
  }
}

function collectFitPoints(xPrefix, yPrefix, xPoints){
  const points = [];
  for(let i = 0; i < xPoints.length; i++){
    const x = Number($(`${xPrefix}${i}`)?.value);
    const y = Number($(`${yPrefix}${i}`)?.value);
    if(isFinite(x) && isFinite(y)) points.push({x, y});
  }
  return points;
}

function fitLine(points){
  if(!Array.isArray(points) || points.length < 2) return null;
  if(points.length === 2){
    const dx = points[1].x - points[0].x;
    if(Math.abs(dx) < 1e-9) return null;
    const m = (points[1].y - points[0].y) / dx;
    const b = points[0].y - (m * points[0].x);
    return {m, b, n:2};
  }
  let sumX = 0;
  let sumY = 0;
  let sumXX = 0;
  let sumXY = 0;
  const n = points.length;
  points.forEach(p=>{
    sumX += p.x;
    sumY += p.y;
    sumXX += p.x * p.x;
    sumXY += p.x * p.y;
  });
  const denom = (n * sumXX) - (sumX * sumX);
  if(Math.abs(denom) < 1e-9) return null;
  const m = ((n * sumXY) - (sumX * sumY)) / denom;
  const b = (sumY - (m * sumX)) / n;
  return {m, b, n};
}

function fitMapFromTable(){
  const fit = fitLine(collectFitPoints("mapFitV", "mapFitK", MAP_SENSOR_CAL_POINTS));
  if(!fit){
    setText("mapFitMsg", "Need at least two valid MAP points with different voltage values.");
    return;
  }
  const mapLin = fit.m * 5.0;
  const mapOff = fit.b;
  $("mapLin").value = mapLin.toFixed(3);
  $("mapOff").value = mapOff.toFixed(3);
  updateMapFitExpectedTable();
  updateSensorQuickCheckTable();
  setText("mapFitMsg", `MAP fit applied from ${fit.n} points. Span ${mapLin.toFixed(3)}, Offset ${mapOff.toFixed(3)}.`);
}

function fitRailFromTable(){
  const fit = fitLine(collectFitPoints("railFitV", "railFitP", RAIL_SENSOR_CAL_POINTS));
  if(!fit){
    setText("railFitMsg", "Need at least two valid rail points with different voltage values.");
    return;
  }
  const pLin = fit.m;
  const pOff = fit.b;
  $("pLin").value = pLin.toFixed(3);
  $("pOff").value = pOff.toFixed(3);
  updateRailFitExpectedTable();
  updateSensorQuickCheckTable();
  setText("railFitMsg", `Rail fit applied from ${fit.n} points. Slope ${pLin.toFixed(3)}, Offset ${pOff.toFixed(3)}.`);
}

function clearMapFitTable(){
  for(let i = 0; i < MAP_SENSOR_CAL_POINTS.length; i++){
    const y = $(`mapFitK${i}`);
    if(y) y.value = "";
  }
  setText("mapFitMsg", "MAP table cleared.");
}

function clearRailFitTable(){
  for(let i = 0; i < RAIL_SENSOR_CAL_POINTS.length; i++){
    const y = $(`railFitP${i}`);
    if(y) y.value = "";
  }
  setText("railFitMsg", "Rail table cleared.");
}

const REF_DP=58.0;
let uiInjLbhr = 36.0;
const uiInjCount = 3;

function getMixPct(){
  const v = Number($("mixMethPct").value);
  return isFinite(v) ? v : 60.0;
}
function mixDensityFromPct(pct){
  const p = Math.max(0, Math.min(100, pct));
  return ((p/100.0)*0.792) + ((1.0 - p/100.0)*0.998);
}

function readInjModel(){
  uiInjLbhr = Number($("injLb").value) || uiInjLbhr || 36.0;
}

function cloneCurvePoints(points){
  return points.map(p => ({kpa:Number(p.kpa)||0, pct:Number(p.pct)||0}));
}

function applyDefaultSettings(){
  if(!cfg) return;
  cfg.injHz = DEFAULT_SETTINGS_PRESET.injHz;
  cfg.dutyClamp = DEFAULT_SETTINGS_PRESET.dutyClamp;
  cfg.dpMinPsi = DEFAULT_SETTINGS_PRESET.dpMinPsi;
  cfg.dpFaultMs = DEFAULT_SETTINGS_PRESET.dpFaultMs;
  cfg.dpArmPct = DEFAULT_SETTINGS_PRESET.dpArmPct;
  cfg.dpRecover = DEFAULT_SETTINGS_PRESET.dpRecover;
  cfg.dpCriticalPsi = DEFAULT_SETTINGS_PRESET.dpCriticalPsi;
  cfg.dpCriticalMs = DEFAULT_SETTINGS_PRESET.dpCriticalMs;
  cfg.dpArmSettleMs = DEFAULT_SETTINGS_PRESET.dpArmSettleMs;
  cfg.timingCutAutoMs = DEFAULT_SETTINGS_PRESET.timingCutAutoMs;
  cfg.pressureReadyTimeoutMs = DEFAULT_SETTINGS_PRESET.pressureReadyTimeoutMs;
  cfg.lvlDebounce = DEFAULT_SETTINGS_PRESET.lvlDebounce;
  cfg.injLb = DEFAULT_SETTINGS_PRESET.injLb;
  cfg.mixMethPct = DEFAULT_SETTINGS_PRESET.mixMethPct;
  cfg.targetInjectorDp = DEFAULT_SETTINGS_PRESET.targetInjectorDp;
  cfg.mapLin = DEFAULT_SETTINGS_PRESET.mapLin;
  cfg.mapOff = DEFAULT_SETTINGS_PRESET.mapOff;
  cfg.pLin = DEFAULT_SETTINGS_PRESET.pLin;
  cfg.pOff = DEFAULT_SETTINGS_PRESET.pOff;
  cfg.mapDiv = DEFAULT_SETTINGS_PRESET.mapDiv;
  cfg.railDiv = DEFAULT_SETTINGS_PRESET.railDiv;
  cfg.adcGain = DEFAULT_SETTINGS_PRESET.adcGain;
  cfg.adcOffset = DEFAULT_SETTINGS_PRESET.adcOffset;
  cfg.adcCurveY = DEFAULT_SETTINGS_PRESET.adcCurveY.slice();
  cfg.sdbg = DEFAULT_SETTINGS_PRESET.sdbg;
  cfg.sper = DEFAULT_SETTINGS_PRESET.sper;
  cfg.curve = cloneCurvePoints(DEFAULT_SETTINGS_PRESET.curve);
  $("mapPreset").value = `${DEFAULT_SETTINGS_PRESET.mapLin},${DEFAULT_SETTINGS_PRESET.mapOff}`;
  $("railPreset").value = `${DEFAULT_SETTINGS_PRESET.pLin},${DEFAULT_SETTINGS_PRESET.pOff}`;
  populateCfg();
  drawCurve();
  renderTable();
  updateDutyClampFlow();
}

function calcFlowFromDuty(duty, dpPsi){
  duty=Math.max(0,Math.min(100, Number(duty)||0));
  dpPsi = Number(dpPsi); if(!isFinite(dpPsi) || dpPsi<=0) dpPsi=REF_DP;
  const mixDens = mixDensityFromPct(getMixPct());
  const scale=Math.sqrt(dpPsi/REF_DP);
  const lbhr=(uiInjLbhr*uiInjCount)*scale*(duty/100.0);
  const ccmin=(lbhr*453.59237/3600.0)/mixDens*60.0;
  const gph=(ccmin/3785.411784)*60.0;
  return {lbhr, ccmin, gph};
}
function updateDutyClampFlow(){
  readInjModel();
  const duty=Number($("dutyClamp").value)||0;
  const targetDp = Number($("targetInjectorDp").value)||0;
  const dp = targetDp>0 ? targetDp : (status && isFinite(status.dpPsi) ? status.dpPsi : REF_DP);
  const f=calcFlowFromDuty(duty, dp);
  $("dutyClampFlow").textContent = `~ ${fmt(f.ccmin,0)} cc/min | ${fmt(f.lbhr,1)} lb/hr | ${fmt(f.gph,2)} gph`;
}

async function apiGet(p){const r=await fetch(p,{cache:"no-store"}); if(!r.ok) throw new Error(await r.text()); return await r.json();}
async function apiPost(p,obj){const r=await fetch(p,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(obj)}); if(!r.ok) throw new Error(await r.text()); return await r.json();}


function populateCfg(){
  if(!cfg) return;
  $("injHz").value=cfg.injHz;
  $("dutyClamp").value=cfg.dutyClamp;
  $("dpMinPsi").value=cfg.dpMinPsi;
  $("dpFaultMs").value=cfg.dpFaultMs;
  $("dpArmPct").value = cfg.dpArmPct ?? DEFAULTS.dp_arm_duty_pct;
  $("dpRecover").value = cfg.dpRecover ?? DEFAULTS.dp_recover_margin_psi;
  $("dpCriticalPsi").value = cfg.dpCriticalPsi ?? DEFAULTS.dp_critical_psi;
  $("dpCriticalMs").value = cfg.dpCriticalMs ?? DEFAULTS.dp_critical_ms;
  $("dpArmSettleMs").value = cfg.dpArmSettleMs ?? DEFAULTS.dp_arm_settle_ms;
  $("timingCutAutoMs").value = cfg.timingCutAutoMs ?? DEFAULTS.timing_cut_auto_clear_ms;
  $("pressureReadyTimeoutMs").value = cfg.pressureReadyTimeoutMs ?? DEFAULTS.pressure_ready_timeout_ms;
  $("lvlDebounce").value = cfg.lvlDebounce ?? DEFAULTS.level_debounce_ms;
  $("injLb").value = cfg.injLb ?? 36.0;
  readInjModel();
  $("curveStart").value = cfg.curve[0]?.kpa ?? "";
  $("curveMax").value = cfg.curve[cfg.curve.length-1]?.kpa ?? "";

  $("targetInjectorDp").value = cfg.targetInjectorDp ?? DEFAULTS.flow_ref_dp_psi;

  $("mapLin").value=cfg.mapLin;
  $("mapOff").value=cfg.mapOff;
  $("pLin").value=cfg.pLin;
  $("pOff").value=cfg.pOff;
  $("injLb").value = cfg.injLb ?? DEFAULTS.inj_lbhr_at_58psi;
  readInjModel();

  $("wifiMode").value=cfg.wifiMode;
  $("apSsid").value=cfg.apSsid||"";
  $("mdnsHost").value=cfg.mdnsHost||"";
  $("staSsid").value=cfg.staSsid||"";
  $("staPass").value=cfg.staPass||"";

  $("forcePump").value=cfg.forcePump;
  $("forceInj").value=cfg.forceInj;
  $("forceDuty").value=cfg.forceDuty;
  $("forceMapKpa").value = (cfg.forceMapKpa>=0) ? cfg.forceMapKpa : "";
  $("forceRailPsi").value = (cfg.forceRailPsi>=0) ? cfg.forceRailPsi : "";
  $("forceBaroKpa").value = (cfg.forceBaroKpa>=0) ? cfg.forceBaroKpa : "";
  $("forceDpPsi").value = (cfg.forceDpPsi>=0) ? cfg.forceDpPsi : "";
  $("forceLevel").value = cfg.forceLevel ?? 0;
  forceBoostCut = !!cfg.forceBoostCut;
  forceTimingCut = !!cfg.forceTimingCut;
  forceDpFault = !!cfg.forceDpFault;
  forcePressureReadyFault = !!cfg.forcePressureReadyFault;
  forceDpBoostHold = !!cfg.forceDpBoostHold;
  forcePressureReadyBoostHold = !!cfg.forcePressureReadyBoostHold;
  forceDpMonitorOverride = !!cfg.forceDpMonitorOverride;
  forcePressureReadyOverride = !!cfg.forcePressureReadyOverride;
  forceLevelFaultBypass = !!cfg.forceLevelFaultBypass;
  forceDpFaultBypass = !!cfg.forceDpFaultBypass;
  forceDpBoostHoldBypass = !!cfg.forceDpBoostHoldBypass;
  forcePressureReadyFaultBypass = !!cfg.forcePressureReadyFaultBypass;
  forcePressureReadyBoostHoldBypass = !!cfg.forcePressureReadyBoostHoldBypass;
  updateLatchButtons();

  $("mapDivRatio").value = cfg.mapDiv ?? DEFAULTS.map_div_ratio;
  $("railDivRatio").value = cfg.railDiv ?? DEFAULTS.rail_div_ratio;
  const adcGain = cfg.adcGain ?? DEFAULTS.adc_input_gain;
  const adcOffset = cfg.adcOffset ?? DEFAULTS.adc_input_offset_v;
  $("adcGain").value = adcGain;
  $("adcOffset").value = adcOffset;
  $("adcGainVal").textContent = Number(adcGain).toFixed(4);
  $("adcOffsetVal").textContent = Number(adcOffset).toFixed(3);
  ensureCfgAdcCurveY();
  renderAdcBpTable();
  updateFitExpectedTables();
  updateSensorQuickCheckTable();
  $("serialDebug").value = cfg.sdbg ?? DEFAULTS.serial_debug_enable;
  $("serialPeriod").value = cfg.sper ?? DEFAULTS.serial_status_period_ms;
}

function readCurveAt(kpa){
  const pts=cfg.curve;
  if(kpa < pts[0].kpa) return 0;
  for(let i=0;i<pts.length-1;i++){
    const a=pts[i], b=pts[i+1];
    if(kpa<=b.kpa){
      const t=(kpa-a.kpa)/Math.max(1e-6,(b.kpa-a.kpa));
      return a.pct + t*(b.pct-a.pct);
    }
  }
  return pts[pts.length-1].pct;
}

function renderTable(){
  if(!cfg) return;
  readInjModel();
  const tb=$("curveTable").querySelector("tbody");
  tb.innerHTML="";
  const dpPref = Number($("targetInjectorDp").value)||0;
  const dp = dpPref>0 ? dpPref : (status && isFinite(status.dpPsi) ? status.dpPsi : REF_DP);
  const clamp = Number($("dutyClamp").value)||100;
  cfg.curve.forEach((p,i)=>{
    const isClamped = p.pct > clamp;
    const duty = Math.min(p.pct, clamp);
    const f=calcFlowFromDuty(duty, dp);
    const tr=document.createElement("tr");
    const clampTag = isClamped ? `<span class="badge warn" style="margin-left:6px">CLAMP</span>` : "";
    tr.innerHTML=`<td style="padding:4px 6px;border-bottom:1px solid var(--border)">${p.kpa}</td>
                  <td style="padding:4px 6px;border-bottom:1px solid var(--border)">${p.pct}${clampTag}</td>
                  <td style="padding:4px 6px;border-bottom:1px solid var(--border)">${fmt(f.gph,2)}</td>`;
    tb.appendChild(tr);
  });
}

let cfg=null, status=null;
let pollBusy=false;
let dragPt=-1;
let dragMinX=0, dragMaxX=0;
let hoverKpa=undefined;
let forceBoostCut=false;
let forceTimingCut=false;
let forceDpFault=false;
let forcePressureReadyFault=false;
let forceDpBoostHold=false;
let forcePressureReadyBoostHold=false;
let forceDpMonitorOverride=false;
let forcePressureReadyOverride=false;
let forceLevelFaultBypass=false;
let forceDpFaultBypass=false;
let forceDpBoostHoldBypass=false;
let forcePressureReadyFaultBypass=false;
let forcePressureReadyBoostHoldBypass=false;
let ws=null;
let wsConnected=false;
let wsRetryMs=500;
let lastWsMsgMs=0;
let calGaugeMapFilt=null;
let calGaugeRailFilt=null;
let calGaugeLastMs=0;
const CAL_GAUGE_TAU_MS = 350;
const wsBadgeOn = ()=>{ const b=$("wsBadge"); if(b){ b.textContent="WS: ON"; b.className="badge ok"; } };
const wsBadgeOff = ()=>{ const b=$("wsBadge"); if(b){ b.textContent="WS: OFF"; b.className="badge warn"; } };
const wsBadgeStale = ()=>{ const b=$("wsBadge"); if(b){ b.textContent="WS: STALE"; b.className="badge warn"; } };
const WS_STALE_MS = 2000;
const WS_OFF_MS = 8000;
function updateWsBadge(){
  if(!wsConnected){
    wsBadgeOff();
    return;
  }
  const age = Date.now() - (lastWsMsgMs || 0);
  if(age > WS_OFF_MS){
    wsConnected = false;
    wsBadgeOff();
    try{ if(ws) ws.close(); }catch(e){}
    ws = null;
    return;
  }
  if(age > WS_STALE_MS){
    wsBadgeStale();
  }else{
    wsBadgeOn();
  }
}

function updateLatchButtons(){
  const bBoost = $("btnBoostCut");
  const bTiming = $("btnTimingCut");
  const bDpFault = $("btnDpFault");
  const bPressureReadyFault = $("btnPressureReadyFault");
  const bDpBoostHold = $("btnDpBoostHold");
  const bPressureReadyBoostHold = $("btnPressureReadyBoostHold");
  const bDpMonOverride = $("btnDpMonOverride");
  const bPressureReadyOverride = $("btnPressureReadyOverride");
  const bLevelBypass = $("btnLevelBypass");
  const bDpFaultBypass = $("btnDpFaultBypass");
  const bDpHoldBypass = $("btnDpHoldBypass");
  const bPressureReadyFaultBypass = $("btnPressureReadyFaultBypass");
  const bPressureReadyHoldBypass = $("btnPressureReadyHoldBypass");
  const boostActive = !!(forceBoostCut || (status && (
    status.boostOn === false ||
    status.safetyState === "Boost Cut" ||
    status.faultReason === "Low Level" ||
    status.safetyReason === "Low Level"
  )));
  const timingActive = !!(forceTimingCut || (status && (
    status.iatOn ||
    status.safetyState === "Timing Cut" ||
    status.faultReason === "Low Level (Timing Cut)" ||
    status.safetyReason === "Low Level (Timing Cut)"
  )));
  const dpFaultActive = !!(forceDpFault || (status && status.dpFault));
  const pressureReadyFaultActive = !!(forcePressureReadyFault || (status && status.pressureReadyFault));
  const dpBoostHoldActive = !!(forceDpBoostHold || (status && status.dpBoostHold));
  const pressureReadyBoostHoldActive = !!(forcePressureReadyBoostHold || (status && status.pressureReadyBoostHold));
  const dpMonOverrideActive = !!(forceDpMonitorOverride || (status && status.dpMonitorOverride));
  const pressureReadyOverrideActive = !!(forcePressureReadyOverride || (status && status.pressureReadyOverride));
  const levelBypassActive = !!(forceLevelFaultBypass || (status && status.levelFaultBypass));
  const dpFaultBypassActive = !!(forceDpFaultBypass || (status && status.dpFaultBypass));
  const dpHoldBypassActive = !!(forceDpBoostHoldBypass || (status && status.dpBoostHoldBypass));
  const pressureReadyFaultBypassActive = !!(forcePressureReadyFaultBypass || (status && status.pressureReadyFaultBypass));
  const pressureReadyHoldBypassActive = !!(forcePressureReadyBoostHoldBypass || (status && status.pressureReadyBoostHoldBypass));
  if(bBoost){
    bBoost.classList.toggle("active", !!boostActive);
    bBoost.textContent = `Manual Boost Cut: ${forceBoostCut ? "ON" : "OFF"}`;
  }
  if(bTiming){
    bTiming.classList.toggle("active", !!timingActive);
    bTiming.textContent = `Manual Timing Cut: ${forceTimingCut ? "ON" : "OFF"}`;
  }
  if(bDpFault){
    bDpFault.classList.toggle("active", !!dpFaultActive);
    bDpFault.textContent = `Rail dP Fault: ${forceDpFault ? "ON" : "OFF"}`;
  }
  if(bPressureReadyFault){
    bPressureReadyFault.classList.toggle("active", !!pressureReadyFaultActive);
    bPressureReadyFault.textContent = `Pressure-Ready Fault: ${forcePressureReadyFault ? "ON" : "OFF"}`;
  }
  if(bDpBoostHold){
    bDpBoostHold.classList.toggle("active", !!dpBoostHoldActive);
    bDpBoostHold.textContent = `Rail dP Boost Hold: ${forceDpBoostHold ? "ON" : "OFF"}`;
  }
  if(bPressureReadyBoostHold){
    bPressureReadyBoostHold.classList.toggle("active", !!pressureReadyBoostHoldActive);
    bPressureReadyBoostHold.textContent = `Pressure-Ready Boost Hold: ${forcePressureReadyBoostHold ? "ON" : "OFF"}`;
  }
  if(bDpMonOverride){
    bDpMonOverride.classList.toggle("active", !!dpMonOverrideActive);
    bDpMonOverride.textContent = `dP Monitor Override: ${forceDpMonitorOverride ? "ON" : "OFF"}`;
  }
  if(bPressureReadyOverride){
    bPressureReadyOverride.classList.toggle("active", !!pressureReadyOverrideActive);
    bPressureReadyOverride.textContent = `Pressure-Ready Override: ${forcePressureReadyOverride ? "ON" : "OFF"}`;
  }
  if(bLevelBypass){
    bLevelBypass.classList.toggle("active", !!levelBypassActive);
    bLevelBypass.textContent = `Level Fault Bypass: ${forceLevelFaultBypass ? "ON" : "OFF"}`;
  }
  if(bDpFaultBypass){
    bDpFaultBypass.classList.toggle("active", !!dpFaultBypassActive);
    bDpFaultBypass.textContent = `dP Fault Bypass: ${forceDpFaultBypass ? "ON" : "OFF"}`;
  }
  if(bDpHoldBypass){
    bDpHoldBypass.classList.toggle("active", !!dpHoldBypassActive);
    bDpHoldBypass.textContent = `dP Hold Bypass: ${forceDpBoostHoldBypass ? "ON" : "OFF"}`;
  }
  if(bPressureReadyFaultBypass){
    bPressureReadyFaultBypass.classList.toggle("active", !!pressureReadyFaultBypassActive);
    bPressureReadyFaultBypass.textContent = `Pressure-Ready Fault Bypass: ${forcePressureReadyFaultBypass ? "ON" : "OFF"}`;
  }
  if(bPressureReadyHoldBypass){
    bPressureReadyHoldBypass.classList.toggle("active", !!pressureReadyHoldBypassActive);
    bPressureReadyHoldBypass.textContent = `Pressure-Ready Hold Bypass: ${forcePressureReadyBoostHoldBypass ? "ON" : "OFF"}`;
  }
}


async function toggleBoostCut(){
  forceBoostCut = !forceBoostCut;
  await apiPost("/api/config", {forceBoostCut, forceTimingCut});
  if(cfg){ cfg.forceBoostCut = forceBoostCut; }
  updateLatchButtons();
  await poll();
}
async function toggleTimingCut(){
  forceTimingCut = !forceTimingCut;
  await apiPost("/api/config", {forceBoostCut, forceTimingCut});
  if(cfg){ cfg.forceTimingCut = forceTimingCut; }
  updateLatchButtons();
  await poll();
}
async function toggleDpFault(){
  forceDpFault = !forceDpFault;
  await apiPost("/api/config", {forceDpFault});
  if(cfg){ cfg.forceDpFault = forceDpFault; }
  updateLatchButtons();
  await poll();
}
async function togglePressureReadyFault(){
  forcePressureReadyFault = !forcePressureReadyFault;
  await apiPost("/api/config", {forcePressureReadyFault});
  if(cfg){ cfg.forcePressureReadyFault = forcePressureReadyFault; }
  updateLatchButtons();
  await poll();
}
async function toggleDpBoostHold(){
  forceDpBoostHold = !forceDpBoostHold;
  await apiPost("/api/config", {forceDpBoostHold});
  if(cfg){ cfg.forceDpBoostHold = forceDpBoostHold; }
  updateLatchButtons();
  await poll();
}
async function togglePressureReadyBoostHold(){
  forcePressureReadyBoostHold = !forcePressureReadyBoostHold;
  await apiPost("/api/config", {forcePressureReadyBoostHold});
  if(cfg){ cfg.forcePressureReadyBoostHold = forcePressureReadyBoostHold; }
  updateLatchButtons();
  await poll();
}
async function toggleDpMonOverride(){
  forceDpMonitorOverride = !forceDpMonitorOverride;
  await apiPost("/api/config", {forceDpMonitorOverride});
  if(cfg){ cfg.forceDpMonitorOverride = forceDpMonitorOverride; }
  updateLatchButtons();
  await poll();
}
async function togglePressureReadyOverride(){
  forcePressureReadyOverride = !forcePressureReadyOverride;
  await apiPost("/api/config", {forcePressureReadyOverride});
  if(cfg){ cfg.forcePressureReadyOverride = forcePressureReadyOverride; }
  updateLatchButtons();
  await poll();
}
async function toggleLevelBypass(){
  forceLevelFaultBypass = !forceLevelFaultBypass;
  await apiPost("/api/config", {forceLevelFaultBypass});
  if(cfg){ cfg.forceLevelFaultBypass = forceLevelFaultBypass; }
  updateLatchButtons();
  await poll();
}
async function toggleDpFaultBypass(){
  forceDpFaultBypass = !forceDpFaultBypass;
  await apiPost("/api/config", {forceDpFaultBypass});
  if(cfg){ cfg.forceDpFaultBypass = forceDpFaultBypass; }
  updateLatchButtons();
  await poll();
}
async function toggleDpHoldBypass(){
  forceDpBoostHoldBypass = !forceDpBoostHoldBypass;
  await apiPost("/api/config", {forceDpBoostHoldBypass});
  if(cfg){ cfg.forceDpBoostHoldBypass = forceDpBoostHoldBypass; }
  updateLatchButtons();
  await poll();
}
async function togglePressureReadyFaultBypass(){
  forcePressureReadyFaultBypass = !forcePressureReadyFaultBypass;
  await apiPost("/api/config", {forcePressureReadyFaultBypass});
  if(cfg){ cfg.forcePressureReadyFaultBypass = forcePressureReadyFaultBypass; }
  updateLatchButtons();
  await poll();
}
async function togglePressureReadyHoldBypass(){
  forcePressureReadyBoostHoldBypass = !forcePressureReadyBoostHoldBypass;
  await apiPost("/api/config", {forcePressureReadyBoostHoldBypass});
  if(cfg){ cfg.forcePressureReadyBoostHoldBypass = forcePressureReadyBoostHoldBypass; }
  updateLatchButtons();
  await poll();
}


function drawCurve(){
  const c=$("curve");
  const ctx=c.getContext("2d");
  const w=c.width=c.clientWidth*2;
  const h=c.height=c.clientHeight*2;

  ctx.clearRect(0,0,w,h);
  ctx.lineWidth=3;

  const pad=70;
  ctx.strokeStyle="#262b33";
  ctx.beginPath();
  ctx.moveTo(pad,h-pad); ctx.lineTo(w-pad,h-pad); // x
  ctx.moveTo(pad,h-pad); ctx.lineTo(pad,pad);     // y
  ctx.stroke();

  ctx.fillStyle="#9aa1ae";
  ctx.font="24px system-ui";
  ctx.fillText("MAP kPa", w/2-50, h-18);
  ctx.save();
  ctx.translate(22,h/2+50);
  ctx.rotate(-Math.PI/2);
  ctx.fillText("Duty %",0,0);
  ctx.restore();

  if(!cfg) return;
  const pts=cfg.curve;

  const minX=pts[0].kpa, maxX=pts[pts.length-1].kpa;
  const clampVal = Number($("dutyClamp").value)||100;
  const minY=0, maxY=100;

  function x(kpa){return pad + (kpa-minX)/(maxX-minX||1)*(w-2*pad);}
  function y(pct){return (h-pad) - (pct-minY)/(maxY-minY||1)*(h-2*pad);}

  ctx.lineWidth=1;
  ctx.strokeStyle="#171b22";
  for(let i=0;i<=5;i++){
    const yy=pad + i*(h-2*pad)/5;
    ctx.beginPath(); ctx.moveTo(pad,yy); ctx.lineTo(w-pad,yy); ctx.stroke();
    const val = Math.round(100 - i*20);
    ctx.fillStyle="#55606e"; ctx.font="18px system-ui";
    ctx.fillText(val.toString(), pad-42, yy+6);
  }
  for(let i=0;i<=5;i++){
    const xx=pad + i*(w-2*pad)/5;
    ctx.beginPath(); ctx.moveTo(xx,pad); ctx.lineTo(xx,h-pad); ctx.stroke();
    const val = Math.round((minX + (maxX-minX)*(i/5)) * 10) / 10;
    ctx.fillStyle="#55606e"; ctx.font="18px system-ui";
    ctx.fillText(val.toString(), xx-14, h-pad+32);
  }

  const clampY = y(Math.min(100, Math.max(0, clampVal)));
  ctx.strokeStyle="#ffb300"; ctx.setLineDash([8,6]); ctx.lineWidth=2;
  ctx.beginPath(); ctx.moveTo(pad, clampY); ctx.lineTo(w-pad, clampY); ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle="#ffb300"; ctx.font="18px system-ui";
  ctx.fillText(`Clamp ${Math.round(clampVal)}%`, w-pad-120, clampY-6);

  ctx.lineWidth=4;
  ctx.strokeStyle="#00e676";
  ctx.beginPath();
  ctx.moveTo(x(pts[0].kpa),y(Math.min(pts[0].pct, clampVal)));
  for(let i=1;i<pts.length;i++) ctx.lineTo(x(pts[i].kpa),y(Math.min(pts[i].pct, clampVal)));
  ctx.stroke();

  for(let i=0;i<pts.length;i++){
    ctx.fillStyle="#00c853";
    ctx.beginPath(); ctx.arc(x(pts[i].kpa),y(Math.min(pts[i].pct, clampVal)),10,0,Math.PI*2); ctx.fill();
    ctx.strokeStyle="#0b0d10"; ctx.lineWidth=3;
    ctx.stroke();
  }

  if(status){
    const mkpa = (status.mapKpa!==undefined && status.mapKpa !== null) ? status.mapKpa : (status.mapKpaRaw||0);
    const cp=Math.min(readCurveAt(mkpa), clampVal); // honor clamp
    ctx.strokeStyle="#ffb300"; ctx.lineWidth=3;
    ctx.beginPath(); ctx.arc(x(mkpa),y(cp),12,0,Math.PI*2); ctx.stroke();
  }

  if(window.hoverKpa!==undefined){
    const hk=hoverKpa;
    const hv=Math.min(readCurveAt(hk), clampVal);
    ctx.strokeStyle="#00c853"; ctx.lineWidth=2;
    ctx.beginPath(); ctx.moveTo(x(hk),pad); ctx.lineTo(x(hk),h-pad); ctx.stroke();
    ctx.beginPath(); ctx.arc(x(hk),y(hv),10,0,Math.PI*2); ctx.stroke();
    const text = `${fmt(hk,1)} kPa | ${fmt(hv,1)} % (clamp ${fmt(clampVal,0)}%)`;
    ctx.font="20px system-ui";
    const tw = ctx.measureText(text).width;
    const padBox = 8;
    const bx = x(hk) - tw/2 - padBox;
    const by = y(hv) - 34;
    const bw = tw + padBox*2;
    const bh = 26 + padBox;
    ctx.fillStyle="rgba(0,0,0,0.8)";
    ctx.strokeStyle="#00c853";
    ctx.lineWidth=2;
    roundRect(ctx, bx, by, bw, bh, 8, true, true);
    ctx.fillStyle="#00e676";
    ctx.fillText(text, bx+padBox, by+padBox+16);
  }
}

function roundRect(ctx, x, y, w, h, r, fill, stroke){
  if(w<2*r) r=w/2;
  if(h<2*r) r=h/2;
  ctx.beginPath();
  ctx.moveTo(x+r, y);
  ctx.arcTo(x+w, y,   x+w, y+h, r);
  ctx.arcTo(x+w, y+h, x,   y+h, r);
  ctx.arcTo(x,   y+h, x,   y,   r);
  ctx.arcTo(x,   y,   x+w, y,   r);
  ctx.closePath();
  if(fill) ctx.fill();
  if(stroke) ctx.stroke();
}

function canvasToPoint(ev){
  const c=$("curve"); const r=c.getBoundingClientRect();
  const px=(ev.clientX-r.left)/r.width;
  const py=(ev.clientY-r.top)/r.height;
  return {px,py};
}

function hitTest(ev){
  const c=$("curve"); const r=c.getBoundingClientRect();
  const pts=cfg.curve;
  const minX=pts[0].kpa, maxX=pts[pts.length-1].kpa;

  const w=r.width*2, h=r.height*2;
  const pad=70;

  const mx=((ev.clientX-r.left)/r.width)*w;
  const my=((ev.clientY-r.top)/r.height)*h;

  function px(kpa){return pad + (kpa-minX)/(maxX-minX||1)*(w-2*pad);}
  function py(pct){return (h-pad) - (pct/100)*(h-2*pad);}

  let bestI=-1, bestD2=1e18;
  for(let i=0;i<pts.length;i++){
    const dx=px(pts[i].kpa)-mx;
    const dy=py(pts[i].pct)-my;
    const d2=dx*dx + dy*dy;
    if(d2<bestD2){bestD2=d2; bestI=i;}
  }

  const hitR=28; // internal px (canvas is 2x)
  return (bestD2 < hitR*hitR) ? bestI : -1;
}

function onDown(ev){
  if(!cfg) return;
  const i=hitTest(ev);
  if(i>=0){
    dragPt=i;
    dragMinX=cfg.curve[0].kpa;
    dragMaxX=cfg.curve[cfg.curve.length-1].kpa;
    try { $("curve").setPointerCapture(ev.pointerId); } catch(e){}
    ev.preventDefault();
  }
}
function onMove(ev){
  if(dragPt<0||!cfg) return;
  const c=$("curve"); const r=c.getBoundingClientRect();
  const w=r.width*2, h=r.height*2;
  const pad=70;

  const mx=((ev.clientX-r.left)/r.width)*w;
  const my=((ev.clientY-r.top)/r.height)*h;

  let pct=((h-pad)-my)/(h-2*pad||1)*100;
  pct=Math.max(0,Math.min(100,pct));

  let kpa=dragMinX + ((mx-pad)/(w-2*pad||1))*(dragMaxX-dragMinX);
  kpa=Math.max(dragMinX,Math.min(dragMaxX,kpa));

  const pts=cfg.curve;
  const minSpacing=1;
  const prevK = (dragPt>0) ? (pts[dragPt-1].kpa + minSpacing) : -1e9;
  const nextK = (dragPt<pts.length-1) ? (pts[dragPt+1].kpa - minSpacing) : 1e9;
  kpa=Math.max(prevK,Math.min(nextK,kpa));

  pts[dragPt].kpa=Math.round(kpa);
  pts[dragPt].pct=Math.round(pct);
  $("curveStart").value = pts[0].kpa;
  $("curveMax").value = pts[pts.length-1].kpa;
  drawCurve();
  renderTable();
  ev.preventDefault();
}
function onUp(ev){
  if(dragPt>=0){
    try { $("curve").releasePointerCapture(ev.pointerId); } catch(e){}
  }
  dragPt=-1;
  clearHover();
}

function onHover(ev){
  if(!cfg) return;
  const c=$("curve"); const r=c.getBoundingClientRect();
  const w=r.width*2, h=r.height*2, pad=70;
  const mx=((ev.clientX-r.left)/r.width)*w;
  const pts=cfg.curve;
  const minX=pts[0].kpa, maxX=pts[pts.length-1].kpa;
  const kpa=minX + (Math.max(0,Math.min(w-2*pad,mx-pad))/(w-2*pad||1))*(maxX-minX||1);
  hoverKpa=kpa;
  const hv=readCurveAt(kpa);
  $("hoverLine").style.display="block";
  $("hoverLine").textContent=`Hover: ${fmt(kpa,1)} kPa -> ${fmt(hv,1)} %`;
  drawCurve();
}

function clearHover(){
  hoverKpa=undefined;
  $("hoverLine").textContent="";
  $("hoverLine").style.display="none";
  drawCurve();
}

async function loadCfg(){
  cfg=await apiGet("/api/config");
  populateCfg();
  drawCurve();
  renderTable();
}

function smoothCalGauge(prev, raw, alpha){
  if(prev===null || !isFinite(prev) || !isFinite(raw)) return raw;
  return prev + ((raw - prev) * alpha);
}

function applyStatus(s){
  if(!s) return;
  status = s;
  const mapDisp = (status.mapKpa!==undefined && status.mapKpa !== null) ? status.mapKpa : (status.mapKpaRaw||0);
  const mapPsiNow = (Number(mapDisp)||0) * 0.1450377377;
  const baroPsiNow = Number(status.baroPsi)||14.6959;
  setText("mapKpa", Math.round(mapDisp));
  setText("mapPsi", fmt(mapPsiNow,1));
  setText("baroKpa", fmt((Number(status.baroKpa)||101.325),1));
  setText("baroPsi", fmt(baroPsiNow,2));
  setText("boostPsiKv", fmt(mapPsiNow - baroPsiNow,1));
  setText("mapKpa2", Math.round(mapDisp));
  setText("railPsi", fmt(status.railPsi,1));
  setText("dpPsi", fmt(status.dpPsi,1));
  setText("sprayPct", Math.round(status.sprayPct||0));
  setText("curvePct", Math.round(status.curvePct||0));
  setText("clampPct", Math.round(status.dutyClamp||0));
  const nowMs = Date.now();
  const dtMs = (calGaugeLastMs > 0) ? Math.min(2000, Math.max(1, nowMs - calGaugeLastMs)) : 0;
  calGaugeLastMs = nowMs;
  const alpha = (dtMs > 0) ? (1.0 - Math.exp(-dtMs / CAL_GAUGE_TAU_MS)) : 1.0;
  const calMapRaw = Number(mapDisp);
  const calRailRaw = Number(status.railPsi);
  calGaugeMapFilt = smoothCalGauge(calGaugeMapFilt, calMapRaw, alpha);
  calGaugeRailFilt = smoothCalGauge(calGaugeRailFilt, calRailRaw, alpha);
  setTextByClass("calMapKpa2dCopy", fmt(calGaugeMapFilt,2));
  setTextByClass("calRailPsi2dCopy", fmt(calGaugeRailFilt,2));
  updateDutyClampFlow();
  updateLatchButtons();
  const currentFault = (status.currentFaultStatus && status.currentFaultStatus !== "None")
    ? status.currentFaultStatus
    : ((status.faultReason && status.faultReason !== "None")
      ? status.faultReason
      : ((status.safetyReason && status.safetyReason !== "none") ? status.safetyReason : "None"));
  setText("faultKv", currentFault === "None" ? "OK" : currentFault);

  setText("stPump", status.pumpOn ? "ON" : "OFF");
  setText("stInj", status.injOn ? "ON" : "OFF");
  setText("stSpray", status.sprayActive ? "ACTIVE" : "INACTIVE");
  setText("stLevel", status.levelOk ? "OK" : "LOW");
  setText("stRedLed", status.redLedOn ? "ON" : "OFF");
  setText("stBlueLed", status.blueLedOn ? "ON" : "OFF");
  setText("stGreenLed", status.greenLedOn ? "ON" : "OFF");
  setText("stBoost", status.boostOn ? "OFF" : "ON");
  setText("stSafety", status.safetyState || (status.levelLow ? "Boost Cut" : "OK"));
  setText("stIat", status.iatOn ? "ON" : "OFF");
  setText("stIatSsr", status.iatOn ? "ON" : "OFF");
  setText("stLvlRaw", status.levelRaw ? "LOW(raw)" : "OK(raw)");
  setText("stPressureReady", status.pressureReady ? "READY" : "BUILDING");
  setText("stLevelFault", status.levelFault ? "LOW" : "OK");
  setText("stDpFault", status.dpFault ? "ACTIVE" : "OK");
  setText("stDpFaultType", status.railDpStateDetail || "NONE");
  setText("stPressureReadyFault", status.pressureReadyFault ? "ACTIVE" : "OK");
  setText("stDpPending", status.dpPending ? "YES" : "NO");
  setText("stDpState", status.dpState || (status.dpFault ? "TIMING_CUT" : (status.dpBoostHold ? "BOOST_HOLD" : (status.dpPending ? "PENDING" : "IDLE"))));
  setText("stDpOverride", status.dpMonitorOverride ? "ON" : "OFF");
  setText("stPressureReadyOverride", status.pressureReadyOverride ? "ON" : "OFF");
  setText("stLevelBypass", status.levelFaultBypass ? "ON" : "OFF");
  setText("stDpFaultBypass", status.dpFaultBypass ? "ON" : "OFF");
  setText("stPressureReadyFaultBypass", status.pressureReadyFaultBypass ? "ON" : "OFF");
  setText("stDpHoldBypass", status.dpBoostHoldBypass ? "ON" : "OFF");
  setText("stPressureReadyHoldBypass", status.pressureReadyBoostHoldBypass ? "ON" : "OFF");
  setText("stDpLowMs", Math.round(status.dpLowMs||0));
  setText("stDpMinSeen", (status.dpMinSeen!==undefined && status.dpMinSeen!==null && isFinite(status.dpMinSeen) && status.dpMinSeen > -0.5) ? fmt(status.dpMinSeen,1) : "-");
  setText("stDpArmed", status.dpArmed ? "YES" : "NO");
  setText("stDpSettle", `${Math.round(status.dpArmSettleRemainingMs||0)} ms`);
  setText("stReason", status.faultHistory || "None");
  setText("stCurrentFault", currentFault);

  setText("netLine", status.netLine || "");

  drawCurve();
  renderTable();
}
async function poll(){
  if(pollBusy) return;
  updateWsBadge();
  if(wsConnected && (Date.now() - lastWsMsgMs) < 1000) return;
  pollBusy=true;
  try{
    const s=await apiGet("/api/status");
    applyStatus(s);
  } catch(e){
  } finally {
    pollBusy=false;
  }
}

function wsConnect(){
  try{
    const host = location.hostname;
    ws = new WebSocket(`ws://${host}:81/`);
  } catch(e){
    ws = null;
    wsConnected = false;
    wsBadgeOff();
    setTimeout(wsConnect, wsRetryMs);
    return;
  }
  ws.onopen = ()=>{
    wsConnected = true;
    wsRetryMs = 500;
    lastWsMsgMs = Date.now();
    updateWsBadge();
  };
  ws.onmessage = (ev)=>{
    try{
      const s = JSON.parse(ev.data);
      lastWsMsgMs = Date.now();
      updateWsBadge();
      applyStatus(s);
    } catch(e){
    }
  };
  ws.onclose = ()=>{
    wsConnected = false;
    ws = null;
    lastWsMsgMs = 0;
    wsRetryMs = Math.min(5000, wsRetryMs * 2);
    wsBadgeOff();
    setTimeout(wsConnect, wsRetryMs);
  };
  ws.onerror = ()=>{
    try{ ws.close(); }catch(e){}
  };
}

async function saveAll(){
  const injHzVal = Number($("injHz").value);
  if(isFinite(injHzVal)) cfg.injHz = injHzVal;
  const dutyClampVal = Number($("dutyClamp").value);
  if(isFinite(dutyClampVal)) cfg.dutyClamp = dutyClampVal;
  const dpMinVal = Number($("dpMinPsi").value);
  if(isFinite(dpMinVal)) cfg.dpMinPsi = dpMinVal;
  const dpFaultMsVal = Number($("dpFaultMs").value);
  if(isFinite(dpFaultMsVal)) cfg.dpFaultMs = dpFaultMsVal;
  const dpArmPctVal = Number($("dpArmPct").value);
  if(isFinite(dpArmPctVal)) cfg.dpArmPct = dpArmPctVal;
  const dpRecoverVal = Number($("dpRecover").value);
  if(isFinite(dpRecoverVal)) cfg.dpRecover = dpRecoverVal;
  const dpCriticalPsiVal = Number($("dpCriticalPsi").value);
  if(isFinite(dpCriticalPsiVal)) cfg.dpCriticalPsi = dpCriticalPsiVal;
  const dpCriticalMsVal = Number($("dpCriticalMs").value);
  if(isFinite(dpCriticalMsVal)) cfg.dpCriticalMs = dpCriticalMsVal;
  const dpArmSettleMsVal = Number($("dpArmSettleMs").value);
  if(isFinite(dpArmSettleMsVal)) cfg.dpArmSettleMs = dpArmSettleMsVal;
  const tcAuto = Number($("timingCutAutoMs").value);
  cfg.timingCutAutoMs = isFinite(tcAuto) ? Math.max(0, tcAuto) : DEFAULTS.timing_cut_auto_clear_ms;
  const pressureReadyTimeoutVal = Number($("pressureReadyTimeoutMs").value);
  cfg.pressureReadyTimeoutMs = isFinite(pressureReadyTimeoutVal) ? pressureReadyTimeoutVal : DEFAULTS.pressure_ready_timeout_ms;
  const lvlDebounceVal = Number($("lvlDebounce").value);
  cfg.lvlDebounce = isFinite(lvlDebounceVal) ? lvlDebounceVal : DEFAULTS.level_debounce_ms;
  const injLbVal = Number($("injLb").value);
  if(isFinite(injLbVal)) cfg.injLb = injLbVal;
  const mixMethPctVal = Number($("mixMethPct").value);
  if(isFinite(mixMethPctVal)) cfg.mixMethPct = mixMethPctVal;
  const targetInjectorDpVal = Number($("targetInjectorDp").value);
  if(isFinite(targetInjectorDpVal)) cfg.targetInjectorDp = targetInjectorDpVal;

  const cStart = Number($("curveStart").value);
  const cMax = Number($("curveMax").value);
  if(isFinite(cStart) && isFinite(cMax) && cMax > cStart){
    const span = cMax - cStart;
    const pts = cfg.curve;
    for(let i=0;i<pts.length;i++){
      pts[i].kpa = Math.round(cStart + (span * i)/(pts.length-1));
    }
  }

  const mapLinVal = Number($("mapLin").value);
  if(isFinite(mapLinVal)) cfg.mapLin = mapLinVal;
  const mapOffVal = Number($("mapOff").value);
  if(isFinite(mapOffVal)) cfg.mapOff = mapOffVal;
  const pLinVal = Number($("pLin").value);
  if(isFinite(pLinVal)) cfg.pLin = pLinVal;
  const pOffVal = Number($("pOff").value);
  if(isFinite(pOffVal)) cfg.pOff = pOffVal;

  const wifiModeVal = Number($("wifiMode").value);
  cfg.wifiMode = isFinite(wifiModeVal) ? wifiModeVal : 0;
  cfg.apSsid = $("apSsid").value||"";
  cfg.mdnsHost = $("mdnsHost").value||"";
  cfg.staSsid = $("staSsid").value||"";
  cfg.staPass = $("staPass").value||"";

  const forcePumpVal = Number($("forcePump").value);
  cfg.forcePump = isFinite(forcePumpVal) ? forcePumpVal : 0;
  const forceInjVal = Number($("forceInj").value);
  cfg.forceInj = isFinite(forceInjVal) ? forceInjVal : 0;
  const forceDutyVal = Number($("forceDuty").value);
  cfg.forceDuty = isFinite(forceDutyVal) ? forceDutyVal : 0;
  const fm = $("forceMapKpa").value;
  const fr = $("forceRailPsi").value;
  const fb = $("forceBaroKpa").value;
  const fd = $("forceDpPsi").value;
  cfg.forceMapKpa = fm==="" ? -1 : Number(fm);
  cfg.forceRailPsi = fr==="" ? -1 : Number(fr);
  cfg.forceBaroKpa = fb==="" ? -1 : Number(fb);
  cfg.forceDpPsi = fd==="" ? -1 : Number(fd);
  const forceLevelVal = Number($("forceLevel").value);
  cfg.forceLevel = isFinite(forceLevelVal) ? forceLevelVal : 0;
  cfg.forceBoostCut = !!forceBoostCut;
  cfg.forceTimingCut = !!forceTimingCut;
  cfg.forceDpFault = !!forceDpFault;
  cfg.forcePressureReadyFault = !!forcePressureReadyFault;
  cfg.forceDpBoostHold = !!forceDpBoostHold;
  cfg.forcePressureReadyBoostHold = !!forcePressureReadyBoostHold;
  cfg.forceDpMonitorOverride = !!forceDpMonitorOverride;
  cfg.forcePressureReadyOverride = !!forcePressureReadyOverride;
  cfg.forceLevelFaultBypass = !!forceLevelFaultBypass;
  cfg.forceDpFaultBypass = !!forceDpFaultBypass;
  cfg.forceDpBoostHoldBypass = !!forceDpBoostHoldBypass;
  cfg.forcePressureReadyFaultBypass = !!forcePressureReadyFaultBypass;
  cfg.forcePressureReadyBoostHoldBypass = !!forcePressureReadyBoostHoldBypass;

  const mapDiv = Number($("mapDivRatio").value);
  if(isFinite(mapDiv)) cfg.mapDiv = mapDiv;
  const railDiv = Number($("railDivRatio").value);
  if(isFinite(railDiv)) cfg.railDiv = railDiv;
  const adcGain = Number($("adcGain").value);
  if(isFinite(adcGain)) cfg.adcGain = adcGain;
  const adcOffset = Number($("adcOffset").value);
  if(isFinite(adcOffset)) cfg.adcOffset = adcOffset;
  cfg.adcCurveY = ensureCfgAdcCurveY();
  const sdbgVal = Number($("serialDebug").value);
  cfg.sdbg = isFinite(sdbgVal) ? sdbgVal : DEFAULTS.serial_debug_enable;
  const sper = Number($("serialPeriod").value);
  if(isFinite(sper)) cfg.sper = Math.max(0, Math.round(sper));

  await apiPost("/api/config", cfg);
  await loadCfg();
}

function spreadCurve(){
  if(!cfg) return;
  let cStart = Number($("curveStart").value);
  let cMax = Number($("curveMax").value);
  if(!isFinite(cStart) || !isFinite(cMax) || cMax <= cStart){
    cStart = cfg.curve[0].kpa;
    cMax = cfg.curve[cfg.curve.length-1].kpa;
    $("curveStart").value = cStart;
    $("curveMax").value = cMax;
  }
  if(cMax > cStart){
    const span = cMax - cStart;
    const pts = cfg.curve;
    const y0 = pts[0].pct;
    const yN = pts[pts.length-1].pct;
    for(let i=0;i<pts.length;i++){
      pts[i].kpa = Math.round(cStart + (span * i)/(pts.length-1));
      pts[i].pct = Math.round(y0 + (yN - y0) * (i/(pts.length-1)));
    }
  }
  drawCurve();
  renderTable();
  updateDutyClampFlow();
}

async function unforceAll(){
  $("forcePump").value = 0;
  $("forceInj").value = 0;
  $("forceDuty").value = 0;
  $("forceMapKpa").value = "";
  $("forceRailPsi").value = "";
  $("forceBaroKpa").value = "";
  $("forceDpPsi").value = "";
  $("forceLevel").value = 0;
  forceBoostCut = false;
  forceTimingCut = false;
  forceDpFault = false;
  forcePressureReadyFault = false;
  forceDpBoostHold = false;
  forcePressureReadyBoostHold = false;
  forceDpMonitorOverride = false;
  forcePressureReadyOverride = false;
  forceLevelFaultBypass = false;
  forceDpFaultBypass = false;
  forceDpBoostHoldBypass = false;
  forcePressureReadyFaultBypass = false;
  forcePressureReadyBoostHoldBypass = false;
  updateLatchButtons();
  try{ await apiPost("/api/config", {
    forceBoostCut:false,
    forceTimingCut:false,
    forceDpFault:false,
    forcePressureReadyFault:false,
    forceDpBoostHold:false,
    forcePressureReadyBoostHold:false,
    forceDpMonitorOverride:false,
    forcePressureReadyOverride:false,
    forceLevelFaultBypass:false,
    forceDpFaultBypass:false,
    forceDpBoostHoldBypass:false,
    forcePressureReadyFaultBypass:false,
    forcePressureReadyBoostHoldBypass:false
  }); }catch(e){}
  await applyTestForces();
}

async function applyTestForces(){
  const fm = $("forceMapKpa").value;
  const fr = $("forceRailPsi").value;
  const fb = $("forceBaroKpa").value;
  const fd = $("forceDpPsi").value;
  const payload = {
    forcePump: Number($("forcePump").value)||0,
    forceInj: Number($("forceInj").value)||0,
    forceDuty: Number($("forceDuty").value)||0,
    forceLevel: Number($("forceLevel").value)||0,
    forceMapKpa: fm==="" ? -1 : Number(fm),
    forceRailPsi: fr==="" ? -1 : Number(fr),
    forceBaroKpa: fb==="" ? -1 : Number(fb),
    forceDpPsi: fd==="" ? -1 : Number(fd)
  };
  try{ await apiPost("/api/config", payload); }catch(e){}
  await poll();
}

function wireSliderKeyboard(id, valueId, decimals){
  const slider = $(id);
  if(!slider) return;
  slider.addEventListener("click", ()=>slider.focus());
  slider.addEventListener("keydown", (ev)=>{
    const step = Number(slider.step) || 1;
    const min = Number(slider.min);
    const max = Number(slider.max);
    const hasMin = isFinite(min);
    const hasMax = isFinite(max);
    let delta = 0;
    if(ev.key === "ArrowRight" || ev.key === "ArrowUp") delta = step;
    else if(ev.key === "ArrowLeft" || ev.key === "ArrowDown") delta = -step;
    else if(ev.key === "PageUp") delta = step * 10.0;
    else if(ev.key === "PageDown") delta = -step * 10.0;
    else if(ev.key === "Home" && hasMin){
      slider.value = String(min);
      if($(valueId)) $(valueId).textContent = Number(slider.value).toFixed(decimals);
      ev.preventDefault();
      return;
    }else if(ev.key === "End" && hasMax){
      slider.value = String(max);
      if($(valueId)) $(valueId).textContent = Number(slider.value).toFixed(decimals);
      ev.preventDefault();
      return;
    }else{
      return;
    }
    ev.preventDefault();
    let next = (Number(slider.value) || 0) + delta;
    if(hasMin) next = Math.max(min, next);
    if(hasMax) next = Math.min(max, next);
    slider.value = String(next);
    if($(valueId)) $(valueId).textContent = Number(slider.value).toFixed(decimals);
  });
}

$("targetInjectorDp").addEventListener("input", renderTable);
$("injLb").addEventListener("input", ()=>{ readInjModel(); updateDutyClampFlow(); renderTable(); });
$("mixMethPct").addEventListener("input", ()=>{ updateDutyClampFlow(); renderTable(); });
["mapLin","mapOff"].forEach(id=>{
  const el = $(id);
  if(el) el.addEventListener("input", ()=>{ updateMapFitExpectedTable(); updateSensorQuickCheckTable(); });
});
["pLin","pOff"].forEach(id=>{
  const el = $(id);
  if(el) el.addEventListener("input", ()=>{ updateRailFitExpectedTable(); updateSensorQuickCheckTable(); });
});
["forcePump","forceInj","forceDuty","forceMapKpa","forceRailPsi","forceBaroKpa","forceDpPsi","forceLevel"].forEach(id=>{
  const el = $(id);
  if(!el) return;
  el.addEventListener("input", applyTestForces);
  el.addEventListener("change", applyTestForces);
});
function setMapPreset(mapLinear, mapOffset){
  $("mapLin").value = mapLinear.toFixed(3);
  $("mapOff").value = mapOffset;
  updateMapFitExpectedTable();
  updateSensorQuickCheckTable();
}
function presetChange(v){
  if(!v) return;
  const parts = v.split(",");
  if(parts.length===2){
    const lin=parseFloat(parts[0]);
    const off=parseFloat(parts[1]);
    if(isFinite(lin) && isFinite(off)){
      setMapPreset(lin, off);
    }
  }
}
function presetRail(v){
  if(!v) return;
  const parts=v.split(",");
  if(parts.length===2){
    const lin=parseFloat(parts[0]);
    const off=parseFloat(parts[1]);
    if(isFinite(lin) && isFinite(off)){
      $("pLin").value = lin;
      $("pOff").value = off;
      updateRailFitExpectedTable();
      updateSensorQuickCheckTable();
    }
  }
}
$("dutyClamp").addEventListener("input", updateDutyClampFlow);
$("curve").style.touchAction="none";
$("curve").addEventListener("pointerdown", onDown, {passive:false});
$("curve").addEventListener("pointermove", onHover, {passive:true});
$("curve").addEventListener("pointerleave", clearHover, {passive:true});
window.addEventListener("pointermove", onMove, {passive:false});
window.addEventListener("pointerup", onUp, {passive:false});
window.addEventListener("pointercancel", onUp, {passive:false});

function toggleCard(el){
  const card = el && el.closest && el.closest(".card");
  if(card) card.classList.toggle("collapsed");
}

function initCardToggles(){
  document.querySelectorAll(".card").forEach((card, idx)=>{
    if(!card.dataset.title){
      const titleEl = card.querySelector(".h1");
      if(titleEl) card.dataset.title = titleEl.textContent.replace(/\s+/g," ").trim();
      else if(idx === 0) card.dataset.title = "Status";
    }
    card.classList.add("collapsed");
  });
}

(async ()=>{
  initCardToggles();
  renderMapFitTable();
  renderRailFitTable();
  renderSensorQuickCheckTable();
  await loadCfg();
  await poll();
  wsBadgeOff();
  wsConnect();
  setInterval(()=>poll().catch(()=>{}), 100);
  setInterval(updateWsBadge, 500);
})();

wireSliderKeyboard("adcGain", "adcGainVal", 4);
wireSliderKeyboard("adcOffset", "adcOffsetVal", 3);
</script>

</body>
</html>

)HTML";

// ================================================================














