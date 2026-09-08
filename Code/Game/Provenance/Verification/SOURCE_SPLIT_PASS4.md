# Source split, pass 4: rooted detachment and partitioning

## Implemented

- Extracted the 1,641-line structural bridge, tool-strike and detachment section
  into separately compiled `Geometry/GraniteRootedDetachment.cpp`.
- Extracted the 3,589-line rooted-head separation, principal-cast fitting,
  parent-socket mesh and conserved partition section into separately compiled
  `Geometry/GraniteRootedPartition.cpp`.
- These sections require no new shared helpers or public declarations. Both
  reuse the existing private math/formation headers. Local helpers remain static.
- `GraniteGeometry.cpp` decreased from 14,128 to 8,898 physical lines.
- Added both units to the Game Runtime and ProvenanceVerifier projects and
  filters, and to both maintained isolated compile/link recipes.

All moved function bodies, constants and ordering remain unchanged. Read-back
text checks confirmed the intended extraction in the remaining file and both
new units. Public API/type layouts, algorithm versions and baseline fixtures
were not changed. This is organization of the existing rooted authority path,
not a new connection to the playable outcrop's distinct strike implementation.

## Validation actually performed

- Recompiled the pre-edit authority sources and matched the preserved baseline.
- Recompiled all ten post-edit authority sources and matched the same complete
  report, including all 32 granite/surface fingerprints. The maintained recipe
  gates exit status on both verifier success and baseline report comparison.
- Granite: 16 cases, 1,156 gate checks, 27 negative controls, zero failures;
  tool strike 4 cases / 268 checks; local excavation 88 checks; determinism
  16 fingerprints / 48 checks.
- Surface: 16 cases / 16 fingerprints / 32 determinism checks, two negative
  controls, zero failures.
- Actual Game Runtime and ProvenanceVerifier Debug/x64 project builds succeeded.
  The actual verifier executable also exited zero and its full output matched
  `Fixtures/GraniteGeometrySplitBaseline.txt`.
- Updated isolated Game Runtime recipe compiled and linked successfully.
- Playable Outcrop V2 optimized and Debug each passed 8,684 checks, zero failures.
  Initial volume/mass stayed 9 m3 / 24,300 kg; excavation accounting closed as
  before within the same printed floating-point residuals.
- All 42 inventoried Geometry/Data entries other than the intentionally edited
  `GraniteGeometry.cpp` retain their previous hashes. Authored shape, terrain,
  grass, materials, textures and the map are untouched.

Project builds used `BuildProjectReferences=false`,
`PreBuildEventUseInBuild=false`, and the checkout root as `SolutionDir`.
Existing engine/Base dependencies were reused, not rebuilt from scratch.
The editor was not holding the DLL; seven resource compilers and one resource
server were. Only those helpers were stopped after checking their executable
paths and loaded module. The broad process-killing pre-build script was not run.
The live Game Runtime DLL is updated and ready for interactive validation.

## Remaining boundaries

No post-build playable smoke test or Release/Shipping build was performed.
The latest user screenshot `codex-clipboard-082de8e1-f74d-45f6-b519-eb0e6806cb70.png`
is evidence of the prior playable state, not this pass's new DLL. Matching
geometry fingerprints do not certify rendering.

The movement seam assertion remains deferred, unchanged and not rerun here.
Solid construction/fracture and the remaining historical debug workbench are
the next responsibility boundaries. No source/asset cleanup deletion, Git
staging, commit or push occurred. Local validation recipes remain machine-specific.
