# Full editor-floor landscape — 2026-09-07

## Scope and authority

User requested the entire editor floor, preserving the playable granite lab,
with exposed dirt and a natural, rounded/asymmetric dry creek concept.

Read-only Blender FBX inspection (`InspectEditorFloor.py`) found the authored
`Data/Editor/Floor/floor.fbx` Plane world bounds approximately
(-50.000004,-50.000004,-1.000008) to (50.000004,50.000004,0.000008) metres.
TestFLoor in ProvenanceSandbox.map uses that mesh with identity placement.
The new surface covers the 100 x 100 metre top footprint during PLAY. The
editor's certificate view and authored floor asset are not replaced or deleted.

`Geometry/PlayableLandscape.h` owns the exterior height, material cover, grid,
and triangle-exact movement sampling. `Systems/GraniteOutcropLab.cpp` publishes
one static tile per frame and releases tile meshes with the playable world.
No ongoing mesh rebuild occurs after publication. F9 waits for all tiles before
sampling; F9 still compares grass CASTING, not landscape ON/OFF performance.

The original local soil domain is an exact hole in the extension. Its ground
generator, excavatable soil, granite, editable shape inputs, grass mask and
blade generation were not changed. The extension meets the original perimeter
at local Z=.11 (world Z=.23), blending relief over four metres outside it.
World placement remains X=-1.4, Y=35, Z=.12 for the lab-local coordinate system.

Exterior is currently **walking terrain only**. It does not claim excavation,
soil transport, outer grass wear/regrowth, debris support, or water simulation.
Dense grass blades remain in the original lab domain; elsewhere the existing
grass/soil textures provide ground cover with exposed openings. The material's
reserved UV.x=-2 selects interpolated landscape cover instead of the lab's
finite runtime cover texture; no reflected shader parameter/CPU layout changed.

## Creek recipe limitations

The western meander uses an analytic signed transverse approximation, full
width .3–1.2 m, independent depth .06–.26 m, unequal bank widths, a broad floor,
soft shoulders and small bed variation. This is an authoring recipe, not erosion
physics. It has no overhangs or guaranteed downhill water flow. The 20 cm grid
under-resolves the narrowest 30 cm portions; adaptive creek refinement and a
close-up art review remain necessary before calling the bed finished.

## Verification and publication

- Standalone optimized C++ test: **2,484,357 checks passed**.
  Exterior area 9,883.418 m2 plus preserved core 116.5824 m2 = full floor.
  496,066 triangles; 256 publication tile slots; local surface .044–1.560 m.
  Checked every exterior cell's triangle interpolation, finite/upward geometry,
  editor-floor clearance, material coverage, exact hole and perimeter, and
  bidirectional walking across four creek stations.
- Existing grass suite: **32,839 checks passed** in optimized and debug runs.
- Existing movement-core suite: **37 checks passed** in optimized and debug runs.
- Grass shadow receiving source contracts: **17 passed**.
- Shader reflection/compilation succeeded; Engine Runtime and Game Runtime
  Debug x64 builds succeeded with prebuild process-killing hooks disabled.
- Initial Engine link was refused by remaining compiler DLL locks. Only the
  approved Esoterica resource helpers in this build directory were stopped.
  No unrelated processes were stopped.
- Computer-use check: opened ProvenanceSandbox.map, entered Play, observed
  original granite/grass/assets, then full exterior terrain with HUD 256/256,
  496066 triangles and publication 0.00 ms. Live editor snapshot about 62 FPS,
  RAM 109.7 MB and VRAM 998.7 MB. This is a capped spot observation, **not an
  uncapped GPU benchmark or a landscape before/after cost certificate**.

Published SHA256:

- Engine Runtime: `6327E6BE1F6554FA70BEEB268C216DDB396E3A05F971462F982554843BA309D5`
- Game Runtime: `6B1EC615541E6DA85158C92C28E284DA999393152D07A3B0912230945D44E95F`

## Next validation and extension

Review the creek up close and walk the old/new terrain seam, including after
excavation near the finite lab edge. Measure stationary and moving frame times
at a creek view and wide landscape view, both capped and uncapped. Add bounded
near-player blade patches only with separate cover identity/paging (never
stretch the original 128 x 128 lab mask), then wire incoming dirt textures.
Extend physical excavation/debris support deliberately before advertising those
behaviors across the new floor. Preserve the current working core throughout.
