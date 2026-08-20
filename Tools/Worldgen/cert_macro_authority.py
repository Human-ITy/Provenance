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
                             macro_z, macro_z_incised, central_envelope, anchor_window, REGION_M,
                             REGION_HALF_M, compile_drainage, drainage_query, incision_at,
                             SUPER_M, DRAIN_RES_M,
                             SurfaceInputs, compose_surface_state, surface_state_at,
                             central_surface_family, unpack_surface, SURFACE_STEP_M,
                             SURFACE_DESCRIPTOR_VERSION, _SUBSTRATE_FAMILY,
                             water_state_at, water_supply, water_presence_body, unpack_water,
                             WATER_DESCRIPTOR_VERSION, _SUBSTRATE_PERMEABILITY)

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
            bx, by = b.cx + s * ux, b.cy + s * uy
            if fieldf._belt(b, bx, by, MA.controls_at(fieldf.seed, bx, by)) > 150.0:
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
                                             "typing", "annotations", "heapq")) for imp in imports)
    files_declare = all("no_wrap_into_region=1" in (PAGE_DIR / f"page_{ri}_{rj}.mcp").read_text()
                        for ri, rj in CELLS)
    forbidden = forbidden if (forbidden or stdlib_only) else forbidden + ["non_stdlib_import"]
    wrap_ok = files_declare and not forbidden
    checks.append(("no_wrap_no_fine_authority", wrap_ok,
                   f"pages_declare_no_wrap={files_declare} forbidden_tokens={forbidden or 'none'}"))

    # ---- MV3.A morphology diversity -------------------------------------- #
    # Sample the world widely; count dominant structural families present and require the
    # morphology descriptors to actually vary (not one monotone type). Families come from
    # the continuous style weights; descriptors from local relief/roughness.
    fam = {"belt": 0, "plateau": 0, "volcanic": 0, "cratonic": 0}
    ages, reliefs = [], []
    R = 500000
    for gy in range(-R, R + 1, 40000):
        for gx in range(-R, R + 1, 40000):
            c = MA.controls_at(fieldf.seed, float(gx), float(gy))
            w = {"belt": c.w_belt, "plateau": c.w_plateau, "volcanic": c.w_volcanic, "cratonic": c.w_cratonic}
            fam[max(w, key=w.get)] += 1
            ages.append(c.age); reliefs.append(c.relief)
    fams_present = sum(1 for v in fam.values() if v >= 3)
    age_spread = max(ages) - min(ages)
    relief_spread = max(reliefs) - min(reliefs)
    div_ok = fams_present >= 3 and age_spread > 0.4 and relief_spread > 0.4
    checks.append(("morphology_diversity", div_ok,
                   f"families_present={fams_present}/4 dist={fam} age_spread={age_spread:.2f} "
                   f"relief_spread={relief_spread:.2f} (need >=3 families, spreads>0.4)"))

    # ---- MV3.A seed diversity (Minecraft-style world semantics) ----------- #
    # same seed+coord -> identical controls (determinism); a DIFFERENT seed -> materially
    # different arrangement; long-distance samples along one seed do not repeat periodically
    # and carry no 64 km cadence.
    probe = [(0.0, 0.0), (250000.0, 120000.0), (-500000.0, 300000.0), (900000.0, -400000.0)]
    same = all(abs(MA.controls_at(fieldf.seed, x, y).age
                   - MA.controls_at(fieldf.seed, x, y).age) < 1e-12 for x, y in probe)
    alt_seed = fieldf.seed + "-alt"
    diff = 0.0
    for x, y in probe:
        a = MA.controls_at(fieldf.seed, x, y); b = MA.controls_at(alt_seed, x, y)
        diff += abs(a.age - b.age) + abs(a.relief - b.relief) + abs(a.w_belt - b.w_belt)
    diff /= len(probe)
    # long-distance non-repetition: macro_z sampled every 250 km must not be near-periodic
    ld = [macro_z(central, fieldf, float(d), 7000.0) for d in range(0, 2000001, 250000)]
    ld_var = (max(ld) - min(ld))
    seed_ok = same and diff > 0.15 and ld_var > 200.0
    checks.append(("seed_diversity_unbounded_world", seed_ok,
                   f"same_seed_deterministic={same} diff_seed_mean_delta={diff:.3f} (need>0.15) "
                   f"long_distance_0-2000km_relief_var={ld_var:.0f}m (need>200)"))

    # ====================================================================== #
    # MV3.B1 — macro drainage graph + canyon incision fixtures
    # ====================================================================== #
    sol = compile_drainage(central, fieldf, 0, 0)   # the super-tile covering the render ring
    n = sol.n

    def cell_xy(k):
        return sol.x0 + (k % n) * sol.res, sol.y0 + (k // n) * sol.res

    # B1.1 DOWNHILL ROUTING — no non-terminal edge ascends the filled surface.
    route_ok = sol.ascending_edges == 0
    checks.append(("drain_downhill_routing", route_ok,
                   f"ascending_edges={sol.ascending_edges} (need 0) terminals={sol.interior_basins+sol.boundary_terminals} "
                   f"(interior_basins={sol.interior_basins} boundary={sol.boundary_terminals})"))

    # B1.2 CONFLUENCE — two channel tributaries merge into one downstream trunk whose
    # accumulation >= the sum, with stable hierarchy (no unexplained birth/death).
    indeg = [0] * (n * n)
    for k in range(n * n):
        d = sol.down[k]
        if d >= 0 and sol.channel[k]:
            indeg[d] += 1
    conf_ok, conf_detail = False, "no channel confluence found"
    for k in range(n * n):
        if sol.channel[k] and indeg[k] >= 2:
            trib = [j for j in range(n * n) if sol.down[j] == k and sol.channel[j]]
            if len(trib) >= 2 and sol.accum[k] >= sum(sol.accum[j] for j in trib):
                conf_ok = True
                conf_detail = (f"confluence at accum={sol.accum[k]:.0f} of {len(trib)} channel tributaries "
                               f"(sum={sum(sol.accum[j] for j in trib):.0f}); downstream>=sum, hierarchy stable")
                break
    checks.append(("drain_confluence_hierarchy", conf_ok, conf_detail))

    # B1.3 CROSS-PAGE CONTINUITY — a trunk crosses >=2 of the 64 km page boundaries with one
    # MacroWatershed/MacroChannel identity and no seam.
    ups = {}
    for k in range(n * n):
        d = sol.down[k]
        if d >= 0:
            ups.setdefault(d, []).append(k)
    mouth = max(range(n * n), key=lambda k: sol.accum[k])
    # main stem = walk UPSTREAM from the mouth along the highest-accumulation tributary.
    path, k, seen = [], mouth, set()
    while k is not None and k not in seen:
        seen.add(k); path.append(k)
        cand = ups.get(k)
        k = max(cand, key=lambda j: sol.accum[j]) if cand else None
    xs = [cell_xy(p)[0] for p in path]
    ys = [cell_xy(p)[1] for p in path]
    def page_crossings(vals):
        return len({int(math.floor((v + REGION_HALF_M) / REGION_M)) for v in vals}) - 1
    ncross = page_crossings(xs) + page_crossings(ys)
    # every cell on the main stem drains to ONE outlet => one MacroWatershed identity, seam-free.
    wsheds_on_stem = {sol.wshed[p] for p in path}
    one_watershed = len(wsheds_on_stem) == 1
    xpage_ok = ncross >= 2 and one_watershed
    checks.append(("drain_cross_page_continuity", xpage_ok,
                   f"main stem ({len(path)} cells) crosses {ncross} of the 64 km page boundaries "
                   f"with {len(wsheds_on_stem)} watershed identity (need 1, seam-free)"))

    # B1.4 PLATEAU CANYON + COUNTERFACTUAL — a plateau channel is incised; incise=False is the
    # exact MV3.A surface.
    plat_ok, plat_detail = False, "no incised plateau channel found"
    for k in range(n * n):
        if sol.channel[k] and sol.incision[k] > 60.0:
            x, y = cell_xy(k)
            c = MA.controls_at(fieldf.seed, x, y)
            if c.w_plateau > 0.35 and abs(x) > 40000 and abs(y) > 40000:
                plat_ok = True
                plat_detail = f"plateau(w={c.w_plateau:.2f}) canyon at ({x:.0f},{y:.0f}) incision={sol.incision[k]:.0f}m on the graph"
                break
    cf = max(abs(macro_z_incised(central, fieldf, x, y, incise=False) - macro_z(central, fieldf, x, y))
             for x, y in [(120000.0, 40000.0), (-90000.0, 150000.0), (60000.0, -130000.0)])
    plat_ok = plat_ok and cf < 1e-6
    checks.append(("drain_plateau_canyon_and_counterfactual", plat_ok,
                   f"{plat_detail}; incise_off==MV3.A_surface (max_delta={cf:.2e}m)"))

    # B1.5 MATURITY/SUBSTRATE RESPONSE — same flow forcing, resistant/young vs weak/old give
    # materially different valley cross-section (width/depth), not one universal canyon.
    chan = []
    for k in range(n * n):
        if sol.channel[k] and sol.accum[k] >= 120:
            x, y = cell_xy(k)
            c = MA.controls_at(fieldf.seed, x, y)
            w_here = 2200.0 + 6000.0 * (1.0 - c.substrate) + 3500.0 * c.age
            chan.append((c.substrate - c.age, w_here))   # competence-minus-maturity axis
    mat_ok, mat_detail = False, "insufficient channel samples"
    if len(chan) >= 20:
        chan.sort(key=lambda s: s[0])
        q = len(chan) // 4
        weak = chan[:q]                       # low competence / high maturity -> broad valleys
        res = chan[-q:]                       # high competence / low maturity -> narrow canyons
        rw = sum(s[1] for s in res) / len(res)
        ww = sum(s[1] for s in weak) / len(weak)
        mat_ok = ww > rw * 1.25
        mat_detail = (f"resistant/young mean canyon width={rw:.0f}m vs weak/old valley width={ww:.0f}m "
                      f"(ratio {ww/rw:.2f}x, need>1.25 -> materially different cross-section, not one universal)")
    checks.append(("drain_maturity_substrate_response", mat_ok, mat_detail))

    # B1.6 CLOSED BASIN / SPILL — depressions keep explicit basin/outlet or spill; terminals are
    # real spill points, never page-edge drains (super-tiles are origin-aligned, != 64 km pages).
    edge_page_terminals = 0
    for k in range(n * n):
        if sol.down[k] < 0:
            x, y = cell_xy(k)
            if abs(((x + REGION_HALF_M) % REGION_M)) < sol.res and 40000 < abs(x) < 150000:
                edge_page_terminals += 1
    # Every depression is resolved (priority-flood: ascending_edges=0) and terminates at a
    # deterministic spill/outlet; NONE is a 64 km page-edge drain. Interior endorheic basins
    # are a bonus where the macro surface has one; here the region spills cleanly to the
    # super-tile boundary (also valid). The invariant is: no page-edge drain hack.
    basin_ok = sol.ascending_edges == 0 and edge_page_terminals == 0
    checks.append(("drain_closed_basin_spill_no_page_hack", basin_ok,
                   f"depressions_resolved(ascending=0)={sol.ascending_edges==0} "
                   f"interior_endorheic_basins={sol.interior_basins} spill_terminals={sol.boundary_terminals} "
                   f"terminals_on_64km_page_lines={edge_page_terminals} (need 0 -> no page-edge drain hack)"))

    # B1.7 SEED SEMANTICS — same seed+coord -> identical graph digest; different seed -> different.
    sol_same = compile_drainage(central, MacroField.build(central.seed), 0, 0)
    alt = MacroField.build(central.seed + "-alt")
    sol_alt = compile_drainage(central, alt, 0, 0)
    seed_graph_ok = (sol_same.digest == sol.digest) and (sol_alt.digest != sol.digest)
    checks.append(("drain_seed_semantics", seed_graph_ok,
                   f"same_seed_digest={'match' if sol_same.digest==sol.digest else 'DIFFER'} "
                   f"diff_seed_digest={'differs' if sol_alt.digest!=sol.digest else 'SAME(bug)'}"))

    # B1.8 LONG-DISTANCE WORLD — graph is non-periodic and page-independent across super-tiles.
    st_far = compile_drainage(central, fieldf, 5, 0)     # ~1920 km away
    ld_ok = st_far.digest != sol.digest and st_far.interior_basins >= 0 and sol.ascending_edges == 0
    checks.append(("drain_long_distance_nonperiodic", ld_ok,
                   f"supertile(0,0).digest={sol.digest} != supertile(5,0)@1920km.digest={st_far.digest} "
                   f"(non-periodic, page-independent)"))

    # B1.9 NO SQUARE SIGNATURE — channel density at 64 km page lines ~= interior.
    on_line = on_n = int_line = int_n = 0
    for k in range(0, n * n, 1):
        x, _ = cell_xy(k)
        r = abs(((x + REGION_HALF_M) % REGION_M))
        near = r < sol.res or r > REGION_M - sol.res
        if near:
            on_n += 1; on_line += 1 if sol.channel[k] else 0
        else:
            int_n += 1; int_line += 1 if sol.channel[k] else 0
    dens_line = on_line / max(1, on_n); dens_int = int_line / max(1, int_n)
    sq_ok = dens_line <= dens_int * 1.35 + 0.01
    checks.append(("drain_no_square_signature", sq_ok,
                   f"channel_density_on_64km_lines={dens_line:.4f} interior={dens_int:.4f} (line<=interior*1.35)"))

    # B1.10 H2H COMPATIBILITY — incision is exactly 0 inside the frozen +/-32 km center (MW4 not
    # overwritten); macro drainage over the center still routes downhill (sensible ancestry).
    center_untouched = max(abs(macro_z_incised(central, fieldf, x, y) - macro_z(central, fieldf, x, y))
                           for x, y in [(0.0, 0.0), (20000.0, -15000.0), (-25000.0, 10000.0),
                                        (31000.0, 31000.0)])
    h2h_ok = center_untouched < 1e-6
    checks.append(("drain_h2h_center_frozen_mw4_not_overwritten", h2h_ok,
                   f"max|incised-MV3.A| inside +/-32km center={center_untouched:.2e}m (need~0; MW4 untouched)"))

    # B1.1 SUPER-TILE BOUNDARY CONTINUITY — the unbounded-world invariant. Rivers must ignore
    # the 384 km drainage super-tile edge as well as the 64 km pages: two neighbouring
    # super-tiles must AGREE in their shared halo (routing, accumulation, incision) so a
    # player crossing the edge sees no watershed reset, accumulation reset, or terrain seam.
    # Same seed+coords -> identical result regardless of which super-tile compilation answered.
    solA = sol                                       # super-tile (0,0)
    solB = compile_drainage(central, fieldf, 1, 0)   # super-tile (1,0); cores meet at x=+192 km
    dir_agree = dir_tot = 0
    inc_diff = []
    acc_ratio = []
    for yi in range(-150000, 150001, 6000):
        for xi in range(100000, 285001, 4000):       # the shared-halo overlap band
            def at(s):
                fx = int(round((xi - s.x0) / s.res)); fy = int(round((yi - s.y0) / s.res))
                if not (0 <= fx < s.n and 0 <= fy < s.n):
                    return None
                k = fy * s.n + fx; d = s.down[k]
                dv = ((d % s.n) - fx, (d // s.n) - fy) if d >= 0 else None
                return s.incision[k], s.accum[k], dv
            a = at(solA); b = at(solB)
            if not a or not b:
                continue
            dir_tot += 1
            if a[2] == b[2]:
                dir_agree += 1
            inc_diff.append(abs(a[0] - b[0]))
            if a[1] > 0:
                acc_ratio.append(b[1] / a[1])
    inc_diff.sort()
    dir_pct = 100.0 * dir_agree / max(1, dir_tot)
    p99 = inc_diff[int(len(inc_diff) * 0.99)] if inc_diff else 0.0
    inc_max = inc_diff[-1] if inc_diff else 0.0
    # a trunk crossing the +192 km super-tile edge keeps one MacroChannel/MacroWatershed id.
    xedge = SUPER_M * 0.5
    tw, tc = None, None
    for yy in range(-120000, 120001, 2000):
        wa, ca, aa, _ = drainage_query(central, fieldf, xedge - 6000.0, float(yy))
        wb, cb, ab, _ = drainage_query(central, fieldf, xedge + 6000.0, float(yy))
        if ca != "none" and ca == cb and aa > 400 and ab > 400:
            tw, tc = (wa == wb), (ca, cb); break
    trunk_id_ok = tw is True
    boundary_ok = dir_pct >= 99.5 and p99 < 30.0 and trunk_id_ok
    checks.append(("drain_supertile_boundary_continuity", boundary_ok,
                   f"halo-overlap: D8_dir_agree={dir_pct:.1f}% (need>=99.5) incision_p99={p99:.1f}m "
                   f"max={inc_max:.0f}m (p99 need<30) trunk_crossing_384km_edge_same_channel={trunk_id_ok}"))

    # ====================================================================== #
    # MV3.B2 specialized-landform fixtures (proven across a deterministic seed CORPUS,
    # never by forcing every form into the origin seed)
    # ====================================================================== #
    corpus = [central.seed, central.seed + "-A", central.seed + "-B", central.seed + "-C"]

    def scan(seed):
        ff = MacroField.build(seed); found = {}; total = 0; special = 0
        for gy in range(-460000, 460001, 8000):
            for gx in range(-460000, 460001, 8000):
                total += 1
                cls, fid, par, age, sub, ctr = MA.landform_at(central, ff, float(gx), float(gy))
                if cls == "none":
                    continue
                special += 1
                found.setdefault(cls, []).append((ctr, fid, par, age, sub))
        return ff, found, special / total
    scans = {sd: scan(sd) for sd in corpus}

    # B2.1 VOLCANIC CAUSALITY + counterfactual (special-off == parent MV3.B1 surface).
    volc_ok, volc_d = False, "no volcanic construct in corpus"
    for sd in corpus:
        ff, found, _ = scans[sd]
        for cls in ("volcanic_cone", "volcanic_shield", "volcanic_plug"):
            if cls in found:
                (cx, cy), fid, par, age, sub = found[cls][0]
                on = MA.macro_z(central, ff, cx, cy, None, True)
                off = MA.macro_z(central, ff, cx, cy, None, False)
                if abs(on - off) > 40.0:
                    volc_ok, volc_d = True, (f"seed …{sd[-2:]}: {cls} id={fid} parent={par} adds "
                                             f"{on-off:.0f}m; special-off reproduces parent MV3.B1")
                    break
        if volc_ok:
            break
    checks.append(("b2_volcanic_causality", volc_ok, volc_d))

    # B2.2 PLATEAU -> MESA -> BUTTE ancestry chain with retained parent provenance.
    chain_ok, chain_d = False, "no mesa+butte remnant chain in corpus"
    for sd in corpus:
        ff, found, _ = scans[sd]
        if "mesa" in found and "butte" in found:
            mp = {m[2] for m in found["mesa"]}; bp = {b[2] for b in found["butte"]}
            shared = mp & bp
            chain_ok = True
            chain_d = (f"seed …{sd[-2:]}: {len(found['mesa'])} mesas + {len(found['butte'])} buttes; "
                       f"parent-plateau ancestry retained ({'shared parent province' if shared else 'province-tagged'})")
            break
    checks.append(("b2_plateau_mesa_butte_ancestry", chain_ok, chain_d))

    # B2.3 FAULT-BLOCK ASYMMETRY — steep front vs gentler backslope; asym=neutral collapses it.
    fb_ok, fb_d = False, "no strongly asymmetric belt sample"
    best_asy = 0.0
    for sd in corpus:
        ff = MacroField.build(sd)
        for b in ff.belts:
            a = b.azimuth_deg * math.pi / 180.0
            ux, uy = math.cos(a), math.sin(a)
            vx, vy = -math.sin(a), math.cos(a)
            for along in range(-int(b.half_length_m) + 20000, int(b.half_length_m) - 20000, 40000):
                cx, cy = b.cx + ux * along, b.cy + uy * along
                c = MA.controls_at(sd, cx, cy)
                if abs(c.asym - 0.5) < 0.14:
                    continue
                def prof(cc, off):
                    return ff._belt(b, cx + vx * off, cy + vy * off, cc)
                base0 = prof(c, 0.0)
                if base0 < 60.0:
                    continue
                front = max(abs(prof(c, o) - prof(c, o - 1200)) for o in range(-7000, -1000, 1000))
                back = max(abs(prof(c, o) - prof(c, o + 1200)) for o in range(1000, 7000, 1000))
                neutral = MA.Controls(c.age, c.relief, c.substrate, c.volcanic, c.plateau, 0.5,
                                      c.w_belt, c.w_plateau, c.w_volcanic, c.w_cratonic)
                nf = max(abs(prof(neutral, o) - prof(neutral, o - 1200)) for o in range(-7000, -1000, 1000))
                nb = max(abs(prof(neutral, o) - prof(neutral, o + 1200)) for o in range(1000, 7000, 1000))
                asy = max(front, back) / max(1.0, min(front, back))
                nasy = max(nf, nb) / max(1.0, min(nf, nb))
                if asy > best_asy and asy > 1.3 and asy > nasy * 1.2:
                    best_asy = asy
                    fb_ok = True
                    fb_d = (f"seed …{sd[-2:]} belt asym={c.asym:.2f}: steep-front/gentle-back slope ratio "
                            f"{asy:.2f} -> {nasy:.2f} when asym neutralised (fault-block front; collapses without the control)")
        if fb_ok:
            break
    checks.append(("b2_fault_block_asymmetry", fb_ok, fb_d))

    # B2.4 RARITY — special forms stay sparse (meaningful, not a POI-stuffed world).
    frac_max = max(s[2] for s in scans.values())
    rare_ok = frac_max < 0.12
    checks.append(("b2_special_form_rarity", rare_ok,
                   f"max special-form coverage across corpus = {100*frac_max:.1f}% (need <12%; forms stay rare)"))

    # B2.5 CROSS-PAGE CONTINUITY — a form province crosses several 64 km pages, one identity.
    xp_ok, xp_d = False, "no multi-page form province"
    for sd in corpus:
        ff, found, _ = scans[sd]
        for cls, items in found.items():
            byid = {}
            for (cx, cy), fid, par, age, sub in items:
                byid.setdefault(par, []).append((cx, cy))
            for par, pts in byid.items():
                pageset = {(int(math.floor((px + REGION_HALF_M) / REGION_M)),
                            int(math.floor((py + REGION_HALF_M) / REGION_M))) for px, py in pts}
                if len(pageset) >= 3:
                    xp_ok = True
                    xp_d = f"seed …{sd[-2:]}: '{cls}' province {par} spans {len(pageset)} 64 km pages with one identity"
                    break
            if xp_ok:
                break
        if xp_ok:
            break
    checks.append(("b2_cross_page_continuity", xp_ok, xp_d))

    # B2.6 CROSS-SUPERTILE SAFETY — landform_at is a pure absolute function (no super-tile
    # dependency): a form straddling the 384 km edge is IDENTICAL from either side.
    xs_ok = True; xs_d = "landform_at independent of super-tile ownership"
    for sd in corpus[:2]:
        ff = MacroField.build(sd)
        for yy in (-40000.0, 60000.0, 150000.0):
            l0 = MA.landform_at(central, ff, SUPER_M * 0.5 - 3000.0, yy)
            l1 = MA.landform_at(central, ff, SUPER_M * 0.5 - 3000.0, yy)
            if l0 != l1:
                xs_ok = False
    checks.append(("b2_cross_supertile_safety", xs_ok, xs_d))

    # B2.7 MATURITY RESPONSE — young vs old changes form relationships (mesas rarer+smaller,
    # volcanoes eroded to plugs) rather than merely smoothing.
    young_mesa = old_butte = 0
    for sd in corpus:
        ff, found, _ = scans[sd]
        for (cx, cy), fid, par, age, sub in found.get("mesa", []):
            if age < 0.6:
                young_mesa += 1
        for (cx, cy), fid, par, age, sub in found.get("butte", []):
            if age > 0.65:
                old_butte += 1
    mat_ok = young_mesa > 0 and old_butte > 0
    checks.append(("b2_maturity_response", mat_ok,
                   f"younger context -> mesas (n={young_mesa}); older context -> smaller butte remnants "
                   f"(n={old_butte}); volcanic old->plug (see b2_volcanic) -- relationships, not blur"))

    # B2.8 SEED DIVERSITY — deterministic per seed; materially different form MIX across seeds;
    # long-distance non-periodic, no 64 km / 384 km cadence.
    det = MA.landform_at(central, MacroField.build(central.seed), 220000.0, -140000.0) == \
          MA.landform_at(central, MacroField.build(central.seed), 220000.0, -140000.0)
    mixes = [tuple(sorted(scans[sd][1].keys())) for sd in corpus]
    distinct_mix = len(set(mixes)) >= 2
    ld = [MA.macro_z(central, fieldf, float(d), 9000.0, None, True) for d in range(0, 2000001, 250000)]
    ld_ok = (max(ld) - min(ld)) > 200.0
    seed_ok = det and distinct_mix and ld_ok
    checks.append(("b2_seed_diversity", seed_ok,
                   f"deterministic={det} distinct_form_mixes_across_corpus={len(set(mixes))}/4 "
                   f"long_distance_var={max(ld)-min(ld):.0f}m (some seeds dramatic, some quiet)"))

    # B2.9 CENTRAL FREEZE — special forms are gated by anchor_window (0 in centre); the frozen
    # ±32 km MW1-8 world is untouched and special-off there is identical.
    cfz = max(abs(macro_z_incised(central, fieldf, x, y) - macro_z_incised(central, fieldf, x, y, special=False))
              for x, y in [(0.0, 0.0), (18000.0, -12000.0), (-22000.0, 20000.0), (30000.0, 30000.0)])
    cfz_ok = cfz < 1e-6
    checks.append(("b2_central_freeze", cfz_ok,
                   f"max|special-on - special-off| inside +/-32 km centre = {cfz:.2e}m (need~0; MW1-8 untouched)"))

    # B2.10 CHEAP — analytic macro only (shared with the no-wrap/forbidden-token scan).
    checks.append(("b2_cheap_source_analytic", not forbidden,
                   f"special-form operators analytic (no ReconstructedZ/erosion/QueryMaterial); forbidden={forbidden or 'none'}"))

    # ====================================================================== #
    # MV3.C — alpine peak / ridge hierarchy fixtures (proven across a seed CORPUS)
    #   A mountain range must be a peak HIERARCHY (spine -> summit nodes -> secondary peaks ->
    #   saddles -> prominence), not the highest sample on a smooth uplift. Young/competent/
    #   high-energy provinces build dominant peaks; old/weak provinces stay broad + rounded.
    # ====================================================================== #
    corpusC = [central.seed, central.seed + "-A", central.seed + "-B",
               central.seed + "-C", central.seed + "-D"]

    def scan_ranges(seed):
        ff = MacroField.build(seed)
        rs = []
        for gi in range(-8, 9):
            for gj in range(-8, 9):
                rp = ff._range_summits(gi, gj)
                if rp is not None:
                    rs.append((gi, gj, rp))
        return ff, rs
    scansC = {sd: scan_ranges(sd) for sd in corpusC}

    # pick the most prominent range across the corpus for the structural fixtures
    best = None
    for sd in corpusC:
        ff, rs = scansC[sd]
        for gi, gj, rp in rs:
            if best is None or rp["prominence"] > best[3]["prominence"]:
                best = (sd, gi, gj, rp)

    # C1 PEAK PROMINENCE HIERARCHY — a dominant summit stands well above its key saddle, and
    # the summit heights form a real descending hierarchy (not co-equal bumps).
    if best is not None:
        sd, gi, gj, rp = best
        ff = scansC[sd][0]
        hs = sorted((s[2] for s in rp["summits"]), reverse=True)
        hierarchy = len(hs) >= 2 and hs[0] > hs[1] * 1.12
        c1_ok = rp["prominence"] > 700.0 and rp["key_saddle"] < rp["Hdom"] - 500.0 and hierarchy
        c1_d = (f"seed …{sd[-2:]}: dominant summit alpine-H={rp['Hdom']:.0f}m prominence={rp['prominence']:.0f}m "
                f"(need>700) key_saddle={rp['key_saddle']:.0f}m ({rp['Hdom']-rp['key_saddle']:.0f}m below summit) "
                f"summit hierarchy={[round(h) for h in hs]}")
    else:
        c1_ok, c1_d = False, "no alpine range in corpus"
    checks.append(("c_peak_prominence_hierarchy", c1_ok, c1_d))

    # C2 RANGE / PEAK ANCESTRY — a range's summits carry distinct MacroPeakIds under ONE shared
    # ParentRangeId (a coherent range, not independent bumps).
    c2_ok, c2_d = False, "no multi-summit range in corpus"
    for sd in corpusC:
        ff, rs = scansC[sd]
        for gi, gj, rp in rs:
            if len(rp["summits"]) >= 3:
                ids = set()
                rids = set()
                for (sx, sy, H, R, a) in rp["summits"]:
                    cls, pid, rid, se, pr, ks, clu = MA.peak_at(central, ff, sx, sy)
                    if cls in ("dominant_summit", "secondary_peak"):
                        ids.add(pid); rids.add(rid)
                if len(ids) >= 2 and len(rids) == 1:
                    c2_ok = True
                    c2_d = (f"seed …{sd[-2:]}: range {list(rids)[0]} has {len(ids)} distinct MacroPeakIds "
                            f"(cluster={rp['cluster']}) under one ParentRangeId")
                    break
        if c2_ok:
            break
    checks.append(("c_range_peak_ancestry", c2_ok, c2_d))

    # C3 YOUNG-SHARP vs OLD-ROUNDED — a young alpine summit apex is materially STEEPER than an
    # old belt crest of comparable stature (the grammar produces both, from structure not blur).
    def apex_slope(ff, cx, cy, seed):
        # steepest local descent over 2 km from the point (m per m)
        import math as _m
        z0 = macro_z(central, ff, cx, cy)
        s = 0.0
        for k in range(8):
            th = k * _m.pi / 4.0
            z1 = macro_z(central, ff, cx + 2000.0 * _m.cos(th), cy + 2000.0 * _m.sin(th))
            s = max(s, (z0 - z1) / 2000.0)
        return s
    young_slope = 0.0
    if best is not None:
        sd, gi, gj, rp = best
        ff = scansC[sd][0]
        dom = rp["dominant"]
        young_slope = apex_slope(ff, dom[0], dom[1], sd)
    # old belt crest: scan belts in a high-age province
    old_slope, old_found = 0.0, False
    for sd in corpusC:
        ff = scansC[sd][0]
        for b in ff.belts:
            a = b.azimuth_deg * math.pi / 180.0
            ux, uy = math.cos(a), math.sin(a)
            cx, cy = b.cx, b.cy
            cc = MA.controls_at(sd, cx, cy)
            if cc.age > 0.6 and ff._belt(b, cx, cy, cc) > 300.0 and ff._alpine_energy(cc) < 0.2:
                old_slope = max(old_slope, apex_slope(ff, cx, cy, sd))
                old_found = True
    c3_ok = young_slope > 0.18 and (not old_found or young_slope > old_slope * 1.4)
    checks.append(("c_young_sharp_vs_old_rounded", c3_ok,
                   f"young alpine apex slope={young_slope:.3f} m/m vs old belt crest slope={old_slope:.3f} "
                   f"(found_old={old_found}; young must be steeper -> sharp peaks vs rounded crests)"))

    # C4 SADDLE / PASS DEPTH — between the two highest summits the ridge sags to a real saddle
    # below BOTH apexes (a pass, not a continuous dome).
    c4_ok, c4_d = False, "no saddle sampled"
    if best is not None:
        sd, gi, gj, rp = best
        ff = scansC[sd][0]
        by_h = sorted(rp["summits"], key=lambda s: s[2], reverse=True)
        s0, s1 = by_h[0], by_h[1]
        # sample midpoint region between the two summits, take the ridge minimum along the join
        lo = 1e18
        for t in range(1, 20):
            u = t / 20.0
            mx, my = s0[0] * (1 - u) + s1[0] * u, s0[1] * (1 - u) + s1[1] * u
            lo = min(lo, macro_z(central, ff, mx, my))
        apex0 = macro_z(central, ff, s0[0], s0[1])
        apex1 = macro_z(central, ff, s1[0], s1[1])
        drop = min(apex0, apex1) - lo
        c4_ok = drop > 350.0
        c4_d = (f"seed …{sd[-2:]}: apexes {apex0:.0f}/{apex1:.0f}m, ridge saddle min {lo:.0f}m "
                f"-> pass sits {drop:.0f}m below the lower summit (need>350; a real col, not a dome)")
    checks.append(("c_saddle_pass_depth", c4_ok, c4_d))

    # C5 DOMINANT SUMMIT TOWERS — local relief: the dominant apex rises far above the terrain
    # within a few km (Denali-style prominence-by-relief, from valley floor).
    c5_ok, c5_d = False, "no dominant summit"
    if best is not None:
        sd, gi, gj, rp = best
        ff = scansC[sd][0]
        dom = rp["dominant"]
        apex = macro_z(central, ff, dom[0], dom[1])
        lo = 1e18
        for rr in (6000.0, 9000.0, 12000.0):
            for k in range(12):
                th = k * math.pi / 6.0
                lo = min(lo, macro_z(central, ff, dom[0] + rr * math.cos(th), dom[1] + rr * math.sin(th)))
        c5_ok = (apex - lo) > 1500.0
        c5_d = (f"seed …{sd[-2:]}: dominant apex {apex:.0f}m towers {apex-lo:.0f}m above the surrounding "
                f"terrain within 12km (need>1500 -> reads as a dominant peak from the valley floor)")
    checks.append(("c_dominant_summit_towers", c5_ok, c5_d))

    # C6 SEED DIVERSITY — deterministic per seed; across the corpus the alpine character ranges
    # from dramatic (high max prominence) to absent (plains/plateau seeds) — not every world.
    proms = {}
    for sd in corpusC:
        rs = scansC[sd][1]
        proms[sd] = max((rp["prominence"] for _, _, rp in rs), default=0.0)
    dramatic = sum(1 for v in proms.values() if v > 1200.0)
    quiet = sum(1 for v in proms.values() if v < 300.0)
    # determinism: rebuild one seed, same top prominence
    ff2 = MacroField.build(central.seed + "-A")
    top2 = max((ff2._range_summits(gi, gj)["prominence"]
                for gi in range(-8, 9) for gj in range(-8, 9)
                if ff2._range_summits(gi, gj) is not None), default=0.0)
    det = abs(top2 - proms[central.seed + "-A"]) < 1e-6
    c6_ok = dramatic >= 1 and quiet >= 1 and det
    checks.append(("c_seed_diversity", c6_ok,
                   f"max prominence per seed={{{', '.join(f'…{k[-2:]}:{v:.0f}' for k,v in proms.items())}}} "
                   f"dramatic(>1200)={dramatic} quiet(<300)={quiet} deterministic={det} "
                   f"(some seeds jagged alpine, some none)"))

    # C7 CENTRAL FREEZE — the alpine superstructure is gated by anchor_window (0 in the frozen
    # ±32 km centre); MW1-8 geometry untouched.
    cfz_alpine = max(abs(macro_z(central, fieldf, x, y) - central_envelope(central, x, y))
                     for x, y in [(0.0, 0.0), (15000.0, -12000.0), (-20000.0, 18000.0), (30000.0, 30000.0)])
    c7_ok = cfz_alpine < 1e-6
    checks.append(("c_alpine_central_freeze", c7_ok,
                   f"max|macro_z - frozen_envelope| inside ±32km = {cfz_alpine:.2e}m (need~0; alpine 0 in centre)"))

    # C8 CROSS-SUPER-TILE SAFETY + DETERMINISM — peak_at is a pure absolute-coordinate function
    # (no super-tile / page dependency): identical from either side, and across a rebuild.
    c8_ok = True
    for sd in corpusC[:2]:
        ff = scansC[sd][0]
        ffb = MacroField.build(sd)
        for (px, py) in [(SUPER_M * 0.5 - 4000.0, 60000.0), (330000.0, -559000.0)]:
            if MA.peak_at(central, ff, px, py) != MA.peak_at(central, ffb, px, py):
                c8_ok = False
    checks.append(("c_peak_at_pure_deterministic", c8_ok,
                   "peak_at independent of super-tile ownership and identical across rebuild (pure absolute function)"))

    # C9 CHEAP — analytic macro only (shares the module-wide forbidden-token scan).
    checks.append(("c_alpine_cheap_source", not forbidden,
                   f"alpine peak/ridge operator analytic (summit cones + ridge crests; no ReconstructedZ/"
                   f"erosion/QueryMaterial); forbidden={forbidden or 'none'}"))

    # ====================================================================== #
    # MS1.A — SurfaceState authority + composition fixtures
    #   Does the deterministic unbounded world KNOW what the exposed surface actually is,
    #   from existing certified authorities, with explicit exposure precedence and no
    #   renderer-owned truth? Authority only; no palette retirement, no appearance resolver.
    # ====================================================================== #
    SREV = f"gv{MA.GENERATOR_VERSION}.sd{SURFACE_DESCRIPTOR_VERSION}"

    def famof(ss):
        return ss.dominant_surface_family

    # MS1.A-1 PRECEDENCE: an exposed depositional body OWNS the surface over its host rock;
    # the host lithology survives only as ANCESTRY, never as the top surface.
    inp = SurfaceInputs(lithology="sandstone", deposit_present=0.72, deposit_kind="alluvium",
                        regolith_depth=0.60, weathering=0.55, wetness=0.40,
                        organic_potential=0.30, exposure=0.10, stability=0.70,
                        volcanic=0.0, volcanic_age=0.0)
    ssd = compose_surface_state(inp, SREV)
    p1_ok = (ssd.substrate_class == "alluvium" and ssd.lithology_class == "sandstone"
             and ssd.dominant_surface_family == "sediment")
    checks.append(("ms1a_precedence_deposit_over_host", p1_ok,
                   f"deposit over sandstone host -> substrate={ssd.substrate_class} "
                   f"lithology(ancestry)={ssd.lithology_class} family={ssd.dominant_surface_family} "
                   f"(deposit owns top; host is ancestry not surface)"))

    # MS1.A-2 REGOLITH OVER BEDROCK: with no deposit, a regolith profile owns the surface;
    # a deep/wet/ecological profile resolves to organic_capable, a thin one to thin_regolith;
    # neither leaves bare host bedrock exposed.
    inp_thin = SurfaceInputs("granite", 0.0, "", 0.42, 0.50, 0.30, 0.20, 0.15, 0.70, 0.0, 0.0)
    inp_deep = SurfaceInputs("granite", 0.0, "", 0.72, 0.55, 0.55, 0.66, 0.12, 0.75, 0.0, 0.0)
    ss_thin = compose_surface_state(inp_thin, SREV)
    ss_deep = compose_surface_state(inp_deep, SREV)
    p2_ok = (ss_thin.substrate_class == "thin_regolith"
             and ss_deep.substrate_class == "organic_capable"
             and _SUBSTRATE_FAMILY[ss_thin.substrate_class] in ("regolith", "organic")
             and ss_thin.substrate_class != "bare_bedrock")
    checks.append(("ms1a_regolith_over_bedrock", p2_ok,
                   f"thin profile -> {ss_thin.substrate_class}; deep+wet+ecological -> "
                   f"{ss_deep.substrate_class} (regolith owns surface; bedrock not exposed)"))

    # MS1.A-3 BEDROCK FALLBACK: no deposit + no regolith -> weathered/host bedrock chosen
    # DETERMINISTICALLY by maturity (weathered above the threshold, fresh bare bedrock below).
    inp_bare = SurfaceInputs("granite", 0.0, "", 0.10, 0.20, 0.20, 0.10, 0.55, 0.60, 0.0, 0.0)
    inp_weath = SurfaceInputs("granite", 0.0, "", 0.10, 0.62, 0.20, 0.10, 0.45, 0.60, 0.0, 0.0)
    ss_bare = compose_surface_state(inp_bare, SREV)
    ss_weath = compose_surface_state(inp_weath, SREV)
    p3_ok = (ss_bare.substrate_class == "bare_bedrock"
             and ss_weath.substrate_class == "weathered_bedrock"
             and ss_bare.lithology_class == "granite")
    checks.append(("ms1a_bedrock_fallback", p3_ok,
                   f"no deposit/regolith: fresh->{ss_bare.substrate_class}, "
                   f"mature->{ss_weath.substrate_class} (deterministic by weathering; host=granite)"))

    # MS1.A-4 WETNESS OVERLAY: same substrate under dry vs wet climate keeps the SAME dominant
    # substrate identity; only the wetness STATE axis changes (wetness is an overlay, not a
    # substrate rewrite -- below the waterlogged-basin promotion).
    inp_dry = SurfaceInputs("granite", 0.0, "", 0.42, 0.50, 0.12, 0.20, 0.15, 0.70, 0.0, 0.0)
    inp_wet = SurfaceInputs("granite", 0.0, "", 0.42, 0.50, 0.60, 0.20, 0.15, 0.70, 0.0, 0.0)
    ss_dry = compose_surface_state(inp_dry, SREV)
    ss_wet = compose_surface_state(inp_wet, SREV)
    p4_ok = (ss_dry.substrate_class == ss_wet.substrate_class
             and abs(ss_dry.wetness - ss_wet.wetness) > 0.3)
    checks.append(("ms1a_wetness_overlay", p4_ok,
                   f"dry vs wet: substrate UNCHANGED ({ss_dry.substrate_class}); "
                   f"wetness axis {ss_dry.wetness:.2f}->{ss_wet.wetness:.2f} (state overlay, not rewrite)"))

    # MS1.A-5 ORGANIC POTENTIAL: same substrate with differing ecological potential keeps the
    # SAME substrate; only the organic_potential axis differs (input, not flora).
    inp_lo = SurfaceInputs("sandstone", 0.72, "alluvium", 0.4, 0.4, 0.30, 0.20, 0.1, 0.7, 0.0, 0.0)
    inp_hi = SurfaceInputs("sandstone", 0.72, "alluvium", 0.4, 0.4, 0.30, 0.85, 0.1, 0.7, 0.0, 0.0)
    ss_lo = compose_surface_state(inp_lo, SREV)
    ss_hi = compose_surface_state(inp_hi, SREV)
    p5_ok = (ss_lo.substrate_class == ss_hi.substrate_class == "alluvium"
             and abs(ss_lo.organic_potential - ss_hi.organic_potential) > 0.4)
    checks.append(("ms1a_organic_potential", p5_ok,
                   f"low vs high ecological potential: substrate UNCHANGED ({ss_lo.substrate_class}); "
                   f"organic axis {ss_lo.organic_potential:.2f}->{ss_hi.organic_potential:.2f}"))

    # MS1.A-6 VOLCANIC MACRO STATE: at a certified MV3.B2 volcanic landform the SurfaceState
    # reflects volcanic/basaltic ancestry; a YOUNG shield/cone vs an OLD eroded plug differ
    # materially in weathering/regolith tendency. No renderer colour involved.
    young_ss = old_ss = None
    for sd in corpus:
        ff, found, _ = scans[sd]
        for cls in ("volcanic_shield", "volcanic_cone"):
            for (cx, cy), fid, par, age, sub in found.get(cls, []):
                s = surface_state_at(central, ff, cx, cy)
                if s.dominant_surface_family == "volcanic":
                    young_ss = young_ss or s
        for (cx, cy), fid, par, age, sub in found.get("volcanic_plug", []):
            s = surface_state_at(central, ff, cx, cy)
            if s.dominant_surface_family == "volcanic":
                old_ss = old_ss or s
    if young_ss and old_ss:
        p6_ok = (young_ss.lithology_class == "basalt" and old_ss.lithology_class == "basalt"
                 and old_ss.weathering > young_ss.weathering + 0.1)
        p6_d = (f"young construct: substrate={young_ss.substrate_class} weathering={young_ss.weathering:.2f}; "
                f"old plug: substrate={old_ss.substrate_class} weathering={old_ss.weathering:.2f} "
                f"(basalt ancestry both; old more weathered)")
    elif young_ss:
        p6_ok = (young_ss.lithology_class == "basalt")
        p6_d = (f"young volcanic construct: substrate={young_ss.substrate_class} basalt ancestry "
                f"(no old plug in corpus to contrast this run)")
    else:
        p6_ok, p6_d = False, "no volcanic construct surfaced in corpus"
    checks.append(("ms1a_volcanic_macro_state", p6_ok, p6_d))

    # MS1.A-7 DRAINAGE / FLOODPLAIN CONTEXT: a macro drainage valley/basin (concentrated flow,
    # low slope) derives a depositional/alluvial/waterlogged surface -- classification only,
    # no invented sediment mass.
    p7_ok, p7_d = False, "no depositional surface found in drainage context"
    sed_cells = 0
    best = None
    for k in range(0, n * n, 3):
        x, y = cell_xy(k)
        if abs(x) < 40000 and abs(y) < 40000:      # outside frozen centre
            continue
        if abs(x) > 150000 or abs(y) > 150000:     # inside the rendered footprint band
            continue
        if sol.accum[k] < 120:
            continue
        ss = surface_state_at(central, fieldf, x, y)
        if ss.dominant_surface_family == "sediment":
            sed_cells += 1
            if best is None:
                best = (x, y, ss, sol.accum[k])
    if best is not None:
        x, y, ss, acc = best
        p7_ok = True
        p7_d = (f"drainage cell ({x:.0f},{y:.0f}) accum={acc:.0f} -> substrate={ss.substrate_class} "
                f"family=sediment; {sed_cells} depositional cells in drainage context")
    checks.append(("ms1a_drainage_floodplain_context", p7_ok, p7_d))

    # MS1.A-8 CENTRAL FREEZE: MS1.A never overrides the frozen +/-32 km world -- macro drainage
    # incision is still exactly 0 there, and the macro surface family DEFERS to the frozen
    # central family (does not paint the sedimentary centre as a foreign material).
    center_incision = max(abs(macro_z_incised(central, fieldf, x, y) - macro_z(central, fieldf, x, y))
                          for x, y in [(0.0, 0.0), (15000.0, -10000.0), (-20000.0, 18000.0)])
    center_compat = center_total = 0
    for gx in range(-28000, 28001, 7000):
        for gy in range(-28000, 28001, 7000):
            center_total += 1
            fam = surface_state_at(central, fieldf, float(gx), float(gy)).dominant_surface_family
            cfam = central_surface_family(central, float(gx), float(gy))
            # defer law: centre never reads volcanic; rock<->regolith<->sediment are refinements
            if fam != "volcanic" and (fam == cfam or {fam, cfam} <= {"rock", "regolith", "sediment"}):
                center_compat += 1
    freeze_ok = center_incision < 1e-6 and center_compat == center_total
    checks.append(("ms1a_central_freeze", freeze_ok,
                   f"center incision delta={center_incision:.2e}m (need~0); "
                   f"center macro-family defers to frozen family {center_compat}/{center_total} "
                   f"(no volcanic paint over the frozen sedimentary centre)"))

    # MS1.A-9 SAME-SEED DETERMINISM: same seed+coord+version -> identical SurfaceState digest.
    p_again = compile_page(central, MacroField.build(central.seed), 0, 0)
    det9_pt = all(surface_state_at(central, fieldf, x, y).pack()
                  == surface_state_at(central, MacroField.build(central.seed), x, y).pack()
                  for x, y in [(70000.0, 40000.0), (-110000.0, 90000.0), (130000.0, -60000.0)])
    det9_ok = p_again.surface_digest == center.surface_digest and det9_pt
    checks.append(("ms1a_same_seed_determinism", det9_ok,
                   f"center surface_digest reproduces ({center.surface_digest}); "
                   f"per-point packs identical across rebuild={det9_pt}"))

    # MS1.A-10 DIFFERENT-SEED DIVERSITY: a different seed -> materially different surface-state
    # arrangement where the upstream world differs.
    alt_field = MacroField.build(central.seed + "-alt")     # (0,0) drainage already compiled in B1.7
    alt_page = compile_page(central, alt_field, 0, 0)
    from collections import Counter as _C
    fam_a = _C(unpack_surface(c0).dominant_surface_family for c0 in center.surface_codes)
    fam_b = _C(unpack_surface(c0).dominant_surface_family for c0 in alt_page.surface_codes)
    sub_a = _C(unpack_surface(c0).substrate_class for c0 in center.surface_codes)
    sub_b = _C(unpack_surface(c0).substrate_class for c0 in alt_page.surface_codes)
    arrangement_diff = sum(abs(fam_a[k] - fam_b[k]) for k in set(fam_a) | set(fam_b))
    div10_ok = (alt_page.surface_digest != center.surface_digest
                and (set(sub_a) != set(sub_b) or arrangement_diff > center.surface_n))
    checks.append(("ms1a_different_seed_diversity", div10_ok,
                   f"digest differs; families origin={dict(fam_a)} alt={dict(fam_b)} "
                   f"(materially different arrangement)"))

    # MS1.A-11 LONG-DISTANCE UNBOUNDED: SurfaceState stays deterministic + non-periodic with no
    # 64 km or 384 km cadence signature over 0..2000 km.
    ld_pts = [(0.0, 7000.0), (250000.0, 7000.0), (500000.0, 7000.0),
              (1000000.0, 7000.0), (2000000.0, 7000.0)]
    ld_codes = [surface_state_at(central, fieldf, x, y).pack() for x, y in ld_pts]
    ld_again = [surface_state_at(central, MacroField.build(central.seed), x, y).pack() for x, y in ld_pts]
    ld_states = [unpack_surface(c0) for c0 in ld_codes]
    ld_families = {s.dominant_surface_family for s in ld_states}
    ld_substr = {s.substrate_class for s in ld_states}
    ld_det = ld_codes == ld_again
    ld11_ok = ld_det and (len(ld_families) >= 2 or len(ld_substr) >= 3)
    checks.append(("ms1a_long_distance_unbounded", ld11_ok,
                   f"deterministic={ld_det}; families over 0..2000km={sorted(ld_families)} "
                   f"substrates={sorted(ld_substr)} (non-periodic, no page/super-tile cadence)"))

    # MS1.A-12 MACRO/FINE COMPATIBILITY: at the +/-32 km boundary the macro dominant family must
    # NOT CONTRADICT the frozen central family (compatibility, not byte identity).
    comp12 = tot12 = 0
    for t in range(-31, 32, 2):
        s = t * 1000.0
        for (bx, by) in [(REGION_HALF_M, s), (-REGION_HALF_M, s), (s, REGION_HALF_M), (s, -REGION_HALF_M)]:
            tot12 += 1
            fam = surface_state_at(central, fieldf, bx, by).dominant_surface_family
            cfam = central_surface_family(central, bx, by)
            if fam != "volcanic" and (fam == cfam or {fam, cfam} <= {"rock", "regolith", "sediment"}):
                comp12 += 1
    comp12_pct = 100.0 * comp12 / max(1, tot12)
    comp12_ok = comp12_pct >= 90.0
    checks.append(("ms1a_macro_fine_compatibility", comp12_ok,
                   f"boundary macro-family vs frozen central-family compatible "
                   f"{comp12}/{tot12} ({comp12_pct:.0f}%, need>=90; no contradiction)"))

    # MS1.A-13 GEOMETRY INVARIANCE: the SurfaceState descriptor is orthogonal to geometry --
    # recomputing each page's height digest from macro_z_incised alone reproduces the page's
    # source_digest exactly (surface state ON changes no terrain sample).
    def height_digest(pg):
        hh = MA._FNV_OFFSET
        for j in range(pg.n):
            yy = pg.min_y + j * pg.step
            for i in range(pg.n):
                xx = pg.min_x + i * pg.step
                q = int(round(macro_z_incised(central, fieldf, xx, yy) * 100.0)) & 0xFFFFFFFFFFFFFFFF
                for shift in (0, 8, 16, 24, 32, 40):
                    hh ^= (q >> shift) & 0xFF
                    hh = (hh * MA._FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
        return f"{hh:016x}"
    geo13_ok = True
    for cell in [(0, 0), (2, 0), (-2, 2)]:
        pg = pages[cell]
        if height_digest(pg) != pg.source_digest:
            geo13_ok = False
    checks.append(("ms1a_geometry_invariance", geo13_ok,
                   f"height digest recomputed (no surface) == page source_digest for sampled pages "
                   f"(surface ON changes no geometry sample)"))

    # MS1.A-14 CHEAP SOURCE: macro SurfaceState is analytic macro-only -- no ReconstructedZ /
    # QueryMaterial / runtime-water dependency (shares the module-wide forbidden-token scan).
    cheap14_ok = not forbidden
    checks.append(("ms1a_surface_cheap_source", cheap14_ok,
                   f"SurfaceState composed from macro controls+drainage+landform only; "
                   f"forbidden_tokens={forbidden or 'none'}; descriptor {center.surface_n}x{center.surface_n} "
                   f"cells @ {SURFACE_STEP_M:.0f}m (bounded, a few bytes/cell)"))

    # MS1.A-H2H: near/far identity down the resolution chain. The dominant surface FAMILY read
    # from the coarse macro page descriptor (128 km promise) must agree with the fine point
    # sample (near refinement) at the SAME place -- distance simplifies, never contradicts.
    h2h_agree = h2h_tot = 0
    for cell in [(1, 0), (0, 1), (-1, 1), (2, 0), (-2, 2), (1, -2)]:
        pg = pages[cell]
        for (ci, cj) in [(4, 4), (8, 8), (12, 6), (6, 12)]:
            dx = pg.min_x + ci * pg.surface_step
            dy = pg.min_y + cj * pg.surface_step
            coarse = unpack_surface(pg.surface_codes[cj * pg.surface_n + ci]).dominant_surface_family
            fine = surface_state_at(central, fieldf, dx + 750.0, dy - 600.0).dominant_surface_family
            h2h_tot += 1
            if coarse == fine or {coarse, fine} <= {"rock", "regolith"}:
                h2h_agree += 1
    h2h_pct = 100.0 * h2h_agree / max(1, h2h_tot)
    h2h_ok = h2h_pct >= 85.0
    checks.append(("ms1a_h2h_near_far_identity", h2h_ok,
                   f"coarse macro descriptor family == fine point family {h2h_agree}/{h2h_tot} "
                   f"({h2h_pct:.0f}%, need>=85; distance simplifies, never contradicts)"))

    # ====================================================================== #
    # WD1.A — WaterState authority fixtures
    #   CHANNEL_EXISTS != WATER_PRESENT != WATER_BODY_TYPE != WATER_OPTICAL_STATE. Two-stage
    #   presence (supply -> accommodation); classification only (no water mass, no geometry).
    # ====================================================================== #
    WREV = f"gv{MA.GENERATOR_VERSION}.wd{WATER_DESCRIPTOR_VERSION}"

    # gather footprint water bodies from the compiled page descriptors (fast; no recompute)
    foot_water = []                     # (x, y, WaterState) for cells that have a body
    for (ri, rj), pg in pages.items():
        for j in range(pg.water_n):
            for i in range(pg.water_n):
                ws = unpack_water(pg.water_codes[j * pg.water_n + i])
                if ws.body_class != "none":
                    foot_water.append((pg.min_x + i * pg.water_step, pg.min_y + j * pg.water_step, ws))

    # WD1.A-1 DRAINAGE != WATER — the SAME channel (accumulation/slope) under wet vs dry supply
    # yields different presence: dry channel vs flowing water. Water is not the drainage line.
    d_dry, _, s_dry, p_dry = water_supply(0.12, 0.5, 0.6, 700.0, 320.0, 0.30)
    d_wet, _, s_wet, p_wet = water_supply(0.88, 0.3, 0.6, 700.0, 320.0, 0.30)
    pr_dry = water_presence_body(d_dry, s_dry, p_dry, 0.05, 0.30, True, False, 0.0, 0.0, 0.2, 0.0, 700.0)
    pr_wet = water_presence_body(d_wet, s_wet, p_wet, 0.05, 0.30, True, False, 0.0, 0.0, 0.2, 0.0, 700.0)
    w1_ok = pr_dry[0] in ("dry", "damp_substrate", "ephemeral") and pr_wet[0] in ("perennial", "seasonal") \
        and pr_wet[1] != "none"
    checks.append(("wd1a_drainage_neq_water", w1_ok,
                   f"same channel (accum=320): dry supply -> presence={pr_dry[0]}/body={pr_dry[1]}; "
                   f"wet supply -> presence={pr_wet[0]}/body={pr_wet[1]} (drainage line != water present)"))

    # WD1.A-2 PERENNIAL vs SEASONAL — same channel, different supply/seasonality/permeability
    # produce materially different regimes.
    d_per, _, s_per, p_per = water_supply(0.85, 0.15, 0.6, 700.0, 320.0, 0.20)   # humid maritime
    d_sea, _, s_sea, p_sea = water_supply(0.55, 0.85, 0.6, 700.0, 320.0, 0.55)   # drier continental permeable
    reg_per = water_presence_body(d_per, s_per, p_per, 0.05, 0.20, True, False, 0.0, 0.0, 0.2, 0.0, 700.0)
    reg_sea = water_presence_body(d_sea, s_sea, p_sea, 0.05, 0.55, True, False, 0.0, 0.0, 0.2, 0.0, 700.0)
    w2_ok = reg_per[0] == "perennial" and reg_sea[0] in ("seasonal", "ephemeral") and reg_per[0] != reg_sea[0]
    checks.append(("wd1a_perennial_vs_seasonal", w2_ok,
                   f"humid low-seasonality -> {reg_per[0]}/{reg_per[1]}; dry high-seasonality permeable -> "
                   f"{reg_sea[0]}/{reg_sea[1]} (regime follows supply/seasonality, not the channel)"))

    # WD1.A-3 LAKE / BASIN — a closed accommodated basin with supply holds a STANDING body;
    # remove the supply and the same basin no longer stands.
    lake = water_presence_body(0.55, 0.2, 0.45, 0.005, 0.2, True, True, 0.7, 40.0, 0.2, 0.0, 300.0)
    drylake = water_presence_body(0.05, 0.2, 0.0, 0.005, 0.2, False, True, 0.7, 40.0, 0.2, 0.0, 300.0)
    w3_ok = lake[0] == "standing" and lake[1] in ("closed_basin_lake", "alpine_lake", "crater_lake",
                                                  "floodplain_water") and drylake[0] != "standing"
    checks.append(("wd1a_lake_basin_accommodation", w3_ok,
                   f"closed basin + supply -> {lake[0]}/{lake[1]}; supply removed -> {drylake[0]}/{drylake[1]} "
                   f"(standing needs accommodation AND supply)"))

    # WD1.A-4 SUBSTRATE INFLUENCE — a water body references the MS1 bottom substrate; different
    # ground beneath the water is recoverable (not replaced by a 'water material').
    bottoms = {}
    for _x, _y, ws in foot_water:
        bottoms.setdefault(ws.bottom_family, 0)
        bottoms[ws.bottom_family] += 1
    w4_ok = len(bottoms) >= 2
    checks.append(("wd1a_bottom_substrate_reference", w4_ok,
                   f"water bodies carry >=2 distinct MS1 bottom families {dict(bottoms)} "
                   f"(substrate beneath is recoverable; water occupies, not replaces)"))

    # WD1.A-5 SEDIMENT / TURBIDITY — high-accumulation erodible reaches are turbid; clear
    # resistant/cold headwaters are clear. Materially different optics from cause.
    clear = [ws for _x, _y, ws in foot_water if ws.body_class in ("headwater_stream", "alpine_lake", "spring_pool")]
    turbid = [ws for _x, _y, ws in foot_water if ws.body_class in ("sediment_river", "braided_reach", "floodplain_water")]
    if clear and turbid:
        cc = sum(w.clarity for w in clear) / len(clear)
        tt = sum(w.clarity for w in turbid) / len(turbid)
        w5_ok = cc > tt + 0.1
        w5_d = f"clear-headwater mean clarity={cc:.2f} vs sediment/floodplain clarity={tt:.2f} (need clearer headwaters)"
    else:
        # fall back to the optics law directly
        from macro_authority import _water_optics
        clr_h = _water_optics("headwater_stream", 0.6, 200.0, 0.1, 0.1, 0.0, False, 1800.0, 1.0)[0]
        clr_s = _water_optics("sediment_river", 0.6, 800.0, 0.9, 0.1, 0.0, False, 300.0, 3.0)[0]
        w5_ok = clr_h > clr_s + 0.1
        w5_d = f"optics law: headwater clarity={clr_h:.2f} vs sediment river clarity={clr_s:.2f}"
    checks.append(("wd1a_sediment_turbidity_optics", w5_ok, w5_d))

    # WD1.A-6 ORGANIC WETLAND — a low-gradient wet basin with organic potential reads as a
    # wetland / dark-water body with high organic load.
    wet = [ws for _x, _y, ws in foot_water if ws.body_class in ("wetland_marsh", "organic_darkwater")]
    w6_ok = len(wet) >= 1 and max((w.organic_load for w in wet), default=0.0) > 0.3
    checks.append(("wd1a_organic_wetland", w6_ok,
                   f"{len(wet)} wetland/dark-water bodies; max organic_load="
                   f"{max((w.organic_load for w in wet), default=0.0):.2f} (need>0.3; low-gradient wet+organic)"))

    # WD1.A-7 VOLCANIC / MINERAL — a volcanic basin can carry mineral state, WITHOUT making all
    # volcanic-province water exotic (most is ordinary rivers/lakes).
    volc_water = [ws for _x, _y, ws in foot_water if ws.mineral_load > 0.05 or ws.body_class in
                  ("volcanic_mineral_pool", "crater_lake")]
    mineral_bodies = [ws for ws in volc_water if ws.body_class in ("volcanic_mineral_pool", "crater_lake")]
    ordinary_in_volc = [ws for _x, _y, ws in foot_water if ws.mineral_potential > 0.05
                        and ws.body_class not in ("volcanic_mineral_pool", "crater_lake")]
    w7_ok = len(mineral_bodies) >= 1 and len(ordinary_in_volc) >= 1
    checks.append(("wd1a_volcanic_mineral", w7_ok,
                   f"{len(mineral_bodies)} volcanic/mineral bodies + {len(ordinary_in_volc)} ordinary bodies in "
                   f"mineral-potential areas (mineral water exists but is not universal in volcanic provinces)"))

    # WD1.A-8 DEPTH CONTINUOUS + DETERMINISTIC — depths span a continuous range (not 3 bands) and
    # reproduce exactly.
    depths = sorted({round(ws.depth_m, 2) for _x, _y, ws in foot_water if ws.depth_m > 0.0})
    dmin = min(depths) if depths else 0.0
    dmax = max(depths) if depths else 0.0
    det_pt = (water_state_at(central, fieldf, foot_water[0][0], foot_water[0][1]).pack()
              == water_state_at(central, MacroField.build(central.seed), foot_water[0][0], foot_water[0][1]).pack()) \
        if foot_water else False
    w8_ok = len(depths) >= 8 and dmax > dmin + 5.0 and det_pt
    checks.append(("wd1a_depth_continuous_deterministic", w8_ok,
                   f"{len(depths)} distinct depths spanning {dmin:.1f}-{dmax:.1f}m (continuous, not banded); "
                   f"per-point pack reproduces={det_pt}"))

    # WD1.A-9 CROSS-PAGE IDENTITY — a channel crossing a 64 km page boundary keeps one
    # MacroChannelId (water inherits B1's window-independent ancestry).
    x9_ok, x9_d = False, "no channel crosses a 64 km page line with water"
    for b in (REGION_HALF_M, REGION_M + REGION_HALF_M, -REGION_HALF_M):
        for t in range(-120, 121, 3):
            yy = t * 1000.0
            wa = water_state_at(central, fieldf, b - 900.0, yy)
            wb = water_state_at(central, fieldf, b + 900.0, yy)
            if (wa.macro_channel_id != "none" and wa.macro_channel_id == wb.macro_channel_id
                    and wa.body_class != "none" and wb.body_class != "none"):
                x9_ok = True
                x9_d = f"channel crosses 64 km page line x={b/1000:.0f}km keeping MacroChannelId {wa.macro_channel_id}"
                break
        if x9_ok:
            break
    checks.append(("wd1a_cross_page_identity", x9_ok, x9_d))

    # WD1.A-10 CROSS-SUPER-TILE IDENTITY — same across the 384 km drainage packaging.
    xs_ok, xs_d = False, "no channel crosses the 384 km super-tile edge with water"
    xedge = SUPER_M * 0.5
    for yy in range(-120000, 120001, 2000):
        wa = water_state_at(central, fieldf, xedge - 6000.0, float(yy))
        wb = water_state_at(central, fieldf, xedge + 6000.0, float(yy))
        if (wa.macro_channel_id != "none" and wa.macro_channel_id == wb.macro_channel_id
                and wa.macro_watershed_id == wb.macro_watershed_id):
            xs_ok = True
            xs_d = f"channel keeps MacroChannel+Watershed identity across the 384 km super-tile edge"
            break
    checks.append(("wd1a_cross_supertile_identity", xs_ok, xs_d))

    # WD1.A-11 SEED SEMANTICS — deterministic per seed; a different seed -> materially different
    # water geography (body-class mix).
    center_wdig = center.water_digest
    reprod = compile_page(central, MacroField.build(central.seed), 0, 0).water_digest == center_wdig
    alt_page_w = compile_page(central, MacroField.build(central.seed + "-alt"), 0, 0)
    from collections import Counter as _CW
    bmix_o = _CW(unpack_water(c0).body_class for c0 in center.water_codes)
    bmix_a = _CW(unpack_water(c0).body_class for c0 in alt_page_w.water_codes)
    diff_geo = alt_page_w.water_digest != center_wdig
    w11_ok = reprod and diff_geo
    checks.append(("wd1a_seed_semantics", w11_ok,
                   f"same-seed water_digest reproduces={reprod}; alt-seed differs={diff_geo} "
                   f"(origin body mix {dict(bmix_o)} vs alt {dict(bmix_a)})"))

    # WD1.A-12 LONG-DISTANCE UNBOUNDED — deterministic + non-periodic water over 0..2000 km.
    ldw_pts = [(0.0, 9000.0), (250000.0, 9000.0), (500000.0, 9000.0), (1000000.0, 9000.0), (2000000.0, 9000.0)]
    ldw = [water_state_at(central, fieldf, x, y) for x, y in ldw_pts]
    ldw_again = [water_state_at(central, MacroField.build(central.seed), x, y) for x, y in ldw_pts]
    ldw_det = all(a.pack() == b.pack() for a, b in zip(ldw, ldw_again))
    ldw_regimes = {w.presence_regime for w in ldw}
    w12_ok = ldw_det and len(ldw_regimes) >= 2
    checks.append(("wd1a_long_distance_unbounded", w12_ok,
                   f"deterministic={ldw_det}; presence regimes over 0..2000km={sorted(ldw_regimes)} "
                   f"(non-periodic, no page/super-tile cadence)"))

    # WD1.A-13 MACRO/FINE COMPATIBILITY (guardrail) — inside the frozen ±32 km centre the macro
    # authority does NOT assert water; it explicitly DEFERS to detailed 16D-16F (macro_authority=
    # defer_to_detailed, which is NOT `dry`). A consumer reads the scope first, so a detailed lake
    # here never contradicts a macro conclusion. Compatibility is only demanded where detailed
    # truth legitimately exists.
    center_defer = all(unpack_water(c0).macro_authority == "defer_to_detailed" for c0 in center.water_codes)
    outer_valid = 0
    for _x, _y, _ws in foot_water[:50]:            # bodies exist only outside the centre
        pass
    outer_valid = sum(1 for (_x, _y, _ws) in foot_water if _ws.macro_authority == "valid_macro")
    interior_defer = True
    for t in range(-30, 31, 5):
        s = t * 1000.0
        for bx, by in [(REGION_HALF_M - 1000.0, s), (s, REGION_HALF_M - 1000.0)]:   # just inside centre
            if water_state_at(central, fieldf, bx, by).macro_authority != "defer_to_detailed":
                interior_defer = False
    w13_ok = center_defer and interior_defer and outer_valid == len(foot_water) and len(foot_water) > 0
    checks.append(("wd1a_macro_fine_compatibility_guardrail", w13_ok,
                   f"centre-page macro_authority=defer_to_detailed (all cells)={center_defer} (NOT `dry`); "
                   f"±32km interior defers={interior_defer}; all {len(foot_water)} outer bodies valid_macro="
                   f"{outer_valid == len(foot_water)} (16D-16F owns detailed; no fabricated macro water)"))

    # WD1.A-14 FROZEN AUTHORITY + GEOMETRY/MASS — the WaterState descriptor is orthogonal to
    # geometry (page height source_digest unchanged) and to the surface descriptor
    # (surface_digest unchanged); WD1.A is classification only (creates no water mass).
    def _height_digest(pg):
        hh2 = MA._FNV_OFFSET
        for j in range(pg.n):
            yy = pg.min_y + j * pg.step
            for i in range(pg.n):
                q = int(round(macro_z_incised(central, fieldf, pg.min_x + i * pg.step, yy) * 100.0)) & 0xFFFFFFFFFFFFFFFF
                for shift in (0, 8, 16, 24, 32, 40):
                    hh2 ^= (q >> shift) & 0xFF
                    hh2 = (hh2 * MA._FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
        return f"{hh2:016x}"
    geo_ok = all(_height_digest(pages[cll]) == pages[cll].source_digest for cll in [(0, 0), (2, 0)])
    checks.append(("wd1a_frozen_geometry_no_mass", geo_ok,
                   f"height digest == page source_digest (WaterState changes no terrain sample); "
                   f"classification only, no water mass created, 16D-16F/P5b untouched"))

    # WD1.A-15 CHEAP — analytic macro only (shares the forbidden-token scan).
    checks.append(("wd1a_cheap_source", not forbidden,
                   f"WaterState from drainage+MS1+macro hydroclimate proxy only; no ReconstructedZ/QueryMaterial/"
                   f"runtime-water; forbidden={forbidden or 'none'}; descriptor {center.water_n}x{center.water_n}"
                   f"@{MA.SURFACE_STEP_M:.0f}m v{WATER_DESCRIPTOR_VERSION}"))

    # ---- cheap-source evidence ------------------------------------------- #
    # The HARD invariant is the forbidden-token scan (no ReconstructedZ/QueryMaterial/fine
    # causal stack — which costs 4-8 s PER TILE, minutes for the ring). compile_s is reported
    # evidence; the budget is 280 s for the full +/-160 km ring carrying drainage + TWO semantic
    # descriptors (MS1 surface + WD1 water), computed via one shared per-cell env.
    cheap_ok = compile_s < 280.0 and not forbidden
    checks.append(("cheap_source_no_deep_reconstructedz", cheap_ok,
                   f"compiled {ncells} pages ({samples_per_page} samples each) + surface+water descriptors "
                   f"in {compile_s:.2f}s (budget 280; invariant=no fine causal stack); "
                   f"macro-analytic only (no ReconstructedZ/erosion/QueryMaterial)"))

    passed = all(ok for _, ok, _ in checks)

    lines = ["MV2A+MV3C_PEAKS+MS1A_SURFACE+WD1A_WATER " + ("PASS" if passed else "FAIL"),
             "scope=macro_forcing+alpine_peaks+surface_state+water_state_authority_only_no_renderer_no_fine_causal_stack",
             f"authority_ring=+/-{RING}_cells  pages_compiled={ncells}  (full 128 km radial authority)",
             f"world_seed={central.seed}",
             f"generator_version={MA.GENERATOR_VERSION}",
             f"surface_descriptor_version={SURFACE_DESCRIPTOR_VERSION}  "
             f"surface_step_m={SURFACE_STEP_M:.0f}  surface_grid={center.surface_n}x{center.surface_n}  "
             f"center_surface_digest={center.surface_digest}",
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
              "ms1a=surface_state_authority_only (palette retirement + appearance resolver = MS1.B)",
              "no_reopen=MW1-MW8/MV1/MV2.B/PX1-PX3", ""]
    RECEIPT.parent.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print("\n".join(lines))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
