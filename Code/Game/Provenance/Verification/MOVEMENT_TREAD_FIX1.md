# Movement tread detection, fix 1 — 2026-09-07

Follow-up small-clearance implementation and results: `MOVEMENT_CLEARANCE_FIX2.md`.
This record retains the results as measured after fix 1.

## Changed

Only production file changed: `Geometry/GraniteLabContact.h`, `StepSurface`.
It now queries the current collision-boundary BVH in a bounded vertical column
instead of using the tool ray's single-face traversal through the tetrahedral
partition. Coincident edge hits within 1e-8 metres prefer the upward tread;
a distinct higher steep face still hides any lower walkable surface. Nonfinite
queries are refused. The original .65 normal cutoff, downward query reach and
ceiling are retained. Tool raycasting and strike geometry are untouched.

The explicit pre-fix reproducer queried x=2.803, y=1.2 at five ceiling heights.
Three returned only the soil (0.285723 m); two found the rock tread (0.779204 m).
All five now find the rock tread, in optimized and Debug builds. This establishes
the query-height-dependent failure separately from movement or slope policy.

Added `GraniteTreadSurfaceTest.cpp` and `Scripts/RunGraniteTreadSurfaceTest.cmd`.
The synthetic tests cover reach, ceiling, steep occlusion, absent/changed
boundaries and invalid input. Authored integration cases cover the recorded
query and both cross-crown directions, plus the tall exterior approach. These
authored cases describe the current sculpt, not a restriction on future art:
version their fixture expectations when the specimen intentionally changes.

## Results

- Targeted tread tests: 16 checks, zero failures in optimized and Debug.
- Existing lab contact: 20 checks, zero failures in both configurations.
- Existing soil suite: 203,562 checks, zero failures in both configurations.
- Full headless movement diagnostic: 32 routes, 10 flagged (previously 14),
  zero diagnostic safety flags. Both builds exit 1 intentionally while flags
  remain. Normalized reports match exactly between optimized and Debug.
- Fresh and edited routes 03/04 now traverse the crown in both directions with
  no persistent catch. Other tested steep ground approaches stop at unchanged
  positions. No capsule overlap is reported on the top routes.
- Top-route trajectory fingerprint is now `78f3e4888429c5a5`, matching between
  builds. This is expected to change when movement changes; it is not a matter
  conservation fingerprint. Timings are excluded.
- The isolated runtime compile/link succeeded using existing dependency
  products. It did not replace the live DLL or stop any editor/helper process.

Evidence snapshot: `Fixtures/MovementTreadFix1_2026-09-07.txt`. The original
failing snapshot and diagnosis remain preserved. Full timing reports and test
binaries remain in `Build/Verification/MovementBaseline`. No performance gain
or passing wall-clock budget is claimed.

## Still open

Five routes per specimen remain flagged: 06, 07, 11, 12 and 16. Routes 06/07/12
involve the current slope cutoff; routes 11/16 still need a supported-contour
following investigation (small capsule-clearance changes without a positive
step tread). Do not use a general upward-clearance search as permission to
climb exterior walls. Keep ceilings, legitimate falls and steep-face regressions.

`GraniteLabMovement.h`, authored geometry, material assets, and the original
movement test remain unchanged. The original seam assertion still uses its
old setup; this pass does not retire it or claim that suite is green. No full
client build, live DLL publication or interactive smoke test was performed.
No Git staging, commit, push or cleanup deletion occurred.
