# Worldgen Baseline Performance Floor

## Purpose

`WORLDGEN_BASELINE_PERF` measures the cheapest current traversable world before adding causal geography, material variety, biome fields, vegetation, water, structures, NPCs, or interaction terrain.

Correctness and cost are separate gates:

- **Isolation/continuity:** the terrain stays resident ahead of traversal and does not wake excluded systems.
- **Performance:** the artifact records cost without declaring a budget pass. This first run is the reference from which later stages are compared.

## Fixed configuration

| Setting | Value |
|---|---|
| Fixture | `baseline` |
| Seed | `0x5747424153453031` (`WGBASE01`) |
| Terrain | Constant-grade dirt floor |
| Configured far radius | 96 cells |
| Current effective residency radius | 64 cells |
| Effective diameter | 128 m |
| HF draw radius | 64 cells |
| Cell width | 1 m |
| Warm-up | 30 frames per phase |
| Measurement | 300 frames per phase |

The fixture performs no FBM, geological classification, morphology, water, vegetation, or structures. Rendering stops after the HF floor, before galleries, interaction stencils, cavities, loose bodies, scale props, and the normal HUD.

## Traversal phases

1. Stand still.
2. Walk at 5 m/s.
3. Run at 11 m/s.
4. Forced traversal at 22 m/s.
5. Forced traversal at 44 m/s.
6. Teleport 128 m every 60 measured frames.

Continuous phases use the same measured/clamped frame delta as the live client, so slow frames do not quietly reduce requested streaming pressure.

## Run

```bat
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-baseline-perf
```

The test is self-contained and does not require the Python bridge.

Outputs:

- `Docs/provenance_worldgen_baseline_perf.txt` — phase summary
- `Docs/provenance_worldgen_baseline_trace.csv` — one row for each of 1,800 measured frames

## Measurements

Each phase records:

- mean, median, p95, p99, p99.9, worst frame time, and average FPS;
- 1% and 0.1% low FPS;
- frame-budget exceedance counts;
- requested, completed, and resident cell counts;
- new cells per second;
- local residency-request latency and terrain-generation time;
- HF rebuild count, rate, total time, and maximum time;
- worst single-frame terrain work;
- queue depth, outstanding work, and coverage misses;
- process working set and a lower-bound resident-cell payload estimate.

The invariant gate requires zero:

- D2 rebuilds;
- occupancy rebuilds and mutations;
- EditedRegion changes;
- support queries;
- fracture events and loose matter bodies;
- water coupling;
- residency coverage misses.

## Baseline result — 10 August 2026

The isolation/continuity gate passed. All excluded-system counters and all coverage misses remained zero.

| Phase | Achieved speed | Mean / p99 / worst frame | New cells/s | HF rebuilds | HF time | Resident cells end |
|---|---:|---:|---:|---:|---:|---:|
| Stand | 0.00 m/s | 29.42 / 46.48 / 47.06 ms | 0 | 0 | 0 ms | 12,853 |
| Walk | 5.00 m/s | 29.06 / 49.72 / 50.15 ms | 651 | 44 | 768 ms | 19,045 |
| Run | 10.98 m/s | 28.34 / 49.57 / 54.76 ms | 1,426 | 94 | 1,551 ms | 32,332 |
| 2× run | 21.97 m/s | 28.54 / 50.20 / 51.11 ms | 2,833 | 188 | 3,041 ms | 58,906 |
| 4× run | 43.99 m/s | 29.90 / 36.01 / 37.24 ms | 5,657 | 291 | 7,576 ms | 114,406 |
| Teleport | 73.08 m/s effective | 29.19 / 52.78 / 61.54 ms | 7,337 | 5 | 95 ms | 191,518 |

Startup generated 12,853 cells in 2.046 ms and built the first HF mesh in 16 ms.

## Historical baseline reading

The local constant-floor generator kept residency complete at every speed. Its ordinary request latency stayed below 1.1 ms; a 128 m teleport peaked at 13.769 ms. The generator is not the dominant cost in this slice.

At the time of this baseline, HF packaging was the dominant traversal cost. Residency expansion invalidated terrain whenever a new cell appeared, causing a full 128 m-diameter HF rebuild. At 44 m/s, 291 of 300 measured frames rebuilt the HF mesh and accumulated 7.576 seconds of HF work.

The other exposed issue was retention. The path grew `g.cells` as traversal continued and did not evict cells outside the current residency disk. The stress route ended with 191,518 resident cells and a 193,036,288-byte process working set.

The stand phase also averages about 29.4 ms with no generation or remeshing. That cost is outside the measured terrain stages and must be decomposed before adopting a 16.667 ms frame budget.

## Resolution

The Stage-0 performance sprint completed this list without changing the fixture,
seed, 64 m active radius, or isolation boundary:

1. Persistent 8×8 m HF presentation blocks replaced full rebuilds.
2. A 68 m cache margin now bounds deterministic virgin residency.
3. Full CPU/residency/generation/HF/draw/present/pacing decomposition identified
   the stationary floor as Win32 timer pacing.
4. The playable performance-max lane now runs uncapped and requests swap interval zero.
5. The exact permanent certificate was rerun after each architectural cut.

See [WORLDGEN_STAGE0_PERFORMANCE_MAX.md](WORLDGEN_STAGE0_PERFORMANCE_MAX.md)
for the final evidence and before/after results.

Do not spend this headroom on geology, vegetation, water, structures, or NPCs yet. P5b remains closed.
