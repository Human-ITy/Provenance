# WD1.B — Shared Water Appearance

Second WD1 cut (design `c2f9bd55`; WD1.A authority `78284b1d` + scope `f0a62f2d`).
**Presentation only** — a shared `WaterState + MS1 bottom + depth → WaterAppearance` resolver
that renders the macro water the WD1.A descriptor already knows about, as an **overlay over the
MS1 substrate**. No authority, no water mass, no geometry change; the WD1.A pages are unchanged
(pure C++ consumption).

## The one question it answers

> Can the renderer show water's real optical character — bottom visible in the shallows,
> attenuating continuously to the body's colour with depth, and clear / sediment / organic /
> mineral bodies visibly diverging — from the WD1.A WaterState + the MS1 bottom substrate,
> **never "water = blue," never depth colour bands**?

## What was built

- **`Ms1::WaterState` + `Ms1::UnpackWater`** (in `Ms1SurfaceAppearance.h`) mirroring the WD1.A
  Python packed descriptor exactly (presence/body/flow/bottom-family/depth/optical axes/hooks/
  `macro_authority`).
- **`Ms1::ResolveWaterAppearance(WaterState, bottom RGB, depth, sky, fresnel)`** — the single
  optical resolver, an absorption/transmission system (NOT a palette):
  `observed = bottom·T(depth) + body_optical·(1−T) + surface_reflection`, with
  `T = exp(−k·depth)` and `k = 0.10 + 1.35·turbidity + 0.85·organic + 0.25·sediment`. Shallow →
  the MS1 bottom shows through; deep → the body's optical colour dominates; **continuous, no
  bands**. `WaterBodyOptical` derives the body colour from cause: clear cold blue-green, sediment
  olive-brown, organic tea-dark, mineral turquoise. Surface reflection is modulated by flow
  (still reflects more) and a fresnel term.
- **Macro water overlay (MV2/MV3):** `Mv2Page` parses the WD1.A water descriptor (`Mv2WaterAt`);
  `Mv2BuildTile` emits a flat water quad at `surface_z` (= terrain + depth) for each
  **macro-authoritative STANDING** cell, coloured by `ResolveWaterAppearance` over the MS1
  bottom; drawn as a second pass over the terrain in `DrawMv2Horizon`. **Water is an overlay —
  the terrain geometry/colour is untouched.** Rivers are sub-cell at macro scale and deferred
  (the representation doctrine: keep identity, do not exaggerate sub-pixel water); detailed
  16D–16F water stays separately owned.
- **`macro_authority` respected:** only `valid_macro` + present bodies render; the frozen ±32 km
  centre (`defer_to_detailed`) renders **no** macro water (16D–16F owns it) — never read as `dry`.
- **Opt-in flag** `--wd1b` / `--play-wd1b` (default OFF == frozen no-macro-water presentation;
  `--wd1b-off` forces frozen). `--cert-wd1b` (optical cert), `--cert-wd1b-showcase` (real-client).

## Certification — `Docs/provenance_wd1b_water_appearance_cert.txt` — **PASS**

| Fixture | Result |
|---|---|
| optical depth law | PASS — bottom→body distance grows monotonically with depth (0.15→20 m: 0.09→0.20), shallow shows bottom, deep near body; **continuous, no bands** |
| families diverge | PASS — clear/sediment/organic/mineral deep colours separate by ≥0.159 (need >0.08) |
| geometry / water-truth invariant | PASS — WD1.B on/off `ReconstructedZ` **bit-exact** (overlay only; no water mass; 16D–16F/P5b untouched) |
| macro descriptor consumed | PASS — resident page carries 10 standing-water cells the resolver reads (no recompute) |

**Perf/compat:** MW8 24 m/s Test B with `--wd1b` (`--cert-streaming-soak-mw8 --mv2b-on --wd1b`)
— **PX2 all-green** (engine_cpu 0-over worst 11.7 ms, 0 presented/stage-owned/os-gap misses,
cadence no-regression, 0 movement frames > 16.667). MV1 below-terrain coverage is **unaffected**
(the water overlay is macro-only, mv2b-domain; MV1 coverage runs mv2b-off). `--wd1b-off`
reproduces the frozen presentation (the water build is gated; no water geometry is emitted).

## Visual evidence — `Docs/provenance_wd1b_water_showcase.png` (real C++ client `--cert-wd1b-showcase`)

- **Volcanic crater lakes (27 km):** mineral **turquoise** water bodies stand out against the
  near-black basalt bottom — chemistry-driven optical character over the MS1 substrate.
- **Closed-basin / organic lowland (17 km):** darker organic water in the green wetland lowland.
- **Lowland water (44 km wider):** standing bodies read across the horizon as material water.

## HARD CLOSED (later cuts)

Waterfalls / foam / spray / cascades (**WD1.C** — `waterfall_potential` is a reserved hook),
macro river presentation (sub-cell at macro scale), detailed-water re-routing through the shared
resolver (16D–16F stays separately owned for now), volcanic/mineral chemistry, snow/glaciers,
weather/season, flora, full PBR. Geometry, water mass/occupancy, 16D–16F/P5b, MS1 SurfaceState,
and all frozen cuts are invariant.

## Board

```
WD1     design                     LOCKED @ c2f9bd55
WD1.A   WaterState authority       CERTIFIED @ 78284b1d (+ scope f0a62f2d)
WD1.B   shared water appearance    CERTIFIED (this cut)
WD1.C   waterfalls / cascades      NEXT / READY FOR DESIGN
FL1     flora · AT1 atmosphere     CLOSED
```
