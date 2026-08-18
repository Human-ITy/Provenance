"""MV2.A / MV2.A2 certification — compile the macro-authority page ring and prove
the authority fixtures. Macro authority only; no renderer, no fine causal stack.

MV2.A2: the ring is the full +/-2 grid (5x5 = 25 pages), so neighbouring authority
reaches 128 km+ in ALL directions (cardinal as well as diagonal), not just the 3x3
diagonal corners. No new macro-generation semantics — the SAME continuous field from
macro_authority.py, just more compiled pages.

Run:  python Tools/Worldgen/cert_macro_authority.py
Emits: Data/Worldgen/MacroAuthority/page_<ri>_<rj>.mcp  (25 pages, ri,rj in -2..2)
       Docs/provenance_mv2a_macro_authority_cert.txt     (receipt, PASS/FAIL)
Exit 0 iff every fixture passes.
"""

from __future__ import annotations

import math
import time
from pathlib import Path

import macro_authority as MA
from macro_authority import (CentralProgram, MacroField, compile_page, serialize_page,
                             macro_z, central_envelope, anchor_window, REGION_M,
                             REGION_HALF_M)

ROOT = Path(__file__).resolve().parents[2]
PAGE_DIR = ROOT / "Data" / "Worldgen" / "MacroAuthority"
RECEIPT = ROOT / "Docs" / "provenance_mv2a_macro_authority_cert.txt"

RING = 2                                   # +/-2 cells => 128 km+ authority in all directions
CELLS = [(ri, rj) for rj in range(RING, -RING - 1, -1) for ri in range(-RING, RING + 1)]
NAMED = {(0, 0): "center", (0, 1): "north", (0, -1): "south", (1, 0): "east",
         (-1, 0): "west", (1, 1): "ne", (-1, 1): "nw", (1, -1): "se", (-1, -1): "sw",
         (2, 0): "east2", (-2, 0): "west2", (0, 2): "north2", (0, -2): "south2",
         (2, 2): "ne2", (-2, -2): "sw2"}


