# Healing and stamina berry bushes

Two original fictional game plants created in Blender 5.2.1. Red fruit is tagged for healing; blue fruit is tagged for stamina. These roles are game metadata, not implemented stat changes or claims about real plants.

## Assets

- `berry_bushes.blend`: editable ripe bushes, packed textures and separate fruit objects.
- `berry_bushes_ripe.glb`: both bushes together.
- `healing_red_bush_ripe.glb` / `healing_red_bush_harvested.glb`: taller, broad-leaf healing shrub, 24 red berries.
- `stamina_blue_bush_ripe.glb` / `stamina_blue_bush_harvested.glb`: compact shrub with narrower leaves, 30 blue berries.
- `healing_berry_pickup.glb` / `stamina_berry_pickup.glb`: separate single-fruit pickup meshes centered at the origin.
- `berry_bushes_ripe.png` / `berry_bushes_harvested.png`: rendered comparison.
- `harvest_sockets.json`: stable berry IDs and attachment positions, supplied in Blender Z-up and glTF Y-up coordinates, in meters.

## Removable fruit contract

Each berry is a separate mesh node under a named empty attachment node. The berry's calyx/crown is part of the removable fruit. The short fruit stalk is part of the persistent bush body and stays behind.

Harvest one berry by hiding/removing its mesh node and marking its attachment as empty. Preserve the bush body and attachment node. Regrowth can reuse the same attachment transform and stable ID. The fully harvested exports demonstrate removal of all fruit while retaining the same branches, leaves, stalks and sockets.

Node extras carry `asset_id`, `game_role`, `effect_role` or `berry_effect`, and `socket_id` where applicable. A socket's `occupied` flag is true in ripe exports and false in harvested exports. No healing amounts, stamina amounts, harvesting interactions, inventory effects, regrowth timing or save-state integration were added to the C++ client.

## Verification and technical notes

The combined export reimports as 56 meshes: two persistent bodies plus 54 independently removable berries. All textures are embedded. Ripe and harvested GLBs were compared: each body's geometry, normals and UV data are identical, and all socket positions/IDs survive. See `validation.json` and `harvest_validation.json`.

The asset set totals 82,604 triangles. Each berry is 450 triangles including its crown; body meshes are 29,008 and 29,296 triangles. Leaf silhouettes are actual geometry with double-sided materials. Heights are approximately 0.8 m and 0.57 m before outward leaf tips. Individual-bush exports reset their roots to the origin; the combined scene uses display offsets.

These are initial textured models. Collision, LODs, draw-call optimization and game integration remain separate work. The scene's floor, lighting and camera are excluded from game exports.

`build_berry_bushes.py` reproduces the outputs. Save manual edits under a different filename before rebuilding, since the script rewrites its generated assets.
