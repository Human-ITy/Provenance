# Source split, pass 1: playable outcrop adapter

## Implemented

Moved `Systems/GraniteOutcropLab.inl` into a separately compiled
`Systems/GraniteOutcropLab.cpp`. The new `GraniteOutcropLab.h` exposes only
`EE::OutcropLab::Tick` and `Release`; state and helper functions remain private
to the implementation. `WorldSystem_Provenance_Debug.cpp` includes the header
instead of the implementation and retains its existing call sites.

Added the header and compilation unit to the Game Runtime project and filters,
preserving existing unrelated project edits. The development-tools guard is
defined through `Base/Esoterica.h`, so the lab remains development-only.

Text comparison against the pre-extraction implementation confirmed the entire
lab body is unchanged apart from namespace qualification and the two entry
points' linkage. No simulation, rendering, shape, collision or test algorithms
were changed. The old `.inl` path was replaced, not discarded as obsolete work.

## Validation actually performed

- Compiled both the new lab unit and debug host with the existing project's
  recorded Debug compiler options, including warnings-as-errors.
- Linked an isolated Game Runtime DLL with those new objects and existing
  dependency objects/libraries. Compilation and linking returned zero.
- Outcrop V2: optimized and Debug each passed 8,684 checks, zero failures.
  Initial volume remained 9.000000000000 m3 / 24,300 kg; the excavation
  boundary plus removed volume remained 9.000000000000 m3 within printed
  floating-point residuals.
- Lab contact: optimized and Debug each passed 20 checks, zero failures.
- All 43 Geometry and Data/Provenance entries selected from the prior inventory
  retained their hashes, including the authored shape and playable map.

The normal build's process-killing pre-build event was not invoked. The running
editor and live Game Runtime DLL were not replaced. This is isolated compilation
and link validation, not a clean full rebuild or a new playable-preview test.
Headless certificates validate their own paths; they do not execute the newly
extracted rendering adapter.

## Known failure deferred by user agreement

The previously reproduced movement assertion
`continuous soil reaches the buried granite apron without a wall seam` remains
open. No assertion was removed, relaxed or marked passing. Movement was not
rerun in this pass. Its relationship to the current authored shape and live
combined soil/granite walking path will be revisited separately; it does not
block a behavior-preserving file extraction.

## Remaining work

This is the first actual compilation-unit split, not completion of the broader
refactor. `GraniteGeometry.cpp` is unchanged and still needs staged extraction
by responsibility. The remaining debug workbench and excavation adapter are
also unchanged. A normal rebuild and playable smoke test remain necessary
before accepting the new DLL as the interactive baseline.

`Verification/Scripts/CheckOutcropExtraction.cmd` and its link response file
preserve this local validation recipe. They use machine-specific paths and
existing build products captured from compiler/linker tracking logs; they are
not portable build definitions or replacements for the Visual Studio project.
Generated isolated products go to `Build/Verification/OutcropExtraction`.

No Git staging, commit, push or additional build-artifact cleanup was performed.
