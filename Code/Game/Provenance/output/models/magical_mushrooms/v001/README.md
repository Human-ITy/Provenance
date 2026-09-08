# Luminous mushroom models

Original Blender 5.2.1 models based on the user's two magical mushroom references.

## Deliverables

- `magical_mushrooms.blend`: editable scene, packed textures, original references and transparent cutouts. Opens with dark scene lighting. Reference image collection is hidden by default.
- `magical_mushrooms.glb`: both designs together, with three mushroom meshes and their optional glow lights.
- `cyan_pair.glb`: the two cyan mushrooms, with the group origin reset for standalone placement.
- `violet_lantern.glb`: the violet mushroom, with the group origin reset for standalone placement.
- `magical_mushrooms_daylight.png` and `magical_mushrooms_darkness.png`: rendered lighting checks.
- `references/cyan_cutout.png` and `references/violet_cutout.png`: genuine RGBA cutouts. The cyan image is 1268 x 1241; violet is 1254 x 1254. The original references are preserved separately.
- `manifest.json` and `validation.json`: model counts, scale, triangle counts and export checks.

## Modeling and appearance

Cyan: two curved fibrous stems, domed blue caps, ribbed luminous undersides, surface light spots, hanging curled filaments and droplets. Heights 29 and 18 cm.

Violet: broad scalloped cap, mottled surface with raised scale details, pleated warm-colored gills and luminous edge veins, hanging ragged collar, textured purple stem. Height 31.5 cm.

The geometry and authored materials interpret the references; they are not exact reconstructions of the pictures. Base-color and tangent-space normal maps are embedded in the exports. This first detailed set totals 92,696 triangles before engine-side optimization: cyan tall 26,716, cyan young 21,980, violet 44,000. Thin cap, gill and collar surfaces are rendered surfaces rather than watertight manufacturing meshes.

## Low-light behavior

The materials emit constantly at restrained strength. Their glow becomes more apparent as environmental lighting falls; no automatic darkness detection or day/night behavior has been implemented. In Cycles, emissive surfaces contribute light. Three optional low-power point lights help represent nearby-ground illumination in real-time rendering.

GLB exports use `KHR_materials_emissive_strength` and `KHR_lights_punctual`. The game importer/renderer must support those features or map the exported emission and lights to its own systems. Bloom, light culling, LODs, collision and C++ runtime integration remain separate. The preview camera, floor and studio lights are excluded from GLB.

## Reference cutouts

The built-in image editor isolated each design onto a contrasting flat color; the bundled chroma helper then created alpha channels. Fine filaments and gill/skirt edges were reviewed over light and dark backgrounds. These AI-assisted extractions complete small regions hidden by foreground vegetation and are not pixel-identical crops. Exact prompts and the original images are retained in `references/`.

The reproducible build is `build_magical_mushrooms.py`. Preserve manual Blender edits under a new filename before rebuilding, as the build rewrites outputs in this directory.
