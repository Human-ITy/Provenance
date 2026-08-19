# MS1.A — SurfaceState Authority + Composition

First MS1 implementation cut (design `04d2c4ab`, `MS1_SURFACE_MATERIAL_DESIGN.md`).
**Python world-authority only** (`Tools/Worldgen/macro_authority.py`); the renderer is
**unchanged** and consumes the re-emitted `.mcp` pages. Authority-only: **no palette
retirement, no appearance resolver, no geometry change** (those are MS1.B).

## The one question it answers

> At any absolute coordinate, does the world **know what the exposed surface actually
> is** — substrate, lithology/parent, and material state — independent of how the
> renderer currently colours it?

Yes. One composition law resolves a compact **semantic** `SurfaceState` from existing
certified authorities, with explicit exposure precedence and **no renderer-owned truth**.

## What was built

- **`SurfaceState`** (compact semantic record; **no stored RGB**, no renderer material id
  as authority): `substrate_class · lithology_class · wetness · weathering · soil_depth ·
  stability · organic_potential · exposure · roughness_proxy · dominant_surface_family ·
  source_rev`. State axes are quantised to nibbles; the whole record packs into **≤40 bits**.
- **Vocabulary** (reuses the MW7 regolith-profile vocabulary + the volcanic substrates B2
  needs): substrates `bare_bedrock · weathered_bedrock · thin_regolith · colluvium · talus ·
  alluvium · floodplain_sediment · basin_fill · organic_capable · waterlogged_mineral ·
  fresh_lava · scoria_ash · weathered_basalt · volcanic_soil`; lithologies `granite · basalt ·
  sandstone · shale · limestone · quartzite · metamorphic · mixed_unknown`; coarse families
  `rock · regolith · sediment · volcanic · organic` (the near/far agreement axis).
- **`compose_surface_state(SurfaceInputs)` — Law A exposure precedence** (single-writer;
  the SAME law for macro and fine): exposed depositional body → regolith/soil profile →
  weathered parent → host bedrock, **then** state overlays (wetness→waterlogged promotion in
  fine basin/floodplain deposits; deep+wet+ecological regolith→organic_capable;
  exposure/weathering/stability carried as axes). A volcanic-ancestry province **remaps** the
  chosen bedrock/weathered/regolith tiers to the shared volcanic substrates — same vocabulary,
  same precedence, **not** a parallel path.
- **Macro derivation** (`_macro_surface_inputs` + `surface_state_at`): builds `SurfaceInputs`
  from the MV3 morphology controls + drainage (`drainage_query`) + landform ancestry
  (`landform_at`) — cheap analytic macro proxies of MW2/5/6/7/8, **never** the fine causal
  stack. This is the far-authority **promise**. Inside the frozen ±32 km centre the
  province-specific macro signals are scaled by `anchor_window` (0 in the centre), so the
  macro family transitions continuously to the frozen central family and **never contradicts
  it** (the sedimentary centre is never painted basaltic).
- **Macro page descriptor**: each page gains a **17×17 @ 4 km** `SurfaceState` grid
  (`surface_grid_row_major`, hex-packed) + `surface_digest` + version. A few KB/page; the
  renderer parser **ignores** the new keys (verified) so old and new pages both load. Heights
  and `source_digest` are **byte-identical** to `HEAD` (the page diff is 6 additive lines,
  0 deletions) — geometry is provably untouched.

## Certification — all 15 MS1.A fixtures PASS (`Docs/provenance_mv2a_macro_authority_cert.txt`)

Preserved every prior MV2.A/A2 + MV3.A/B1/B1.1/B2 fixture (49 total, 0 FAIL), **plus**:

