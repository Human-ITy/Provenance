# Source split, pass 6: historical debug workbench isolation

## Implemented

- Reduced `Systems/WorldSystem_Provenance_Debug.cpp` from 12,206 physical lines
  to a 35-line routing and cleanup host. Preserved historical implementation in
  separately compiled `Systems/WorldSystem_Provenance_DebugWorkbench.cpp`
  (12,192 lines). This is responsibility separation, not deletion of diagnostics.
- Preserved dispatch order: settings guard, seed capture, playable OutcropLab
  Tick for game worlds, then the existing workbench fallback if not handled.
  The fallback is not restricted to editor worlds.
- Preserved cleanup order: release playable outcrop state, then historical
  excavation and owned tool-strike state. Existing workbench helpers, caches,
  certificates and tool-strike methods remain together without algorithm edits.
  `GraniteExcavationLab.inl` remains included by the workbench implementation.
- Added two private, nonvirtual, development-only helper methods in
  `Systems/WorldSystem_Provenance.h`; no state fields, reflected members or
  virtual interface changes. Updated the file-responsibility comment.
- Added the workbench compilation unit to the Game Runtime project and filters,
  and to the maintained isolated runtime compile/link recipe. The verifier
  project and authority baseline fixture are unchanged in this pass.

## Validation actually performed

- Read-back comparisons matched the intended thin host and mechanical
  workbench extraction.
- Isolated Game Runtime compile/link succeeded.
- Actual Game Runtime Debug/x64 build succeeded and updated the live DLL.
  No Esoterica process held the DLL; no processes were stopped this pass.
  The broad process-killing pre-build event was disabled.
- The authority source rebuild succeeded; its full report and all 32
  granite/surface fingerprints matched the preserved baseline exactly.
  Granite: 16 cases / 1,156 gate checks / 27 negative controls / zero failures;
  tool strike: 4 cases / 268 checks; excavation: 88 checks; granite determinism:
  16 fingerprints / 48 checks. Surface: 16 cases / 16 fingerprints /
  32 determinism checks / two negative controls / zero failures.
- Playable Outcrop V2: optimized and Debug each passed 8,684 checks with zero
  failures. Initial volume/mass remained 9 m3 / 24,300 kg, with unchanged
  reported excavation accounting residuals.
- Lab contact: optimized and Debug each passed 20 checks with zero failures.
- All 42 protected inventoried Geometry/Data entries, excluding the previously
  refactored `GraniteGeometry.cpp`, retained their inventory hashes.
- Game project/filter whitespace checks passed, with line-ending warnings only.

The actual runtime build reused existing dependencies (`BuildProjectReferences=false`)
and disabled its pre-build event (`PreBuildEventUseInBuild=false`). This was not
a clean full-checkout rebuild. The standalone authority recipe was rebuilt;
the actual ProvenanceVerifier project was not separately rebuilt this pass.

## Boundaries and next decisions

No post-build interactive smoke test or Release/Shipping build was performed.
Geometry fingerprints validate the authority corpus, not debug dispatch or
rendering. Authored shapes, terrain, grass, materials, textures and map were
not edited. No appearance or performance improvement is claimed by this split.

The known movement soil/apron seam assertion remains deferred, unchanged and
not rerun. Revisit that movement baseline before treating the entire gameplay
suite as green. After user confirmation, the next planning step can be the
dedicated Granite Lab workspace and authoring controls. Historical workbench
internals remain available for later extraction if warranted; the routing
boundary is now explicit.

No cleanup deletion, Git staging, commit or push occurred. Local isolated
verification recipes still contain machine-specific dependency paths.
