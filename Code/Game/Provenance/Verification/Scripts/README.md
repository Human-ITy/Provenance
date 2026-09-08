# Maintained lab entry points

These 17 scripts preserve the authoring and verification recipes previously stored only in ignored `Build/x64_Debug`. This directory is outside the Build ignore rule; it still needs inclusion in a reviewed Git checkpoint. No commit is implied by its location.

Run a script by its full path, or from this directory. It resolves `Build/x64_Debug` relative to its own location, runs its recipe there, restores the caller's directory and returns the recipe's exit code. Build output location and compiler flags remain unchanged in this migration. Visual Studio's existing hardcoded vcvars64 path is also unchanged.

The old Build scripts remain untouched for compatibility and rollback. Prefer these maintained entry points for future recipe changes. Do not delete the old scripts yet: conversion to forwarding wrappers and full recipe validation are a separate step. This is a preservation migration, not complete removal of all Build dependencies.

## Common entry points

- `RunGraniteLabPlacementTest.cmd`: intact granite translation, mass/volume,
  targeting, collision, undo and refusal checks; optimized and Debug.
  In-game F2 placement controls are documented in `../GRANITE_LAB_PLACEMENT_V1.md`.

Nature asset trial recipes are documented separately in `../NATURE_TRIAL_V1.md`:
`PrepareNatureTrial.py` (Blender derivative export), `PackageNatureTrial.cjs`
(native descriptor preparation/binary installation), `CompileNatureTrial.ps1`
(native resource validation), and `ProbeNatureTrialPlacement.cmd` (read-only
terrain/root placement check). They are not gameplay or harvesting certificates.

- `RunGraniteOutcropV2Test.cmd`: current authored outcrop, optimized and Debug certificates.
- `RunGraniteLabMovementTest.cmd`: movement regression; stops on the first failing configuration.
  Current maintained setup passes 38 checks in each configuration; see
  `../MOVEMENT_INTEGRATION_FIX4.md` for the seam and wall-fixture corrections.
- `RunGraniteMovementCoreTest.cmd`: shape-independent movement invariants and
  bounded-clearance safety checks, in optimized and Debug configurations.
  See `../MOVEMENT_CLEARANCE_FIX2.md`; original integration assertions remain intact.
- `RunGraniteMovementBaselineDiagnostic.cmd`: separate headless diagnosis using
  the playable collision callbacks; runs optimized and Debug despite route
  flags, stores reports in `Build/Verification/MovementBaseline`, and returns
  nonzero while investigation flags remain. The current 32 sampled routes pass;
  this is not a replacement for all original integration cases or live testing.
  See `../MOVEMENT_SLOPE_FIX3.md` for current results and limitations, and
  `../MOVEMENT_BASELINE_DIAGNOSIS.md` for the original findings.
- `RunGraniteLabContactTest.cmd`: contact routing.
- `RunGraniteMovementHotspotDiagnostic.cmd`: observational standing/crouching
  sweep prompted by live crown/ground-edge catches. Stops are reported, not
  automatically classified as defects; exit 0 is not a traversal certificate.
  See `../MOVEMENT_HOTSPOT_DIAGNOSIS.md`.
- `RunGraniteTreadSurfaceTest.cmd`: supporting-surface regressions, optimized
  and Debug, with outputs isolated under `Build/Verification/MovementBaseline`.
  See `../MOVEMENT_SUPPORT_FIX5.md` for the current 99-check suite and live
  publication status, and `../MOVEMENT_SLOPE_FIX3.md` for explicit slope policy.
  The integration setup was corrected separately;
  see `../MOVEMENT_INTEGRATION_FIX4.md`.
- `RunGraniteContactCastTest.cmd`: contact cuts and export evidence.
- `RunGraniteStructuralSupportTest.cmd`: support preparation; forwards arguments to the optimized test as before.
- `RunGraniteLocalUpdateTest.cmd`, `RunGraniteDebrisPerformanceTest.cmd`: update/debris checks.
- `RunGrassCoverTest.cmd`, `RunSoilScoopTest.cmd`: grass and mutable soil.
- `RunGraniteOutcropShapeTool.cmd export <absolute-output.obj>`: export the current authored surface.
- `ApplyGraniteOutcropAuthoring.cmd <absolute-input.obj>`: bake and certify before replacing the live authored header. This command intentionally changes the specimen input on success; rebuild/reload is needed to see the new compiled shape.

