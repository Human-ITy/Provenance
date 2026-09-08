# Movement / landscape quality-of-life pass — 2026-09-07

## Published

Shared Debug x64 Game Runtime rebuilt successfully. DLL SHA256:
`3E3C4B47E1D01A4DE3861CC939ACF516803482E11AE1B4A9BEE3296E86C66090`.

- Published the previously prepared `TerrainSeam` presentation changes: irregular inward grass-density/height feather at the original lab perimeter; omit its fully buried perimeter triangles from the host soil render. Loose clods retain their complete surfaces. Soil occupancy, collision and conserved matter are unchanged. This is not the planned unified/depth-unbounded editable terrain implementation.
- Resolve the immutable landscape during world setup rather than its first walking query. HUD now separates its one-time setup cost. This relocates and labels initialization; it does not make startup free or demonstrate improved steady-state FPS.
- F11 starts/stops a bounded 30-second movement/view capture. Pre-reserve 8,192 samples; no per-sample allocation or file write. Summarize moving/view versus idle p95/p99/max. Action/publication/active-debris samples remain in CSV, excluded from ordinary movement/idle summaries. Region labels distinguish original lab, creek corridor and outer terrain.
- Pair each frame interval with the preceding update's input/work metadata. Exclude pre-capture and landscape-initialization intervals. Invalid intervals are rejected, not converted to misleading zero-time frames. No automatic FPS pass/fail threshold and no GPU timing claim.
- On completion, write `Build/x64_Debug/MovementFrameCapture.csv`. Starting another capture replaces that output after completion; retain a copy before another comparison run. F9 shadow trial and F11 capture cannot be started concurrently. F10 is a diagnostic teleport, marked as action/busy rather than ordinary travel.

The capture's upload CPU column includes grass update/publication and Rebuild work, not isolated GPU transfer time. Frame intervals can still contain vsync, background workloads and editor overhead. Record resolution, graphics settings and capped/uncapped mode alongside comparisons. Mouse-right held is classified as view input even without measured pointer motion. Zero samples in a category mean insufficient data, not zero cost.

## Checks

- MovementFrameCapture: 12 checks PASS, including previous-frame attribution, busy/setup exclusions, restart, invalid intervals, duration and sample caps, and CSV output.
- Movement core: 37 checks each, optimized and debug; zero failures.
- Landscape: 4,028,721 checks PASS; 694,562 triangles / 231 tiles retained. Maximum sampled mud weight outside the carve remains zero. No creek shape/material change in this pass.
- Terrain seam checks PASS. Pick/dig preparation baseline rerun; no speed optimization is claimed. Pick preparation ~61–123 ms, soil ~17–46 ms in this optimized headless run. Worker preparation remains the larger tool-response target than commit.
- New warmed CPU route observer covers idle, walking, sprinting, both creek crossing directions and the lab apron. Latest optimized outer/creek p99 <=0.0009 ms; apron p99 0.1451 ms, max 0.1486 ms. Terrain initialization 49.265 ms. These are observational CPU-only routes, not rendered FPS acceptance. See `Artifacts/Landscape/movement-routes-optimized.txt` and `MovementRoutePerformance.cpp` for exclusions.

## Publication / live observation

User confirmed editor closure. Stopped only matching resource helpers. The first link was blocked by three compiler helpers that respawned before their server finished stopping; stopped those exact checkout helpers and retried successfully. No editor was forcibly terminated.

Computer-use inspection saw the updated playable creek preview, new F11 HUD and 231/231 terrain tiles. Editor FPS read 61–62 in two observations. Runtime terrain setup displayed approximately 133.67 ms separately from walking. A start-capture input was rejected because user input was detected in the window; re-observed without further input and left control with the user. HUD remained READY at that observation. Editor remains open.

The user then completed a live capture, discovered at 16:47:13. Preserved as `Artifacts/Landscape/MovementFrameCapture_2026-09-07_164713.csv` (1,821 frames). This supersedes the earlier absence of a live capture:

| Region / input | Frames | Mean ms | p95 ms | p99 ms | Max ms |
|---|---:|---:|---:|---:|---:|
| Creek moving/viewing | 344 | 16.515 | 17.124 | 17.338 | 18.558 |
| Lab moving/viewing | 187 | 16.503 | 17.080 | 17.477 | 17.514 |
| Outer moving/viewing | 1,242 | 16.458 | 17.065 | 17.531 | 18.572 |
| Outer idle | 48 | 16.573 | 17.155 | 17.500 | 17.500 |

No samples over 33.333 ms; all recorded samples were ordinary, non-action intervals. Distribution is consistent with an approximately 60 FPS cap; the actual limiter setting was not inspected. This is useful current-build traversal evidence, not an uncapped GPU headroom measurement, a controlled before/after comparison, an action-latency measurement or a native-tree performance test.

## Tree / chopping handoff reviewed, not installed

The supplied READMEs are different: trees/v002 is the revised model library; cut-wood/v001 is fresh wood material art and the intended full-circumference, strike-height-local taper contract. Neither implements runtime chopping/felling. Do not substitute a directional notch or paint bark as an implementation of that geometry rule.

Another authoring revision already created `output/placement/world_trees_v002`: 117 preserved IDs/transforms, 24 structural variants, revised root conformity, no placement clearance failures reported. Whole-layout LOD1 is 1,804,650 triangles; LOD2 is 1,266,201 and retains foliage sprays. This pass opened its overview for review and did not overwrite those assets or claim their runtime integration.

Next: repeat F11 at controlled resolution with uncapped rendering if available; profile worker preparation separately; integrate a bounded native tree cohort with preserved alpha masks, material/mesh sharing, coordinate conversion, revised root overrides, LOD and shadow budgets before full-layout publication. Creek close-up visual acceptance, expanded grass, unified terrain edits, and tree chopping/felling remain unfinished work.
