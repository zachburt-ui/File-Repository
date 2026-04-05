import json
import re
import time
from pathlib import Path
from urllib.parse import urljoin
import urllib.error
import urllib.request

import serial


CANDIDATE_BASES = [
    "http://watermeth.local",
    "http://172.20.10.7",
    "http://172.20.10.8",
    "http://172.20.10.9",
    "http://192.168.4.1",
]
SERIAL_PORT = "COM5"
SERIAL_BAUD = 115200


class Runner:
    def __init__(self):
        self.results = []

    def add(self, name, ok, detail=""):
        self.results.append({"name": name, "ok": bool(ok), "detail": str(detail)})

    def summary(self):
        passed = sum(1 for r in self.results if r["ok"])
        total = len(self.results)
        return {
            "total": total,
            "pass": passed,
            "fail": total - passed,
            "results": self.results,
        }


def http_get_text(base, path, timeout=4):
    req = urllib.request.Request(urljoin(base, path), headers={"Cache-Control": "no-cache"})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read().decode("utf-8", errors="replace"), int(resp.getcode()), dict(resp.headers)


def get_json(base, path, timeout=4):
    body, code, _ = http_get_text(base, path, timeout=timeout)
    return json.loads(body), code


def post_json(base, path, payload, timeout=6):
    req = urllib.request.Request(
        urljoin(base, path),
        data=json.dumps(payload).encode("utf-8"),
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8", errors="replace")), int(resp.getcode())


def find_controller():
    for base in CANDIDATE_BASES:
        try:
            cfg, code = get_json(base, "/api/config", timeout=2)
            if code == 200 and isinstance(cfg, dict):
                return base
        except Exception:
            pass
    return None


def wait_status(base, predicate, timeout_s=5.0, poll_s=0.1):
    end = time.time() + timeout_s
    last = None
    while time.time() < end:
        try:
            last, _ = get_json(base, "/api/status", timeout=3)
            if predicate(last):
                return True, last
        except Exception:
            pass
        time.sleep(poll_s)
    return False, last


def reboot_board_serial():
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.1)
    try:
        time.sleep(0.1)
        ser.setDTR(False)
        ser.setRTS(True)
        time.sleep(0.15)
        ser.setRTS(False)
        time.sleep(0.15)
        ser.setDTR(True)
    finally:
        ser.close()


def reboot_board_via_api(base):
    try:
        cfg, _ = get_json(base, "/api/config", timeout=3)
        current_mode = int(cfg.get("wifiMode", 1))
        post_json(base, "/api/config", {"wifiMode": current_mode}, timeout=8)
        return True
    except Exception:
        return False


def reboot_board(base):
    if reboot_board_via_api(base):
        return
    reboot_board_serial()


def wait_online(base, timeout_s=45):
    end = time.time() + timeout_s
    while time.time() < end:
        try:
            _, code = get_json(base, "/api/status", timeout=2)
            if code == 200:
                return True
        except Exception:
            pass
        time.sleep(0.25)
    return False


def capture_serial(seconds=3.0):
    lines = []
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.1)
    try:
        end = time.time() + seconds
        while time.time() < end:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                lines.append(line)
    finally:
        ser.close()
    return lines


def parse_sta_ip(net_line):
    if not isinstance(net_line, str):
        return None
    m = re.search(r"STA:\s*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)", net_line)
    return m.group(1) if m else None


def clear_forces(base, level_mode=0, map_kpa=-1.0, rail_psi=-1.0, dp_psi=-1.0):
    payload = {
        "forcePump": 0,
        "forceInj": 0,
        "forceDuty": 0,
        "forceLevel": int(level_mode),
        "forceMapKpa": float(map_kpa),
        "forceRailPsi": float(rail_psi),
        "forceDpPsi": float(dp_psi),
        "forceBoostCut": False,
        "forceTimingCut": False,
        "forceDpFault": False,
        "forcePressureReadyFault": False,
        "forceDpBoostHold": False,
        "forceDpMonitorOverride": False,
        "forcePressureReadyOverride": False,
        "forceLevelFaultBypass": False,
        "forceDpFaultBypass": False,
        "forceDpBoostHoldBypass": False,
    }
    post_json(base, "/api/config", payload)


