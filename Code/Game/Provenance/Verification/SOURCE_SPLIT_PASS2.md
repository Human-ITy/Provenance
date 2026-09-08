# Source split, pass 2: granite spall resolution

The user's screenshot `codex-clipboard-f094950c-b018-4d9a-9567-cc994372c265.png`
confirms the playable V5.9 space was loaded before this pass. It does not prove
which prior DLL was loaded or validate this pass's newly built DLL.

## Actual source changes

- Moved the 882-line spall-reserve section from `Geometry/GraniteGeometry.cpp`
  to the separately compiled `Geometry/GraniteSpallResolution.cpp`.
- Moved 250 lines of existing local math, hashing, value-noise and vector helpers
  into private `Geometry/GraniteGeometryInternal.h`, shared by the two units.
  Helpers retain internal linkage in a named implementation namespace.
- Kept the public API and type layouts in `GraniteGeometry.h` unchanged.
  No algorithm version or fingerprint schema changed.
- Added the new source to both Game Runtime and ProvenanceVerifier projects
  and filters; added the private header to Game Runtime's project and filters.
- Updated the isolated playable-adapter build recipe to compile/link the current
  geometry units too, rather than reusing the old monolithic geometry object.

The main geometry file went from 20,162 to 19,034 physical lines. Exact text
checks confirmed both extracted sections are unchanged; the remaining file
differs only by removing those sections and introducing the private include
and namespace lookup. This is separation of existing responsibility, not a
new strike model or a connection between the playable milligram path and the
integer-gram spall path.

## Validation

Before editing geometry, compiled all six source units of the standalone
authority verifier from current source with recorded Debug project options,
linking the existing Base dependency. Captured the full report. After extraction,
rebuilt all seven units and ran the same corpus:

- Granite: 16 cases, 1,156 gate checks, 27 negative controls, zero failures.
- Tool strike: 4 cases / 268 checks; local excavation: 88 checks.
- Granite determinism: 16 fingerprints / 48 checks.
- Surface: 16 cases / 16 fingerprints / 32 determinism checks, 2 negative
  controls, zero failures.
- All 32 before/after fingerprints match. The complete generated reports are
  byte-identical: SHA256
  `9664DC7883613B1779C8D131C9AFECD3A42728CDDAD1A7C246DC2A59121591CA`.
- Preserved the baseline report outside Build in
  `Fixtures/GraniteGeometrySplitBaseline.txt`. The maintained local recipe
  `Scripts/CheckGraniteGeometrySplit.cmd after` rebuilds current sources and
  fails on verifier failure or a text mismatch against this baseline. It was
  executed successfully, including its comparison. Baseline mode refuses to
  run after the extracted source exists.
- Actual ProvenanceVerifier Debug project build and executable both exit zero.
- Actual Game Runtime Debug project build succeeds, including the previously
  extracted playable adapter. The live `Build/x64_Debug/Esoterica.Game.Runtime.dll`
  has now been replaced by the newly linked output.
- Isolated Game Runtime compilation/linking also passes.
- Playable Outcrop V2 optimized and Debug: 8,684 checks / zero failures each.
  Initial mass remains 24,300 kg / 9 m3; excavation accounting closes as before.
- All 42 inventoried Geometry/Data entries other than the intentionally edited
  `GraniteGeometry.cpp` retain their previous hashes. Authored shape, grass,
  soil, textures and map are untouched.

Project builds used Debug/x64, `BuildProjectReferences=false`, and
`PreBuildEventUseInBuild=false`, with the checkout root as `SolutionDir`.
Dependencies were reused, not rebuilt from scratch. No Release/Shipping build
or post-build interactive smoke test was performed.

The first live Game Runtime link failed because the DLL was loaded by the
background resource server and seven compiler processes. Their executable paths
and loaded module were checked. Only those eight build helpers were stopped,
then the retry succeeded. The broad process-killing pre-build script was never
run. The editor was already closed, as the user reported.

## Boundaries

The movement seam assertion remains deferred, unchanged and not rerun here.
The user's screenshot is visual evidence, not a substitute for movement or
strike testing against the new DLL. Reopen the playable preview for that check.

The larger geometry/formation/partition and debug-host splits remain future
passes. No cleanup deletion, Git staging, commit or push was performed.
These local verification scripts still contain machine-specific tool/include
paths; they are validation recipes, not portable build-system replacements.
