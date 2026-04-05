
const $=id=>document.getElementById(id);
const DEFAULTS = {
  dp_arm_duty_pct: 5.0,
  dp_recover_margin_psi: 5.0,
  level_debounce_ms: 30,
  pump_prime_ms: 150,
  inj_lbhr_at_58psi: 36.0,
  mix_meth_pct: 60.0,
  flow_ref_dp_psi: 60.0,
  map_div_ratio: 0.66,
  rail_div_ratio: 0.66,
  serial_status_period_ms: 250,
  serial_debug_enable: 0
};

function fmt(n,d=1){return (Number(n)||0).toFixed(d);}
function cloneCurve(arr){ return arr.map(p=>Object.assign({}, p)); }

// --- Flow helper (for Duty Clamp preview) ---
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
  $("lvlDebounce").value = cfg.lvlDebounce ?? DEFAULTS.level_debounce_ms;
  $("primeMs").value = cfg.primeMs ?? DEFAULTS.pump_prime_ms;
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
  $("forceBaroKpa").value = (cfg.forceBaroKpa>=0) ? cfg.forceBaroKpa : "";
  $("forceLevel").value = cfg.forceLevel ?? 0;
  forceBoostCut = !!cfg.forceBoostCut;
  forceTimingCut = !!cfg.forceTimingCut;
  forceDpFault = !!cfg.forceDpFault;
  forceDpBoostHold = !!cfg.forceDpBoostHold;
  updateLatchButtons();

  $("mapDivRatio").value = cfg.mapDiv ?? DEFAULTS.map_div_ratio;
  $("railDivRatio").value = cfg.railDiv ?? DEFAULTS.rail_div_ratio;
  $("serialDebug").value = cfg.sdbg ?? DEFAULTS.serial_debug_enable;
  $("serialPeriod").value = cfg.sper ?? DEFAULTS.serial_status_period_ms;
}

