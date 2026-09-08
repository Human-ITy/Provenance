# Creek-bank silty mud tile — placement helper

[Full procedural placement guide](../../ASSET_PLACEMENT.md)

Status: 1024-square seam-refined color art; repeat/corner checked.

All proposed numerical ranges are artistic starting points, not implemented C++ defaults.

Recommended files:

- [creek_bank_mud_v001_seamless_color.png](creek_bank_mud_v001_seamless_color.png)

## Habitat

Damp exposed ground, creek banks, silty depositional pockets and wet excavations.

## Placement and scale

The user's mud is the creek-bed undertone beneath and between the gravel surface detail, and becomes more dominant around the curved water-to-bank transition before blending to surrounding soil. Use actual exposed sediment geometry and substrate/moisture/bank masks. Retain the established roughly 0.8-1 m mud period and independently tune gravel to about 0.735 m; do not force identical UV scales. Extend mud into appropriate fine-sediment pockets beneath water.

## Variation and grouping

Pair with imagegen/creek-gravel/creek_bed_gravel_v001_seamless_color.png using broad irregular coverage masks. Let gravel break up into muddier shore margins of varying width. Water and puddles remain separate surfaces/material behavior; footprints and ruts remain separate state/details.

## Interaction and persistence

Wetness may darken/reduce roughness; sediment movement and tracked disturbance need runtime rules. Avoid permanently baking puddles into the color tile.

## Import limits and remaining work

No roughness/normal/height maps or hydrology integration. The creek-gravel color tile is now available; larger raised pebbles/cobbles still need independent mesh assets. Do not bake a fixed waterline into either texture.

Source/version-specific STATUS.md, README.md and manifests remain relevant. This note is generated from output/placement/asset_profiles.json.
