# Creek refinement and selected silty mud — 2026-09-07

## Implementation

Number-one asset placement pass only: refine the dry creek and blend the
provided silty mud color into selected banks and depositional pockets. No
new grass blades, fungi, lichen, mushrooms, bushes, water, or lights.

`PlayableLandscape.h` now samples at 5 cm across the creek corridor
(world X -20.5..-7.5), 12.5 cm along Y, and 50 cm across the surrounding
gentle ground. Same rendered triangles remain walking authority. The
original granite/soil opening and its boundary elevation are unchanged.
The resulting terrain has 694,562 triangles and 231 tile slots (formerly
496,066 and 256). This is a 40% triangle increase, not a free optimization.
Static tiles publish once and require no continuing mesh uploads.

Full creek width remains .3–1.2 m. Narrow portions cap depth around 7.5 cm;
wider portions permit independently varying deeper pockets. Unequal shelf
widths, rounded shoulders and small floor variation form an asymmetric,
single-valued bed, not a true overhang or erosion/hydrology simulation.

Mud is a separate deterministic mask confined to exposed creek sediment.
Alternating pockets and one-sided bank shelves retain unmuddied stretches.
It does not derive water, physical height, or substrate mass from color.
UV.x=-2-mud and UV.y=grass carry static landscape coverage; the existing
lab cover/wear texture and blade shader branch remain separate.

The shared shader adds `m_mudAlbedoTexture`; mud pixels sample one continuous
world-space 90 cm texture period. Derivatives are computed before the mask
branch. No additional decal mesh, normal/height texture, collision or wetness
simulation is fabricated. Existing roughness is retained; normal perturbation
fades toward geometric normals where this color-only source dominates.

F10 is a lab-only creek inspection bookmark/return, available after tiles
finish publishing, outside authoring and an active benchmark. It preserves
material state and rejects returning into occupied granite/soil. R clears the
bookmark. The full-floor extension remains walking-only, not excavatable.

## Source preservation

Source: `output/imagegen/mud/creek_bank_mud_v001_seamless_color.png`.
Unchanged runtime copy: `Data/Provenance/materials/Landscape/` of the same name.
SHA256 for both:
`102A1C8FDA2C5ED6DF45F2D69EDB7231A6F48D4BBEF10C013D36B68E04255008`.
New `creekmud.texture` descriptor uses the existing albedo texture group.
Grass-cover material references it; no artist original was overwritten.

## Evidence so far

- Initial V1 grid: creek analytic/render difference max .175157 m, mean .007242 m
  at 82,000 sampled positions. This caught narrow reaches poorly represented.
- Current recipe/grid: max .018823 m, mean .000546 m over the same sample
  positions. Both recipe and resolution changed, so this is not an isolated
  resolution benchmark.
- 3,906,720 landscape/mud checks passed, including every cell's exact triangle
  interpolation, protected perimeter, floor clearance, mud exclusions, selected
  unmuddied bed portions, size/depth limits and bidirectional creek crossings.
- Movement core: 37 checks passed in optimized and debug builds.
- Generated shader allocation: 4,108 checks passed; logging: 16; retained grass
  shadow receiving source contracts: 17. Regeneration discovered seven layouts.
- Final shader reflection/code generation and Engine/Game Debug x64 builds
  succeeded, including the derivative-hoisting and R/bookmark reset edits.
- Grid creation was about 43–46 ms in optimized headless runs. This is one-time
  initialization, not steady-state frame cost. No new FPS result is claimed.

## Publication — completed, 2026-09-07

User approved stopping resource helpers. The guarded publish attempt found an
Esoterica editor process running and stopped before killing helpers or linking
DLLs. The editor was subsequently confirmed as PID 59376. Asked user to close
it again. This was the earlier blocked attempt, not the final publication state.

After the user's renewed authorization, confirmed the editor was closed and
stopped only the resource server and seven compiler helpers whose executable
paths matched this build's x64_Debug directory. Regenerated shaders, built Engine
and Game with prebuild hooks disabled, and force-compiled `creekmud.texture` and
`grasscover.material`. Both reported "Compiled successfully" (success code 1).

Final rerun: 3,906,720 landscape checks, 4,108 shader allocation checks, 16 logging
checks and 17 grass-shadow source contracts passed. During this run, grid setup
was 74.819 ms and the 82,000 queries plus recipe checks took 39.746 ms; these are
not steady-state GPU timings and should not be compared as isolated query cost.

Published DLL SHA256:

- Engine Runtime: `4D77969EF24ED1F47B476B65C10DA35DE8564C3176E2ED8F1CB9B9E5372A1871`
- Game Runtime: `8255BA04EEF1ED93D759BB5D3AC228648184D121DE547EFEADF286A2FC75FB8E`

Live inspection used the computer-use workflow to launch the editor and load the
sandbox. After input was detected in the editor, a fresh observation showed the
running preview at the creek: 231/231 tiles, 694,562 triangles, grounded player,
visible sediment/grass transitions and the recessed channel. Editor FPS read 62.
Control was left with the user; the editor/preview remains open for inspection.

This validates live loading and a close-up appearance spot check, not exhaustive
visual acceptance. F10 creek view/return is implemented but its full round trip
was not independently exercised in this session. Manual seam traversal, texture
mip behavior in motion, and capped/uncapped stationary/moving performance remain
to be checked. Existing F9 compares grass casting, not isolated creek cost.

## Correction: sediment inside the carve — 2026-09-07

User screenshots rejected the initial bank placement: mud spread over the
shoulders rather than remaining within the depression. The original bank mask
extended to normalized transverse radius 1.9, while the channel ends at 1.0.
The old far-field exclusion assertion did not test that boundary. Earlier test
counts and the live-loading spot check did not establish correct deposition.

`ChannelRemoval` now provides the same channel-minus-floor-ripple depth to both
height construction and mud eligibility. Floodplain lowering does not qualify.
Mud fades in only below 45% of local channel depth and reaches full eligibility
at 80%, retaining selected bottom pockets and lower inner-bank patches. This
is a placement rule, not simulated sediment transport or physical added mass.

A first boundary test caught residual triangle-interpolation bleed. Grid setup
now zeros a mud vertex unless its incident-cell footprint remains inside the
carve (neighboring vertices must have normalized radius below .98). This is a
conservative interior guard, especially in narrow reaches. Bare ordinary soil
outside the creek remains deliberately available for other ground textures.

Verification after correction:

- 4,028,721 landscape checks passed. Additional checks require actual removal
  depth for analytic mud; sample both bank lips and floodplain at 2,000 stations,
  two sides and five radii (20,000 points), using the rendered barycentric mud
  payload; and ensure the guard retains visible bed deposits.
- Maximum interpolated mud at those outside-carve samples: 0.000000000.
- Movement core: 37 checks each, optimized/debug, zero failures.
- Geometry count remains 694,562 triangles / 231 tiles. No added shader samples
  or steady-state mesh uploads. Last optimized grid initialization: 48.930 ms.
  No new live FPS claim is made.

After user confirmed closure, stopped only the validated leftover resource
server and seven compiler helpers. Game Runtime Debug x64 rebuilt successfully
with prebuild hooks disabled. Engine shader/assets did not change in this fix.
New Game Runtime SHA256:
`31DEF43A99E27E06A362BDBDDAB7318D549AA001F57BD644B608A325C11EA6E4`.
Published to the existing playable build; corrected live appearance has not yet
been inspected. Reopen the sandbox and use F10 for visual acceptance.
