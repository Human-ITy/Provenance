# Procedural asset placement handoff

Start here when importing, scattering, or placing Provenance art. The companion [asset profiles](placement/asset_profiles.json) contain recommended files, habitat, anchors, variation, interaction and limitations. [Asset inventory](placement/asset_inventory.json) lists discovered image/model artifacts and their placement profile; earlier sources and review images are recorded as support rather than promoted to runtime assets.

These files describe asset intent and proposed placement rules. They do not install a scatter system or change C++ gameplay. Numeric scatter ranges are initial art tuning suggestions unless identified as measured/current behavior.

## Current asset selection

<!-- PROFILE_TABLE -->
| Asset family | Status | Placement helper |
|---|---|---|
| Existing living grass | Existing C++ lab material; verify importer paths in component settings. | [Notes](placement/asset_profiles.json) |
| Existing granite material | Existing C++ lab material. | [Notes](placement/asset_profiles.json) |
| Existing soil material | Existing C++ lab material. | [Notes](placement/asset_profiles.json) |
| Dry grass atlas | Clean alpha art; atlas import unfinished. | [Notes](imagegen/dry-grass/PLACEMENT.md) |
| Wild wheat atlas | Clean alpha art; no crop gameplay added. | [Notes](imagegen/wild-wheat/PLACEMENT.md) |
| Woodland loam color source | Color source; not yet seam-refined. | [Notes](imagegen/soil/PLACEMENT.md) |
| Moss carpet and irregular patches | Carpet color source plus clean-alpha patch atlas. | [Notes](imagegen/moss/PLACEMENT.md) |
| Lichen colonies | Clean-alpha 2x2 surface atlas. | [Notes](imagegen/lichen/PLACEMENT.md) |
| Thin fungal surface growth | Clean-alpha 2x2 surface atlas. | [Notes](imagegen/fungus/PLACEMENT.md) |
| Raised mushroom cutout reference | Reference/billboard art; not a UV map. | [Notes](imagegen/mushrooms/PLACEMENT.md) |
| Ordinary mushroom source clusters | Editable originals and cluster export; standalone exports now available. | [Notes](models/mushrooms/PLACEMENT.md) |
| Luminous mushroom source clusters | Detailed textured models; separate exports now available. | [Notes](models/magical_mushrooms/PLACEMENT.md) |
| Standalone mushroom spawn assets | 17 independently placeable GLBs; 20 files including optional lights, reimport checked. | [Notes](models/mushrooms_individual/PLACEMENT.md) |
| Healing and stamina bushes | Textured initial models with verified removable fruit; newer shape references are not remodeled yet. | [Notes](models/berry_bushes/PLACEMENT.md) |
| Updated root-clod bush references | User modeling references; RGB checkerboard is baked in. | [Notes](models/berry_bushes/references_v002/PLACEMENT.md) |
| Woodland leaf-litter tile | 1024-square seam-refined color art; repeat/corner checked. | [Notes](imagegen/leaf-litter/PLACEMENT.md) |
| Creek-bank silty mud tile | 1024-square seam-refined color art; repeat/corner checked. | [Notes](imagegen/mud/PLACEMENT.md) |
| Goblin concepts and turnarounds | Character design/reference images; not map-spawn sprites. | [Notes](imagegen/goblins/PLACEMENT.md) |
| Goblin model and sculpt branches | Authoring candidates on separate branches; not production NPCs. | [Notes](models/goblins/PLACEMENT.md) |
| Static nature-trial exports | Integration visual copies; original sources remain metadata authority. | [Notes](integration/nature_trial_v001/PLACEMENT.md) |
| Norway spruce lifecycle references | Eight stage-specific alpha modeling references with authored metre calibration; tree meshes and runtime scattering remain to be built. | [Notes](imagegen/trees/v001/norway_spruce/PLACEMENT.md) |
| Silver birch lifecycle references | Eight stage-specific alpha modeling references with authored metre calibration; tree meshes and runtime scattering remain to be built. | [Notes](imagegen/trees/v001/silver_birch/PLACEMENT.md) |
| Tree lifecycle scale charts and reference index | Eight stage-specific alpha modeling references with authored metre calibration; tree meshes and runtime scattering remain to be built. | [Notes](imagegen/trees/PLACEMENT.md) |
| Spruce and birch 3D lifecycle models | v002: 24 static tree models (16 stages + 8 variants), three canopy-preserving detail levels each; 96 GLBs reimported. Offline assets, engine registration pending. | [Notes](models/trees/PLACEMENT.md) |
| Creek-bed gravel and fine sediment | 1024-square seam-refined opaque color art; 3x3 repeat and corner join checked. C++ material registration pending. | [Notes](imagegen/creek-gravel/PLACEMENT.md) |
| World tree placement authoring scene | v002 offline scene: same 117 saved positions/heights/yaws/IDs, revised tree assets and 24 structural-variant assignments. Original v001 preserved; not integrated into playable client. | [Notes](placement/world_trees_v001/PLACEMENT.md) |
| Fresh spruce/birch interior wood and stump end grain | Four 1024-square color maps; species/stage assignments cover all 16 trees. Eight conical-taper GLB mapping samples; runtime cutting remains separate. | [Notes](imagegen/cut-wood/PLACEMENT.md) |
| Guild hammer pickup | Modeled and installed in the Debug sandbox; E pickup implemented and compiled; interactive editor check pending. | [Notes](models/tools/hammer/v001/PLACEMENT.md) |
| Starter stone shovel, axe and pickaxe | Three models and native resources installed; independent E pickup compiled. Corrected runtime staged; resource-server lock must be released for final installation. | [Notes](models/tools/starter_stone/v001/PLACEMENT.md) |
| Tree runtime trial derivatives v002 | Concurrent tree integration work; indexed for discoverability, not validated by the starter-tool delivery. | [Notes](integration/tree_trial_v002/PLACEMENT.md) |
<!-- /PROFILE_TABLE -->

