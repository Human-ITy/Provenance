# Stage 12 - Surface Breach to Subsurface Continuity

Baseline: `8230f14c` (`Baseline: certify Stage 0-11 worldgen and live streaming lane`)

Stage 12 adds no geology. It certifies that the existing Stage-11 faulted quartz
body physically intersects the compiled present surface and therefore becomes a
real outcrop. Visible material is always re-queried from Stage-11 authority at
the reconstructed surface; the client cannot paint or relabel quartz.

The permanent control raises the queried surface by the descriptor's declared
cover offset. A valid breach sample must cease to be quartz under that control.
The same surface `FeatureId` must be found again below grade and on both sides of
the Stage-11 fault.

## Certified receipts

- Causal scan: 66,049 surface samples; 661 outcrop samples; 388 buried
  continuation samples; quartz FeatureId `da10b0d1e5f01001`.
- Visual proof: the surface and the one-metre-deep flashlight terminal resolve
  that same FeatureId; the deterministic capture contains 10,118 deposit pixels.
- Cold/transition order: 20 runtime cases pass, including clean-to-12,
  11-to-12, 12-to-11, and 12-to-12.
- Distant boundary: north/east/south/west pass with zero sky and fallback pixels.
- Full replacement: 192 m live radius, 408 m outward station, 2,601 resident
  packages, exact return image/package/collision parity in all four bearings;
  movement worst frames range from 7.613 to 8.085 ms in the final pre-commit run.

The full-replacement test found and closed one cold-start integration defect:
Stage 12 could declare required terrain packages without admitting them to the
bounded worker path. Absence of published Stage-12 terrain is no longer allowed
to inherit Stage-11 packages or wait indefinitely.

Closed: mutation, water, active erosion, sediment transport, bodies, P5b,
Stage-13 exact local materialization, and Stage-14 pick fracture.
