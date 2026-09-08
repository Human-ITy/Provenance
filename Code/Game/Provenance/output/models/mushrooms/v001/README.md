# Raised mushrooms — initial 3D set

Created in Blender 5.2.1 LTS from the visual forms in the mushroom atlas.

- `raised_mushrooms.blend`: editable scene with packed textures and a separate preview stage.
- `raised_mushrooms.glb`: 14 textured meshes organized under four group nodes; stage excluded. 2.76 MB.
- `raised_mushrooms_preview.png`: rendered overview.
- `manifest.json`: names, dimensions and triangle counts.
- `validation.json`: successful GLB reimport with all 14 meshes, UVs and materials.

Chestnut group: 3 mushrooms. Bell group: 5. Ivory group: 2. Ochre group: 4.

Each mushroom has a curved tapered stem, modeled cap and ribbed underside, UVs, and shared base-color materials. Six 512-square color maps are packed into the Blender file and embedded in the GLB. Textures were created for these meshes; they are an approximate interpretation of the atlas, not a projection or reconstruction of its photographic detail. Shape and surface refinement can continue in Blender.

Scale is meters, with heights from 4.2 to 15.5 cm. Each mesh origin is at its own ground-contact point. Group transforms arrange the preview; reset those for standalone spawning. GLB uses standard glTF Y-up conversion. Total: 53,536 triangles, 3,824 per mushroom. Caps and stems are separate intersecting closed shells within each object.

These are initial art models. No LODs, collisions, gameplay behavior, native engine import or C++ runtime integration have been added.

The reproducible build is in `build_mushrooms.py`; it rebuilds outputs in this directory, so preserve manual Blender edits under a new filename before running it again.
