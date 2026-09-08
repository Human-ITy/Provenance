# Rounded-foot support and narrow ledges, fix 5 — 2026-09-07

## Implemented

Production files: `Geometry/GraniteLabContact.h` and
`Geometry/GraniteLabMovement.h`. Regression changes:
`Verification/GraniteTreadSurfaceTest.cpp`.

- The walking tread column now searches down to approximately feet minus one
  capsule radius (23 cm), instead of feet minus 2 cm. On an incline the rounded
  foot's datum can be above the local surface. The highest-face rule and 60
  degree tread eligibility cutoff remain unchanged; higher steep faces still
  occlude lower rock treads.
- When the ordinary six samples offer no positive rise, up to four additional
  samples look 4, 8, 12 and 16 cm beyond the leading sample. The first eligible
  positive rise within the existing 22 cm step bound becomes a candidate.
  These samples offer heights, not permission to bypass body collision.
- For qualifying local treads above the fallback floor, continuation permits
  a travel-scaled clearance search: min(5 cm, axis travel * tan(60 degrees)
  + 0.3 mm). The old tiny allowance remains for fallback-floor support.
  The lower continuation window also matches the capsule radius. The 5 cm
  value is a search cap, not a compulsory lift; bisection seeks clearance.
- Lift paths are checked, as are raised midpoints and endpoints. Midpoints
  and endpoints are checked again at the final refined height. The step
  clearance margin is clamped to its search cap, so it cannot exceed the
  22 cm lift limit. Existing support/fall processing follows. This remains
  discrete collision sampling, not mathematically continuous swept collision.

Step height, slope cutoff, player dimensions, speeds, two-horizontal-substep
cap, authored granite, soil shape, textures and map are unchanged. Continuation
and step discovery behavior have intentionally changed.

## Verification

Optimized and Debug both passed:

- Tread suite: 99 checks / zero failures. Previously 44 checks; added one
  below-window query case, 48 hotspot cases (six routes, two stances, four
  rates: 30/60/120/240 Hz), and six independent closed-ramp cases. A 55 degree
  ramp progresses; 70 and 85 degree ramps block in both stances. Existing
  ceiling, tall-face, query-occlusion and other regression cases remain active.
- Movement core: 37 checks / zero failures.
- Maintained integration: 38 checks / zero failures, with unchanged acceptance
  limits. Exterior wall slide: 2.362 m, zero stalls, max 26 probes. Elevated
  cavity-lip slide: 1.831 m, zero stalls, max 40 probes (at its existing bound).
- Contact: 20 checks / zero failures.
- Soil: 203,562 checks / zero failures.
- Earlier fresh/edited route diagnostic: 32 routes, zero flags or safety flags.
  Optimized/Debug reports match after timing normalization. New trajectory
  fingerprint: `4be88ffdb1f73f25`.
- Broader fresh hotspot diagnostic: 180 routes, 11 sustained stops, zero invalid
  starts or sampled overlaps; optimized/Debug reports match exactly. All 144
  upper-region routes have no sustained stops (previously nine). Ground routes
  retain 11 stops, including later steep faces and the step-rise limit. A stop
  count includes legitimate refusals and newly reached terrain, not just bugs.
  It is not meaningful to call all 11 failures or all 11 correct without further
  classification. The selected x=1.8 front lip now crosses its formerly blocked
  seam in both stances; the longer approach later stops at a steeper surface.

Evidence files:

- `Fixtures/MovementTreadFix5_2026-09-07.txt`
- `Fixtures/MovementSupportFix5_2026-09-07.txt`
- `Fixtures/MovementHotspotsFix5_2026-09-07.txt`

Additional current run logs remain under repository-relative
`Build/Verification/MovementBaseline/*-fix5.txt`. Original hotspot evidence
remains unchanged in `Fixtures/MovementHotspots_2026-09-07.txt`.

The added tread probes and clearance checks increase work in blocked cases.
The 32-route sample reaches 41 capsule probes in a frame; no new wall-clock
budget or performance improvement is claimed. These changes do not certify
all shapes, sprint/variable-rate paths or dynamic debris interactions.

## Publication and next live check

The user authorized closing the current resource server and seven compiler
helpers if still running. The publication command restricted termination to
their previously verified IDs and checked names/executable paths before any
stop. The build was gated on no Esoterica/Provenance processes remaining.
No other process was targeted.

The actual Game Runtime Debug/x64 build succeeded and updated the live DLL,
with dependency builds and the broad process-killing pre-build event disabled.
Build result: zero warnings/errors. Published DLL: 7,041,536 bytes at 08:32:41
local time, SHA-256
`E04C6E1B5941818F2429CC7FF8E78990AC846A6E4675A69572E0A6A868D0C772`.
Log: repository-relative `Build/Verification/MovementBaseline/runtime-fix5.log`.
This reuses existing engine dependencies; it is not a clean checkout rebuild.

Next: reopen the sandbox and revisit both user-marked areas, standing and
crouched. The reproductions are approximate regions, not exact recovered
screenshot coordinates. Headless passes do not replace that live acceptance.
No source/assets were deleted, and no Git staging, commit or push occurred.