function readCurveAt(kpa){
  // linear interp over 10 points
  const pts=cfg.curve;
  if(kpa<=pts[0].kpa) return pts[0].pct;
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
let forceDpBoostHold=false;
let ws=null;
let wsConnected=false;
let wsRetryMs=500;
let lastWsMsgMs=0;
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
  const bDpBoostHold = $("btnDpBoostHold");
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
  const dpBoostHoldActive = !!(forceDpBoostHold || (status && status.dpBoostHold));
  if(bBoost){
    bBoost.classList.toggle("active", !!boostActive);
    bBoost.textContent = `Manual Boost Cut Latch: ${forceBoostCut ? "ON" : "OFF"}`;
  }
  if(bTiming){
    bTiming.classList.toggle("active", !!timingActive);
    bTiming.textContent = `Manual Timing Cut Latch: ${forceTimingCut ? "ON" : "OFF"}`;
  }
  if(bDpFault){
    bDpFault.classList.toggle("active", !!dpFaultActive);
    bDpFault.textContent = `Rail dP Fault: ${forceDpFault ? "ON" : "OFF"}`;
  }
  if(bDpBoostHold){
    bDpBoostHold.classList.toggle("active", !!dpBoostHoldActive);
    bDpBoostHold.textContent = `dP Boost Hold Latch: ${forceDpBoostHold ? "ON" : "OFF"}`;
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
  await apiPost("/api/config", {forceDpFault, forceDpBoostHold});
  if(cfg){ cfg.forceDpFault = forceDpFault; }
  updateLatchButtons();
  await poll();
}
async function toggleDpBoostHold(){
  forceDpBoostHold = !forceDpBoostHold;
  await apiPost("/api/config", {forceDpFault, forceDpBoostHold});
  if(cfg){ cfg.forceDpBoostHold = forceDpBoostHold; }
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

  // axes
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

  // grid
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

  // clamp line
  const clampY = y(Math.min(100, Math.max(0, clampVal)));
  ctx.strokeStyle="#ffb300"; ctx.setLineDash([8,6]); ctx.lineWidth=2;
  ctx.beginPath(); ctx.moveTo(pad, clampY); ctx.lineTo(w-pad, clampY); ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle="#ffb300"; ctx.font="18px system-ui";
  ctx.fillText(`Clamp ${Math.round(clampVal)}%`, w-pad-120, clampY-6);

  // curve (display is clamped to Duty Clamp)
  ctx.lineWidth=4;
  ctx.strokeStyle="#00e676";
  ctx.beginPath();
  ctx.moveTo(x(pts[0].kpa),y(Math.min(pts[0].pct, clampVal)));
  for(let i=1;i<pts.length;i++) ctx.lineTo(x(pts[i].kpa),y(Math.min(pts[i].pct, clampVal)));
  ctx.stroke();

  // points
  for(let i=0;i<pts.length;i++){
    ctx.fillStyle="#00c853";
    ctx.beginPath(); ctx.arc(x(pts[i].kpa),y(Math.min(pts[i].pct, clampVal)),10,0,Math.PI*2); ctx.fill();
    ctx.strokeStyle="#0b0d10"; ctx.lineWidth=3;
    ctx.stroke();
  }

  // current MAP marker
  if(status){
    const mkpa=status.mapKpa||0;
    const cp=Math.min(readCurveAt(mkpa), clampVal); // honor clamp
    ctx.strokeStyle="#ffb300"; ctx.lineWidth=3;
    ctx.beginPath(); ctx.arc(x(mkpa),y(cp),12,0,Math.PI*2); ctx.stroke();
  }

  // hover marker
  if(window.hoverKpa!==undefined){
    const hk=hoverKpa;
    const hv=Math.min(readCurveAt(hk), clampVal);
    ctx.strokeStyle="#00c853"; ctx.lineWidth=2;
    ctx.beginPath(); ctx.moveTo(x(hk),pad); ctx.lineTo(x(hk),h-pad); ctx.stroke();
    ctx.beginPath(); ctx.arc(x(hk),y(hv),10,0,Math.PI*2); ctx.stroke();
    // tooltip box
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
    // capture pointer so drag continues even if you leave the canvas
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
  $("hoverLine").textContent="Hover: -- kPa -> -- %";
  drawCurve();
}

async function loadCfg(){
  cfg=await apiGet("/api/config");
  populateCfg();
  drawCurve();
  renderTable();
}

function applyStatus(s){
  if(!s) return;
  status = s;
  const mapRaw = (status.mapKpaRaw!==undefined && status.mapKpaRaw !== null) ? status.mapKpaRaw : (status.mapKpa||0);
  $("mapKpa").textContent=Math.round(mapRaw);
  $("mapKpa2").textContent=Math.round(mapRaw);
  $("railPsi").textContent=fmt(status.railPsi,1);
  $("dpPsi").textContent=fmt(status.dpPsi,1);
  $("sprayPct").textContent=Math.round(status.sprayPct||0);
  $("curvePct").textContent=Math.round(status.curvePct||0);
  $("clampPct").textContent=Math.round(status.dutyClamp||0);
  updateDutyClampFlow();
  updateLatchButtons();
  const sr = (status.safetyReason && status.safetyReason !== "none") ? status.safetyReason : "";
  const fr = (status.faultReason && status.faultReason !== "None") ? status.faultReason : "";
  const faultText = (status.dpBoostHold ? "Rail dP Fault (Boost Hold)" : (sr || fr || (status.faultActive ? "FAULT" : "OK")));
  $("faultKv").textContent = faultText;

  $("stPump").textContent = status.pumpOn ? "ON" : "OFF";
  $("stInj").textContent = status.injOn ? "ON" : "OFF";
  $("stSpray").textContent = status.sprayActive ? "ACTIVE" : "INACTIVE";
  $("stLevel").textContent = status.levelOk ? "OK" : "LOW";
  $("stRedLed").textContent = status.redLedOn ? "ON" : "OFF";
  $("stBlueLed").textContent = status.blueLedOn ? "ON" : "OFF";
  $("stBoost").textContent = status.boostOn ? "OFF" : "ON"; // Boost cut state
  $("stSafety").textContent = status.safetyState || (status.hardSafe ? "Timing Cut" : (status.levelLow ? "Boost Cut" : "OK"));
  $("stIat").textContent = status.iatOn ? "TIMING CUT" : "OK";
  $("stIatSsr").textContent = status.iatOn ? "GROUNDING IAT" : "NORMAL";
  $("stLvlRaw").textContent = status.levelRaw ? "LOW(raw)" : "OK(raw)";
  $("stPrime").textContent = status.pumpPrimed ? "PRIMED" : "PRIMING";
  $("stLevelFault").textContent = status.levelFault ? "LOW" : "OK";
  $("stDpFault").textContent = status.dpFault ? "LATCHED" : "OK";
  $("stDpPending").textContent = status.dpPending ? "YES" : "NO";
  $("stReason").textContent = status.safetyReason || status.faultReason || "None";

  $("netLine").textContent = status.netLine || "";

  drawCurve();
  renderTable();
  // keep cards closed on load
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
    // swallow
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
  if(wsConnected && (Date.now() - lastWsMsgMs) < 1000) return;
  };
  ws.onmessage = (ev)=>{
    try{
      const s = JSON.parse(ev.data);
      lastWsMsgMs = Date.now();
      updateWsBadge();
  if(wsConnected && (Date.now() - lastWsMsgMs) < 1000) return;
      applyStatus(s);
    } catch(e){
      // ignore
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
  // build cfg from inputs
  cfg.injHz = Number($("injHz").value)||cfg.injHz;
  cfg.dutyClamp = Number($("dutyClamp").value)||0;
  cfg.dpMinPsi = Number($("dpMinPsi").value)||0;
  cfg.dpFaultMs = Number($("dpFaultMs").value)||0;
  cfg.dpArmPct = Number($("dpArmPct").value)||DEFAULTS.dp_arm_duty_pct;
  cfg.dpRecover = Number($("dpRecover").value)||DEFAULTS.dp_recover_margin_psi;
  cfg.lvlDebounce = Number($("lvlDebounce").value)||DEFAULTS.level_debounce_ms;
  cfg.primeMs = Number($("primeMs").value)||DEFAULTS.pump_prime_ms;
  cfg.injLb = Number($("injLb").value)||cfg.injLb||DEFAULTS.inj_lbhr_at_58psi;
  cfg.mixMethPct = Number($("mixMethPct").value)||cfg.mixMethPct||DEFAULTS.mix_meth_pct;
  cfg.targetInjectorDp = Number($("targetInjectorDp").value)||cfg.targetInjectorDp||DEFAULTS.flow_ref_dp_psi;
  // curve start/max: redistribute kPa axis
  const cStart = Number($("curveStart").value);
  const cMax = Number($("curveMax").value);
  if(isFinite(cStart) && isFinite(cMax) && cMax > cStart){
    const span = cMax - cStart;
    const pts = cfg.curve;
    for(let i=0;i<pts.length;i++){
      pts[i].kpa = Math.round(cStart + (span * i)/(pts.length-1));
    }
  }

  cfg.mapLin = Number($("mapLin").value)||cfg.mapLin;
  cfg.mapOff = Number($("mapOff").value)||cfg.mapOff;
  cfg.pLin = Number($("pLin").value)||cfg.pLin;
  cfg.pOff = Number($("pOff").value)||cfg.pOff;

  cfg.wifiMode = Number($("wifiMode").value)||0;
  cfg.apSsid = $("apSsid").value||"";
  cfg.mdnsHost = $("mdnsHost").value||"";
  cfg.staSsid = $("staSsid").value||"";
  cfg.staPass = $("staPass").value||"";

  cfg.forcePump = Number($("forcePump").value)||0;
  cfg.forceInj = Number($("forceInj").value)||0;
  cfg.forceDuty = Number($("forceDuty").value)||0;
  const fm = $("forceMapKpa").value;
  const fr = $("forceRailPsi").value;
  const fd = $("forceDpPsi").value;
  const fb = $("forceBaroKpa").value;
  cfg.forceMapKpa = fm==="" ? -1 : Number(fm);
  cfg.forceRailPsi = fr==="" ? -1 : Number(fr);
  cfg.forceDpPsi = fd==="" ? -1 : Number(fd);
  cfg.forceBaroKpa = fb==="" ? -1 : Number(fb);
  cfg.forceLevel = Number($("forceLevel").value)||0;
  cfg.forceBoostCut = !!forceBoostCut;
  cfg.forceTimingCut = !!forceTimingCut;
  cfg.forceDpFault = !!forceDpFault;
  cfg.forceDpBoostHold = !!forceDpBoostHold;

  const mapDiv = Number($("mapDivRatio").value);
  if(isFinite(mapDiv)) cfg.mapDiv = mapDiv;
  const railDiv = Number($("railDivRatio").value);
  if(isFinite(railDiv)) cfg.railDiv = railDiv;
  cfg.sdbg = Number($("serialDebug").value)||DEFAULTS.serial_debug_enable;
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
  $("forceBaroKpa").value = "";
  $("forceLevel").value = 0;
  forceBoostCut = false;
  forceTimingCut = false;
  forceDpFault = false;
  forceDpBoostHold = false;
  updateLatchButtons();
  try{ await apiPost("/api/config", {forceBoostCut:false, forceTimingCut:false, forceDpFault:false, forceDpBoostHold:false, forceBaroKpa:-1}); }catch(e){}
  await applyTestForces();
}

async function applyTestForces(){
  const fm = $("forceMapKpa").value;
  const fr = $("forceRailPsi").value;
  const fd = $("forceDpPsi").value;
  const fb = $("forceBaroKpa").value;
  const payload = {
    forcePump: Number($("forcePump").value)||0,
    forceInj: Number($("forceInj").value)||0,
    forceDuty: Number($("forceDuty").value)||0,
    forceLevel: Number($("forceLevel").value)||0,
    forceMapKpa: fm==="" ? -1 : Number(fm),
    forceRailPsi: fr==="" ? -1 : Number(fr),
    forceDpPsi: fd==="" ? -1 : Number(fd),
    forceBaroKpa: fb==="" ? -1 : Number(fb)
  };
  try{ await apiPost("/api/config", payload); }catch(e){}
  await poll();
}

$("targetInjectorDp").addEventListener("input", renderTable);
$("injLb").addEventListener("input", ()=>{ readInjModel(); updateDutyClampFlow(); renderTable(); });
$("mixMethPct").addEventListener("input", ()=>{ updateDutyClampFlow(); renderTable(); });
["forcePump","forceInj","forceDuty","forceMapKpa","forceRailPsi","forceDpPsi","forceBaroKpa","forceLevel"].forEach(id=>{
  const el = $(id);
  if(!el) return;
  el.addEventListener("input", applyTestForces);
  el.addEventListener("change", applyTestForces);
});
function setMapPreset(hptLinear, hptOffset){
  // HP Tuners style: Linear is full kPa over 0-5V
  $("mapLin").value = hptLinear.toFixed(3);
  $("mapOff").value = hptOffset;
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
    }
  }
}
function calcRail(){
  const p1=Number($("railP1").value)||0;
  const v1=Number($("railV1").value)||0.5;
  const p2=Number($("railP2").value)||0;
  const v2=Number($("railV2").value)||4.5;
  const slope = (v2!==v1) ? ((p2-p1)/(v2-v1)) : 0;
  const offset = p1 - slope*v1;
  $("pLin").value = slope.toFixed(3);
  $("pOff").value = offset.toFixed(3);
}

$("dutyClamp").addEventListener("input", updateDutyClampFlow);
$("curve").style.touchAction="none";
$("curve").addEventListener("pointerdown", onDown, {passive:false});
$("curve").addEventListener("pointermove", onHover, {passive:true});
$("curve").addEventListener("pointerleave", clearHover, {passive:true});
window.addEventListener("pointermove", onMove, {passive:false});
window.addEventListener("pointerup", onUp, {passive:false});
window.addEventListener("pointercancel", onUp, {passive:false});

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
      if(t.closest("button, a, input, select, textarea, label, canvas, summary, details")) return;
      const isCollapsed = card.classList.toggle("collapsed");
      if(isCollapsed){
        // no-op
      }
    });
  });
}

(async ()=>{
  initCardToggles();
  await loadCfg();
  await poll();
  wsBadgeOff();
  wsConnect();
  // keep cards closed on load
  setInterval(()=>poll().catch(()=>{}), 100);
  setInterval(updateWsBadge, 500);
})();