Local PLACEMENT.md files sit in the corresponding asset-family folders. Read the source STATUS/README too. A higher filename version is not proof of engine readiness; the goblin sculpt branches in particular are separate authoring experiments.

## Shared placement contract

1. Choose the material/habitat and actual receiving surface before selecting a visual. Use substrate, moisture, light/shelter, slope, disturbance, water state and plant population. Query actual height and normal, including separate cave/overhang surfaces where relevant; an XY-only mask must not paint tunnels below a woodland floor.
2. Create stable candidates from world seed, asset family, stable surface/plant identity and candidate ID. Use world-coordinate sampling across chunk borders, with deterministic ownership, so reloading or visiting chunks in a different order neither moves nor doubles the assets.
3. Persist accepted instance identity, chosen variant, transform, harvest/occupied state and disturbance. Re-evaluate only affected surfaces when excavation or construction changes them. A respawn pass must not overwrite player history.
4. Place at a documented root/stem/foot anchor, never at image-center or an arbitrary group bounding-box center. Blender models are authored in metres and Z-up. GLB is standard glTF Y-up; convert once using the importer. The vector conversion from authoring space is (x,y,z) -> (x,z,-y). Treat quaternions through the importer or a proper basis transform.
5. Sample enough ground beneath each footprint to reject floating or unsupported placement. Mushrooms grow mostly upright on banks; moss/lichen follow receiver normals. Bushes need root-footprint checks, cap/branch clearance and access to harvestable fruit.
6. Apply deterministic uniform scale and yaw within the profile's suggested range. Avoid arbitrary stretching of anatomy or fruit and avoid protruding through neighboring geometry. Reject or reselect bad positions rather than burying half a cap to conceal overlap.
7. Let substrate, living cover, deposited litter, independent harvestable bodies and water retain separate state. A root clod is part of a bush mesh; a litter mask is a ground layer; a berry is an independently removable object.
8. Budget geometry, draw calls, lights, collision and visibility per biome. Many current models are detailed authoring assets. Share meshes/materials and use instancing where supported; create LODs or distant impostors before dense deployment.

## Terrain materials and atlas imports

Continuous soil, mud, moss carpet and leaf litter are opaque material sources blended on terrain. Their square image boundary is not the outline of a world object. Use stable world-space UVs or an appropriate terrain projection with consistent physical scale. Leaf litter v002 and mud v001 were seam-processed and visually checked; the older loam and moss carpet still need seam work.

Do not rotate each square tile independently and expect the edges to match. Use a continuous material-coordinate field, continuous blend masks, or a verified stochastic-texturing method. Small and large scale variation should not change the identity of the underlying soil/rock.

Grass/wheat and surface-growth atlases require per-sprite rectangles, alpha-aware bounds, real root/base anchors, gutter padding and mip-safe color dilation. Nominal equal atlas cells are only a starting layout. Dry grass contains an overhanging tuft and an offset root. The full atlas must never appear on a single terrain card. Choose alpha cutout/blending and two-sided foliage settings to suit the renderer, then test dark/light backgrounds and distant mips.

A color texture is not automatically calibrated albedo. Current image art often has some local shading; normal, roughness, height, collision and water behavior must be tracked separately. Do not infer physical terrain relief directly from color brightness as authoritative geology.

## Litter deposited by plants

Use plant canopy footprints to seed irregular, overlapping litter-coverage masks. Sample the actual receiving terrain beneath and slightly beyond the canopy, with variation driven by plant type, age, shelter, moisture and slope. Established woodland can start with plausible old litter; new plants should contribute over time.

