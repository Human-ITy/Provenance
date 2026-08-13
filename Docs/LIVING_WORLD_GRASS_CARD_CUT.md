# L1 Textured Grass-Card Cut

Date: 13 August 2026

Base: living-world ladder `3c45c92a` above Stage 12 `2c51bfe9`

Status: PASS

## Implemented prototype

- five deterministic candidate sites per one-metre cell;
- full density through 20 m, smooth thinning to 50 m, and a final height fade;
- short 0.08-0.25 m alpha-tested tufts;
- two crossed cards generally, with a third near card for a real LOD sub-tier;
- exact certified-surface snapping;
- synthetic admission on sandstone/shale fixture caps below 0.65 m rise per
  one-metre cell;
- one linear client-array submission instead of per-vertex immediate-mode calls;
- required 256 x 256 RGBA texture with digest `7587143d2d23d2d4`;
- explicit candidate, rejection, clump, card, triangle, texture, and visual
  telemetry.

This is presentation/load scaffolding only. The admission mask is not soil,
moisture, ecology, growth, plant structure, persistence, occupancy, support,
collision, or interaction authority.

## Repeated certification

| Run | L1 worst frame | L6 combined worst frame | Result |
|---:|---:|---:|---|
| 1 | 11.491 ms | 11.931 ms | PASS |
| 2 | 12.233 ms | 15.180 ms | PASS |
| 3 | 11.197 ms | 15.724 ms | PASS |

Across the six runs and 96 cardinal/movement cases:

- zero frames exceeded 16.667 ms;
- the complete live radius remained 192 m;
- residency remained bounded at 2,601 terrain packages;
- zero sky holes or fallback pixels were found;
- zero authority or collision mismatches were found;
- the workload texture and player-scale visual receipt were present;
- peak L1 grass reached 18,912 active clumps and 77,554 submitted triangles.

The worst L6 margin is 0.943 ms. The prototype passes the hard 60 FPS floor but
does not establish generous capacity for denser vegetation. A production grass
lane should move to persistent GPU buffers or instancing before raising density
or adding wind, shadows, biological admission, disturbance, or persistence.

Visual receipts:

- `Docs/provenance_living_world_load_1_grass_visual.png`
- `Docs/provenance_living_world_load_6_grass_visual.png`

Repeat receipts:

- `Docs/provenance_living_world_load_1_repeat_1_cert.txt` through `_3_`
- `Docs/provenance_living_world_load_6_repeat_1_cert.txt` through `_3_`
