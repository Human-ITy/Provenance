# MV1 — Multi-Scale Terrain Visibility

MV1 answers one question: can a player inside the certified 192 m interactive
terrain bubble actually **see and understand** the mountains, valleys, basins,
ridges and regional landscape that MW1–MW8 say exist around them?

**Law:** Representation may coarsen with distance; **geographic truth may not.**
All visible terrain bands derive from the *same* absolute-coordinate MW1–MW8
composite authority (`regionalBiomeRuntime->ReconstructedZ`). MV1 is derived
presentation, not a new terrain generator: no decorative backdrops, no skybox
mountains, no far-height noise, no 4096 m wrap, no second terrain world. A
distant mountain at 25 km is the *same* mountain that enters the 192 m near
authority as the player approaches.

192 m is now explicitly **full interactive residency**, not **total visible
world**. Initial visible-terrain target: **32 km** (architecture extends to
100+ km later under MV2, which stays CLOSED).

## Representation hierarchy

```
0–192 m      FULL NEAR AUTHORITY   existing certified terrain; 12.5 cm truth,
                                   materials, collision, dig/water/matter. Untouched.
~128 m–2 km  MESO                  fine 8 m sampling, adaptive to ≤5 m error
~2–20 km     REGIONAL              fine 64 m sampling, adaptive to ≤18 m error
~20–32 km    HORIZON               fine 128 m sampling, adaptive to ≤45 m error
```

Meso underlaps the near terrain (inner 128 m) and each coarser band is z-biased
below the finer one, so the near/far handoff and all band boundaries are
continuous with no gap and no z-fight. The near path draws over MV1, so the
192 m interactive terrain is never disturbed.

## Architecture (fits the current bounded-worker / streaming model)

Not a resurrection of the old synchronous far field. MV1 is a retained
absolute-coordinate tile system:

- **Absolute-world tiles**, keyed by `band | tileX | tileY`, sampling the MW
  composite authority. No wrap; separated locations stay distinct.
- **Terrain-aware adaptive tessellation.** Each tile is sampled at the band fine
  step, then emitted at the *coarsest* resolution (4/8/16/32 cells) whose
  bilinear mesh stays within the band error target. Ridges, valleys, saddles and
  skylines keep fine triangles; genuinely smooth ground uses large faces. This
  is the fix for coarse surfaces flattening landforms — fidelity is measured, not
  assumed.
- **Perimeter skirts** drop each tile edge below the surface, hiding any
  T-junction between neighbouring tiles of different resolution, so a crack never
  shows sky.
