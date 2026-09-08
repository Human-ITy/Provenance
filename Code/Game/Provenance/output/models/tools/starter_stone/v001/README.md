# Starter stone tools — v001

Three reference-led 3D pickup assets: the starter stone shovel, stone axe and stone pickaxe. Stone heads, timber hafts, crossed rawhide lashings and wrapped hide grips are modeled geometry. The shovel retains the supplied broad spade and open wooden D-grip; its blade is interpreted as stone, following the requested tool tier. The multiview axe reference governs its broad blade, smaller poll, curved handle and green inset.

## Models and scale

| Tool | Editable source | Main GLB triangles | Reduced GLB triangles | Approximate height |
|---|---|---:|---:|---:|
| Stone shovel | `stone_shovel/stone_shovel.blend` | 4,860 | 2,523 | 1.19 m |
| Stone axe | `stone_axe/stone_axe.blend` | 5,964 | 3,098 | 0.84 m |
| Stone pickaxe | `stone_pickaxe/stone_pickaxe.blend` | 6,352 | 3,300 | 0.99 m |

Each tool folder contains two GLBs, an editable packed Blender file, an individual manifest, the archived reference, a 900 × 1200 review render and a 512 × 512 transparent inventory icon. The axe and pickaxe also have front and right orthographic alignment renders. `starter_tools_lineup.png` compares all three at the same scale. Textures are authored 1024² color/normal maps; the material treatment is simplified relative to the photographic references.

Units are metres. Blender and the engine use Z-up; GLB uses Y-up and the native importer converts it. All exported mesh transforms are baked. The haft paths start at local Z=0; their shaped wooden ends extend a few millimetres below that plane. Use the measured bounds in each manifest when settling a model on a surface. Grip, strike and head-joint anchors are included in the Blender files and manifests; GLBs contain only the visual meshes.

## Corrected head alignment

The initial axe and pickaxe lashings were incorrectly positioned independently of their curved handles. Both heads now attach at a joint derived from the handle centreline. All four crossed straps pass over that same joint in the front and rear projections; the wooden locking wedges follow the same handle curve. Front and right orthographic views were inspected after the correction. Matching pickup head shapes were shifted with the models.

Do not centre an asymmetric axe head by its overall bounds. Preserve the `head_joint` anchor: the blade extends farther from the haft than the poll. Similarly, the pick point and adze are intentionally unequal while their mounting joint remains on the haft.

## Sandbox placement and E pickup

The native mesh resources are `data://provenance/tools/starterstone/v001/stone_shovel.mesh`, `stone_axe.mesh` and `stone_pickaxe.mesh` in that same folder. Three separate entities are added to `ProvenanceSandbox.map` around the existing granite display, alongside the hammer:

| Entity | World position, metres |
|---|---|
| `ToolPickup_StoneShovel` | -0.925, 35.670, 0.636327 |
| `ToolPickup_StoneAxe` | 0.030, 35.920, 1.063503 |
| `ToolPickup_StonePickaxe` | 1.750, 36.320, 1.091304 |

These are upright selection-display poses at scale 1. Their bases sit above sampled granite; slight clearance accommodates the uneven ledge. Approach the display, aim at a tool within two metres, then press **E** when its named prompt appears. Each disappears independently and the HUD reports the last collected tool and a count out of four, including the hammer. Holding E while moving between tools does not collect another one: release and press again.

Reload the sandbox map and start a fresh Play session after the updated runtime is installed. Preserve any unsaved editor work before reloading the on-disk map. Consult `output/integration/starter_tools_v001/README.md` and its delivery validation for installation status.

## Procedural placement handoff

- Prefer camps, primitive work sites, abandoned digging areas, woodcutting stations or deliberate starter supply caches. Spawn each tool separately; do not merge them into one collectible mesh.
- Preserve scale unless a deliberate character-size variant is required. Keep visual transforms and pickup proxies together. Existing runtime transforms support rotation and uniform scale.
- For a loose tool, support its full rotated shape on the sampled surface, not just its pivot. Keep the useful head visible above grass and ground litter. Upright display poses are fixed props, not balanced rigid-body simulations.
- Use the grip anchor for a later held-tool attachment. The shovel has separate upper-grip and support-hand anchors; axe and pickaxe have a lower grip. Use strike anchors for later contact/animation authoring.
- This adapter recognizes one instance of each of four fixed entity names. Additional duplicates or unlimited procedural item instances require per-instance identifiers and inventory records; do not duplicate names and expect independent ownership.
- Collection lasts only for the current Play session. Stopping and restarting Play restores the tools; the lab's R terrain reset retains ownership. No save persistence, equip/swing animation, dropping, durability or tool-tier effects are added here. F harvesting keeps its existing behavior.
- Selection checks shaped heads and shafts, leaves the shovel's D-grip opening empty, and respects procedural rock, soil, terrain and debris obstruction. Decorative prop meshes are not general-purpose occluders. Placed tools do not fall if their supporting rock is excavated.
- LOD1 and transparent icons are supplied; automatic runtime LOD switching and inventory icon UI are not connected.

## Verification

All six GLBs re-imported successfully with UVs, finite geometry, nonzero-area triangles, matching bounds and embedded textures. The pickup tests cover all four tools, reach, head shape, empty grip space, side-on rays, obstruction, input focus, duplicate prevention and independent ownership. Native compilation passed for 56 resource entries. A live editor E press has not been exercised by this task.
