"""Phase-1 audit: live SampleZ contributions vs GradeToZ compression.

Reconstructs AdoptPage.h SampleGrade/SampleZ (pre-composition) from banked
orographic.phase17 page+context. Does not change any C++.
"""
from __future__ import annotations

import json
import math
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LIVE = os.path.join(ROOT, "Data", "Worldgen", "orographic_phase17", "live")
OUT = os.path.join(ROOT, "Docs", "provenance_terrain_vertical_hierarchy_audit.txt")

GRADE_MIN, GRADE_MAX = 0.15, 2.60
CREST_SHARPEN = 0.16
SHOULDER_DROP = 0.09
SPUR_TAPER = 0.62
SADDLE_NECK = 0.10
RIDGE_REACH = 1.6
SADDLE_NECK_REACH = 1.25
DATUM, RELIEF, VOXEL = 0.5, 64.0, 0.125
K_GZ = RELIEF * VOXEL  # 8 m per grade unit


def smoothstep(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def falloff(d, r):
    if r <= 0.0 or d >= r:
        return 0.0
    return smoothstep(1.0 - d / r)


def dist_poly(pts, x, y):
    best, best_t = 1e300, 0.0
    if len(pts) < 2:
        return best, 0.0
    total = sum(math.hypot(pts[i + 1][0] - pts[i][0], pts[i + 1][1] - pts[i][1]) for i in range(len(pts) - 1))
    if total <= 0:
        total = 1.0
    run = 0.0
    for i in range(len(pts) - 1):
        ax, ay = pts[i]
        bx, by = pts[i + 1]
        vx, vy = bx - ax, by - ay
        L = math.hypot(vx, vy)
        if L == 0:
            d = math.hypot(x - ax, y - ay)
            if d < best:
                best, best_t = d, run / total
            continue
        f = max(0.0, min(1.0, ((x - ax) * vx + (y - ay) * vy) / (L * L)))
        px, py = ax + vx * f, ay + vy * f
        d = math.hypot(x - px, y - py)
        if d < best:
            best, best_t = d, (run + f * L) / total
        run += L
    return best, best_t


def load_pages():
    pages = []
    for name in (
        "canonical_orographic_page_1_1.json",
        "canonical_orographic_page_2_1.json",
    ):
        path = os.path.join(LIVE, name)
        if os.path.isfile(path):
            pages.append(json.load(open(path, encoding="utf-8")))
    ctx = json.load(open(os.path.join(LIVE, "canonical_orographic_context_1_1.json"), encoding="utf-8"))
    return pages, ctx


def feats_of(page):
    defs = page.get("feature_definitions") or {}
    feats = page.get("features") or {}
    out = {k: list(defs.get(k) or []) for k in ("peaks", "ridges", "saddles", "spurs", "valleys")}
    for k in out:
        seen = {f.get("id") for f in out[k]}
        for f in feats.get(k) or []:
            if f.get("id") not in seen:
                out[k].append(f)
    return out


def sample_smooth(page, x, y):
    x0, y0 = page["bounds"][0], page["bounds"][1]
    st = page["step"]
    n = int(page["n"])
    smooth = page["smooth"]
    fi, fj = (x - x0) / st, (y - y0) / st
    i, j = math.floor(fi), math.floor(fj)
    u, v = fi - i, fj - j
    m = n - 1

    def at(a, b):
        a = max(0, min(m, int(a)))
        b = max(0, min(m, int(b)))
        return float(smooth[b][a])

    top = at(i, j) * (1 - u) + at(i + 1, j) * u
    bot = at(i, j + 1) * (1 - u) + at(i + 1, j + 1) * u
    return top * (1 - v) + bot * v


def page_at(pages, x, y, skirt=128.0):
    best, best_area = None, 1e300
    for p in pages:
        b = p["bounds"]
        if x < b[0] - skirt or x > b[2] + skirt or y < b[1] - skirt or y > b[3] + skirt:
            continue
        area = (b[2] - b[0]) * (b[3] - b[1])
        if area < best_area:
            best, best_area = p, area
    if best is None and pages:
        # nearest live stretch
        best_d = 1e300
        for p in pages:
            b = p["bounds"]
            cx = min(max(x, b[0]), b[2])
            cy = min(max(y, b[1]), b[3])
            d = math.hypot(x - cx, y - cy)
            if d < best_d:
                best_d, best = d, p
    return best


def sharp_terms(page, x, y):
    f = feats_of(page)
    peak_c = ridge_c = spur_c = saddle_c = 0.0
    crest = shoulder = taper = neck = 0.0
    ids = {"peak": "", "ridge": "", "spur": "", "saddle": ""}
    best = {"peak": 0.0, "ridge": 0.0, "spur": 0.0, "saddle": 0.0}
    for p in f["peaks"]:
        d = math.hypot(x - p["pos"][0], y - p["pos"][1])
        fo = falloff(d, p["radius"])
        if fo > 0:
            c = p["prominence"] * fo * fo
            peak_c += c
            if c > best["peak"]:
                best["peak"], ids["peak"] = c, p["id"]
    for r in f["ridges"]:
        d, t = dist_poly(r["axis"], x, y)
        hw = r["half_width"]
        fo = falloff(d, hw)
        if fo > 0:
            c = r["crest"] * fo
            ridge_c += c
            if c > best["ridge"]:
                best["ridge"], ids["ridge"] = c, r["id"]
        if d < hw * RIDGE_REACH:
            crest += r["crest"] * CREST_SHARPEN * falloff(d, hw * 0.45)
            if hw * 0.5 < d < hw * RIDGE_REACH:
                band = (d - hw * 0.5) / (hw * (RIDGE_REACH - 0.5))
                shoulder -= r["crest"] * SHOULDER_DROP * smoothstep(1.0 - abs(2.0 * band - 1.0))
    for sp in f["spurs"]:
        d, t = dist_poly(sp["axis"], x, y)
        fo = falloff(d, sp["half_width"])
        if fo > 0:
            c = sp["crest"] * fo
            spur_c += c
            taper -= sp["crest"] * SPUR_TAPER * t * fo
            if c > best["spur"]:
                best["spur"], ids["spur"] = c, sp["id"]
    for sd in f["saddles"]:
        d = math.hypot(x - sd["pos"][0], y - sd["pos"][1])
        fo = falloff(d, sd["radius"])
        s_term = 0.0
        if fo > 0:
            s_term -= (ridge_c + peak_c) * sd["drop"] * fo
        fn = falloff(d, sd["radius"] * SADDLE_NECK_REACH)
        if fn > 0:
            neck -= sd["drop"] * SADDLE_NECK * fn * fn
        saddle_c += s_term
        mag = abs(s_term) + abs(neck)
        if mag > best["saddle"]:
            best["saddle"], ids["saddle"] = mag, sd["id"]
    total = peak_c + ridge_c + spur_c + saddle_c + crest + shoulder + taper + neck
    return {
        "peak": peak_c, "ridge": ridge_c, "spur": spur_c, "saddle": saddle_c + neck,
        "crest": crest, "shoulder": shoulder, "taper": taper, "neck": neck,
        "total": total, **{f"{k}_id": v for k, v in ids.items()},
    }


def massif_grade(ctx, x, y):
    s = 0.0
    best_id, best = "", 0.0
    for m in ctx.get("massifs") or []:
        d = math.hypot(x - m["centre"][0], y - m["centre"][1])
        c = m["lift"] * falloff(d, m["radius"])
        s += c
        if c > best:
            best, best_id = c, m["id"]
    return s, best_id


def nearest_valley(pages, ctx, x, y):
    best, vid = 1e300, ""
    for p in pages:
        for v in feats_of(p)["valleys"]:
            pos = v.get("pos") or [0, 0]
            d = math.hypot(x - pos[0], y - pos[1])
            if d < best:
                best, vid = d, v["id"]
    for v in ctx.get("valleys") or []:
        pos = v.get("outlet") or v.get("pos") or [0, 0]
        d = math.hypot(x - pos[0], y - pos[1])
        if d < best:
            best, vid = d, v["id"]
    return vid, best


def trace(pages, ctx, x, y):
    page = page_at(pages, x, y)
    if page is None:
        return None
    carrier = sample_smooth(page, x, y)
    sh = sharp_terms(page, x, y)
    massif_g, massif_id = massif_grade(ctx, x, y)
    vid, vd = nearest_valley(pages, ctx, x, y)
    raw = carrier + sh["total"]
    grade = max(GRADE_MIN, min(GRADE_MAX, raw))
    z = (grade - DATUM) * K_GZ
    return {
        "x": x, "y": y, "px": page.get("page"),
        "carrier": carrier,
        "massif_grade_unused": massif_g, "massif_id": massif_id,
        "peak": sh["peak"], "ridge": sh["ridge"], "saddle": sh["saddle"],
        "spur": sh["spur"], "crest": sh["crest"], "shoulder": sh["shoulder"],
        "taper": sh["taper"], "sharp_total": sh["total"],
        "valley_term": 0.0, "valley_id": vid, "valley_dist": vd,
        "raw_unclamped": raw, "clamped": raw - grade,
        "grade": grade, "z": z, "collision_z": z,
        "peak_id": sh["peak_id"], "ridge_id": sh["ridge_id"],
        "saddle_id": sh["saddle_id"], "spur_id": sh["spur_id"],
        "z_if_massif_in_grade": (max(GRADE_MIN, min(GRADE_MAX, raw + massif_g)) - DATUM) * K_GZ,
    }


def fmt(t):
    if t is None:
        return "MISS"
    return (
        f"xy=({t['x']:.1f},{t['y']:.1f}) page={t['px']}\n"
        f"  carrier={t['carrier']:.6f}  massif_UNUSED={t['massif_grade_unused']:.6f} ({t['massif_id']})\n"
        f"  peak={t['peak']:.6f} ({t['peak_id']})  ridge={t['ridge']:.6f} ({t['ridge_id']})\n"
        f"  saddle={t['saddle']:.6f} ({t['saddle_id']})  spur={t['spur']:.6f} ({t['spur_id']})\n"
        f"  valley_term=0 (id={t['valley_id']} dist={t['valley_dist']:.1f}m)\n"
        f"  sharp_total={t['sharp_total']:.6f}  raw={t['raw_unclamped']:.6f}  clamp_delta={t['clamped']:.6f}\n"
        f"  grade={t['grade']:.6f}  GradeToZ={t['z']:.3f}m  collision={t['collision_z']:.3f}m\n"
        f"  GradeToZ_if_massif_added={t['z_if_massif_in_grade']:.3f}m  (still *8 m/grade)"
    )


def main():
    pages, ctx = load_pages()
    lines = []
    def log(s=""):
        lines.append(s)
        print(s)

    log("TERRAIN VERTICAL HIERARCHY / PHASE 1 AUDIT")
    log(f"K_GZ = relief*voxel = {K_GZ} m per grade unit")
    log(f"grade domain [{GRADE_MIN},{GRADE_MAX}] -> Z [{(GRADE_MIN-DATUM)*K_GZ:.2f}, {(GRADE_MAX-DATUM)*K_GZ:.2f}] m")
    log(f"theoretical max relief = {(GRADE_MAX-GRADE_MIN)*K_GZ:.2f} m")
    log("")

    # collect representative features from all pages + context
    peaks, ridges, saddles, spurs, valleys = [], [], [], [], []
    for p in pages:
        f = feats_of(p)
        peaks.extend(f["peaks"])
        ridges.extend(f["ridges"])
        saddles.extend(f["saddles"])
        spurs.extend(f["spurs"])
        valleys.extend(f["valleys"])
    # unique by id
    def uniq(arr, key="id"):
        seen, out = set(), []
        for a in arr:
            i = a.get(key)
            if i in seen:
                continue
            seen.add(i)
            out.append(a)
        return out
    peaks, ridges, saddles, spurs, valleys = map(uniq, (peaks, ridges, saddles, spurs, valleys))
    peaks.sort(key=lambda p: -p["prominence"])
    ridges.sort(key=lambda r: -r["crest"])
    saddles.sort(key=lambda s: -s["drop"])
    spurs.sort(key=lambda s: -s["crest"])
    valleys.sort(key=lambda v: -v.get("accumulation", 0))

    log(f"page peaks={len(peaks)} ridges={len(ridges)} saddles={len(saddles)} spurs={len(spurs)} valleys={len(valleys)}")
    log(f"ctx massifs={len(ctx.get('massifs') or [])} peaks={len(ctx.get('peaks') or [])}")
    log("peak prominence range: %.4f .. %.4f -> Z contrib %.2f .. %.2f m" % (
        peaks[-1]["prominence"], peaks[0]["prominence"],
        peaks[-1]["prominence"] * K_GZ, peaks[0]["prominence"] * K_GZ))
    log("ridge crest range: %.4f .. %.4f -> Z contrib %.2f .. %.2f m" % (
        ridges[-1]["crest"], ridges[0]["crest"],
        ridges[-1]["crest"] * K_GZ, ridges[0]["crest"] * K_GZ))
    lifts = [m["lift"] for m in ctx["massifs"]]
    log("massif lift range: %.4f .. %.4f -> Z if used %.2f .. %.2f m" % (
        min(lifts), max(lifts), min(lifts) * K_GZ, max(lifts) * K_GZ))

    # spawn
    spawn = trace(pages, ctx, 1536.0, 1536.0)
    log("\n=== SPAWN 1536,1536 (ordinary / page 1,1 interior) ===")
    log(fmt(spawn))

    # dominant peak
    dp = peaks[0]
    t_peak = trace(pages, ctx, dp["pos"][0], dp["pos"][1])
    log(f"\n=== DOMINANT PEAK {dp['id']} prom={dp['prominence']:.4f} r={dp['radius']:.1f} ===")
    log(fmt(t_peak))

    # secondary
    spk = peaks[3] if len(peaks) > 3 else peaks[-1]
    t_sec = trace(pages, ctx, spk["pos"][0], spk["pos"][1])
    log(f"\n=== SECONDARY PEAK {spk['id']} prom={spk['prominence']:.4f} ===")
    log(fmt(t_sec))

    # ridge crest midpoint
    rd = ridges[0]
    mid = rd["axis"][len(rd["axis"]) // 2]
    t_ridge = trace(pages, ctx, mid[0], mid[1])
    log(f"\n=== RIDGE CREST {rd['id']} crest={rd['crest']:.4f} hw={rd['half_width']:.1f} ===")
    log(fmt(t_ridge))

    # adjacent drainage: offset perpendicular to ridge
    ax = rd["axis"]
    dx, dy = ax[-1][0] - ax[0][0], ax[-1][1] - ax[0][1]
    L = math.hypot(dx, dy) or 1.0
    px, py = -dy / L, dx / L
    drain_pt = (mid[0] + px * rd["half_width"] * 1.35, mid[1] + py * rd["half_width"] * 1.35)
    t_drain = trace(pages, ctx, drain_pt[0], drain_pt[1])
    log(f"\n=== ADJACENT DRAINAGE to ridge (offset 1.35*hw) ===")
    log(fmt(t_drain))

    sd = saddles[0]
    t_saddle = trace(pages, ctx, sd["pos"][0], sd["pos"][1])
    log(f"\n=== SADDLE {sd['id']} drop={sd['drop']:.4f} ===")
    log(fmt(t_saddle))

    # neighboring peaks to saddle
    near_peaks = sorted(peaks, key=lambda p: math.hypot(p["pos"][0] - sd["pos"][0], p["pos"][1] - sd["pos"][1]))[:2]
    t_np = []
    for p in near_peaks:
        tp = trace(pages, ctx, p["pos"][0], p["pos"][1])
        t_np.append(tp)
        log(f"\n=== SADDLE-NEIGHBOR PEAK {p['id']} ===")
        log(fmt(tp))

    su = spurs[0]
    smid = (
        0.5 * (su["axis"][0][0] + su["axis"][-1][0]),
        0.5 * (su["axis"][0][1] + su["axis"][-1][1]),
    )
    t_spur = trace(pages, ctx, smid[0], smid[1])
    log(f"\n=== SPUR {su['id']} crest={su['crest']:.4f} ===")
    log(fmt(t_spur))

    vl = valleys[0]
    t_val = trace(pages, ctx, vl["pos"][0], vl["pos"][1])
    log(f"\n=== VALLEY {vl['id']} acc={vl.get('accumulation')} pos={vl['pos']} ===")
    log(fmt(t_val))

    # valley nearer the dominant peak
    best_v, best_d = None, 1e300
    for v in valleys:
        d = math.hypot(v["pos"][0] - dp["pos"][0], v["pos"][1] - dp["pos"][1])
        if d < best_d:
            best_d, best_v = d, v
    # also ctx valleys by outlet
    for v in ctx.get("valleys") or []:
        pos = v.get("outlet") or [0, 0]
        d = math.hypot(pos[0] - dp["pos"][0], pos[1] - dp["pos"][1])
        if d < best_d:
            best_d, best_v = d, {"id": v["id"], "pos": pos, "accumulation": v.get("max_accumulation", 0)}
    t_near_v = trace(pages, ctx, best_v["pos"][0], best_v["pos"][1]) if best_v else None
    log(f"\n=== NEAREST VALLEY TO DOMINANT PEAK {best_v and best_v['id']} d={best_d:.1f}m ===")
    log(fmt(t_near_v))

    # local hill: small peak
    t_hill = t_sec
    # ordinary non-feature: spawn plus a point far from features
    # search a grid on page 1,1 for min sharp
    min_sh, ord_xy = 1e9, (1536, 1536)
    b = pages[0]["bounds"]
    for y in range(int(b[1] + 80), int(b[3]), 64):
        for x in range(int(b[0] + 80), int(b[2]), 64):
            t = trace(pages, ctx, x, y)
            if t and abs(t["sharp_total"]) < min_sh and t["massif_grade_unused"] < 0.02:
                min_sh, ord_xy = abs(t["sharp_total"]), (x, y)
    t_ord = trace(pages, ctx, ord_xy[0], ord_xy[1])
    log(f"\n=== ORDINARY NON-FEATURE (min sharp, low massif) {ord_xy} ===")
    log(fmt(t_ord))

    log("\n========== WORLD-SPACE RELIEF (current GradeToZ) ==========")
    if t_peak and t_near_v:
        log(f"dominant peak to nearby valley: {t_peak['z'] - t_near_v['z']:.3f} m   (peakZ={t_peak['z']:.3f} valleyZ={t_near_v['z']:.3f})")
    if t_peak and t_val:
        log(f"dominant peak to page-max-acc valley: {t_peak['z'] - t_val['z']:.3f} m")
    if t_ridge and t_drain:
        log(f"ridge crest to adjacent drainage: {t_ridge['z'] - t_drain['z']:.3f} m")
    if t_saddle and t_np:
        log(f"saddle to neighbor peak A: {t_np[0]['z'] - t_saddle['z']:.3f} m")
        if len(t_np) > 1:
            log(f"saddle to neighbor peak B: {t_np[1]['z'] - t_saddle['z']:.3f} m")
    if t_spur and t_drain:
        log(f"local spur vs drainage: {t_spur['z'] - t_drain['z']:.3f} m")
    if t_ord:
        # local variation around ordinary point
        zs = []
        for dx, dy in ((0, 0), (80, 0), (-80, 0), (0, 80), (0, -80), (40, 40)):
            tt = trace(pages, ctx, t_ord["x"] + dx, t_ord["y"] + dy)
            if tt:
                zs.append(tt["z"])
        log(f"ordinary non-feature local relief (80m neighborhood): {max(zs) - min(zs):.3f} m  z=[{min(zs):.3f},{max(zs):.3f}]")
    if spawn:
        log(f"spawn local Z={spawn['z']:.3f} m")

    # smooth field stats in metres
    sm = []
    for p in pages:
        for row in p["smooth"]:
            sm.extend(row)
    log(f"\nsmooth carrier grade [{min(sm):.4f},{max(sm):.4f}] -> Z [{(min(sm)-DATUM)*K_GZ:.3f},{(max(sm)-DATUM)*K_GZ:.3f}] m")
    log(f"smooth step={pages[0]['step']} n={pages[0]['n']}  (no sub-500m local carrier)")

    log("\n========== WHERE THE ~10 m COMPRESSION OCCURS ==========")
    log("1. Feature amplitudes are GRADE units: peak prom~0.28-0.60, ridge crest~0.19-0.35,")
    log("   massif lift~0.22-0.26, smooth~0.45-1.01. Designed as tectonic.grade_at, not metres.")
    log("2. GradeToZ is the SINGLE scale: Z=(grade-0.5)*64*0.125 = *8 m/grade.")
    log("   Peak 0.60 * 8 = 4.8 m. Carrier contrast 0.56 * 8 = 4.5 m. Together ~10 m.")
    log("3. Massif lift is computed in SharpFromContext (ecology) but NOT added to SampleZ.")
    log("   Even if added, 0.26*8 = 2.1 m — still compressed by the same knob.")
    log("4. kGradeMax=2.60 clamp is NOT the live cause (raw at dominant peak stays < 2.3).")
    log("5. Valley incision term in SampleZ is hard-coded 0; drainage lives in 500 m smooth.")
    log("DO NOT globally multiply the reconstructed grade field: that would scale local")
    log("carrier and features together and turn lowlands into mountains.")

    # profile cone check: radial samples from dominant peak
    log("\n========== PROFILE SHAPE (current peak function = isotropic f^2) ==========")
    r = dp["radius"]
    log(f"peak radius={r:.1f}m  samples along +X:")
    for k in range(0, 6):
        d = r * k / 5.0
        tt = trace(pages, ctx, dp["pos"][0] + d, dp["pos"][1])
        log(f"  d={d:7.1f} peakC={tt['peak']:.4f} Z={tt['z']:.3f}")
    log("ridge cross-section (perp to first segment):")
    a0, a1 = rd["axis"][0], rd["axis"][1]
    vx, vy = a1[0] - a0[0], a1[1] - a0[1]
    L = math.hypot(vx, vy) or 1
    nx, ny = -vy / L, vx / L
    mx, my = 0.5 * (a0[0] + a1[0]), 0.5 * (a0[1] + a1[1])
    hw = rd["half_width"]
    for k in range(-5, 6):
        d = hw * k / 5.0
        tt = trace(pages, ctx, mx + nx * d, my + ny * d)
        log(f"  d={d:7.1f} ridgeC={tt['ridge']:.4f} Z={tt['z']:.3f}")

    # 10 km novelty along +X from spawn
    log("\n========== 10 km TRAVERSAL NOVELTY (current IDs / massif / valley) ==========")
    last_key = None
    changes = []
    x0, y0 = 1536.0, 1536.0
    dist = 10000.0
    step = 50.0
    n = int(dist / step)
    for i in range(n + 1):
        x = x0 + i * step
        t = trace(pages, ctx, x, y0)
        if not t:
            continue
        key = (t["massif_id"], t["peak_id"], t["ridge_id"], t["valley_id"])
        if last_key is None:
            last_key = key
            continue
        if key != last_key:
            changes.append((i * step, last_key, key, t["z"]))
            last_key = key
    gaps = [changes[i][0] - (changes[i - 1][0] if i else 0.0) for i in range(len(changes))]
    log(f"line ({x0},{y0}) +X {dist}m, step {step}m, context-id changes={len(changes)}")
    if gaps:
        log(f"gap min/median/max = {min(gaps):.0f} / {sorted(gaps)[len(gaps)//2]:.0f} / {max(gaps):.0f} m")
        log(f"mean gap = {sum(gaps)/len(gaps):.0f} m")
    for c in changes[:25]:
        log(f"  @{c[0]:.0f}m Z={c[3]:.2f} {c[1]} -> {c[2]}")

    log("\n========== PROPOSED SEAM (not yet implemented) ==========")
    log("Keep SampleGrade / SharpFromFeatures / kGradeMinMax / _grade_at UNCHANGED")
    log("(MW8 ecology + carrier cert stay in grade domain).")
    log("New explicit metre composition:")
    log("  TerrainElevationComponents { regional_z, massif_z, peak_z, ridge_z,")
    log("    saddle_z, spur_z, valley_z, local_z }")
    log("  FinalSurfaceZ = sum(components)   // independently scaled")
    log("  SampleZ := compose  (NOT GradeToZ(SampleGrade))")
    log("Scales (m per existing grade-unit amplitude):")
    log("  regional/local smooth residual: 8 m/grade (current, metres-tens)")
    log("  massif: ~1800 m/unit  (lift 0.25 -> ~450 m envelope)")
    log("  peak:   ~720 m/unit   (prom 0.60 -> ~430 m summit add)")
    log("  ridge:  ~380 m/unit   (crest 0.35 -> ~130 m)")
    log("  spur:   ~220 m/unit")
    log("  saddle: relative fraction of local peak+ridge metres + neck")
    log("  valley: explicit incision from valley/drainage IDs (topology), not Z<sea")
    log("Profile functions: anisotropic peak (massif trend + id skew), ridge")
    log("cross-section sharp-crest/concave-shoulder with end-tapered width.")
    log("No new noise. No global grade multiply. Sea stays PresentationSeaZ=0.")
    log("Lowland = no massif/peak/ridge coverage -> only regional+local metres.")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("wrote", OUT)


if __name__ == "__main__":
    main()
