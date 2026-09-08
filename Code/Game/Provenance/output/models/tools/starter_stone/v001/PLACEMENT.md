# Starter stone shovel, axe and pickaxe — placement helper

[Full procedural placement guide](../../../../ASSET_PLACEMENT.md)

Status: Three models and native resources installed; independent E pickup compiled. Corrected runtime staged; resource-server lock must be released for final installation.

All proposed numerical ranges are artistic starting points, not implemented C++ defaults.

Recommended files:

- [stone_shovel.blend](stone_shovel/stone_shovel.blend)
- [stone_shovel_lod0.glb](stone_shovel/stone_shovel_lod0.glb)
- [stone_shovel_lod1.glb](stone_shovel/stone_shovel_lod1.glb)
- [stone_axe.blend](stone_axe/stone_axe.blend)
- [stone_axe_lod0.glb](stone_axe/stone_axe_lod0.glb)
- [stone_axe_lod1.glb](stone_axe/stone_axe_lod1.glb)
- [stone_pickaxe.blend](stone_pickaxe/stone_pickaxe.blend)
- [stone_pickaxe_lod0.glb](stone_pickaxe/stone_pickaxe_lod0.glb)
- [stone_pickaxe_lod1.glb](stone_pickaxe/stone_pickaxe_lod1.glb)
- [manifest.json](manifest.json)
- [starter_tools_lineup.png](starter_tools_lineup.png)

## Habitat

Starter supply cache, primitive campsite, excavation area, woodcutting station, workshop or deliberate selection display.

## Placement and scale

Each tool is independent. Authored metre-scale heights: shovel 1.19 m, axe 0.84 m, pickaxe 0.99 m. Sample the full rotated footprint for loose placement and keep the useful head visible above ground cover. Upright sandbox poses are fixed on the granite display. Use measured bounds, head_joint and grip/strike anchors from the manifests; centre asymmetric heads by their mounting joint, not their overall bounds.

## Variation and grouping

Use varied yaw for loose finds; preserve dimensions unless deliberately adapting to character size. GLB LOD1 is supplied; automatic runtime LOD selection is not connected.

## Interaction and persistence

Aim within 2 m and press E. Each of four starter-display tool kinds, including the hammer, is collected independently once per Play session. Release E between pickups. Named prompts, terrain obstruction, and input-focus checks apply. New Play restores items; R terrain reset retains ownership.

## Import limits and remaining work

The current adapter is a development-lab collectible list with four fixed entity names, not saved inventory or unlimited procedural instances. No equip/swing/durability/tier effects, falling, dropping or general decorative-prop occlusion. F harvesting is unchanged. Interactive editor E check remains pending.

Source/version-specific STATUS.md, README.md and manifests remain relevant. This note is generated from output/placement/asset_profiles.json.