| Fixture | Result |
|---|---|
| precedence: deposit over host | PASS — alluvium owns surface; sandstone survives as ancestry only |
| regolith over bedrock | PASS — thin→thin_regolith, deep+wet+ecological→organic_capable; bedrock not exposed |
| bedrock fallback | PASS — fresh→bare_bedrock, mature→weathered_bedrock (deterministic by weathering) |
| wetness overlay | PASS — substrate UNCHANGED; only the wetness axis moves 0.12→0.60 |
| organic potential | PASS — substrate UNCHANGED (alluvium); organic axis 0.20→0.85 |
| volcanic macro state | PASS — young construct scoria_ash/weathering 0.43 vs old plug weathered_basalt/0.88 (basalt ancestry both) |
| drainage/floodplain context | PASS — drainage cell → floodplain_sediment/sediment; 107 depositional cells |
| central freeze | PASS — centre incision 0; macro family defers to frozen family 81/81 (no volcanic paint) |
| same-seed determinism | PASS — surface_digest reproduces; per-point packs identical across rebuild |
| different-seed diversity | PASS — origin regolith/rock-dominant vs alt organic-dominant (materially different) |
| long-distance unbounded | PASS — deterministic, rock↔volcanic over 0..2000 km, no page/super-tile cadence |
| macro/fine compatibility | PASS — boundary macro vs frozen central family compatible **128/128 (100%)** |
| geometry invariance | PASS — height digest recomputed (no surface) == page source_digest |
| cheap source | PASS — analytic macro only (controls+drainage+landform); no ReconstructedZ/QueryMaterial |
| H2H near/far identity | PASS — coarse descriptor family == fine point family 23/24 (96%); distance simplifies, never contradicts |

`compile_seconds=152.95` (< 200 cheap-source budget), the whole 25-page ±160 km ring.

## Renderer compatibility (the descriptor is inert to MS1.A rendering; MS1.B consumes it)

The re-emitted pages differ from `HEAD` only by inert descriptor lines, so the renderer
(existing exe, no rebuild — MS1.A changed no C++) behaves identically:

- **MV2.B** PASS — 25 descriptor pages resident, 25 unique digests, 128 km raster classes
  populated, seam 5.27 m, **0 MV2 coverage holes ≥32 km**.
- **MV2.C** PASS — far-vs-sky separation 0.061 (≥0.03).
- **MV1 coverage** PASS — 0 below-terrain holes, `resident_set_delta_under_yaw=0`, digest stable.
- **Test B 24 m/s (horizon on)** PASS — 90 s NE fly, `--mv2b-on`, default soak kernel
  `stage_filter=29` = **P5b.3B.3B Compaction** (the sim kernel is idle at the 192 m live
  radius — all `PASS_idle` — so the pacing measured is the render/travel path, which is the
  point): `engine_cpu_over_16_667=0` (worst 9.25 ms), 0 stage-owned misses, cadence
  no-regression, 0 movement frames >16.667. The descriptor pages are inert to the sim kernel,
  so the stage choice does not affect the MS1.A result.

## Visual evidence (DIAGNOSTIC category maps, NOT MS1.B appearance)

- `Docs/provenance_ms1a_surface_state_maps.png` — family / substrate / lithology / wetness /
  weathering / soil-depth / exposure over the ±160 km origin world. The frozen centre reads
  rock/regolith/granite (belt as a rock streak, **not** volcanic); a volcanic province to the
  E/N; sediment corridors trace the drainage; coherent state fields.
- `Docs/provenance_ms1a_seed_corpus.png` — surface family across a seed corpus: origin
  volcanic-dominant, one seed rocky, one organic/vegetated, one mixed — **different seed →
  different surface world**.

## What MS1.A is NOT (HARD CLOSED here — MS1.B and later)

Diagnostic-palette retirement, the shared `appearance()` resolver, PBR/normal maps/material
microgeometry, columnar-basalt/strata geometry, water diversity/colour/depth, snow/ice, flora
population, atmosphere/weather/time. Inside ±32 km the descriptor **defers** to the frozen fine
MW authority (MS1.B / detail-on-approach); it is certified non-contradicting for the rendered
origin world. MW1–MW8 / MV1 / MV2.* / MV3.* / PX1–PX3 frozen; central geometry untouched.

## Board

```
MS1     design                         LOCKED @ 04d2c4ab
MS1.A   SurfaceState authority         CERTIFIED
MS1.B   shared appearance resolver     NEXT (retire diagnostic palette + Certificate C)
WD1     water diversity                CLOSED
FL1     flora                          CLOSED
AT1     atmosphere / time              CLOSED
```
