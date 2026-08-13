# Provenance Stage-0 Performance-Max Report

Date: 10 August 2026

Fixture and workload remained fixed: `BASELINE`, seed `WGBASE01`, flat dirt,
64 m active radius, 128 m diameter, and zero D2/occupancy/edit/support/body/
fracture/water coupling.

## Findings

The original flat-floor playtest showed roughly 27–30 ms frames, append-only
cell residency, and whole-heightfield rebuilds during traversal.

The performance sprint established three independent causes and remedies:

1. The terrain was packaged as one disposable 128 m display list. Stage 0 now
   owns persistent 8×8 m presentation blocks and updates only blocks touched by
   newly resident vertices.
2. Resident virgin cells were never removed. Stage 0 now retains a 68 m cache
   around the unchanged 64 m active radius, evicts deterministic virgin cells,
   and preserves cells carrying authoritative state.
3. The stationary 29 ms floor was not rendering cost. Frame decomposition found
   about 29.075 ms of pacing wait in the low-resolution `WM_TIMER` loop. The
   playable lane now runs continuously and disables swap interval when supported.

## Certificate evidence

The final permanent certificate passed all six phases with zero coverage misses,
zero full HF rebuilds, and clean isolation.

```text
resident band across measured trace: 12,853–13,583 cells
final cumulative virgin evictions:   179,000 cells
full HF rebuilds:                    0

stationary mean frame:               29.153 ms
stationary CPU frame:                 0.079 ms
stationary draw submission:           0.010 ms
stationary present wait:              0.043 ms
stationary pacing wait:              29.075 ms
```

The certificate intentionally retains its historical timer cadence. Its timing
decomposition is the evidence that the remaining certificate frame interval is
pacing rather than terrain or renderer work.

## Visible uncapped playtest evidence

A 1280×800 visible-window smoke run used normal Stage-0 rendering, then sprinted
for two seconds:

```text
frames:                              10,819
all-frame mean:                       0.356 ms / 2,805 FPS
idle mean:                            approximately 2,900 FPS
moving mean:                          approximately 2,718 FPS
moving p99:                           0.685 ms
worst frame:                          3.484 ms
resident maximum:                    13,583 cells
evicted cells:                        2,108
HF local updates:                       890
HF full rebuilds:                         0
swap interval:                            0
```

This is an engineering headroom measurement, not a proposed shipping frame rate.
A later presentation policy may cap the player build deliberately, but the empty
substrate no longer consumes the finished game's frame budget.

## Representation notes

- `hf_build_ms` includes legacy OpenGL display-list compilation/upload because
  that backend does not expose a separable upload stage.
- GPU timer-query instrumentation is not present; `gpu_frame_ms=-1` is honest.
- The next renderer cut can replace display lists with explicit vertex/index
  buffers without changing the block ownership or residency lifecycle proven here.
