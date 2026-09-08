# Revised woodland preview — v002

The existing 117-tree layout now has revised birch foliage, fuller healthy crown tips, less regular spruce tiers, smoother trunk/root flares and foliage-preserving detail levels. **This remains an offline Blender scene; nothing was integrated into EsotericaEditor or the playable map.** The v001 scene and its source models remain available.

- [Eye-level grove preview](creek_birch_view.png)
- [Whole-layout overview](world_overview.png)
- [Editable Blender scene](WorldTreePlacement.blend)
- [Placement manifest](world_tree_placements.json)
- [Saved-position and variant checks](revision_checks.json)
- [Reopened-scene validation](validation.json)
- [Revised model library](../../models/trees/v002/README.md)

All 117 stable IDs, positions, target heights, species/stages and yaw angles were carried forward exactly. The seven groves, open meadow and reserved creek/lab corridors remain. Twenty-four young/grown placements use a new structural variant. Deterministic candidate selection tries the preferred variant, then alternatives, and accepts only candidates that pass the existing terrain, root-footprint and crown-clearance policy. There were no failed placements. This is not a regenerated spatial scatter.

Roots were copied per instance and conformed again to the exported C++ terrain, with the previous 35 mm outer-root / 70 mm collar exposure allowances and 25 mm collar burial. Old deformation arrays were not applied to the revised topology. Imported custom normals were cleared after deformation. Masters remain independent and unchanged by that placement step. These static grounding checks do not simulate loss of support from future excavation.

The scene uses LOD1. Whole-layout triangle totals:

| Detail level | v001 | v002 |
|---|---:|---:|
| LOD0 | 3,059,786 | 3,254,178 |
| LOD1 | 1,728,006 | 1,804,650 |
| LOD2 | 774,168 | 1,266,201 |

LOD1 increases by about 4.4% while retaining the full revised foliage layout. LOD2 is heavier than the old sparse version because it no longer deletes most of the foliage. A separate distant impostor/cluster-bake step is still needed before calling this a completed forest optimization. Counts are not frame-time measurements; alpha overdraw, shadows and resource reuse require actual-client profiling.

Ground/rock shading remains an authoring approximation; grass blades, water rendering, moss and dynamic ecology are not reproduced here. Birch broadleaf litter masks remain separate from tree geometry. Spruce needle litter remains disabled pending its own asset. Previously accumulated litter should persist after tree removal under the future lifecycle system.

Read world_tree_placements.json as world metres, Z-up. The GLB importer must convert Y-up assets once before applying transforms. Preserve root-conforming overrides or reproduce the documented policy. Revised tree crown/root bounds are recorded per instance; old v001 bounds are not authoritative for this scene.

## Reproduction and handoff

Rebuild and validate the v002 model library first. Run ../../models/trees/v002/prepare_layout_review.py to recheck the accepted v001 positions and select eligible revised variants. Then run build_world.py in Blender, followed by reopening WorldTreePlacement.blend and running validate_scene.py. The latter checks all saved transforms, packed textures, finite geometry, triangle totals and unchanged positions/heights/yaws/IDs. Source hashes accompany the review.

The copied terrain exporter and binary terrain are the previously verified source snapshot used for this comparison. For a changed terrain version, export new terrain and rerun placement checks before accepting the result. Do not assume the old terrain snapshot still represents a future edited map. Save manual scene edits separately before rebuilding generated outputs.

No wind, tree cutting/felling, collision registration, runtime LOD switching, resource compilation or map publishing is part of this revision.