def main() -> int:
    central = CentralProgram.load()
    fieldf = MacroField.build(central.seed)
    checks: list[tuple[str, bool, str]] = []

    # Compile all 25 pages (timed → cheap-source evidence).
    PAGE_DIR.mkdir(parents=True, exist_ok=True)
    t0 = time.perf_counter()
    pages = {}                              # keyed by (ri,rj)
    for ri, rj in CELLS:
        p = compile_page(central, fieldf, ri, rj)
        pages[(ri, rj)] = p
        (PAGE_DIR / f"page_{ri}_{rj}.mcp").write_text(serialize_page(p), encoding="utf-8", newline="\n")
    compile_s = time.perf_counter() - t0
    center = pages[(0, 0)]
    samples_per_page = center.n ** 2
    ncells = len(CELLS)

    # ---- Fixture 1: determinism ------------------------------------------- #
    f2 = MacroField.build(central.seed)
    det_ok, det_detail = True, ""
    for ri, rj in CELLS:
        again = compile_page(central, f2, ri, rj)
        if again.source_digest != pages[(ri, rj)].source_digest or again.region_id != pages[(ri, rj)].region_id:
            det_ok, det_detail = False, f"({ri},{rj}) digest/id changed"
            break
    checks.append(("determinism_same_seed_same_digest", det_ok,
                   det_detail or f"all {ncells} pages reproduce digest+region_id"))

    # ---- Fixture 2: non-repetition (no 64 km tiling) ---------------------- #
    ids = [p.region_id for p in pages.values()]
    digs = [p.source_digest for p in pages.values()]
    unique_ok = len(set(ids)) == ncells and len(set(digs)) == ncells
    rms, worst, cnt = 0.0, 0.0, 0
    for j in range(-3, 4):
        for i in range(1, 5):
            x, y = i * REGION_M + 5000.0, j * REGION_M + 5000.0
            d = abs(macro_z(central, fieldf, x, y) - macro_z(central, fieldf, x + REGION_M, y))
            rms += d * d; worst = max(worst, d); cnt += 1
    rms = math.sqrt(rms / cnt)
    nonrep_ok = unique_ok and rms > 40.0
    checks.append(("non_repetition_no_64km_tiling", nonrep_ok,
                   f"unique_ids={len(set(ids))}/{ncells} unique_digests={len(set(digs))}/{ncells} "
                   f"period64_rms={rms:.1f}m worst={worst:.1f}m (need rms>40)"))

    # ---- Fixture 3: boundary continuity across the 4 CENTER edges --------- #
    eps = 1.0
    max_pos, max_slope = 0.0, 0.0
    for t in range(-31, 32):
        s = t * 1000.0
        edges = [( REGION_HALF_M, s, 1, 0), (-REGION_HALF_M, s, -1, 0),
                 ( s, REGION_HALF_M, 0, 1), ( s, -REGION_HALF_M, 0, -1)]
        for (bx, by, nx, ny) in edges:
            inside = central_envelope(central, bx, by)
            outside = macro_z(central, fieldf, bx + nx * eps, by + ny * eps)
            max_pos = max(max_pos, abs(outside - inside))
            out2 = macro_z(central, fieldf, bx + nx * 2 * eps, by + ny * 2 * eps)
            in2 = central_envelope(central, bx - nx * eps, by - ny * eps)
            max_slope = max(max_slope, abs((out2 - outside) / eps - (inside - in2) / eps))
    cont_ok = max_pos < 0.5 and max_slope < 0.05
    checks.append(("boundary_continuity_4_center_edges", cont_ok,
                   f"max_position_step={max_pos:.4f}m (need<0.5) max_slope_step={max_slope:.4f} (need<0.05)"))

    # ---- Fixture 3b: continuity across the NEW +/-1<->+/-2 ring boundaries - #
    # The field is one continuous function, so every internal page boundary is C0/C1
    # by construction; prove it explicitly at the cardinal +/-96 km seams the +/-2
    # ring introduces (x=+/-96 km, y=+/-96 km).
    # The field is one continuous analytic function, so a seam would show as an
    # ANOMALOUS step at the boundary relative to the identical-length steps just
    # either side (a true discontinuity spikes; smooth-but-steep terrain does not).
    max_excess, max_slope2 = 0.0, 0.0
    for b in (REGION_M + REGION_HALF_M, -(REGION_M + REGION_HALF_M)):   # +/-96 km
        for t in range(-90, 91, 3):
            s = t * 1000.0
            for (bx, by, nx, ny) in [(b, s, 1, 0), (s, b, 0, 1)]:
                v = [macro_z(central, fieldf, bx + nx * k * eps, by + ny * k * eps)
                     for k in (-3, -1, 1, 3)]
                across = abs(v[2] - v[1])            # 2 m centred on the boundary
                inner_p = abs(v[3] - v[2])           # 2 m just outside
                inner_m = abs(v[1] - v[0])           # 2 m just inside
                max_excess = max(max_excess, across - max(inner_p, inner_m))
                max_slope2 = max(max_slope2, abs((v[3] - v[2]) / (2 * eps) - (v[1] - v[0]) / (2 * eps)))
    ring2_ok = max_excess < 0.10 and max_slope2 < 0.05
    checks.append(("boundary_continuity_ring2_96km_seams", ring2_ok,
                   f"max_anomalous_step_vs_interior={max_excess:.4f}m (need<0.10) "
                   f"max_slope_step={max_slope2:.4f} (need<0.05)"))

    # ---- Fixture 3c: cardinal +/-2 authority reaches 128 km --------------- #
    # Prove neighbouring authority exists and is distinct out to 128 km in the four
    # CARDINAL directions (not only the diagonal corners of the old 3x3).
    card_ok = True
    card_detail = []
    for nm, (dx, dy) in (("E", (1, 0)), ("W", (-1, 0)), ("N", (0, 1)), ("S", (0, -1))):
        c2 = (2 * dx, 2 * dy)
        p2 = pages[c2]
        # farthest absolute coordinate this cardinal +/-2 page covers
        far = max(abs(p2.min_x), abs(p2.min_x + REGION_M)) if dx else max(abs(p2.min_y), abs(p2.min_y + REGION_M))
        relief = max(p2.heights) - min(p2.heights)
        distinct = p2.source_digest != pages[(dx, dy)].source_digest
        ok = far >= 128000.0 and relief > 50.0 and distinct
        card_ok = card_ok and ok
        card_detail.append(f"{nm}:reach={far/1000:.0f}km relief={relief:.0f}m distinct={distinct}")
    checks.append(("cardinal_ring2_reaches_128km", card_ok, "  ".join(card_detail)))

    # ---- Fixture 5: central anchoring (center page IS the frozen macro) --- #
    max_anchor = 0.0
    for j in range(center.n):
        for i in range(center.n):
            x, y = center.min_x + i * center.step, center.min_y + j * center.step
            max_anchor = max(max_anchor, abs(center.heights[j * center.n + i] - central_envelope(central, x, y)))
    anchor_ok = max_anchor < 0.01
    checks.append(("central_anchor_matches_frozen_macro", anchor_ok,
                   f"max|center_page - frozen_envelope|={max_anchor:.5f}m (need<0.01)"))

    # ---- Fixture 7: feature scale independence (a feature spans >64 km) --- #
    longest, which = 0.0, -1
    for bi, b in enumerate(fieldf.belts):
        a = b.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        lo = hi = None
        s = -b.half_length_m - 20000.0
        while s <= b.half_length_m + 20000.0:
            if fieldf._belt(b, b.cx + s * ux, b.cy + s * uy) > 150.0:
                lo = s if lo is None else lo; hi = s
            s += 2000.0
        if lo is not None and hi is not None and (hi - lo) > longest:
            longest, which = hi - lo, bi
    scale_ok = longest > 64000.0
    checks.append(("feature_scale_independence_gt_64km", scale_ok,
                   f"longest_belt_extent={longest/1000.0:.1f}km (belt#{which}; need>64km)"))

    # ---- Fixture 4: cross-page landform continuity (through the +/-2 ring)- #
    cross_ok = False
    cross_detail = "no belt crosses a footprint page boundary"
    footprint = RING * REGION_M + REGION_HALF_M   # +/-160 km
    for b in fieldf.belts:
        a = b.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        s = -b.half_length_m
        while s <= b.half_length_m:
            x, y = b.cx + s * ux, b.cy + s * uy
            if abs(x) > footprint or abs(y) > footprint:
                s += 1000.0; continue
            k = round((x + REGION_HALF_M) / REGION_M)
            bxr = k * REGION_M - REGION_HALF_M
            if abs(x - bxr) < 1500.0 and abs(bxr) <= footprint and anchor_window(x, y) > 0.2:
                left = macro_z(central, fieldf, bxr - 1.0, y)
                right = macro_z(central, fieldf, bxr + 1.0, y)
                here = macro_z(central, fieldf, bxr, y)
                base = central_envelope(central, bxr, y) + anchor_window(bxr, y) * fieldf._base(bxr, y)
                if abs(left - right) < 5.0 and (here - base) > 120.0:
                    cross_ok = True
                    cross_detail = (f"belt crest crosses x={bxr/1000:.0f}km page boundary; "
                                    f"jump={abs(left-right):.3f}m crest_above_base={here-base:.0f}m")
                    break
            s += 1000.0
        if cross_ok:
            break
    checks.append(("cross_page_landform_continuity", cross_ok, cross_detail))

    # ---- Fixture 6: no square-world signature (across the full +/-160 km) -- #
    yline = 9000.0
    xs = list(range(-160000, 160001, 500))
    zs = [macro_z(central, fieldf, float(x), yline) for x in xs]
    interior_max, boundary_max = 0.0, 0.0
    for idx in range(1, len(xs)):
        d = abs(zs[idx] - zs[idx - 1])
        if abs(((xs[idx] + REGION_HALF_M) % REGION_M)) < 600.0:
            boundary_max = max(boundary_max, d)
        else:
            interior_max = max(interior_max, d)
    sig_ok = boundary_max <= interior_max * 1.25 + 0.5
    checks.append(("no_square_world_signature", sig_ok,
                   f"max_step_on_page_boundary={boundary_max:.2f}m max_step_interior={interior_max:.2f}m"))

    # ---- Fixture 8: no wrap ---------------------------------------------- #
    import tokenize, io as _io
    src = (Path(MA.__file__)).read_text(encoding="utf-8")
    code_names, imports = set(), set()
    for tok in tokenize.generate_tokens(_io.StringIO(src).readline):
        if tok.type == tokenize.NAME:
            code_names.add(tok.string)
    for line in src.splitlines():
        ls = line.strip()
        if ls.startswith("import ") or ls.startswith("from "):
            imports.add(ls)
    forbidden = [t for t in ("WrapIntoRegion", "ReconstructedZ", "QueryMaterial",
                             "SampleForcing", "CausalBareEarthGeography") if t in code_names]
    stdlib_only = all(any(m in imp for m in ("math", "struct", "dataclasses", "pathlib",
                                             "typing", "annotations")) for imp in imports)
    files_declare = all("no_wrap_into_region=1" in (PAGE_DIR / f"page_{ri}_{rj}.mcp").read_text()
                        for ri, rj in CELLS)
    forbidden = forbidden if (forbidden or stdlib_only) else forbidden + ["non_stdlib_import"]
    wrap_ok = files_declare and not forbidden
    checks.append(("no_wrap_no_fine_authority", wrap_ok,
                   f"pages_declare_no_wrap={files_declare} forbidden_tokens={forbidden or 'none'}"))

    # ---- cheap-source evidence ------------------------------------------- #
    cheap_ok = compile_s < 40.0 and not forbidden
    checks.append(("cheap_source_no_deep_reconstructedz", cheap_ok,
                   f"compiled {ncells} pages ({samples_per_page} samples each) in {compile_s:.2f}s; "
                   f"macro-analytic only (no ReconstructedZ/erosion/QueryMaterial)"))

    passed = all(ok for _, ok, _ in checks)

    lines = ["MV2A_REGIONAL_MACRO_AUTHORITY " + ("PASS" if passed else "FAIL"),
             "scope=macro_forcing_only_no_renderer_no_fine_causal_stack",
             f"authority_ring=+/-{RING}_cells  pages_compiled={ncells}  (full 128 km radial authority)",
             f"world_seed={central.seed}",
             f"generator_version={MA.GENERATOR_VERSION}",
             f"central_region_key={central.region_key}",
             f"central_world_identity_hash={central.world_identity_hash}",
             f"page_cell_size_m={REGION_M:.0f}  (packaging only; feature wavelengths independent)",
             f"sample_step_m={center.step:.0f}  samples_per_page={samples_per_page}",
             f"compile_seconds={compile_s:.3f}",
             ""]
    for name, ok, detail in checks:
        lines.append(f"fixture.{name}={'PASS' if ok else 'FAIL'}  {detail}")
    lines += ["", "page_ids (named subset of the 25-page ring):"]
    for cell, name in NAMED.items():
        p = pages[cell]
        lines.append(f"  {name}={cell} region_id={p.region_id} digest={p.source_digest} "
                     f"bounds=[{p.min_x:.0f},{p.min_x+REGION_M:.0f}]x[{p.min_y:.0f},{p.min_y+REGION_M:.0f}]")
    lines += ["", "mv2c=closed mw9=closed presentation_diversity=closed",
              "no_reopen=MW1-MW8/MV1/MV2.B/PX1-PX3", ""]
    RECEIPT.parent.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print("\n".join(lines))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
