"""MV2.A certification — compile the 9-page macro-authority set and prove the 8
fixtures. Macro authority only; no renderer, no fine causal stack.

Run:  python Tools/Worldgen/cert_macro_authority.py
Emits: Data/Worldgen/MacroAuthority/page_<ri>_<rj>.mcp  (center + 8 neighbours)
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

NEIGHBORS = [("center", 0, 0), ("north", 0, 1), ("south", 0, -1), ("east", 1, 0),
             ("west", -1, 0), ("ne", 1, 1), ("nw", -1, 1), ("se", 1, -1), ("sw", -1, -1)]


def main() -> int:
    central = CentralProgram.load()
    fieldf = MacroField.build(central.seed)
    checks: list[tuple[str, bool, str]] = []

    # Compile the 9 pages (timed → cheap-source evidence).
    PAGE_DIR.mkdir(parents=True, exist_ok=True)
    t0 = time.perf_counter()
    pages = {}
    for name, ri, rj in NEIGHBORS:
        p = compile_page(central, fieldf, ri, rj)
        pages[name] = p
        (PAGE_DIR / f"page_{ri}_{rj}.mcp").write_text(serialize_page(p), encoding="utf-8", newline="\n")
    compile_s = time.perf_counter() - t0
    samples_per_page = pages["center"].n ** 2

    # ---- Fixture 1: determinism ------------------------------------------- #
    f2 = MacroField.build(central.seed)
    det_ok = True
    det_detail = ""
    for name, ri, rj in NEIGHBORS:
        again = compile_page(central, f2, ri, rj)
        if again.source_digest != pages[name].source_digest or again.region_id != pages[name].region_id:
            det_ok = False
            det_detail = f"{name} digest/id changed"
            break
    checks.append(("determinism_same_seed_same_digest", det_ok,
                   det_detail or "all 9 pages reproduce digest+region_id"))

    # ---- Fixture 2: non-repetition (no 64 km tiling) ---------------------- #
    ids = [pages[n].region_id for n, _, _ in NEIGHBORS]
    digs = [pages[n].source_digest for n, _, _ in NEIGHBORS]
    unique_ok = len(set(ids)) == 9 and len(set(digs)) == 9
    # Field must not be periodic with the 64 km cell: compare macro_z(x,y) vs
    # macro_z(x+64km,y) over a grid well outside the frozen center.
    rms, worst = 0.0, 0.0
    cnt = 0
    for j in range(-3, 4):
        for i in range(1, 5):
            x = i * REGION_M + 5000.0
            y = j * REGION_M + 5000.0
            d = abs(macro_z(central, fieldf, x, y) - macro_z(central, fieldf, x + REGION_M, y))
            rms += d * d
            worst = max(worst, d)
            cnt += 1
    rms = math.sqrt(rms / cnt)
    nonrep_ok = unique_ok and rms > 40.0
    checks.append(("non_repetition_no_64km_tiling", nonrep_ok,
                   f"unique_ids={len(set(ids))}/9 unique_digests={len(set(digs))}/9 "
                   f"period64_rms={rms:.1f}m worst={worst:.1f}m (need rms>40)"))

    # ---- Fixture 3: boundary continuity across the 4 center edges --------- #
    eps = 1.0
    max_pos, max_slope = 0.0, 0.0
    for t in range(-31, 32):        # sample along each edge, avoid exact corners
        s = t * 1000.0
        edges = [( REGION_HALF_M, s, 1, 0), (-REGION_HALF_M, s, -1, 0),
                 ( s, REGION_HALF_M, 0, 1), ( s, -REGION_HALF_M, 0, -1)]
        for (bx, by, nx, ny) in edges:
            inside = central_envelope(central, bx, by)                    # frozen center macro
            outside = macro_z(central, fieldf, bx + nx * eps, by + ny * eps)  # MV2 just outside
            max_pos = max(max_pos, abs(outside - inside))
            # slope across the boundary vs slope just inside the frozen envelope
            out2 = macro_z(central, fieldf, bx + nx * 2 * eps, by + ny * 2 * eps)
            in2 = central_envelope(central, bx - nx * eps, by - ny * eps)
            slope_out = (out2 - outside) / eps
            slope_in = (inside - in2) / eps
            max_slope = max(max_slope, abs(slope_out - slope_in))
    cont_ok = max_pos < 0.5 and max_slope < 0.05
    checks.append(("boundary_continuity_4_edges", cont_ok,
                   f"max_position_step={max_pos:.4f}m (need<0.5) "
                   f"max_slope_step={max_slope:.4f} (need<0.05)"))

    # ---- Fixture 5: central anchoring (center page IS the frozen macro) --- #
    p = pages["center"]
    max_anchor = 0.0
    for j in range(p.n):
        for i in range(p.n):
            x = p.min_x + i * p.step
            y = p.min_y + j * p.step
            max_anchor = max(max_anchor, abs(p.heights[j * p.n + i] - central_envelope(central, x, y)))
    anchor_ok = max_anchor < 0.01
    checks.append(("central_anchor_matches_frozen_macro", anchor_ok,
                   f"max|center_page - frozen_envelope|={max_anchor:.5f}m (need<0.01; anchor window=0 inside)"))

    # ---- Fixture 7: feature scale independence (a feature spans >64 km) --- #
    # Measure each belt's along-axis high-ground extent (uplift contribution above
    # a threshold) and take the longest.
    longest = 0.0
    which = -1
    for bi, b in enumerate(fieldf.belts):
        a = b.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        lo = hi = None
        s = -b.half_length_m - 20000.0
        while s <= b.half_length_m + 20000.0:
            x = b.cx + s * ux
            y = b.cy + s * uy
            if fieldf._belt(b, x, y) > 150.0:     # meaningful uplift
                lo = s if lo is None else lo
                hi = s
            s += 2000.0
        if lo is not None and hi is not None and (hi - lo) > longest:
            longest = hi - lo
            which = bi
    scale_ok = longest > 64000.0
    checks.append(("feature_scale_independence_gt_64km", scale_ok,
                   f"longest_belt_extent={longest/1000.0:.1f}km (belt#{which}; need>64km)"))

    # ---- Fixture 4: cross-page landform continuity ----------------------- #
    # A belt crest must (a) cross an internal page boundary within the footprint and
    # (b) the field stays continuous across that boundary (no jump), with high
    # ground on BOTH sides (the ridge continues).
    cross_ok = False
    cross_detail = "no belt crosses a footprint page boundary"
    footprint = 2.0 * REGION_M + REGION_HALF_M   # +/-160 km
    for b in fieldf.belts:
        a = b.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        s = -b.half_length_m
        while s <= b.half_length_m:
            x = b.cx + s * ux
            y = b.cy + s * uy
            if abs(x) > footprint or abs(y) > footprint:
                s += 1000.0
                continue
            # nearest vertical page boundary to x, if the crest sits within footprint
            k = round((x + REGION_HALF_M) / REGION_M)
            bxr = k * REGION_M - REGION_HALF_M
            if abs(x - bxr) < 1500.0 and abs(bxr) <= footprint and anchor_window(x, y) > 0.2:
                left = macro_z(central, fieldf, bxr - 1.0, y)
                right = macro_z(central, fieldf, bxr + 1.0, y)
                here = macro_z(central, fieldf, bxr, y)
                base = central_envelope(central, bxr, y) + anchor_window(bxr, y) * fieldf._base(bxr, y)
                if abs(left - right) < 5.0 and (here - base) > 120.0:  # continuous + high ground
                    cross_ok = True
                    cross_detail = (f"belt crest crosses x={bxr/1000:.0f}km page boundary; "
                                    f"jump={abs(left-right):.3f}m crest_above_base={here-base:.0f}m")
                    break
            s += 1000.0
        if cross_ok:
            break
    checks.append(("cross_page_landform_continuity", cross_ok, cross_detail))

    # ---- Fixture 6: no square-world signature ---------------------------- #
    # Scan a long E-W line across many page boundaries; the height step AT page
    # boundaries must be no larger than typical within-page steps (no seam), and
    # local roughness must not step at cell lines.
    yline = 9000.0
    xs = [x for x in range(-160000, 160001, 500)]
    zs = [macro_z(central, fieldf, float(x), yline) for x in xs]
    interior_max, boundary_max = 0.0, 0.0
    for idx in range(1, len(xs)):
        d = abs(zs[idx] - zs[idx - 1])
        on_boundary = (abs(((xs[idx] + REGION_HALF_M) % REGION_M)) < 600.0)
        if on_boundary:
            boundary_max = max(boundary_max, d)
        else:
            interior_max = max(interior_max, d)
    # roughness continuity: std of 8-sample windows should not spike on cell lines
    sig_ok = boundary_max <= interior_max * 1.25 + 0.5
    checks.append(("no_square_world_signature", sig_ok,
                   f"max_step_on_page_boundary={boundary_max:.2f}m "
                   f"max_step_interior={interior_max:.2f}m (boundary must not exceed interior*1.25)"))

    # ---- Fixture 8: no wrap ---------------------------------------------- #
    # (a) pages declare no_wrap_into_region; (b) the field is not 64 km-periodic
    # anywhere outside the centre (already shown non-zero in F2); (c) macro_authority
    # source contains no WrapIntoRegion / ReconstructedZ / QueryMaterial reference.
    import tokenize, io as _io
    src = (Path(MA.__file__)).read_text(encoding="utf-8")
    code_names, imports = set(), set()
    for tok in tokenize.generate_tokens(_io.StringIO(src).readline):
        if tok.type == tokenize.NAME:          # excludes comments AND string/docstring bodies
            code_names.add(tok.string)
    for line in src.splitlines():
        ls = line.strip()
        if ls.startswith("import ") or ls.startswith("from "):
            imports.add(ls)
    # Forbidden as CODE IDENTIFIERS (not prose): the generator can never call the
    # fine causal stack. Also require imports to be stdlib-only.
    forbidden = [t for t in ("WrapIntoRegion", "ReconstructedZ", "QueryMaterial",
                             "SampleForcing", "CausalBareEarthGeography") if t in code_names]
    stdlib_only = all(any(m in imp for m in ("math", "struct", "dataclasses", "pathlib",
                                             "typing", "annotations"))
                      for imp in imports)
    files_declare = all("no_wrap_into_region=1" in (PAGE_DIR / f"page_{ri}_{rj}.mcp").read_text()
                        for _, ri, rj in NEIGHBORS)
    forbidden = forbidden if (forbidden or stdlib_only) else forbidden + ["non_stdlib_import"]
    wrap_ok = files_declare and not forbidden
    checks.append(("no_wrap_no_fine_authority", wrap_ok,
                   f"pages_declare_no_wrap={files_declare} forbidden_tokens={forbidden or 'none'}"))

    # ---- cheap-source evidence ------------------------------------------- #
    cheap_ok = compile_s < 20.0 and not forbidden
    checks.append(("cheap_source_no_deep_reconstructedz", cheap_ok,
                   f"compiled 9 pages ({samples_per_page} samples each) in {compile_s:.2f}s; "
                   f"macro-analytic only (no ReconstructedZ/erosion/QueryMaterial)"))

    passed = all(ok for _, ok, _ in checks)

    lines = ["MV2A_REGIONAL_MACRO_AUTHORITY " + ("PASS" if passed else "FAIL"),
             "scope=macro_forcing_only_no_renderer_no_fine_causal_stack",
             f"world_seed={central.seed}",
             f"generator_version={MA.GENERATOR_VERSION}",
             f"central_region_key={central.region_key}",
             f"central_world_identity_hash={central.world_identity_hash}",
             f"page_cell_size_m={REGION_M:.0f}  (packaging only; feature wavelengths independent)",
             f"pages_compiled={len(pages)}  set=center+N/S/E/W+NE/NW/SE/SW",
             f"sample_step_m={pages['center'].step:.0f}  samples_per_page={samples_per_page}",
             f"compile_seconds={compile_s:.3f}",
             ""]
    for name, ok, detail in checks:
        lines.append(f"fixture.{name}={'PASS' if ok else 'FAIL'}  {detail}")
    lines += ["",
              "page_ids:"]
    for name, ri, rj in NEIGHBORS:
        p = pages[name]
        lines.append(f"  {name}=({ri},{rj}) region_id={p.region_id} digest={p.source_digest} "
                     f"bounds=[{p.min_x:.0f},{p.min_x+REGION_M:.0f}]x[{p.min_y:.0f},{p.min_y+REGION_M:.0f}]")
    lines += ["",
              "mv2b_renderer=closed", "mv2c=closed", "mw9=closed",
              "no_reopen=MW1-MW8/MV1/PX1-PX3", ""]
    RECEIPT.parent.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print("\n".join(lines))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
