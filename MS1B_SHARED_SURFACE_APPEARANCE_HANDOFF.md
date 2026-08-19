# MS1.B — Shared Surface Appearance + Near/Far Semantic Continuity

Second MS1 cut (design `04d2c4ab`; MS1.A authority `b970585b`). **Presentation only** —
one shared C++ `SurfaceState → appearance` resolver consumed by BOTH the near MV1 surface
and the MV2/MV3 macro horizon. No authority, no geometry, no water/snow/flora.

## The one question it answers

> Can the renderer show the **same semantic surface identity continuously** from detailed
> near terrain through the 128 km macro horizon, using ONE shared resolver, so distance
> changes fidelity but never changes what the surface fundamentally appears to be?

Yes. There is now exactly **one palette, derived from material** (not from elevation, not
from which render path draws). `far=olive / mid=white / near=gray` is retired.

## What was built

- **`Ms1SurfaceAppearance.h`** — a self-contained resolver (no engine deps):
  - `Ms1::SurfaceState` + `Ms1::Unpack(uint64_t)` mirroring the MS1.A Python packed-code
    bit layout **exactly** (substrate/lithology/family index order identical to the authority).
  - **`Ms1::ResolveAppearance(SurfaceState, wx, wy, distanceM) → base albedo`** — the single
    semantic palette: lithology-family rock albedo, substrate-cover albedo (soil/talus/
    alluvium/volcanic/…), wet-darkening, weathering dulling, exposure pull-back toward host
    rock, and a deterministic grain/breakup whose amplitude is the **only** distance-dependent
    term (full within ~2 km, gone by ~20 km). Distance changes fidelity, never the family.
  - `Ms1::DebugColor(state, axis)` — explicit debug colouring for substrate / lithology /
    family / wetness / weathering / soil-depth / exposure (behind `--ms1b-debug=<n>`).
  - HARD RULES honoured: `organic_capable` is dark humic **soil, never green**; `waterlogged`
    is dark wet **mineral, never a blue water surface**; bare high ground is rock, **never
    snow/white**. Water/snow/flora remain closed (MW9 / WD1 / FL1).
- **Macro consumption (MV2/MV3)** — `Mv2Page` gains the parsed 17×17 SurfaceState descriptor;
  `Mv2SurfaceAt` (nearest-cell, no cross-category interpolation) → `Mv2ShadeSurface` (resolver
  + the same directional hillshade). The renderer **consumes the MS1.A descriptor**; it does
  not recompute macro SurfaceState. Malformed/absent descriptors fail closed to the frozen
  palette for that page.
- **Fine central composition (MV1)** — `FineSurfaceState(x,y)` composes one SurfaceState from
  the certified fine authorities via `regionalBiomeRuntime->QueryBiome(x,y)` (its `.regolith`
  is the full MW7 query: `ProfileClass` = the exposure-precedence substrate winner; `.host` =
  MW2 lithology; drainage→wetness, weatheringIntensity, organicMatterPotential, surfaceStability,
  grain). Same vocabulary, same resolver as the macro path — **no single-diagnostic-layer
  colour query**. `Mv1SampleAuthority` feeds it; MV1's `shadeOf` drops the elevation-brightness
  term under MS1.B (material, not elevation, drives appearance).
- **Opt-in flag** — `--ms1b` / `--play-ms1b` / `--ms1b-on` enable it; **default OFF** so every
  frozen byte-cert (`--mv2b-off`, MV1.C/D, …) is unaffected. `--ms1b-off` forces frozen.
  `--cert-ms1b` runs the semantic cert. Geometry is untouched either way.

## Certification — `Docs/provenance_ms1b_surface_appearance_cert.txt` — **PASS**

A SEMANTIC cert (samples the fine composition + macro descriptor down the resolution chain;
no pixel capture needed for the ladder):

| Fixture | Result |
|---|---|
| near/far continuity | PASS — fine-vs-macro dominant family compatible **120/120 (100%)**; avg luma delta 0.098 (no palette pop) |
| 32 km handoff continuity | PASS — fine(31 km) vs macro(33 km) family compatible **48/48 (100%)** |
| geometry invariance | PASS — MS1.B on/off `ReconstructedZ` **bit-exact** (max_dz = 0) |
| far family diversity | PASS — over 36–150 km: rock 237 / regolith 89 / sediment 13 / **volcanic 621** (4 families) |
| volcanic dark-not-snow | PASS — 621 volcanic macro cells, max luma **0.244**, zero white |
| Certificate C | 4/6 contexts present in the rendered origin world, **4/4 compatible** |

**Certificate C honesty:** `exposed_basalt_volcanic_slope` and `deep_regolith_soil` are
reported **NOT PRESENT** in the frozen central 0–32 km origin world — it is a sedimentary
belt/basin, so those fine contexts genuinely do not occur there. The volcanic proof lives in
the far-diversity ring (621 dark cells) and the MS1.A H2H (coarse descriptor family ==
fine-point family 23/24). Not faked to force a green result.

## Renderer compatibility + performance (MS1.B **on**)

- **MV1 coverage** (`--cert-mv1-coverage --ms1b-on`) — **0 below-terrain holes**,
  `resident_set_delta_under_yaw=0`, resident + authority digests stable. The material recolour
  does not reintroduce holes (terrain tones are earth-toned, never sky-blue).
- **Test B 24 m/s on the MW8 / worldgen presentation path** (`--cert-streaming-soak-mw8
  --mv2b-on --ms1b-on`, stage_filter=38) — **all PX2 gates PASS**: `engine_cpu_over_16_667=0`
  (worst 11.7 ms), `presented_misses=0`, `stage_owned_misses=0`, `os_gap_misses=0`,
  `cadence_regressed=0`, 0 movement frames > 16.667. (The p5b* sim checks read `PASS_idle` —
  the sim kernel is idle at the 192 m live radius; this is the MW8 **render/travel** path, per
  the requirement, and is **not** an MW8-stage worldgen proof. The frozen
  `provenance_mw8_streaming_soak_cert.txt` was restored, not overwritten.)
- **Geometry / water / mass untouched** — MS1.B only writes vertex colours.

## Visual evidence — `Docs/provenance_ms1b_ab_contact_sheet.png`

Real-renderer near/mid A/B (origin world, MV1): **OFF** the terrain is diagnostic **white**
(the biome-CapColor 'alpine' material — the very palette this cut retires); **ON** it reads as
hillshaded material (cool-grey wet rock/regolith with a tan sediment corridor). Plus a top-down
`±160 km` far-world **material** map rendered from the shipped descriptor through the MS1.B
palette — volcanic (dark) / rock / regolith / sediment provinces read as MATERIALS, not an
elevation ramp. The near ON is deliberately plain ("not beauty yet"); MW9/WD1/FL1/AT1 add the
richness later.

## HARD CLOSED (later cuts)

Full PBR / material library, normal maps / microgeometry, columnar-basalt & strata geometry,
WD1 water diversity / depth / colour / waterfalls, FL1 flora / moss / grass, snow / ice, AT1
atmosphere / weather / time, settlement grammar. MW1–MW8 / MV1 / MV2.* / MV3.* / PX1–PX3
frozen; SurfaceState authority (MS1.A) untouched; geometry untouched.

## Board

```
MS1     design                          LOCKED @ 04d2c4ab
MS1.A   SurfaceState authority          CERTIFIED @ b970585b
MS1.B   shared surface appearance       CERTIFIED (this cut)
WD1     water diversity                 NEXT / READY FOR DESIGN
FL1     flora                           CLOSED
AT1     atmosphere / time               CLOSED
```
