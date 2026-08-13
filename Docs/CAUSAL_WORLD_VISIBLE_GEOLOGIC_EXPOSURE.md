# Stage 7 - Certified Visible Geologic Exposure

Status: certified player-visible reconstruction of the Stage-6 exposure authority.

## Run

Permanent headless certificate:

```text
CERT_CAUSAL_WORLD_VISIBLE_EXPOSURE.cmd
```

Playable runtime:

```text
PLAY_WORLDGEN_STAGE0.cmd
```

Press `7` in game. The menu does not need to be open.

## Scope

Stage 7 adds no geology. It materializes the existing Stage-6 folded
sandstone/shale exposure as a continuous visible and walkable surface.

The reconstruction uses a globally aligned 0.5 m dual surface grid. Each 8 m
presentation block owns a half-open 16 by 16 range of surface crossings and
queries the same canonical halo vertices as its neighbors. A block is published
only after all 512 of its triangles have been generated. This removes the
partial-block residency failure that produced the Stage-6 sky tears.

Player grounding calls the same two-triangle reconstruction used by rendering.
It does not interpolate the older one-metre Stage-6 residency samples.

## Certified gates

- Complete quad coverage with finite, non-degenerate triangles.
- Every interior mesh edge has exactly two owners.
- Only the outer boundary of the 64 m certification patch is open.
- Tiled and monolithic reconstruction have the same geometry digest.
- Every dual vertex lies on the Stage-6 present erosion authority.
- Material and `FeatureId` remain Stage-6 geology queries.
- Formation-contact transitions remain observable through the reconstructed patch.
- Collision and rendering use the same triangle generation.
- Runtime mesh and cell residency remain bounded around the 64 m player neighborhood.

The artifact is:

```text
Docs/provenance_causal_world_visible_geologic_exposure_cert.txt
```

## Still excluded

- No new geology, intrusion, fault, or mineralization.
- No differential erosion or runtime geomorphology.
- No digging, occupancy mutation, support, or detached bodies.
- No groundwater, rivers, water coupling, or P5b.

Differential erosion is implemented separately as Stage 8. It consumes this
watertight surface contract without changing the Stage-7 certificate or silently
adding new geology.
