# Guild hammer — pickup asset v001

Reference-led 3D hammer for Provenance's playable sandbox. The broad iron striking face, tapered peen, brass bands and geometric marks, wooden shaft, spiral leather grip, and faceted pommel are modeled geometry. Hidden construction is inferred from the supplied view. The current material treatment is cleaner than the heavily weathered reference.

## Delivery

| File | Use |
|---|---|
| `GuildHammer.blend` | Editable model, packed textures, grip/strike anchor empties |
| `guild_hammer_lod0.glb` | 22,520 triangles, four material parts |
| `guild_hammer_lod1.glb` | 12,380 triangles, matching origin and scale |
| `hammer_review.png` | 1000 × 1200 transparent studio render |
| `hammer_inventory_icon.png` | 512 × 512 transparent icon, supplied for future inventory UI |
| `manifest.json` | Dimensions, coordinates and attachment points |
| `validation.json` | Both GLBs re-imported successfully; UVs, textures, bounds and triangles checked |

The hammer is 0.662 m tall, 0.389 m across the head and 0.121 m deep. Units are metres. This is an authored one-handed scale, not a real-world measurement inferred from the picture. Blender and engine are Z-up; GLB is Y-up and the native importer converts it. Mesh transforms are baked. Pivot is the pommel base; hold it using the grip anchor at local `(0, 0, 0.143)`. GLB geometry excludes the anchor empties; consumers can use the manifest.

## In the playable editor

Native mesh: `data://provenance/tools/hammer/v001/guild_hammer.mesh`.

The sandbox contains one `ToolPickup_GuildHammer` entity at world `(1.03, 35.40, 0.514029303)`, scale 1, upright on the front granite ledge near the starting approach. Walk toward the ledge, aim at the head or handle within two metres, and press **E** when **E - Pick up guild hammer** appears. The model disappears and **Collected: Guild hammer** confirms ownership. Existing F harvesting remains separate.

Start a fresh Play session after reloading `ProvenanceSandbox.map` to see the newly added entity. If the editor has an unsaved map open, preserve that work before reloading; do not overwrite the newer on-disk map with an older editor copy.

Ownership currently lasts for that Play session. Repeated or held E does not duplicate the item. Menus/text input and terrain obstruction block pickup. Stopping Play and starting again restores it; the lab's R terrain reset does not clear ownership. This is a first collectible, not a persistent inventory, equipped hammer, attack animation, drop system, or shipping-game inventory component. It currently lives in the development lab adapter.

## Placement and future integration

- Use a workshop, workbench, ruin, campsite or deliberately placed salvage location. Retain roughly this scale; randomize yaw for loose finds, not dimensions without a design reason.
- Treat the upright ledge pose as a selection display. For natural loose placement, rotate the tool, sample support under the full head and grip, and avoid grass burying the head. Use transformed geometry bounds; the pommel pivot alone is insufficient when laid flat.
- Keep the head/shaft pickup proxies and visual transform together. The current adapter recognizes exactly one named sandbox entity. A multi-item system needs unique instance IDs and inventory records before duplicating pickups procedurally.
- The current proxy detects selection and checks procedural terrain/rock/debris obstruction. It is not rigid-body collision and does not test every decorative prop as an occluder. Do not place the item behind a decorative wall and expect that wall to block selection.
- The displayed item is fixed in place. Excavating its supporting ledge does not yet make it fall. A later loose-object physics adapter should own gravity/support and pickup removal.
- LOD1 is supplied, but automatic engine LOD switching is not installed; the native resource uses LOD0. The icon is supplied but not yet displayed by an inventory panel.

## Verification

Both GLBs were re-imported in Blender, with four meshes, eight embedded 1024² textures, finite geometry, UVs, nonzero-area triangles and matching bounds. The native compiler built 18 resource entries (including the sandbox map) without warnings. The Debug game runtime build passed. Standalone C++ tests pass for aim shape, reach, occlusion, scaled rays, input edge handling, duplicate prevention and a fresh session. An interactive in-editor E press has not been exercised by this task.

Build and native installation records are in `output/integration/hammer_v001`; procedural placement notes are also indexed by `output/ASSET_PLACEMENT.md`.
