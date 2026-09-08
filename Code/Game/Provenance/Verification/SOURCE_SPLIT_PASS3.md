# Source split, pass 3: formation and surface fields

## Accepted visual context

The user supplied screenshots `codex-clipboard-d9057d13-c622-497f-8178-7751a4408744.png`
and `codex-clipboard-161353d3-e38f-410b-8036-050162241a13.png` after pass 2,
reporting welcomed granite staining/contrast and grass improvements. These are
the current user-accepted visual reference. Their cause was not independently
traced; do not claim the source split changed material authority or that
geometry fingerprints certify rendering. This pass intentionally makes no
visual changes. The user closed the editor and authorized continuation.

## Implemented

- Extracted profiles, formation/surface-stone descriptor generation, joint
  fields, curved surface events and relief evaluation into separately compiled
  `Geometry/GraniteFormation.cpp`.
- Moved `EvaluateGraniteOutcropGeometry` with the formation code it evaluates.
- Added `Geometry/GraniteFormationInternal.h` for three existing helpers used
  by both formation and the remaining solid/rooted geometry:
  `GetGraniteFormationRotation`, `GraniteSuperellipseRadius`, and
  `GetGraniteFormationLocalCoordinates`.
- Public declarations, types, algorithm versions, tolerances and formulas are
  unchanged. Private shared helpers retain internal linkage in a named namespace.
- `GraniteGeometry.cpp` decreased from 19,034 to 14,128 physical lines.
  The new formation implementation is 4,878 lines, including its wrappers.
- Added the compilation unit to both Game Runtime and ProvenanceVerifier
  projects/filters, and the private header to Game Runtime's project/filters.
- Updated both maintained isolated compile/link recipes to include the new unit.

Sections and helper bodies were moved mechanically without arithmetic edits.
Read-back text comparison confirmed the intended extraction in all three files.

## Actual validation

- Before editing, rebuilt the authority sources with the maintained Debug
  recipe and confirmed the preserved baseline still matched.
- After editing, rebuilt all eight authority source units. Full report text
  and all 32 granite/surface fingerprints match
  `Fixtures/GraniteGeometrySplitBaseline.txt`; the comparison gates exit status.
- Granite: 16 cases, 1,156 gate checks, 27 negative controls, zero failures;
  tool strike 4 cases / 268 checks; local excavation 88 checks; determinism
  16 fingerprints / 48 checks.
- Surface: 16 cases / 16 fingerprints / 32 determinism checks, two negative
  controls, zero failures.
- Actual Game Runtime and ProvenanceVerifier Debug/x64 MSBuild projects succeed.
  The actual rebuilt verifier executable exits zero and its complete report
  also matches the preserved baseline.
- The isolated Game Runtime recipe compiles the current adapter, host and all
  three geometry units and links successfully.
- Playable Outcrop V2 optimized and Debug each pass 8,684 checks, zero failures.
  Initial volume/mass remains 9 m3 / 24,300 kg and excavation accounting closes
  as before. These headless checks do not certify interactive rendering.
- All 42 inventoried Geometry/Data entries other than the intentionally edited
  `GraniteGeometry.cpp` retain their previous hashes. The authored shape,
  terrain/grass logic, materials, textures and playable map are untouched.

Builds used `BuildProjectReferences=false` and `PreBuildEventUseInBuild=false`
with the checkout root as `SolutionDir`; existing engine/Base dependencies were
reused. Eight resource helper processes still held the runtime DLL after the
editor closed. Their executable paths and loaded DLL were verified; only those
seven resource compilers and one resource server were stopped. The broad
process-killing pre-build script was not run. The live Game Runtime DLL has
been updated successfully and is ready for the next interactive validation.

## Remaining boundaries

No post-build interactive smoke test or Release/Shipping build was performed.
The movement seam assertion remains deferred, unchanged and not rerun.
Solid geometry/fracture, rooted separation and the remaining debug workbench
still need staged responsibility-based extraction. No Git staging, commit,
push or cleanup deletion occurred. Existing local validation recipes remain
machine-specific, not portable build-system replacements.