def main():
    runner = Runner()
    base = find_controller()
    if not base:
        print(json.dumps({"fatal": "controller_not_reachable"}))
        return

    cfg0, _ = get_json(base, "/api/config")
    status0, _ = get_json(base, "/api/status")
    html_root, code_root, _ = http_get_text(base, "/")
    preview_html = Path("Preview UI/ui_preview.html").read_text(encoding="utf-8", errors="replace")

    # Save original values for touched settings.
    restore_cfg = {
        "injHz": cfg0.get("injHz"),
        "dutyClamp": cfg0.get("dutyClamp"),
        "dpMinPsi": cfg0.get("dpMinPsi"),
        "dpFaultMs": cfg0.get("dpFaultMs"),
        "dpArmPct": cfg0.get("dpArmPct"),
        "dpRecover": cfg0.get("dpRecover"),
        "dpCriticalPsi": cfg0.get("dpCriticalPsi"),
        "dpCriticalMs": cfg0.get("dpCriticalMs"),
        "dpArmSettleMs": cfg0.get("dpArmSettleMs"),
        "timingCutAutoMs": cfg0.get("timingCutAutoMs"),
        "pressureReadyTimeoutMs": cfg0.get("pressureReadyTimeoutMs"),
        "lvlDebounce": cfg0.get("lvlDebounce"),
        "targetInjectorDp": cfg0.get("targetInjectorDp", cfg0.get("desiredRailDp")),
        "injLb": cfg0.get("injLb"),
        "mixMethPct": cfg0.get("mixMethPct"),
        "mapLin": cfg0.get("mapLin"),
        "mapOff": cfg0.get("mapOff"),
        "pLin": cfg0.get("pLin"),
        "pOff": cfg0.get("pOff"),
        "adcGain": cfg0.get("adcGain"),
        "adcOffset": cfg0.get("adcOffset"),
        "sdbg": cfg0.get("sdbg"),
        "sper": cfg0.get("sper"),
    }

    # ---------------- API contract checks ----------------
    required_cfg_keys = [
        "injHz", "dutyClamp", "dpMinPsi", "dpFaultMs", "dpArmPct", "dpRecover",
        "dpCriticalPsi", "dpCriticalMs", "dpArmSettleMs", "timingCutAutoMs",
        "pressureReadyTimeoutMs", "lvlDebounce", "injLb", "mixMethPct",
        "targetInjectorDp", "mapLin", "mapOff", "pLin", "pOff", "mapDiv", "railDiv",
        "adcGain", "adcOffset", "wifiMode", "apSsid", "mdnsHost", "staSsid", "staPass",
        "forcePump", "forceInj", "forceDuty", "forceLevel", "forceMapKpa", "forceRailPsi",
        "forceDpPsi", "forceBoostCut", "forceTimingCut", "forceDpFault", "forcePressureReadyFault",
        "forceDpBoostHold", "forceDpMonitorOverride", "forcePressureReadyOverride",
        "forceLevelFaultBypass", "forceDpFaultBypass", "forceDpBoostHoldBypass",
        "sdbg", "sper", "curve"
    ]
    for k in required_cfg_keys:
        runner.add(f"api_config_key_{k}", k in cfg0, "present" if k in cfg0 else "missing")

    required_status_keys = [
        "mapKpa", "mapKpaRaw", "railPsi", "dpPsi", "sprayPct", "curvePct", "dutyClamp",
        "pumpOn", "injOn", "sprayActive", "levelOk", "redLedOn", "blueLedOn", "greenLedOn", "boostOn",
        "iatOn", "levelFault", "dpFault", "dpBoostHold", "dpMonitorOverride",
        "pressureReadyOverride", "levelFaultBypass", "dpFaultBypass", "dpBoostHoldBypass",
        "dpPending", "dpState", "dpLowMs", "dpMinSeen", "dpArmed", "dpArmSettleRemainingMs",
        "pressureReady", "faultReason", "safetyState", "safetyReason", "netLine"
    ]
    for k in required_status_keys:
        runner.add(f"api_status_key_{k}", k in status0, "present" if k in status0 else "missing")

    # ---------------- UI/preview structure checks ----------------
    runner.add("ui_root_http_200", code_root == 200, f"code={code_root}")
    ui_ids = [
        "mapKpa", "railPsi", "dpPsi", "sprayPct", "curvePct", "clampPct", "faultKv",
        "stSafety", "stReason", "stPressureReady", "btnBoostCut", "btnTimingCut",
        "btnDpFault", "btnPressureReadyFault", "btnDpBoostHold", "btnDpMonOverride",
        "btnPressureReadyOverride", "btnLevelBypass", "btnDpFaultBypass", "btnDpHoldBypass",
        "forceMapKpa", "forceRailPsi", "forceDpPsi", "forceLevel", "adcGain", "adcOffset",
        "targetInjectorDp", "pressureReadyTimeoutMs", "mapLin", "mapOff", "pLin", "pOff",
        "serialDebug", "serialPeriod", "wifiMode", "apSsid", "mdnsHost", "staSsid", "staPass"
    ]
    for k in ui_ids:
        runner.add(f"ui_root_id_{k}", f'id="{k}"' in html_root, "present" if f'id="{k}"' in html_root else "missing")
        runner.add(f"ui_preview_id_{k}", f'id="{k}"' in preview_html, "present" if f'id="{k}"' in preview_html else "missing")

    # ---------------- Logic/state checks ----------------
    try:
        # Use fast test timings while preserving real logic paths.
        post_json(base, "/api/config", {
            "dpMinPsi": 60,
            "dpFaultMs": 250,
            "dpArmPct": 5,
            "dpRecover": 5,
            "dpCriticalPsi": 40,
            "dpCriticalMs": 80,
            "dpArmSettleMs": 50,
            "timingCutAutoMs": 500,
            "pressureReadyTimeoutMs": 2500,
            "targetInjectorDp": 60
        })

        clear_forces(base, level_mode=2, map_kpa=0, rail_psi=0)
        time.sleep(0.4)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_baseline_zero_map_no_spray", (float(s.get("sprayPct", -1)) == 0.0 and not s.get("injOn", True) and not s.get("pumpOn", True)),
                   f"spray={s.get('sprayPct')} inj={s.get('injOn')} pump={s.get('pumpOn')}")
        runner.add("logic_baseline_no_cut", (s.get("boostOn", False) and not s.get("iatOn", True)),
                   f"boostOn={s.get('boostOn')} iatOn={s.get('iatOn')}")

        clear_forces(base, level_mode=1, map_kpa=0, rail_psi=0)
        time.sleep(0.3)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_level_low_before_spray_boost_only",
                   ((not s.get("boostOn", True)) and (not s.get("iatOn", True)) and s.get("blueLedOn", False) and s.get("redLedOn", False)),
                   f"boostOn={s.get('boostOn')} iatOn={s.get('iatOn')} blue={s.get('blueLedOn')} red={s.get('redLedOn')}")

        post_json(base, "/api/config", {"forceLevelFaultBypass": True})
        time.sleep(0.3)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_level_fault_bypass", (s.get("boostOn", False) and not s.get("levelFault", True)),
                   f"boostOn={s.get('boostOn')} levelFault={s.get('levelFault')}")
        post_json(base, "/api/config", {"forceLevelFaultBypass": False})

        clear_forces(base, level_mode=2, map_kpa=178, rail_psi=0)
        time.sleep(0.7)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_pressure_building_state",
                   ((not s.get("pressureReady", True)) and s.get("pumpOn", False) and not s.get("injOn", True)),
                   f"ready={s.get('pressureReady')} pump={s.get('pumpOn')} inj={s.get('injOn')}")

        post_json(base, "/api/config", {"forceRailPsi": 120})
        ok, s = wait_status(base, lambda x: bool(x.get("pressureReady", False)) and bool(x.get("injOn", False)), timeout_s=4.0)
        runner.add("logic_pressure_ready_state", ok, f"ready={None if s is None else s.get('pressureReady')} inj={None if s is None else s.get('injOn')}")

        post_json(base, "/api/config", {"forceLevel": 1})
        ok, s = wait_status(base, lambda x: bool(x.get("iatOn", False)) and (not bool(x.get("boostOn", True))), timeout_s=2.0)
        runner.add("logic_level_low_during_spray_timing_and_boost_cut", ok,
                   f"iatOn={None if s is None else s.get('iatOn')} boostOn={None if s is None else s.get('boostOn')}")
        post_json(base, "/api/config", {"forceLevel": 2})
        ok, s = wait_status(base, lambda x: (not bool(x.get("iatOn", True))) and bool(x.get("boostOn", False)), timeout_s=3.0)
        runner.add("logic_level_recovery_clears_cuts", ok,
                   f"iatOn={None if s is None else s.get('iatOn')} boostOn={None if s is None else s.get('boostOn')}")

        clear_forces(base, level_mode=2, map_kpa=0, rail_psi=0)
        time.sleep(1.0)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_dp_fault_before_spray_inactive", ((not s.get("dpFault", True)) and (not s.get("iatOn", True))),
                   f"dpFault={s.get('dpFault')} iatOn={s.get('iatOn')}")

        clear_forces(base, level_mode=2, map_kpa=178, rail_psi=120)
        ok, _ = wait_status(base, lambda x: bool(x.get("pressureReady", False)), timeout_s=3.0)
        runner.add("logic_dp_fault_during_spray_arm_ready", ok, "ready reached" if ok else "ready timeout")
        post_json(base, "/api/config", {"forceRailPsi": 20})
        ok, s = wait_status(base, lambda x: bool(x.get("dpFault", False)) and bool(x.get("iatOn", False)), timeout_s=3.0)
        runner.add("logic_dp_fault_during_spray_latches", ok, f"dpFault={None if s is None else s.get('dpFault')} iatOn={None if s is None else s.get('iatOn')}")
        ok, s = wait_status(base, lambda x: (not bool(x.get("dpFault", True))) and bool(x.get("dpBoostHold", False)) and (not bool(x.get("iatOn", True))), timeout_s=3.0)
        runner.add("logic_timing_auto_clear_boost_hold_retained", ok,
                   f"dpFault={None if s is None else s.get('dpFault')} dpBoostHold={None if s is None else s.get('dpBoostHold')} iatOn={None if s is None else s.get('iatOn')}")

        post_json(base, "/api/config", {"forceDpBoostHoldBypass": True})
        time.sleep(0.3)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_dp_hold_bypass", bool(s.get("boostOn", False)), f"boostOn={s.get('boostOn')}")
        post_json(base, "/api/config", {"forceDpBoostHoldBypass": False})

        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        runner.add("logic_reboot_online_after_dp_hold_test", online, f"online={online}")
        if online:
            s, _ = get_json(base, "/api/status")
            runner.add("logic_reboot_clears_dp_hold", not bool(s.get("dpBoostHold", True)), f"dpBoostHold={s.get('dpBoostHold')}")

        # Critical path check.
        clear_forces(base, level_mode=2, map_kpa=178, rail_psi=120)
        ok, _ = wait_status(base, lambda x: bool(x.get("pressureReady", False)), timeout_s=3.0)
        runner.add("logic_critical_path_arm_ready", ok, "ready reached" if ok else "ready timeout")
        post_json(base, "/api/config", {"forceRailPsi": 10})
        ok, s = wait_status(
            base,
            lambda x: (not bool(x.get("boostOn", True))) and ("Rail dP Fault" in str(x.get("faultReason", ""))),
            timeout_s=2.0
        )
        runner.add("logic_dp_critical_fast_fault", ok,
                   f"faultReason={None if s is None else s.get('faultReason')} boostOn={None if s is None else s.get('boostOn')}")

        # Pressure-ready timeout fault path (explicit).
        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        runner.add("logic_reboot_online_before_pressure_ready_timeout_fault", online, f"online={online}")
        if not online:
            runner.add("logic_pressure_ready_timeout_fault", False, "board offline before test")
        else:
            clear_forces(base, level_mode=2, map_kpa=178, rail_psi=0)
            post_json(base, "/api/config", {"pressureReadyTimeoutMs": 400})
            ok, s = wait_status(base, lambda x: bool(x.get("dpFault", False)) and bool(x.get("iatOn", False)), timeout_s=2.5)
            runner.add("logic_pressure_ready_timeout_fault", ok,
                       f"dpFault={None if s is None else s.get('dpFault')} iatOn={None if s is None else s.get('iatOn')}")
            post_json(base, "/api/config", {"pressureReadyTimeoutMs": 2500})

        # Manual states.
        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        runner.add("logic_reboot_online_before_manual_states", online, f"online={online}")
        clear_forces(base, level_mode=2, map_kpa=0, rail_psi=0)
        post_json(base, "/api/config", {"forceBoostCut": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_manual_boost_cut", ((not s.get("boostOn", True)) and s.get("redLedOn", False)),
                   f"boostOn={s.get('boostOn')} red={s.get('redLedOn')}")
        post_json(base, "/api/config", {"forceBoostCut": False})

        post_json(base, "/api/config", {"forceTimingCut": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_manual_timing_cut", (s.get("iatOn", False) and (not s.get("boostOn", True))),
                   f"iatOn={s.get('iatOn')} boostOn={s.get('boostOn')}")
        post_json(base, "/api/config", {"forceTimingCut": False})

        post_json(base, "/api/config", {"forceDpFault": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_manual_dp_fault", (s.get("dpFault", False) and s.get("iatOn", False)),
                   f"dpFault={s.get('dpFault')} iatOn={s.get('iatOn')}")
        post_json(base, "/api/config", {"forceDpFault": False})

        post_json(base, "/api/config", {"forcePressureReadyFault": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        reason = str(s.get("faultReason", ""))
        runner.add("logic_manual_pressure_ready_fault", (s.get("iatOn", False) and ("Pressure-Ready" in reason)),
                   f"iatOn={s.get('iatOn')} reason={reason}")
        post_json(base, "/api/config", {"forcePressureReadyFault": False})

        post_json(base, "/api/config", {"forceDpBoostHold": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_manual_dp_boost_hold", (s.get("dpBoostHold", False) and (not s.get("boostOn", True)) and (not s.get("iatOn", True))),
                   f"dpBoostHold={s.get('dpBoostHold')} boostOn={s.get('boostOn')} iatOn={s.get('iatOn')}")
        post_json(base, "/api/config", {"forceDpBoostHold": False})

        post_json(base, "/api/config", {"forceDpMonitorOverride": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_dp_monitor_override_state", (str(s.get("dpState", "")) == "OVERRIDE"),
                   f"dpState={s.get('dpState')}")
        post_json(base, "/api/config", {"forceDpMonitorOverride": False})

        # Pressure-ready override + dP override map-only spray.
        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        runner.add("logic_reboot_online_before_map_only_override_test", online, f"online={online}")
        clear_forces(base, level_mode=2, map_kpa=178, rail_psi=0)
        post_json(base, "/api/config", {"forcePressureReadyOverride": True, "forceDpMonitorOverride": True})
        ok, s = wait_status(base, lambda x: bool(x.get("injOn", False)) and bool(x.get("pressureReady", False)), timeout_s=3.0)
        runner.add("logic_map_only_spray_with_required_overrides", ok,
                   f"injOn={None if s is None else s.get('injOn')} ready={None if s is None else s.get('pressureReady')}")
        post_json(base, "/api/config", {"forcePressureReadyOverride": False, "forceDpMonitorOverride": False})

        # dP fault bypass.
        clear_forces(base, level_mode=2, map_kpa=0, rail_psi=0)
        post_json(base, "/api/config", {"forceDpFault": True, "forceDpFaultBypass": True})
        time.sleep(0.25)
        s, _ = get_json(base, "/api/status")
        runner.add("logic_dp_fault_bypass_manual_fault", ((not s.get("dpFault", True)) and (not s.get("iatOn", True))),
                   f"dpFault={s.get('dpFault')} iatOn={s.get('iatOn')}")
        post_json(base, "/api/config", {"forceDpFault": False, "forceDpFaultBypass": False})

        # Zero-save regressions.
        zero_payload = {
            "pressureReadyTimeoutMs": 0,
            "mapOff": 0,
            "pOff": 0,
            "sdbg": 0,
            "dpArmPct": 0,
            "dpRecover": 0,
        }
        post_json(base, "/api/config", zero_payload)
        cfg_zero, _ = get_json(base, "/api/config")
        zero_ok = (
            int(cfg_zero.get("pressureReadyTimeoutMs", -1)) == 0
            and float(cfg_zero.get("mapOff", 999)) == 0.0
            and float(cfg_zero.get("pOff", 999)) == 0.0
            and int(bool(cfg_zero.get("sdbg", True))) == 0
            and float(cfg_zero.get("dpArmPct", 999)) == 0.0
            and float(cfg_zero.get("dpRecover", 999)) == 0.0
        )
        runner.add("logic_zero_value_save_roundtrip", zero_ok, str({
            "pressureReadyTimeoutMs": cfg_zero.get("pressureReadyTimeoutMs"),
            "mapOff": cfg_zero.get("mapOff"),
            "pOff": cfg_zero.get("pOff"),
            "sdbg": cfg_zero.get("sdbg"),
            "dpArmPct": cfg_zero.get("dpArmPct"),
            "dpRecover": cfg_zero.get("dpRecover"),
        }))
        post_json(base, "/api/config", {
            "pressureReadyTimeoutMs": restore_cfg["pressureReadyTimeoutMs"],
            "mapOff": restore_cfg["mapOff"],
            "pOff": restore_cfg["pOff"],
            "sdbg": restore_cfg["sdbg"],
            "dpArmPct": restore_cfg["dpArmPct"],
            "dpRecover": restore_cfg["dpRecover"],
        })

        # ADC trim write/read/restore.
        cfg_before, _ = get_json(base, "/api/config")
        ag = float(cfg_before.get("adcGain", 0.98))
        ao = float(cfg_before.get("adcOffset", 0.0))
        ag_t = round(ag + 0.01, 4)
        ao_t = round(ao - 0.01, 3)
        post_json(base, "/api/config", {"adcGain": ag_t, "adcOffset": ao_t})
        cfg_after, _ = get_json(base, "/api/config")
        ok_set = abs(float(cfg_after.get("adcGain", 0)) - ag_t) < 1e-6 and abs(float(cfg_after.get("adcOffset", 0)) - ao_t) < 1e-6
        post_json(base, "/api/config", {"adcGain": ag, "adcOffset": ao})
        cfg_restore, _ = get_json(base, "/api/config")
        ok_restore = abs(float(cfg_restore.get("adcGain", 0)) - ag) < 1e-6 and abs(float(cfg_restore.get("adcOffset", 0)) - ao) < 1e-6
        runner.add("logic_adc_trim_roundtrip", (ok_set and ok_restore), f"set={ok_set} restore={ok_restore}")

        # Captive redirect target.
        try:
            class NoRedirect(urllib.request.HTTPErrorProcessor):
                def http_response(self, request, response):
                    return response
                https_response = http_response

            opener = urllib.request.build_opener(NoRedirect)
            req = urllib.request.Request(urljoin(base, "/not-a-real-path"), method="GET")
            resp = opener.open(req, timeout=3)
            loc = resp.headers.get("Location", "")
            code = int(resp.getcode())
            runner.add("logic_captive_redirect_unknown_path", (code == 302 and loc.startswith("http://")), f"code={code} location={loc}")
        except urllib.error.HTTPError as e:
            loc = e.headers.get("Location", "")
            runner.add("logic_captive_redirect_unknown_path", (e.code == 302 and loc.startswith("http://")), f"code={e.code} location={loc}")
        except Exception as e:
            runner.add("logic_captive_redirect_unknown_path", False, f"error={e.__class__.__name__}")

        # mDNS and STA-IP reachability pre and post reboot.
        cfg_now, _ = get_json(base, "/api/config")
        status_now, _ = get_json(base, "/api/status")
        mdns_host = str(cfg_now.get("mdnsHost", "watermeth"))
        sta_ip = parse_sta_ip(status_now.get("netLine", ""))
        ip_ok = False
        local_ok = False
        if sta_ip:
            try:
                _, code_ip, _ = http_get_text(f"http://{sta_ip}", "/", timeout=4)
                ip_ok = code_ip == 200
            except Exception:
                ip_ok = False
        try:
            _, code_local, _ = http_get_text(f"http://{mdns_host}.local", "/", timeout=4)
            local_ok = code_local == 200
        except Exception:
            local_ok = False
        runner.add("logic_mdns_gate_pre_reboot", (ip_ok and local_ok), f"ip_ok={ip_ok} local_ok={local_ok} sta_ip={sta_ip}")

        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        runner.add("logic_reboot_online_mdns_gate", online, f"online={online}")
        if online:
            cfg_now, _ = get_json(base, "/api/config")
            status_now, _ = get_json(base, "/api/status")
            mdns_host = str(cfg_now.get("mdnsHost", "watermeth"))
            sta_ip = parse_sta_ip(status_now.get("netLine", ""))
            ip_ok = False
            local_ok = False
            if sta_ip:
                try:
                    _, code_ip, _ = http_get_text(f"http://{sta_ip}", "/", timeout=4)
                    ip_ok = code_ip == 200
                except Exception:
                    ip_ok = False
            try:
                _, code_local, _ = http_get_text(f"http://{mdns_host}.local", "/", timeout=4)
                local_ok = code_local == 200
            except Exception:
                local_ok = False
            runner.add("logic_mdns_gate_post_reboot", (ip_ok and local_ok), f"ip_ok={ip_ok} local_ok={local_ok} sta_ip={sta_ip}")

        # sdbg persistence across reboot.
        post_json(base, "/api/config", {"sdbg": 0})
        cfg_tmp, _ = get_json(base, "/api/config")
        pre_ok = (bool(cfg_tmp.get("sdbg", True)) is False)
        reboot_board(base)
        online = wait_online(base, timeout_s=45)
        post_ok = False
        if online:
            cfg_tmp, _ = get_json(base, "/api/config")
            post_ok = (bool(cfg_tmp.get("sdbg", True)) is False)
        post_json(base, "/api/config", {"sdbg": restore_cfg["sdbg"]})
        runner.add("logic_sdbg_persistence_reboot", (pre_ok and online and post_ok), f"pre={pre_ok} online={online} post={post_ok}")

        # Serial status field checks.
        lines = capture_serial(seconds=3.0)
        stat_lines = [l for l in lines if l.startswith("[STAT]")]
        runner.add("serial_stat_present", len(stat_lines) > 0, f"count={len(stat_lines)}")
        if stat_lines:
            one = stat_lines[-1]
            for token in ["mapRaw=", "curve=", "lvlRaw=", "fPump=", "fInj=", "fLvl=", "fMap=", "fRail=", "fDp=", "ovrDp=", "ovrPr=", "bypLvl=", "bypDp=", "bypHold="]:
                runner.add(f"serial_stat_token_{token[:-1]}", token in one, "present" if token in one else "missing")

    finally:
        # Restore settings that were intentionally changed during test.
        try:
            clear_forces(base, level_mode=0, map_kpa=-1.0, rail_psi=-1.0, dp_psi=-1.0)
        except Exception:
            pass
        try:
            post_json(base, "/api/config", restore_cfg)
        except Exception:
            pass

    out = {"base": base}
    out.update(runner.summary())
    print(json.dumps(out, separators=(",", ":")))


if __name__ == "__main__":
    main()
