# Woodland loam color source — placement helper

[Full procedural placement guide](../../ASSET_PLACEMENT.md)

Status: Color source; not yet seam-refined.

All proposed numerical ranges are artistic starting points, not implemented C++ defaults.

Recommended files:

- [soil_continuous_v001_color_source.png](soil_continuous_v001_color_source.png)

## Habitat

Bare woodland earth beneath grass, litter and bushes.

## Placement and scale

Opaque terrain material layer, never a raised square plane. Proposed source scale about 1 m/tile, tune against leaves and stones.

## Variation and grouping

Blend with litter and mud using substrate/moisture/cover masks.

## Interaction and persistence

Excavation reveals the correct soil material; litter and live vegetation may be removed separately.

## Import limits and remaining work

Opposing-edge continuity remains unfinished. Apparent local lighting remains; no calibrated PBR kit.

Source/version-specific STATUS.md, README.md and manifests remain relevant. This note is generated from output/placement/asset_profiles.json.
