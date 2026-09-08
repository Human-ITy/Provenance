# Guild hammer pickup — placement helper

[Full procedural placement guide](../../../../ASSET_PLACEMENT.md)

Status: Modeled and installed in the Debug sandbox; E pickup implemented and compiled; interactive editor check pending.

All proposed numerical ranges are artistic starting points, not implemented C++ defaults.

Recommended files:

- [GuildHammer.blend](GuildHammer.blend)
- [guild_hammer_lod0.glb](guild_hammer_lod0.glb)
- [guild_hammer_lod1.glb](guild_hammer_lod1.glb)
- [hammer_inventory_icon.png](hammer_inventory_icon.png)
- [manifest.json](manifest.json)

## Habitat

Workshop, workbench, salvage cache, ruin or deliberately placed selection display.

## Placement and scale

Metre scale: 0.662 m tall, 0.389 m head span. Pivot at pommel; grip anchor local (0,0,0.143). One upright sandbox item sits at world (1.03,35.40,0.514029303) on granite. For loose placement, align the whole head/grip footprint to support and retain visibility above grass.

## Variation and grouping

Keep authored size unless deliberate design calls for resizing. Randomize yaw for loose props. LOD1 is supplied but automatic switching is not connected.

## Interaction and persistence

Aim within 2 m and press E to collect the hammer once per Play session. It shares the four-kind collector with the stone shovel, axe and pickaxe; each is independent. HUD names the last collected tool and shows the count. Terrain obstruction and text input block collection. New Play restores it; R terrain reset retains ownership.

## Import limits and remaining work

Development-lab collection only, not persistent inventory or equipping. Four fixed tool names support one instance each; further procedural duplicates require unique instance records. No rigid-body falling or general decorative-prop occlusion. See starter-stone integration status for the latest shared runtime.

Source/version-specific STATUS.md, README.md and manifests remain relevant. This note is generated from output/placement/asset_profiles.json.
