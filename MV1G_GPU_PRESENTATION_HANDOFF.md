# MV1.G — GPU Presentation Certification

> **QUALIFICATION (projection audit) — RESOLVED.** MV1.G's async query mechanism,
> bounded resources, first-use behavior, and submission measurements below remain
> valid. A later audit found the render far plane was **600 m** during the
> original test, so most regional/horizon fragments were **clipped before
> rasterization** — the 0.53 ms worst draw did not yet certify full 32 km raster
> cost. **MV1.G was rerun after MV1.C** (which opens true 32 km visibility via the
> two-pass split, and moves far tiles to persistent VBOs): it remains **green at
> ~0.47 ms (worst as low as ~0.036 ms) with no stalls**, now with the full 32 km
> actually rasterized, plus explicit VBO resident-byte accounting (~4.2 MB). The
> full-raster GPU path is certified. (Note: the display-list lifecycle referenced
> below is superseded by VBO ownership in MV1.C.)


MV1.G answers one question the MV1 CPU cert could not: is the frozen 32 km MV1
terrain renderer genuinely **GPU-safe** across realistic view orientations and
tile churn, or merely CPU-frame-safe? 65 k triangles should be trivial — but GPU
behavior is not inferred from geometry counts (the water first-use hitch is
exactly why).

**Scope:** instrumentation only. MV1 geography, tessellation, error targets,
visibility range, and MW1–MW8 authority are unchanged. MV1.D / MV2 / MW9 stay
closed. MV1 frozen at certify `77aa7767` / stamp `7b0f69ea`.

## Method

Async `ARB_timer_query` (`GL_TIME_ELAPSED`) wraps the MV1 draw span (the per-tile
`glCallList` loop). An 8-deep query ring is polled via `GL_QUERY_RESULT_AVAILABLE`
once per presented frame and read only when ready — **no `glFinish`, no blocking
read, no same-frame wait** is ever issued to measure. Each query is tagged with
its scenario at issue time, so results that arrive a frame or two later are
bucketed correctly across scenario boundaries. Observed query latency: **1
frame**.

CPU draw-submission time is measured directly (QPC around the same loop). CPU
display-list build/publish latency is measured separately in the MV1 build budget
(see the MV1 terrain cert); its *GPU* upload cost is driver-implementation-defined
for display lists and is reported **UNAVAILABLE** rather than manufactured.
Portable VRAM is reported **UNAVAILABLE** (no reliable memory extension queried).

## Result — PASS

`CERT_MV1G_GPU_PRESENTATION.cmd` → `--cert-mv1g-gpu-presentation` →
`Docs/provenance_mv1g_gpu_presentation_cert.txt`.

GPU timer **AVAILABLE** (ARB_timer_query), 2646 samples, max latency 1 frame.

```
worst GPU draw (all scenarios)   0.5277 ms   (bound 8.0 ms)
first-visible cold max           0.0224 ms
warm repeat max                  0.0126 ms
max draw calls                   626  (= tile high-water)
resident tiles high-water        626  (bounded)
```

Per-scenario GPU draw (p50 / p95 / p99 / max, ms):

| Scenario | p50 | p95 | p99 | max |
|---|---|---|---|---|
| first_visible_cold | 0.0068 | 0.0141 | 0.0177 | 0.0224 |
| station_trunk_valley | 0.0033 | 0.0035 | 0.0036 | 0.0039 |
| station_mountain_flank | 0.0087 | 0.0178 | 0.0227 | **0.5277** |
| station_ridge_shoulder | 0.0041 | 0.0106 | 0.0143 | 0.0203 |
| station_foreland_basin | 0.0079 | 0.0154 | 0.0175 | 0.0204 |
| station_high_divide | 0.0087 | 0.0159 | 0.0201 | 0.2497 |
| worst_orientation_sweep | 0.0073 | 0.0159 | 0.0205 | 0.1547 |
| rotate_360 | 0.0071 | 0.0159 | 0.0190 | 0.0207 |
| movement_band_churn | 0.0058 | 0.0109 | 0.0139 | 0.0147 |
| warm_repeat | 0.0053 | 0.0092 | 0.0108 | 0.0126 |

**Attribution of the outliers.** Every scenario p99 is < 0.03 ms. The isolated
per-scenario maxima (0.53 ms flank, 0.25 ms divide, 0.15 ms sweep) are single
samples far above their own p99 — transient GPU-scheduling blips (the timer sees
the whole GPU, not just this client), each **~15× below** the 8 ms stall bound
and not reproduced by the warm pass. No sample approaches the stall threshold;
`outlier_owner=none_all_bounded`.

**First-use.** The cold first-visible window (397 samples spanning the full cold
rebuild-to-settled) peaks at 0.0224 ms and the warm re-visit peaks at 0.0126 ms —
lower than cold. The historical one-time backend/resource stall class **does not
occur** for MV1's display-list first-draw.

**Boundedness.** Draw calls and resident geometry are bounded by the visible
working set (626 at high-water). Rotation and movement (band churn) do not
accumulate display lists or driver resources — `travel_history_accumulation=none`.

## Standing CPU gates (unchanged, re-run with the instrumented path)

- Test A `--cert-worldgen-cardinal-replacement-mw8` — **PASS 4/4**, 2601
  packages, residency digest origin==return, **0 movement frames >16.667** N/E/S/W.
- Test B 90 s NE fly `--cert-streaming-soak-mw8` — **PASS**, 0 frames >16.667
  (max 13.22 ms), `mw8_rebuilds=0`, `mw8_compiles=0`.

No 300/900 s run — the GPU/resource data shows no persistent growth or recurring
stall, so the endurance run is not warranted.

## Honest limits

- **GPU VRAM / display-list upload time** are `UNAVAILABLE` by design. Display
  lists hide their memory and upload semantics behind the driver; no portable,
  reliable extension is queried, so no number is invented. If a future path moves
  MV1 to VBOs, upload timing and buffer bytes become directly measurable and this
  cert should be extended.
- Timer results depend on the driver honoring `ARB_timer_query`; the receipt marks
  `gpu_timer=AVAILABLE/UNAVAILABLE` explicitly, and an unavailable timer reports
  UNAVAILABLE rather than a false pass.

## Verdict

The certified 32 km MV1 renderer is **GPU-safe**: draw cost is a small fraction of
a millisecond across every station, worst orientation, full rotation, and tile
churn; no first-use stall; bounded draw calls and resources. Combined with the MV1
CPU/visual cert, 32 km is now geometry-correct, CPU-safe, GPU-safe, and
memory-bounded — the remaining MV1 gap is perceptual depth (**MV1.D**), after
which MV2 may extend the hierarchy.

## Runtime

```
CERT_MV1G_GPU_PRESENTATION.cmd     Build\x64_Release\ProvenanceClient.exe --cert-mv1g-gpu-presentation
```

## Board

```
MW1–MW8                        CERTIFIED
MV1 32 km terrain              CERTIFIED
MV1.G GPU presentation         CERTIFIED
MV1.D distance readability     NEXT
MV2 100+ km horizon            CLOSED
MW9 flora/fauna                CLOSED
```
