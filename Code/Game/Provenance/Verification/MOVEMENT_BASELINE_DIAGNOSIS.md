# Movement baseline diagnosis — 2026-09-07

Historical pre-fix diagnosis. Follow-up implementation/results are recorded in
`MOVEMENT_TREAD_FIX1.md`; the original evidence below remains unchanged.

## Scope

Headless diagnosis only. No production source, authored shape, material, map,
movement settings or original movement assertion was changed. No editor process
was stopped and no live runtime DLL was replaced. The user expects uninterrupted
walking across this specimen's upper contours, but steep exterior faces should
prevent walking up from the ground.

Added `GraniteMovementBaselineDiagnostic.cpp` and its maintained runner. It uses
the map's seed 8675309, current authored granite, soil reset against that granite,
and the same movement callbacks as the playable adapter: rock OR static-soil
capsule collision, floor datum `-double(.12f)`, combined step surface with loose
soil excluded. These callbacks are mirrored, not shared through an extracted
adapter; keep them aligned if production changes.

## Existing seam assertion: setup mismatch demonstrated

The unchanged optimized movement test rebuilt and exited 1 at:
`continuous soil reaches the buried granite apron without a wall seam`.

- Actual feet: (2.100000, -0.093333, 0.299909) metres.
- Soil height there: 0.276010 m. The above-ground condition passes; only the
  fixed `Y > 0.05` expectation fails.
- That approach uses the overload without a rock step-surface callback. Its
  default tread lookup supplies only the soil heightfield.
- A diagnostic control adding the existing rock step callback, with all other
  legacy setup and 45 frames unchanged, reaches (2.100000, 0.600000, 0.713765).
  The original assertion then passes. This is evidence of a missing query in
  that test setup, not evidence requiring a granite reshape or higher step limit.
- The legacy starting feet z=0.003 also lie inside the current soil collision
  (height 0.196598). Transplanting that start directly into live callbacks leaves
  the body blocked. Integration routes instead fall from clear space onto the
  actual combined collision before movement begins.

Do not declare the original suite green: it remains unchanged and stops early.
Its subsequent tests were not reached by the original runner; its Debug run is
also skipped by that runner after the optimized failure.

## Independent top-traversal problem

Sixteen bidirectional/cardinal/diagonal routes were exercised on the untouched
outcrop at 60 Hz, then on an independently edited specimen. Seven routes per
specimen produced sustained catches. All 32 starts were grounded and clear;
no post-frame capsule overlaps occurred on those routes.

| Fresh route | Requested / achieved forward distance | Longest catch | Sampled clear lift at witness |
| --- | --- | --- | --- |
| 03 | 2.100 / 1.493 m | 23 frames | 12 mm |
| 04 | 2.100 / 0.987 m | 42 frames | 20 mm |
| 06 | 2.100 / 1.280 m | 31 frames | 33 mm |
| 07 | 1.200 / 0.320 m | 33 frames | 30 mm |
| 11 | 1.200 / 0.053 m | 43 frames | 4 mm |
| 12 | 1.200 / 0.027 m | 44 frames | 35 mm |
| 16 | 1.935 / 0.160 m | 66 frames | 4 mm |

The clear-lift search is a diagnostic sampled capsule-clearance check for one
frame, not proof of a supported endpoint, acceptable slope, continuous swept
clearance, or permission to climb a wall. It is not used to move the player.

Witnesses indicate more than one mechanism:

- Routes 06/07/12 have leading surface normal Z approximately .610/.610/.569,
  below the existing .65 tread cutoff. This is a slope-policy conflict with the
  user's desired upper-surface traversal, not simply a too-small step height.
- Route 11 has an upward-facing leading tread but its height is slightly below
  the capsule feet; it does not offer the positive rise the step search requires.
  A 4 mm sampled lift clears the next requested position.
- Routes 03/04/16 need closer inspection of tread sampling and surface queries.
  At route 03 the long and short downward rays return the same height but very
  different normals (.890 versus .000276). At route 16 the long ray hits while
  the short tread ray misses. Do not yet label this a specific raycast bug:
  triangulation, edge hits, origin and reach must be isolated with a smaller test.

## Ground approaches, departure and edits

Six fresh ground approaches covered three front positions and both sides/rear.
Side/rear steep approaches stopped grounded and outside the granite. Two front
routes continued over/around the outcrop; one front route stopped. This is a
sample of the current geometry, not certification of every steep face.

The departure route entered airborne descent and subsequently settled on the
editor floor at z=-0.117 without overlap. It includes departure from the finite
soil patch; it does not prove natural falling from every rock edge.

The edited specimen used production operations: 16 top strikes emitted four
chips (0.000040423 m3 removed), and one committed scoop removed 0.001537309 m3
of soil. The same seven top routes caught; the edited ground approach remained
grounded and clear. This small edit did not materially change the sampled top
trajectories. It is not broad excavation/undercut coverage. Dynamic debris,
loose-soil motion/brush response, render publication and grass are not simulated.

## Reproducibility and evidence

- New diagnostic compiled and ran in optimized and Debug configurations. Both
  exited 1 intentionally because investigation flags remain, not due to crashes.
- Both report 32 routes, 14 flagged, zero diagnostic safety flags.
- Exact top-route trajectory fingerprint: `5fe1bd6e0f02c691` in both builds and
  a repeat optimized run. It covers sampled top-route positions/grounded state,
  not all state or ground-approach trajectories. Timings never enter it.
- Complete reports match after removing timing samples, including the repeat.
- Snapshot: `Fixtures/MovementBaselineDiagnostic_2026-09-07.txt`. This is
  observed failing evidence, not a new passing/golden acceptance fixture.
- Raw timings/reports and standalone binaries live under
  `Build/Verification/MovementBaseline`. No wall-clock budget is enforced here.
- Existing authority certificates were not rerun: no production code changed.
  No interactive validation or whole-client build was performed.

## Proposed next change, not implemented

First isolate missing/unstable tread detection into small regressions. Then
separate supported surface-following from climbing an exterior wall, retaining
bounded clearance, overhead checks, legitimate falls and steep-face blocking.
Do not globally increase step height or accept every clear upward movement.
The upper-slope policy needs deliberate treatment alongside the sampling fix.

After that, modernize the original integration setup and separate generic
movement invariants from authored-route expectations. Preserve the old failing
evidence and test why any assertion is replaced. A full green baseline still
requires the rest of the original suite plus manual walking confirmation.
