# World tree placement authoring scene — placement helper

[Full procedural placement guide](../../ASSET_PLACEMENT.md)

Status: v002 offline scene: same 117 saved positions/heights/yaws/IDs, revised tree assets and 24 structural-variant assignments. Original v001 preserved; not integrated into playable client.

All proposed numerical ranges are artistic starting points, not implemented C++ defaults.

Recommended files:

- [world_tree_placements.json](world_tree_placements.json)

## Habitat

Authored mixed groves and regeneration on the existing terrain, with reserved creek, walking corridors and granite lab.

## Placement and scale

Use accepted world_tree_placements.json and its coordinate/root-conforming policy. Keep the original tree models as sources and the layout as instance placement data.

## Variation and grouping

Preserve accepted stable IDs, stage, yaw and scale; do not reroll the saved layout on streaming.

## Interaction and persistence

This scene is an authoring handoff. Existing runtime vegetation and future cutting, litter persistence and root support remain governed by their own systems.

## Import limits and remaining work

Do not treat preview terrain shaders or an offline Blender scene as published runtime integration. Read placement/world_trees_v002/README.md before importing.

Source/version-specific STATUS.md, README.md and manifests remain relevant. This note is generated from output/placement/asset_profiles.json.
