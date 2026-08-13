# Stage 8 playable performance A/B/C

Run `CERT_STAGE8_PLAYABLE_PERF.cmd`.

The real renderer executes six conditions with one build, seed, resolution,
64 m residency radius, and mutation-free path:

- A: Stage 7 stationary and fixed 48 m walk.
- B: Stage 8 equal-resistance stationary and the same walk.
- C: Stage 8 differential-resistance stationary and the same walk.

Each condition warms for 60 frames and records 600 frames. The receipt includes
mean, median, p95, p99, worst, triangles, packages, memory, CPU, presentation,
collision, residency, mesh work, authority compilation, and coverage.

Stationary conditions require zero rebuilds, local updates, authority
recomputes, erosion recompiles, new cells, evictions, and coverage misses after
warm-up. The moving marginal gate bounds Stage 8 mean and p99 degradation
relative to Stage 7 while reporting B-A, C-B, and C-A separately.

Outputs:

```text
Docs/provenance_stage8_playable_perf.txt
Docs/provenance_stage8_playable_perf_trace.csv
```
