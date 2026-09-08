# Strike preparation phase investigation — 2026-09-07

## Scope and result

Continued the performance backlog identified by MOVEMENT_LANDSCAPE_QOL_V1.md
and GRANITE_OUTCROP_LAB.md. No new build/edit UX, crack shading, terrain changes,
or gameplay rules are included. This is a measured profiling step, not a claimed
performance improvement or a published playable build.

Added optional CPU phase observers to GraniteContactCast::PrepareStrike and
UpdatedCollisionQuery. Default gameplay callers take no new clock samples.
Early refusal/no-removal profiles have complete=false; zero phase values in
that case must not be interpreted as a completed free operation.

Extended TerrainResponseBaseline with the same eight-strike evolving fixture,
explicit cut/support/storage/commit phases, a frozen reference collision builder,
and exact output comparisons. The support wrapper and the instrumented sequence
perform the same operations in the same order. Reference work is outside timed
phases but can affect cache state; this is not a controlled speedup experiment.

## Latest optimized headless observation

Run: Verification/Scripts/RunTerrainResponseBaseline.cmd, MSVC C++17 /O2 /W4 /WX.
Full output: Artifacts/Landscape/strike-preparation-phases-2026-09-07.txt.

| CPU phase | Range across eight strikes (ms) |
|---|---:|
| Full production preparation | 63.555–126.403 |
| Contact/cut/chip preparation | 5.739–20.580 |
| Local render-patch construction | 0.395–0.967 |
| Whole walking-collision preparation | 43.810–47.217 |
| Collision triangle assembly (within preceding row) | 7.607–9.251 |
| Collision tree construction (within preceding row) | 35.622–38.645 |
| Structural support, including any release rebuilding | 14.803–59.048 |
| Storage accounting | 0.013–0.138 |
| Commit, instrumented sequence | 0.435–0.861 |

Collision contains 123,708–131,007 triangles. Support nodes grow from 357 to
3,121. Later support releases include work beyond cache/graph/solve subtimers;
do not add those nested timings to the total or call their difference idle time.
The small render-patch time is CPU geometry preparation, NOT GPU upload time.
Input queues, worker scheduling, frame presentation, GPU upload/rendering, and
uncapped frame headroom remain outside this measurement. Eight evolving strikes
are observations, not latency percentiles or broad-scene acceptance.

## Candidate tested and not retained

A temporary ordered sparse update index replaced per-cell linear update-list
searches in collision assembly. Exact triangle and tree comparisons passed.
It only affected the smaller assembly phase and did not remove the dominant
whole-tree rebuild. Total preparation gains were not established reliably, so
the candidate was removed. No additional persistent cache was introduced.

## Validation

Final run exited zero. Seam presentation checks passed. Empty-update and all
eight pre-support collision results exactly match the reference's triangle
positions, normals, node bounds, child links and ranges. Instrumented versus
production sequences have identical final surface positions/normals, revision,
removed volume and integer-milligram ledger. Under-grade early refusal resets
the observer. This does not constitute a new full movement or support regression
certificate, nor a visual test; existing playable binaries were not replaced.

## Performance queue

1. Next: prototype spatially local collision preparation, avoiding a whole-tree
   rebuild per pick strike. Preserve exact surface triangles, process every
   support-release update, and verify inside/outside, lip, steep-face, ground
   entry and upper-surface walking against the full reference. Measure both
   preparation savings and traversal cost; account for retained memory and
   stale-transaction rejection. Do not trade preparation latency for foot stalls.
2. Profile support-release rebuilding separately from cached graph/solve work;
   later strikes show a substantial unpartitioned release cost.
3. Measure actual main-thread publication and worker-to-visible response under
   controlled runtime settings. Headless commit time is not publication time.
4. Retain the F11 traversal evidence; controlled uncapped frame/GPU headroom
   remains a separate measurement, not proven by a 60 FPS capped capture.
