# MV2.B — 100–128 km Horizon Renderer

The renderer that finally consumes the MV2.A regional macro authority. Presentation
only. The player can now see **real, non-repeating neighbouring world authority out
to 128 km** while the frozen 0–32 km MV1 world is byte-for-byte unchanged.

## What it does

Reads the MV2.A compiled macro pages (`Data/Worldgen/MacroAuthority/*.mcp`) — the
cheap deterministic macro height grids — and renders the 32–128 km horizon **behind**
the frozen MV1 world. It never samples fine `ReconstructedZ` for the outer horizon.

```
0–32 km     FROZEN MV1.C / MV1.D detailed terrain (unchanged)
32–128 km   MV2.B macro-horizon from MV2.A pages
```

### Three-pass depth split (extends the MV1.C two-pass)

```
macro pass    24–130 km frustum   draw resident macro pages (recalibrated aerial fog)
              -> depth-only clear
MV1 far pass  0.128–33 km frustum  draw MV1 terrain OVER the macro (MV1 authoritative at the seam)
              -> depth-only clear
near pass     0.03–600 m frustum   frozen near authoritative world
```

Farthest first; each nearer pass paints over the previous. MV1 remains authoritative
in 0–32 km — MV1 does not move to accommodate MV2; MV2 meets it (MV2.A central
anchoring makes the join continuous by construction).

### Page ownership (sync-safe — PX3 lesson)

Each resident page owns a **persistent per-page VBO**, built once from the page's
height grid and never sub-updated while in flight. No shared-slab `glBufferSubData`.
Bounded residency ring (±2 macro cells) around the player; pages outside retire; no
travel-history growth. Macro material is an elevation-banded palette, hillshaded from
the page gradient (coarse; macro silhouette, not fine geology).

### Aerial perspective recalibrated for 128 km

Frozen MV1.D (`EXP2`, 32 km) drove 32 km to near-sky, which is wrong once the world
reaches 128 km. MV2.B applies one continuous distance-based curve (`GL_EXP`,
ρ=1.85e-5/m) to **both** the MV1 far pass and the macro pass when MV2.B is on, so
0–128 km is one attenuation with no band switch and no 32 km seam: 30 km reads as
solid terrain, 50–128 km attenuates progressively, the far horizon stays readable.
`--mv2b-off` restores the frozen MV1.D curve and draws no macro (off == frozen).

## Certification — all fixtures PASS

`CERT_MV2B_HORIZON_RENDERER.cmd` → `Docs/provenance_mv2b_horizon_cert.txt`. Six
viewpoints (five frozen MV1 stations + a ground-level vista aimed across pages).

| Fixture | Result |
|---|---|
| 1 real 128 km raster | PASS — framebuffer px by distance class **32-50=44053, 50-80=53316, 80-100=10200, 100-128=4926**; far terrain at every station (6/6) |
| 2 non-repetition | PASS — 9 resident pages, 9 unique digests; no 64 km tiling |
| 3 32 km seam continuous | PASS — macro vs frozen MV1 `ReconstructedZ` **max 5.27 m** at 32 km (sub-pixel; no wall/trench) |
| 8 MV2.B off == frozen | PASS — default off; MV1.C re-run **PASS** unchanged |
| 9 no fine authority | PASS — horizon from macro pages only; no `ReconstructedZ`/erosion; build 10.7 ms one-time |
| 10 bounded residency | PASS — 9 pages high-water, 0 runtime allocs after warmup, 0 retires standing |
| 11 no MV2 coverage hole | PASS — terrain-present mask (macro+MV1+near) + authority raymarch: **0** sky pixels ≥32 km are holes (377457 negative space) |
| 12 cardinal 128 km coverage (MV2.A2) | PASS — N/E/S/W each contribute far ≥80 km pixels; 100-128 km class N=4929 E=7813 S=6389 W=1871; full radial 128 km, not just the 3×3 diagonal |

