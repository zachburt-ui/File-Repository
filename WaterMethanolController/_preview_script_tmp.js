
const $=id=>document.getElementById(id);
const setText=(id, v)=>{ const el=$(id); if(el) el.textContent=v; };
const ADC_BREAKPOINTS = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3];
const MAP_SENSOR_CAL_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const RAIL_SENSOR_CAL_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const SENSOR_QUICK_CHECK_POINTS = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5];
const DEFAULTS = {
  dp_arm_duty_pct: 5.0,
  dp_recover_margin_psi: 5.0,
  dp_critical_psi: 20.0,
  dp_critical_ms: 100,
  dp_arm_settle_ms: 300,
  timing_cut_auto_clear_ms: 4000,
  pressure_ready_timeout_ms: 2500,
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
  dpMinPsi: 60,
  dpFaultMs: 2500,
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
      <td><span id="mapFitExp${i}">-</span></td>
      <td><button class="btn-info" onclick="captureMapFitRow(${i})">Capture</button></td>`;
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
      <td><span id="railFitExp${i}">-</span></td>
      <td><button class="btn-info" onclick="captureRailFitRow(${i})">Capture</button></td>`;
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
        <td><span id="quickMapErr${i}">-</span></td>
        <td><button class="btn-info" onclick="captureSensorQuickCheckMapRow(${i})">Capture</button></td>`;
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
        <td><span id="quickRailErr${i}">-</span></td>
        <td><button class="btn-info" onclick="captureSensorQuickCheckRailRow(${i})">Capture</button></td>`;
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

function captureMapFitRow(i){
  if(!status) return;
  const mapDisp = (status.mapKpa!==undefined && status.mapKpa !== null) ? status.mapKpa : (status.mapKpaRaw||0);
  const el = $(`mapFitK${i}`);
  if(el) el.value = Number(mapDisp).toFixed(2);
  updateMapFitExpectedRow(i);
}

function captureRailFitRow(i){
  if(!status) return;
  const el = $(`railFitP${i}`);
  if(el) el.value = Number(status.railPsi||0).toFixed(2);
  updateRailFitExpectedRow(i);
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

function captureSensorQuickCheckMapRow(i){
  if(!status) return;
  const mapDisp = (status.mapKpa!==undefined && status.mapKpa !== null) ? status.mapKpa : (status.mapKpaRaw||0);
  const mapEl = $(`quickMap${i}`);
  if(mapEl) mapEl.value = Number(mapDisp).toFixed(2);
  updateSensorQuickCheckMapRow(i);
}

function captureSensorQuickCheckRailRow(i){
  if(!status) return;
  const railEl = $(`quickRail${i}`);
  if(railEl) railEl.value = Number(status.railPsi||0).toFixed(2);
  updateSensorQuickCheckRailRow(i);
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
function cloneCurve(arr){ return arr.map(p=>Object.assign({}, p)); }

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

const PREVIEW_MODE = true;
const baseSensors = {mapKpa:178, railPsi:110.2};
let mockCfg = {
  injHz: 50,
  dutyClamp: 60,
  dpMinPsi: 60,
  dpFaultMs: 2500,
  dpArmPct: 5,
  dpRecover: 5,
  dpCriticalPsi: 20,
  dpCriticalMs: 100,
  dpArmSettleMs: 300,
  timingCutAutoMs: 4000,
  pressureReadyTimeoutMs: 2500,
  lvlDebounce: 300,
  injLb: 36,
  mixMethPct: 60,
  targetInjectorDp: 60,
  mapLin: 312.5,
  mapOff: -11.25,
  pLin: 32.5,
  pOff: -16.25,
  wifiMode: 0,
  apSsid: 'watermeth',
  mdnsHost: 'watermeth',
  staSsid: '',
  staPass: '',
  mapDiv: 0.66,
  railDiv: 0.66,
  adcGain: 1.0000,
  adcOffset: 0.000,
  forcePump: 0,
  forceInj: 0,
  forceDuty: 0,
  forceLevel: 0,
  forceMapKpa: -1,
  forceRailPsi: -1,
  forceDpPsi: -1,
  forceBoostCut: false,
  forceTimingCut: false,
  forceDpFault: false,
  forcePressureReadyFault: false,
  forceDpBoostHold: false,
  forceDpMonitorOverride: false,
  forcePressureReadyOverride: false,
  forceLevelFaultBypass: false,
  forceDpFaultBypass: false,
  forceDpBoostHoldBypass: false,
  sdbg: 1,
  sper: 250,
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
let mockRt = {
  faultLatched: false,
  faultLatchedAtMs: 0,
  dpBoostHoldLatched: false,
  sprayReqHold: false,
  pressureReady: false,
  pressureReadySinceMs: 0,
  armSinceMs: 0,
  lowSinceMs: 0,
  criticalSinceMs: 0,
  dpMinSeen: -1
};
let mockFaultHistory = 'None';

function clone(obj){return JSON.parse(JSON.stringify(obj));}

function applyMockConfig(patch){
  if(!patch) return;
  if(patch.curve && Array.isArray(patch.curve)){
    mockCfg.curve = patch.curve.map(p=>({kpa:Number(p.kpa)||0, pct:Number(p.pct)||0}));
  }
  Object.keys(patch).forEach(k=>{
    mockCfg[k] = patch[k];
  });
  const mono = (arr,key)=>{
    for(let i=1;i<arr.length;i++) if(arr[i][key] < arr[i-1][key]) arr[i][key] = arr[i-1][key];
  };
  mono(mockCfg.curve, 'kpa');
}

function readCurveAtWith(curve, kpa){
  if(!curve || !curve.length) return 0;
  if(kpa < curve[0].kpa) return 0;
  for(let i=0;i<curve.length-1;i++){
    const a=curve[i], b=curve[i+1];
    if(kpa <= b.kpa){
      const t=(kpa-a.kpa)/Math.max(1e-6,(b.kpa-a.kpa));
      return a.pct + t*(b.pct-a.pct);
    }
  }
  return curve[curve.length-1].pct;
}

function computeMockStatus(){
  const now = Date.now();
  const mapKpa = (mockCfg.forceMapKpa >= 0) ? mockCfg.forceMapKpa : baseSensors.mapKpa;
  let railPsi = (mockCfg.forceRailPsi >= 0) ? mockCfg.forceRailPsi : baseSensors.railPsi;
  if(mockCfg.forceDpPsi >= 0){
    railPsi = (mapKpa * 0.1450377377) + mockCfg.forceDpPsi;
  }
  const dpPsi = railPsi - (mapKpa * 0.1450377377);
  const dpMinPsi = Math.max(0, Number(mockCfg.dpMinPsi)||0);
  const dpFaultMs = Math.max(0, Number(mockCfg.dpFaultMs)||0);
  const dpArmPct = Math.max(0, Number(mockCfg.dpArmPct)||0);
  const dpRecover = Math.max(0, Number(mockCfg.dpRecover)||0);
  const dpCriticalPsi = Math.max(0, Number(mockCfg.dpCriticalPsi)||0);
  const dpCriticalMs = Math.max(0, Number(mockCfg.dpCriticalMs)||0);
  const dpArmSettleMs = Math.max(0, Number(mockCfg.dpArmSettleMs)||0);
  const timingCutAutoMs = Math.max(0, Number(mockCfg.timingCutAutoMs)||0);
  const pressureReadyTimeoutMs = Math.max(0, Number(mockCfg.pressureReadyTimeoutMs)||0);
  const dpMonitorOverride = !!mockCfg.forceDpMonitorOverride;
  const pressureReadyOverride = !!mockCfg.forcePressureReadyOverride;
  const levelFaultBypass = !!mockCfg.forceLevelFaultBypass;
  const dpFaultBypass = !!mockCfg.forceDpFaultBypass;
  const dpBoostHoldBypass = !!mockCfg.forceDpBoostHoldBypass;
  const pressureReadyFaultForced = !!mockCfg.forcePressureReadyFault;

  let levelLow = false;
  if(mockCfg.forceLevel === 1) levelLow = true;
  else if(mockCfg.forceLevel === 2) levelLow = false;

  const curve = (cfg && cfg.curve) ? cfg.curve : mockCfg.curve;
  const clampVal = Number(mockCfg.dutyClamp)||0;
  const rawCurve = readCurveAtWith(curve, mapKpa);

  let dutyCmd = 0;
  if(mockCfg.forceInj === 1) dutyCmd = 0;
  else if(mockCfg.forceInj === 2) dutyCmd = Math.min(Math.max(Number(mockCfg.forceDuty)||0,0), clampVal);
  else dutyCmd = Math.min(rawCurve, clampVal);
  if(dutyCmd > 0.5) mockRt.sprayReqHold = true;
  else if(dutyCmd < 0.2) mockRt.sprayReqHold = false;
  const sprayRequested = mockRt.sprayReqHold;
  const levelLowEffective = levelLow && !levelFaultBypass;
  const dpFaultRaw = !!(mockCfg.forceDpFault || pressureReadyFaultForced || mockRt.faultLatched);
  const dpFaultPre = !dpFaultBypass && dpFaultRaw;
  const dpBoostHoldRaw = !!(mockCfg.forceDpBoostHold || mockRt.dpBoostHoldLatched);
  const dpBoostHoldPre = !dpBoostHoldBypass && dpBoostHoldRaw;
  const sprayActiveForSafety = sprayRequested && mockRt.pressureReady;
  const levelLowEmergencyPre = levelLowEffective && sprayActiveForSafety;
  const timingCutPre = !!(mockCfg.forceTimingCut || dpFaultPre || levelLowEmergencyPre);
  const boostCutPre = !!(levelLowEffective || timingCutPre || mockCfg.forceBoostCut || dpBoostHoldPre);
  const sprayRequestNow = !timingCutPre && sprayRequested && !boostCutPre && (mockCfg.forcePump !== 1);
  const sprayEnableDpTargetPsi = Math.max(0, Number(mockCfg.targetInjectorDp)||0);
  const pressureReadyForSpray = (sprayEnableDpTargetPsi <= 0) ? true : (dpPsi >= sprayEnableDpTargetPsi);
  if(!sprayRequestNow){
    mockRt.pressureReady = false;
    mockRt.pressureReadySinceMs = 0;
  }else if(pressureReadyOverride){
    mockRt.pressureReady = true;
    mockRt.pressureReadySinceMs = 0;
  }else if(!mockRt.pressureReady && pressureReadyForSpray){
    mockRt.pressureReady = true;
    mockRt.pressureReadySinceMs = 0;
  }else if(!mockRt.pressureReady){
    if(!mockRt.pressureReadySinceMs){
      mockRt.pressureReadySinceMs = now;
    }
    if(!mockRt.faultLatched && pressureReadyTimeoutMs > 0 && ((now - mockRt.pressureReadySinceMs) >= pressureReadyTimeoutMs)){
      mockRt.faultLatched = true;
      mockRt.faultLatchedAtMs = now;
      mockRt.dpBoostHoldLatched = true;
      mockRt.pressureReadySinceMs = 0;
    }
  }
  const pressureReady = mockRt.pressureReady;
  const sprayActive = sprayRequested && pressureReady;
  if(dpMonitorOverride){
    mockRt.armSinceMs = 0;
    mockRt.lowSinceMs = 0;
    mockRt.criticalSinceMs = 0;
    mockRt.dpMinSeen = -1;
  }
  const dpArmCondition = !dpMonitorOverride && sprayActive && pressureReady && (dutyCmd > dpArmPct);
  if(dpArmCondition){
    if(!mockRt.armSinceMs) mockRt.armSinceMs = now;
  }else{
    mockRt.armSinceMs = 0;
    mockRt.lowSinceMs = 0;
    mockRt.criticalSinceMs = 0;
    mockRt.dpMinSeen = -1;
  }
  const armElapsed = mockRt.armSinceMs ? (now - mockRt.armSinceMs) : 0;
  const dpArmed = dpArmCondition && (armElapsed >= dpArmSettleMs);
  const dpArmSettleRemainingMs = (dpArmCondition && !dpArmed) ? Math.max(0, dpArmSettleMs - armElapsed) : 0;
  if(dpArmed){
    if(mockRt.dpMinSeen < -0.5 || dpPsi < mockRt.dpMinSeen){
      mockRt.dpMinSeen = dpPsi;
    }
    const criticalLow = (dpCriticalPsi > 0) && (dpPsi < dpCriticalPsi);
    if(criticalLow){
      if(!mockRt.criticalSinceMs) mockRt.criticalSinceMs = now;
      if(!mockRt.faultLatched && ((now - mockRt.criticalSinceMs) >= dpCriticalMs)){
        mockRt.faultLatched = true;
        mockRt.faultLatchedAtMs = now;
        mockRt.dpBoostHoldLatched = true;
      }
    }else if(dpCriticalPsi <= 0 || dpPsi > (dpCriticalPsi + dpRecover)){
      mockRt.criticalSinceMs = 0;
    }
    if(dpPsi < dpMinPsi){
      if(!mockRt.lowSinceMs) mockRt.lowSinceMs = now;
      if(!mockRt.faultLatched && ((now - mockRt.lowSinceMs) >= dpFaultMs)){
        mockRt.faultLatched = true;
        mockRt.faultLatchedAtMs = now;
        mockRt.dpBoostHoldLatched = true;
      }
    }else if(dpPsi > (dpMinPsi + dpRecover)){
      mockRt.lowSinceMs = 0;
    }
  }else if(dpArmCondition){
    mockRt.lowSinceMs = 0;
    mockRt.criticalSinceMs = 0;
    mockRt.dpMinSeen = -1;
  }
  if(mockRt.faultLatched && timingCutAutoMs > 0){
    if(!mockRt.faultLatchedAtMs) mockRt.faultLatchedAtMs = now;
    if((now - mockRt.faultLatchedAtMs) >= timingCutAutoMs){
      mockRt.faultLatched = false;
      mockRt.faultLatchedAtMs = 0;
      mockRt.lowSinceMs = 0;
      mockRt.criticalSinceMs = 0;
    }
  }
  const dpFault = !dpFaultBypass && !!(mockCfg.forceDpFault || pressureReadyFaultForced || mockRt.faultLatched);
  const dpBoostHold = !dpBoostHoldBypass && !!(mockCfg.forceDpBoostHold || mockRt.dpBoostHoldLatched);
  const dpPending = !dpMonitorOverride && dpArmed && !mockRt.faultLatched && !!mockRt.lowSinceMs;
  const dpLowMs = mockRt.lowSinceMs ? (now - mockRt.lowSinceMs) : 0;
  const dpMinSeen = (mockRt.dpMinSeen > -0.5) ? mockRt.dpMinSeen : -1;
  let dpState = "IDLE";
  if(dpFault) dpState = "TIMING_CUT";
  else if(dpBoostHold) dpState = "BOOST_HOLD";
  else if(dpMonitorOverride) dpState = "OVERRIDE";
  else if(dpPending) dpState = "PENDING";
  else if(dpArmed) dpState = "ARMED";
  const timingCut = !!(mockCfg.forceTimingCut || dpFault || (levelLowEffective && sprayActive));
  const boostOn = !(levelLowEffective || timingCut || mockCfg.forceBoostCut || dpBoostHold);

  let pumpOn = false;
  if(mockCfg.forcePump === 1) pumpOn = false;
  else if(mockCfg.forcePump === 2) pumpOn = !timingCut && boostOn;
  else pumpOn = sprayActive && !timingCut && boostOn;

  let sprayPct = dutyCmd;
  if(!boostOn || timingCut) sprayPct = 0;
  if(sprayPct > 0 && !pressureReady && !pressureReadyOverride) sprayPct = 0;
  const injOn = (sprayPct > 0.5) && !timingCut && boostOn;
  const sprayActiveNow = injOn && pumpOn && !levelLowEffective;


  const safetyState = timingCut ? 'Timing Cut' : (!boostOn ? 'Boost Cut' : 'OK');
  let safetyReason = '';
  if(mockCfg.forceTimingCut) safetyReason = 'Manual Timing Cut';
  else if(mockCfg.forceBoostCut) safetyReason = 'Manual Boost Cut';
  else if(dpFault) safetyReason = 'Rail dP Fault';
  else if(dpBoostHold) safetyReason = 'Rail dP Fault (Boost Hold)';
  else if(timingCut) safetyReason = 'Low Level (Timing Cut)';
  else if(!boostOn && levelLowEffective) safetyReason = 'Low Level';
  let faultReason = '';
  if(mockCfg.forceTimingCut) faultReason = 'Manual Timing Cut';
  else if(mockCfg.forceBoostCut) faultReason = 'Manual Boost Cut';
  else if(pressureReadyFaultForced && !dpFaultBypass) faultReason = 'Pressure-Ready Timeout Fault (Manual)';
  else if(dpFault) faultReason = 'Rail dP Fault';
  else if(dpBoostHold) faultReason = 'Rail dP Fault (Boost Hold)';
  else if(timingCut) faultReason = 'Low Level (Timing Cut)';
  else if(!boostOn && levelLowEffective) faultReason = 'Low Level';
  if(!faultReason) faultReason = 'None';
  const activeReasonNow = safetyReason || (faultReason !== 'None' ? faultReason : '');
  if(activeReasonNow){
    mockFaultHistory = activeReasonNow;
  }

  return {
    mapKpa,
    mapKpaRaw: mapKpa,
    railPsi,
    dpPsi,
    sprayPct,
    curvePct: rawCurve,
    dutyClamp: clampVal,
    pumpOn,
    injOn,
    sprayActive: sprayActiveNow,
    levelOk: !levelLowEffective,
    redLedOn: !boostOn,
    blueLedOn: levelLow,
    boostOn,
    levelLow,
    iatOn: timingCut,
    levelFault: levelLowEffective,
    dpFault,
    dpBoostHold,
    dpMonitorOverride,
    levelFaultBypass,
    dpFaultBypass,
    dpBoostHoldBypass,
    faultActive: !!safetyReason,
    forceBoostCut: !!mockCfg.forceBoostCut,
    forceTimingCut: !!mockCfg.forceTimingCut,
    forceDpFault: !!mockCfg.forceDpFault,
    forcePressureReadyFault: !!mockCfg.forcePressureReadyFault,
    forceDpBoostHold: !!mockCfg.forceDpBoostHold,
    forceDpMonitorOverride: !!mockCfg.forceDpMonitorOverride,
    pressureReadyOverride: !!mockCfg.forcePressureReadyOverride,
    forcePressureReadyOverride: !!mockCfg.forcePressureReadyOverride,
    forceLevelFaultBypass: !!mockCfg.forceLevelFaultBypass,
    forceDpFaultBypass: !!mockCfg.forceDpFaultBypass,
    forceDpBoostHoldBypass: !!mockCfg.forceDpBoostHoldBypass,
    dpPending,
    dpState,
    dpLowMs,
    dpMinSeen,
    dpArmed,
    dpArmSettleRemainingMs,
    levelDebounceMs: mockCfg.lvlDebounce,
    pressureReadyTimeoutMs,
    pressureReady,
    levelRaw: levelLow,
    dpMinPsi: dpMinPsi,
    dpArmPct: dpArmPct,
    dpRecover: dpRecover,
    dpCriticalPsi: dpCriticalPsi,
    dpCriticalMs: dpCriticalMs,
    dpArmSettleMs: dpArmSettleMs,
    lvlDebounce: mockCfg.lvlDebounce,
    injLb: mockCfg.injLb,
    mixMethPct: mockCfg.mixMethPct,
    targetInjectorDp: mockCfg.targetInjectorDp,
    safetyState,
    safetyReason,
    faultReason,
    faultHistory: mockFaultHistory,
    stateText: timingCut ? 'Timing Cut' : (!boostOn ? 'Boost Cut' : ''),
    netLine: 'PREVIEW MODE'
  };
}

async function apiGet(p){
  if(p === '/api/config') return clone(mockCfg);
  if(p === '/api/status') return computeMockStatus();
  throw new Error('Unknown endpoint');
}

async function apiPost(p,obj){
  if(p === '/api/config'){
    applyMockConfig(obj || {});
    return {ok:true};
  }
  return {ok:true};
}

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
  $("forceDpPsi").value = (cfg.forceDpPsi>=0) ? cfg.forceDpPsi : "";
  $("forceLevel").value = cfg.forceLevel ?? 0;
  forceBoostCut = !!cfg.forceBoostCut;
  forceTimingCut = !!cfg.forceTimingCut;
  forceDpFault = !!cfg.forceDpFault;
  forcePressureReadyFault = !!cfg.forcePressureReadyFault;
  forceDpBoostHold = !!cfg.forceDpBoostHold;
  forceDpMonitorOverride = !!cfg.forceDpMonitorOverride;
  forcePressureReadyOverride = !!cfg.forcePressureReadyOverride;
  forceLevelFaultBypass = !!cfg.forceLevelFaultBypass;
  forceDpFaultBypass = !!cfg.forceDpFaultBypass;
  forceDpBoostHoldBypass = !!cfg.forceDpBoostHoldBypass;
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
let forceDpMonitorOverride=false;
let forcePressureReadyOverride=false;
let forceLevelFaultBypass=false;
let forceDpFaultBypass=false;
let forceDpBoostHoldBypass=false;
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
  const bDpMonOverride = $("btnDpMonOverride");
  const bPressureReadyOverride = $("btnPressureReadyOverride");
  const bLevelBypass = $("btnLevelBypass");
  const bDpFaultBypass = $("btnDpFaultBypass");
  const bDpHoldBypass = $("btnDpHoldBypass");
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
  const pressureReadyFaultActive = !!forcePressureReadyFault;
  const dpBoostHoldActive = !!(forceDpBoostHold || (status && status.dpBoostHold));
  const dpMonOverrideActive = !!(forceDpMonitorOverride || (status && status.dpMonitorOverride));
  const pressureReadyOverrideActive = !!(forcePressureReadyOverride || (status && status.pressureReadyOverride));
  const levelBypassActive = !!(forceLevelFaultBypass || (status && status.levelFaultBypass));
  const dpFaultBypassActive = !!(forceDpFaultBypass || (status && status.dpFaultBypass));
  const dpHoldBypassActive = !!(forceDpBoostHoldBypass || (status && status.dpBoostHoldBypass));
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
    bDpBoostHold.textContent = `dP Boost Hold: ${forceDpBoostHold ? "ON" : "OFF"}`;
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
    const val = Math.round(minX + (maxX-minX)*(i/5));
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
  setText("mapKpa", Math.round(mapDisp));
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
  setText("calMapKpa2d", fmt(calGaugeMapFilt,2));
  setText("calRailPsi2d", fmt(calGaugeRailFilt,2));
  updateDutyClampFlow();
  updateLatchButtons();
  const sr = (status.safetyReason && status.safetyReason !== "none") ? status.safetyReason : "";
  const fr = (status.faultReason && status.faultReason !== "None") ? status.faultReason : "";
  const faultText = (status.dpBoostHold ? "Rail dP Fault (Boost Hold)" : (sr || fr || (status.faultActive ? "FAULT" : "OK")));
  setText("faultKv", faultText);

  setText("stPump", status.pumpOn ? "ON" : "OFF");
  setText("stInj", status.injOn ? "ON" : "OFF");
  setText("stSpray", status.sprayActive ? "ACTIVE" : "INACTIVE");
  setText("stLevel", status.levelOk ? "OK" : "LOW");
  setText("stRedLed", status.redLedOn ? "ON" : "OFF");
  setText("stBlueLed", status.blueLedOn ? "ON" : "OFF");
  setText("stBoost", status.boostOn ? "OFF" : "ON");
  setText("stSafety", status.safetyState || (status.levelLow ? "Boost Cut" : "OK"));
  setText("stIat", status.iatOn ? "ON" : "OFF");
  setText("stIatSsr", status.iatOn ? "ON" : "OFF");
  setText("stLvlRaw", status.levelRaw ? "LOW(raw)" : "OK(raw)");
  setText("stPressureReady", status.pressureReady ? "READY" : "BUILDING");
  setText("stLevelFault", status.levelFault ? "LOW" : "OK");
  setText("stDpFault", status.dpFault ? "ACTIVE" : "OK");
  setText("stDpPending", status.dpPending ? "YES" : "NO");
  setText("stDpState", status.dpState || (status.dpFault ? "TIMING_CUT" : (status.dpBoostHold ? "BOOST_HOLD" : (status.dpPending ? "PENDING" : "IDLE"))));
  setText("stDpOverride", status.dpMonitorOverride ? "ON" : "OFF");
  setText("stPressureReadyOverride", status.pressureReadyOverride ? "ON" : "OFF");
  setText("stLevelBypass", status.levelFaultBypass ? "ON" : "OFF");
  setText("stDpFaultBypass", status.dpFaultBypass ? "ON" : "OFF");
  setText("stDpHoldBypass", status.dpBoostHoldBypass ? "ON" : "OFF");
  setText("stDpLowMs", Math.round(status.dpLowMs||0));
  setText("stDpMinSeen", (status.dpMinSeen!==undefined && status.dpMinSeen!==null && isFinite(status.dpMinSeen) && status.dpMinSeen > -0.5) ? fmt(status.dpMinSeen,1) : "-");
  setText("stDpArmed", status.dpArmed ? "YES" : "NO");
  setText("stDpSettle", `${Math.round(status.dpArmSettleRemainingMs||0)} ms`);
  setText("stReason", status.faultHistory || "None");

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
  wsConnected = false;
  ws = null;
  lastWsMsgMs = 0;
  wsBadgeOff();
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
  const fd = $("forceDpPsi").value;
  cfg.forceMapKpa = fm==="" ? -1 : Number(fm);
  cfg.forceRailPsi = fr==="" ? -1 : Number(fr);
  cfg.forceDpPsi = fd==="" ? -1 : Number(fd);
  const forceLevelVal = Number($("forceLevel").value);
  cfg.forceLevel = isFinite(forceLevelVal) ? forceLevelVal : 0;
  cfg.forceBoostCut = !!forceBoostCut;
  cfg.forceTimingCut = !!forceTimingCut;
  cfg.forceDpFault = !!forceDpFault;
  cfg.forcePressureReadyFault = !!forcePressureReadyFault;
  cfg.forceDpBoostHold = !!forceDpBoostHold;
  cfg.forceDpMonitorOverride = !!forceDpMonitorOverride;
  cfg.forcePressureReadyOverride = !!forcePressureReadyOverride;
  cfg.forceLevelFaultBypass = !!forceLevelFaultBypass;
  cfg.forceDpFaultBypass = !!forceDpFaultBypass;
  cfg.forceDpBoostHoldBypass = !!forceDpBoostHoldBypass;

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
  $("forceDpPsi").value = "";
  $("forceLevel").value = 0;
  forceBoostCut = false;
  forceTimingCut = false;
  forceDpFault = false;
  forcePressureReadyFault = false;
  forceDpBoostHold = false;
  forceDpMonitorOverride = false;
  forcePressureReadyOverride = false;
  forceLevelFaultBypass = false;
  forceDpFaultBypass = false;
  forceDpBoostHoldBypass = false;
  updateLatchButtons();
  try{ await apiPost("/api/config", {
    forceBoostCut:false,
    forceTimingCut:false,
    forceDpFault:false,
    forcePressureReadyFault:false,
    forceDpBoostHold:false,
    forceDpMonitorOverride:false,
    forcePressureReadyOverride:false,
    forceLevelFaultBypass:false,
    forceDpFaultBypass:false,
    forceDpBoostHoldBypass:false
  }); }catch(e){}
  await applyTestForces();
}

async function applyTestForces(){
  const fm = $("forceMapKpa").value;
  const fr = $("forceRailPsi").value;
  const fd = $("forceDpPsi").value;
  const payload = {
    forcePump: Number($("forcePump").value)||0,
    forceInj: Number($("forceInj").value)||0,
    forceDuty: Number($("forceDuty").value)||0,
    forceLevel: Number($("forceLevel").value)||0,
    forceMapKpa: fm==="" ? -1 : Number(fm),
    forceRailPsi: fr==="" ? -1 : Number(fr),
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
["forcePump","forceInj","forceDuty","forceMapKpa","forceRailPsi","forceDpPsi","forceLevel"].forEach(id=>{
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
    card.addEventListener("click", (ev)=>{
      const t = ev.target;
      if(t.closest("button, a, input, select, textarea, label, canvas, summary, details, .h1")) return;
      const isCollapsed = card.classList.toggle("collapsed");
      if(isCollapsed){
      }
    });
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

