# Grass sway V1 — 2026-09-07

**Follow-up:** the first live attempt hit a shader allocation assertion. The
initial build receipt below is historical, not a known-good playable certificate.
See `GRASS_SWAY_LAYOUT_FIX.md` for the corrected allocation/logging paths and
replacement Base/Engine DLLs. Live preview validation remains pending.

## Implemented

Existing playable grass now supplies exact root-to-tip weights in procedural
UV0 and uses two vertical card segments. Root vertices have zero displacement.
Both crossed cards of a tuft share one wind field. A shader-only horizontal
breeze combines coherent traveling waves and per-tuft variation. Default tip
amplitude is 0.04 m; `grasscover.material` can set `m_windTipMeters` to zero to
disable motion or up to 0.05 m. The shader clamps the range independently.

Wind phase follows ordinary game delta time, independent of accelerated grass
regrowth. Integer harmonics produce a continuous 64-second loop. It is
presentation data, not a weather simulation or fingerprinted material state.

The shared default mesh shader has an opt-in deformation hook; only
`ProvenanceGrassSoilPBR` opts in. Its reserved grass-normal marker additionally
excludes soil/stone/ordinary meshes. Rest positions travel through the unused
color varying for atlas evaluation, keeping the existing cutout/gutter/spill
rules while lighting receives actual deformed world positions.

Procedural geometry registration now accepts optional displacement padding.
Only grass requests 0.06 m. Both instance bounds and meshlet sphere radii expand;
triangles, collision and matter are not expanded. CPU grass geometry is rebuilt
only by the existing dirty-patch mechanism, never each frame for wind. Wind
phase is queued by gameplay and uploaded at the renderer's ResourceUpdate stage,
after cover-material initialization. The upload is one small material update.

## Source locations

- `Systems/GraniteOutcropLab.cpp`: card segmentation/weights, phase and padding.
- `Code/Engine/Render/Systems/WorldSystem_Render.{h,cpp}`: procedural UV0,
  conservative bounds, world-owned material phase publication.
- `Code/Engine/Render/Shaders/Renderer/DefaultMeshShader.esh`: optional hook.
- `Code/Engine/Render/Shaders/Materials/ProvenanceGrassSoilPBR.esf`: grass-only
  deformation and rest-coordinate atlas contract.
- `Code/Engine/Render/Shaders/Materials/ProvenanceGrassWind.esh`: shared scalar
  formulas compiled by both HLSL and the headless certificate.
- `Data/Provenance/Materials/grass/grasscover.material`: explicit amplitude.

Engine/Data paths above are relative to the client repository. Generated shader
reflection/bytecode were rebuilt with EsotericaReflector, not manually edited.

## Validation and limitations

- Shader input reflection and shader compilation: success.
- `Scripts/RunGrassWindTest.cmd`: 1,420,038 checks, zero failures. Sampled
  displacement at maximum allowed amplitude 0.050797 m, below 0.06 m padding;
  phase-wrap error 0.000000223 m. Tests exact roots, mid-height weighting,
  repeatability, motion and spatial variation using the shader's scalar source.
- Existing grass test: 32,839 checks pass in BOTH optimized and debug runs.
- Debug engine and playable game runtimes rebuilt; grass material force-compiled
  successfully. An initial parallel build hit an output permission/worker error;
  the serial, non-reused-worker retry and subsequent final builds succeeded.
- Published build is not yet visually certified. Headless scalar checks do not
  exercise GPU interpolation, silhouettes or actual frame cost. In play, inspect
  roots, tips, screen-edge culling, pause/resume, trampled grass and scooped soil.
- Card triangle count doubles (two bending segments). Meshes remain static on
  the CPU, but GPU vertex work and initial dirty-patch build cost increase; no
  measured graphics-performance claim is made. Existing no-shadow foliage
  treatment is unchanged. Tips can lean into nearby rock; no wind collision is
  simulated.
- No change to walking/contact geometry, granite/soil matter, grass wear or
  regrowth algorithms. Previously reported movement catches remain open.
- New dry-grass/moss/fungal/lichen placement is a separate reviewed proposal in
  `WORLD_COVER_ASSET_REVIEW.md`, not installed by this sway change.

Build logs are in `Build/Verification/MovementBaseline/grass-wind-*`; the
maintained headless runner writes its executable under `Build/Verification/GrassWind`.
`Fixtures/GrassSwayV1Build.json` records source and published DLL/material hashes.
Those hashes identify this build; they do not establish historical file authorship.