Relative input/output arguments are interpreted from `Build/x64_Debug`, matching the old workflow. Prefer absolute paths for artist files, especially files outside Build. Omitting the apply input still selects `Build/x64_Debug/GraniteOutcropAuthoring.obj`. No sculpts or candidate files were relocated in this pass.

## Remaining prerequisites

`CheckOutcropExtraction.cmd` is an additional, local-only source-split check.
It compiles the playable adapter and its debug host, then links an isolated
Game Runtime DLL under `Build/Verification/OutcropExtraction`. Its companion
`OutcropExtractionLink.rsp` reuses existing dependency products and contains
machine-specific paths captured from build logs. It does not run the normal
process-killing pre-build event or replace the live DLL. This is not a clean
build recipe; see `../SOURCE_SPLIT_PASS1.md` for results and limitations.

After source split pass 2, this check also compiles `GraniteGeometry.cpp` and
`GraniteSpallResolution.cpp` before linking; it does not reuse the old geometry
object. `CheckGraniteGeometrySplit.cmd after` is the companion local Debug
authority check. It rebuilds the verifier's current source units into isolated
Build output and compares its report to the preserved pre-split fixture. The
old `baseline` option now refuses to run because the extraction exists. See
`../SOURCE_SPLIT_PASS2.md` for full results and the actual project build status.

Source split pass 3 adds `GraniteFormation.cpp` to both isolated recipes.
The same baseline comparison remains in force; no fingerprint fixture was
updated. See `../SOURCE_SPLIT_PASS3.md` for the extraction and build results.

Source split pass 4 adds `GraniteRootedDetachment.cpp` and
`GraniteRootedPartition.cpp` to both recipes. The same baseline fixture remains
unchanged. See `../SOURCE_SPLIT_PASS4.md` for verification and live build status.

Source split pass 5 adds `GraniteFracture.cpp` to both recipes. The same baseline
fixture still gates the authority check. See `../SOURCE_SPLIT_PASS5.md` for the
construction/fracture split and successful live build results.

Source split pass 6 adds `WorldSystem_Provenance_DebugWorkbench.cpp` to the
isolated runtime recipe, leaving a small playable-first routing host in
`WorldSystem_Provenance_Debug.cpp`. The authority recipe and baseline are
unchanged. See `../SOURCE_SPLIT_PASS6.md` for verification and live build status.

The Build directory must exist. The procedural-root test links the existing `Esoterica.Base.lib` and uses its DLL; the cluster test requires `External/MeshOptimizer`; candidate tests require `GraniteOutcropCandidate.h` produced by the baker. Generated resources/runtime products are not reconstructed by these wrappers.

`RunEditorMouseRegression.cmd` now compiles the preserved generated harness at `Verification/Fixtures/EditorMouseStateRegression.cpp`, rather than relying on its ignored Build copy. This harness is a snapshot, not the authority: `Verification/editor_mouse_regression_source.py` emits a new harness from production methods. Regenerate and review that snapshot whenever those production methods change; the migration did not regenerate or alter its test logic.

## Migration verification

`RunGrassWindTest.cmd` compiles the same scalar formulas used by the grass sway
shader and checks fixed roots, bounded displacement, phase continuity and
spatial/temporal variation. Outputs stay under `Build/Verification/GrassWind`.
See `../GRASS_SWAY_V1.md` for integration and validation limits.

`RunShaderParameterLayoutTest.cmd` checks current generated material layouts
against the production allocation helper and source-extracted assertion logging.
`RunLogAssertDllSmokeTest.cmd` additionally calls the actual Debug Base DLL's
early-startup assertion reporter. See `../GRASS_SWAY_LAYOUT_FIX.md`.

The results below describe the original migration. The maintained movement
runner's deferred failure was subsequently resolved in
`../MOVEMENT_INTEGRATION_FIX4.md`; the ignored original Build script was not
rewritten by that follow-up.

From an unrelated working directory:

- Outcrop V2: 8,684 checks / 0 failures in both optimized and Debug runs.
- Mouse regression: 784 checks / 0 failures in both configurations.
- Movement: fails `continuous soil reaches the buried granite apron without a wall seam` in the optimized run. The untouched original Build script produces the same failure and exit code 1. Neither wrapper continues to Debug after that failure. The cause is not diagnosed by this migration.

Do not treat the migration as an all-green gameplay baseline. Other recipes were preserved/reviewed but not all executed.
