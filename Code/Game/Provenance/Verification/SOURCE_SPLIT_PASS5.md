# Source split, pass 5: solid construction / exact fracture

## Implemented

- Moved exact visible-parent-mesh fracture, shared cut topology, child capping,
  surface receipts and child matter reconciliation into separately compiled
  `Geometry/GraniteFracture.cpp` (3,088 lines including wrappers).
- Added private `Geometry/GraniteMeshInternal.h` for the existing triangle-area,
  geometry-extents and mesh-closure helpers, plus the closure helper's edge-use
  record. Helpers retain internal linkage. Removed the now-redundant standalone
  forward declaration of the closure helper from the construction unit.
- `GraniteGeometry.cpp` now concentrates on closed-body construction and its
  surface-history/shape treatment; it decreased from 8,898 to 5,632 lines.
- Public API/type layouts, algorithms, tolerances and fingerprint versions are
  unchanged. Read-back text checks confirm the mechanically moved code and the
  intended remaining file; no arithmetic or cut logic was rewritten.
- Added the fracture unit to Game Runtime and ProvenanceVerifier projects and
  filters; added the private header to Game Runtime's project and filters.
  Both maintained isolated recipes now compile/link the new unit.

## Validation actually performed

- Pre-edit authority source rebuild matched the preserved baseline.
- Post-edit rebuild of all eleven authority source units matched the same full
  report, including all 32 granite/surface fingerprints. The recipe gates exit
  status on verifier success and comparison to the unchanged fixture.
- Granite: 16 cases, 1,156 gate checks, 27 negative controls, zero failures;
  tool strike: 4 cases / 268 checks; local excavation: 88 checks; determinism:
  16 fingerprints / 48 checks.
- Surface: 16 cases / 16 fingerprints / 32 determinism checks, two negative
  controls, zero failures.
- Actual Game Runtime and ProvenanceVerifier Debug/x64 project builds succeeded.
  The actual verifier executable exited zero and its complete report matched
  `Fixtures/GraniteGeometrySplitBaseline.txt`.
- The updated isolated Game Runtime recipe also compiled and linked successfully.
- Playable Outcrop V2 optimized and Debug each passed 8,684 checks, zero failures.
  Initial volume/mass remained 9 m3 / 24,300 kg; excavation accounting closed
  with the same reported floating-point residuals.
- All 42 inventoried Geometry/Data entries other than the intentionally edited
  `GraniteGeometry.cpp` retain their previous hashes. Authored shape, terrain,
  grass, materials, textures and map are untouched.

Project builds used Debug/x64, `BuildProjectReferences=false`,
`PreBuildEventUseInBuild=false`, and the checkout root as `SolutionDir`.
Existing engine/Base dependencies were reused. The editor was closed; seven
resource compilers and one resource server held the DLL. Only those helpers
were stopped after checking paths and loaded module. The broad process-killing
pre-build script was not run. The live Game Runtime DLL was updated successfully.

## Boundaries

No post-build interactive smoke test or Release/Shipping build was performed.
Matching geometry fingerprints does not certify identical rendering. This pass
made no visual changes and did not connect the playable strike model to another
authority path. The movement seam assertion remains deferred and unchanged;
it was not rerun here.

The main planned geometry responsibilities are now separated. Historical debug
workbench isolation is next; further construction internals can be separated
later if dependencies justify it. No cleanup deletion, Git staging, commit or
push occurred. Local verification recipes remain machine-specific.