**The 32 km "sky band" question (resolved rigorously).** A cert-only classifier
raymarches the authority for every sky pixel and unions a terrain-present mask across
all three passes. Result: the mid-distance sky band from elevated/divide viewpoints is
**geographic negative space** (line-of-sight clears the lower mid-ground out to the
distant ranges) — **not** missing terrain. Zero holes exist in MV2.B's ≥32 km domain.
The only terrain-vs-sky discrepancies are 33,175 thin **sub-32 km slivers** at
valley-wall silhouettes where **frozen MV1's** LOD far-mesh undershoots the fine
`ReconstructedZ` — MV1's own frozen domain, not an MV2.B coverage defect. Evidence:
`Docs/provenance_mv2b_gapmap_*.ppm` (green=drawn, blue=negative space, purple=MV1 LOD
sliver, red=MV2 hole — none).

Visual: `Docs/provenance_mv2b_horizon_contact_sheet.png` — distant ranges
(ridge_shoulder, panorama), basin negative space (foreland_basin), nested depth
(near → MV1 32 km horizon → macro ranges) with progressive aerial attenuation.

## Performance (PX2/PX3 contracts)

```
GAMEPLAY HARD GATE  Test B 24 m/s soak with MV2.B on   PASS (exit 0)
  engine_cpu > 16.667 = 0   stage-owned misses = 0   presented misses = 0
  cadence non-regression = PASS   192 m near residency complete (2601 packages)
  presented_worst_ms = 16.478
MV2.B cost           build 10.7 ms one-time (warmup); cpu submit 0.003 ms/frame;
                     resident 5.3 MB / 9 pages; 0 runtime allocs after warmup
Test A 240 m/s       unchanged reported stress ceiling (owner remains MV1 tile
                     construction per PX3; MV2 macro pages are cheap, add no gameplay spike)
```

## Runtime

```
Play:  PLAY_MV2B_HORIZON_RENDERER.cmd    (--play-mv2b; --mv2b-off for frozen MV1)
Cert:  CERT_MV2B_HORIZON_RENDERER.cmd    (--cert-mv2b-horizon)
Gate:  ProvenanceClient.exe --cert-streaming-soak-mw8 --mv2b-on   (Test B with horizon on)
```

## Explicitly NOT done (still closed)

MV2.C / another presentation stage, MW9 flora/fauna, vegetation, glaciers/snow,
weather/clouds, groundwater, new erosion/deposition, 128 km detailed worldgen. MW1–MW8
/ MV1 / MV2.A / PX1–PX3 not reopened. MV2.A macro authority unchanged (consumed as-is).

## Board

```
MV2.A regional macro authority   CERTIFIED
MV2.B 100-128 km horizon         CERTIFIED (real 128 km raster, seam-free, non-repeating,
                                 bounded, cheap; 0 MV2-domain coverage holes)
```

## MV2.A2 — full 128 km radial authority (closed)

The initial cut shipped 9 pages (±1 / ±96 km square; 128 km only on the diagonal).
**MV2.A2** extends the same continuous deterministic field to the **±2 ring — 25
pages** (`Tools/Worldgen/cert_macro_authority.py`), giving neighbouring authority to
128 km+ in every direction. No new macro-generation semantics; just more compiled
pages. Authority cert (`Docs/provenance_mv2a_macro_authority_cert.txt`): all 25 page
IDs/digests deterministic + unique; continuity across the new ±96 km ring seams
(0.0 m anomalous step); no square-world signature over ±160 km; cardinal ±2 reaches
160 km with real relief (N 4544 m, S 2537 m, W 835 m, E 398 m); features cross the
±2 ring; cheap (25 pages / 1.3 s). The renderer's ±2-cell ring now loads all 25
pages (bounded: 25 high-water, 0 runtime allocs after warmup) and the horizon cert's
**fixture 12** proves 100-128 km framebuffer pixels in N/E/S/W as well as diagonal.

Remaining for later (not blocking): macro relief reads modestly at the 1 km page step
from central viewpoints; and surface appearance is still a diagnostic elevation
palette — a coarse long-distance **semantic** presentation (rock / vegetated / desert
/ wetland / snow-when-authorized, derived consistently across MV1 and MV2) is the
natural follow-on, deferred by decision.