- **Incremental frame-budgeted build** (4 ms/frame in play, unbounded only in the
  cert's settle): MV1 is never a hidden main-thread terrain compiler; pending
  tiles drain over subsequent frames, bounded.
- **Band-ring residency**: resident tiles are the band rings around the current
  anchor; tiles outside retire. Resident geometry is bounded by the view working
  set, **never by kilometres travelled** — no travel-history cache.
- **Source-revision lineage**: tiles carry the authority field digest; if the
  authority changes, stale bands are refused and rebuilt rather than mixed.
- **Hillshade + elevation relief shading** aids readability but never substitutes
  for geometry (fidelity is gated separately).

## Certificate

`CERT_MV1_MULTI_SCALE_TERRAIN.cmd` → `--cert-mv1-multi-scale-terrain` →
`Docs/provenance_mv1_multi_scale_terrain_cert.txt`. **PASS.**

Region: 64 km, relief **1829.66 m** (−221.85 → 1607.80 m). Effective visible
range **32 km**. Resident geometry high-water **626 tiles / 65,280 tris**
(bounded). Adaptive histogram: 1662 coarse (smooth) / 291 / 14 refined tiles —
genuinely terrain-aware. No 4096 m wrap. `mv1_seam_failures=0`,
`mv1_rev_mismatch_refusals=0`.

### Five player stations (deterministically located on the real MW geography)

| Station | Location | Elev | Reads as |
|---|---|---|---|
| trunk_valley | (20500, 2000) | 20 m (13%) | valley floor + lake, flank rising to a ridgeline |
| mountain_flank | (7500, −3500) | 858 m (59%), slope 0.95 | steep mountainside facing the summit |
| ridge_shoulder | (6500, −3000) | 1482 m (93%), prom 1444 m | snow-capped ridge/summit dome |
| foreland_basin | (10000, −11500) | −204 m (1%, lowest) | basin floor + lake, distant terrain |
| high_divide | (2000, 5500) | 1608 m (100%, **true global summit**) | snow-capped alpine summit dome over the basin |

All five: `readable=PASS`, `sky_px=0`, captured from **neutral cameras** (eye
+25 m, facing the principal massif, pitch −0.12) — no favorable-camera
substitution. Images `Docs/provenance_mv1_station{0..4}_*.ppm`.

### Six hard fixtures + fidelity + H2H

| Fixture | Result |
|---|---|
| 1 valley_visibility | **PASS** — enclosure 109 m, distant relief 1585 m, 5 secondary ridges |
| 2 ridge_summit_identity | **PASS** — summit order stable, no synthetic peaks (MV1 max ≤ authority) |
| 3 valley_negative_space | **PASS** — cross-valley relief 659 m preserved at coarse sampling (627 m) |
| 4 band_boundary_continuity | **PASS** — max radial jump 8.2 m, all bands resident, seams 0 |
| 5 absolute_no_repeat | **PASS** — separated locations distinct; no 4096 m wrap |
| 6 mv1_off_exact_mw8 | **PASS** — MV1 on/off near-path digest identical (`6d144e3cff00132c`) |
| 7 geometric_fidelity | **PASS** — max error **44.93 m** (≤45 horizon target), mean **0.68 m**, adaptive |
| H2H distant→hand | **PASS** — 32 km massif == 12.5 cm sample: formation `alpine_barren`, same FeatureId |

## Performance (standing gates hold with MV1 active)

Test A (`--cert-worldgen-cardinal-replacement-mw8`) — **PASS 4/4**. Near 192 m
intact: 2601 packages, residency digest `38afaf06ee7edb2b` origin==return,
**movement frames >16.667 = 0** on N/E/S/W. The completeness gate was not
weakened to buy visibility.

Test B 90 s NE fly (`--cert-streaming-soak-mw8`) — **PASS**. 0 frames >16.667
(max 13.818 ms with MV1 active), `mw8_rebuilds=0`, `mw8_compiles=0`, MW1–MW8
travel physics idle. MV1 representation work bounded; no monotonic
travel-history growth. 300/900 not run — MV1 adds no persistent workload.

GPU frame time is not yet instrumented in this harness; the CPU frame gate is
met, and MV1 resident geometry is bounded (65 k tris). Real GPU timing across
multiple view orientations remains future work before extending to 100+ km.

## Off control

`--mv1-off` disables MV1; the near 192 m path is then identity-equivalent to the
MW8 baseline (fixture 6 proves the near-path sample digest is unchanged whether
MV1 is on or off — MV1 mutates no authority, geometry, material, collision, mass,
or worldgen).

## M-stage table

```
MACRO ENVIRONMENT
MW1–MW8                          CERTIFIED

PRESENTATION / SCALE
MV1 multi-scale terrain view     CERTIFIED (32 km)
MV2 extended 100+ km horizon     CLOSED

LIVING WORLD
MW9 flora / fauna                CLOSED
```

## Hard CLOSED

MW9 flora/fauna; tree/grass/animal population; MW climate changes; glaciers;
live snowpack/weather; groundwater; new erosion/deposition; 3C collapse; 16C
remobilization; live P5b / 16D ownership; new macro worldgen semantics; 128 km+
macro certification; MV2 100+ km final horizon. MV1 target is 32 km only.

## Runtime

```
PLAY_MV1_MULTI_SCALE_TERRAIN.cmd     Build\x64_Release\ProvenanceClient.exe --play-mv1-multi-scale-terrain [--mv1-off]
CERT_MV1_MULTI_SCALE_TERRAIN.cmd     Build\x64_Release\ProvenanceClient.exe --cert-mv1-multi-scale-terrain
```

Knobs: `--mv1-off` (disable MV1), plus the existing `--live-radius` /
`--far-extent`. MV1 draws whenever the biome (MW8) view is active.