Use the current broadleaf tile as a general decomposing woodland layer. Species-specific blueberry/redberry leaves, conifer needles and other families can later supplement it. Keep live grass and litter amounts separate; they can coexist.

Walking may compress or redistribute litter; digging removes or exposes it with the surface. Removing a plant stops its contribution but does not erase existing litter. Persist coverage/damage so streaming never repaints a worn path. Thin terrain coverage handles most of the view; use sparse leaf/twig geometry only where close-up silhouettes or interaction justify it.

## Bush root clods and removable berries

The new user references retain a root clod deliberately. Model a planting-height marker near the root collar; place most of the clod below terrain, leaving only a subtle root flare or irregular soil ridge exposed. Use multiple terrain samples to avoid a floating clod on a slope. Scatter litter around the exposed base and beneath the canopy without covering all root detail.

Those reference shapes/root clods have not yet been rebuilt into the v001 bushes. They must be preserved in future remodeling. Both reference images contain a baked checkerboard and are modeling references rather than alpha-ready textures.

Existing v001 models have stable fruit sockets: 24 healing berries and 30 stamina berries. Remove only a berry's mesh while retaining the body, stalk and socket. Persist occupancy per socket. The pickup models represent dropped/inventory fruit and should not be scattered alongside every ripe berry. Ripe/harvested exports demonstrate the visual states; effect magnitudes and regrowth rules are not implemented. Static nature-trial copies must not replace the original socket metadata authority.

## Independent mushrooms and optional clusters

Use [standalone mushroom exports](models/mushrooms_individual/v001/README.md). There are 17 independently placeable designs: 3 chestnut, 5 bell, 2 ivory, 4 ochre, 2 cyan and 1 violet.

Every standard GLB contains exactly one mesh at its stem-base origin with its complete cap, stem, gills and any attached filaments. Do not separate by loose mesh islands: several designs use intersecting cap/stem shells, and a loose-parts split would dismantle a single organism.

A cluster is now a placement recipe containing several independent instances. The [manifest](models/mushrooms_individual/v001/manifest.json) retains original cluster-local offsets, file paths, dimensions and footprint radii. To recreate a grouping:
- Transform each recipe offset by a colony yaw/scale.
- Project each stem onto its own qualified surface; do not apply only one height to the whole cluster on uneven ground.
- Test cap/filament clearance, vary variants if desired, and retain a separate instance ID for each mushroom.
- Use a shared colony ID only for ecological grouping. Removing one mushroom never implicitly removes its neighbors.

The default luminous GLBs retain emissive materials without punctual lights. The three optional *_with_light.glb files include only the corresponding specimen's point light. Use an explicit light budget; emission, bloom and actual nearby illumination are distinct. No automatic day/night response is implemented by these exports.

All 20 files were independently reimported and checked for a single mesh, expected triangles/materials/UVs, finite coordinates, origin and bounds. No original cluster .blend was rewritten. Native engine import, collider/LOD preparation and runtime entity hooks remain downstream tasks.

## Goblins and authoring-only assets

Goblin images, headshots, turnaround charts and review renders guide modeling; do not foliage-scatter them. The workshop tinkerer belongs at authored workshop/settlement encounter anchors and the scavenger at road/salvage/ruin anchors as a proposal. A future actor needs navigation, capsule/headroom, foot placement, behavior, persistence and encounter limits.

Static v003 characters, continuous v007 anatomy and Hunyuan sculpt branches have different purposes. No branch is silently designated the final animated character. Material source images inside character work are attached to that character workflow rather than terrain assets. Keep source-license records with derived assets.

## Import acceptance checklist

- Recommended file/profile chosen; actual status and source licenses understood.
- Units/up-axis checked once against terrain; root/foot anchor verified.
- Atlas bounds/gutters or mesh material textures intact; no painted checkerboard imported as transparency.
- Several flat, sloped, creek-edge, root-adjacent and chunk-boundary placements inspected.
- No hovering roots, buried caps, leaf cards on open water, or litter leaking through overhangs.
- Single-instance harvest/removal and save/reload preserve neighbors and remaining sockets.
- Point-light, triangle, draw-call, collision and distant-LOD budgets tested in engine.
- Rebuild scripts do not overwrite manually edited originals; preserve new revisions.

## Updating this handoff

Edit placement/asset_profiles.json when accepting new art or changing intent, then run placement/build_handoff.py to refresh family notes and inventory. The script validates recommended paths and records unsupported/orphan artifacts. It does not designate newly discovered files as approved automatically. The profile table is generated between explicit markers; prose in this guide remains hand-maintained.

