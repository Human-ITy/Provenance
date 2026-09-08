# User-reported crown and ground-edge catches — 2026-09-07

Follow-up implementation and current results: `MOVEMENT_SUPPORT_FIX5.md`.
The diagnosis and observations below retain the pre-fix behavior.

## User observation and scope

Image 1 marks the smaller crown: already on granite, standing or crouching,
movement catches near that point. Image 2 marks the projecting edge: several
ground-to-rock approaches stop in either stance. Screenshot files end in
`7d0c25aa-e3f3-410e-88ab-3a2b5260a086.png` and
`f64ffc00-1217-4403-9e81-5ca8e04cc0b5.png`.

Screenshots do not establish exact world coordinates or input histories. The
new sweep therefore samples approximate regions, not recovered camera poses.
No exact match to either cursor position is claimed.

## Reproduction

`GraniteMovementHotspotDiagnostic.cpp` uses the current production movement,
granite/soil capsule callbacks, combined tread callback, and editor floor.
Both stances settle from above before moving, with their respective actual
speeds. The fresh specimen has seed 8675309 and crest 1.393005 m.

- 144 upper-region routes: nine starts, eight headings, standing and crouched.
- 36 ground approaches: nine front and nine rear, in both stances.
- All run at 60 updates/second; 12 consecutive low-progress frames count as
  a sustained stop. This is observational, not an assertion that every route
  should be traversable. Some routes encounter legitimate steep exterior faces
  or satellite boundaries.
- Optimized and Debug reports match exactly: 180 routes, 25 sustained stops,
  zero invalid starts and zero sampled overlap frames. Upper region: three
  standing stops and six crouched stops. Front/rear: four per stance per side.
- The executable exits nonzero for invalid starts or overlap. A zero exit does
  NOT certify clear traversal: sustained stops remain printed for classification.

Runner: `Scripts/RunGraniteMovementHotspotDiagnostic.cmd`.
Preserved output: `Fixtures/MovementHotspots_2026-09-07.txt`.

## Concrete candidate causes

1. **Tread query's lower bound misses some rounded-foot support.** The column
   spans from feet + StepHeight down to approximately feet - .02. At the
   crouched stop (3.273333, 1.3, .941984), the leading rock sample is .919221 m,
   about 22.8 mm below feet, with upward normal Z .883615. It is outside that
   column despite its gentle slope. The offered height instead yields a
   -.588827 m rise, so movement refuses both continuation and full step search.
2. **Small-clearance cap catches on other eligible surfaces.** At the crouched
   stop (3.006667, 1.3, .927311), the offered rise is -.007701 m. Continuation
   may lift at most 3.333 mm at crouch speed, but the diagnostic finds the first
   clear next endpoint at 4 mm. At the upper diagonal start (3.5, 1.7, .471846),
   the X move finds a tread below the foot datum and requires about 21 mm of
   endpoint lift standing / 11 mm crouched, beyond the respective allowance.
3. **Ground riser can hide an eligible tread beyond the fixed leading sample.**
   The front x=1.8 approach stops near y=-.093333 in both stances. Its ordinary
   offered rise is only 1.2–1.8 mm; an additional forward sample 4 cm standing /
   6 cm crouched finds a tread about 13.2 cm above feet, within the 22 cm limit.
   That is evidence of a missed candidate, not proof the complete body can
   safely step onto it. Some other stops have steep leading faces with no such
   nearby candidate and must not be indiscriminately made traversable.

`endpointClearLift` checks vertical-start and next-endpoint clearance at 1 mm
intervals. It does not certify the full swept path, landing support or legal
slope. Likewise the farther-tread search is diagnostic only. Neither search
is fed into production movement. Leading normals are tool-ray observations;
offeredRise is computed with the production walking query and its six samples.

## Proposed next implementation

Separate local support continuation from finding a new step. Test a bounded
support-footprint query that accounts for rounded capsule feet, then validate
short forward tread candidates with full-body lift/travel clearance and real
landing support. Preserve the 22 cm step limit, steep-wall and ceiling refusals,
bounded query work, and standing/crouched route tests. Do not merely increase
the global slope cutoff or permit arbitrary lifting from nearby soil.

The earlier 32 passing routes and 38 integration checks remain valid for their
specific fixtures, not a complete live acceptance baseline. These new stops
must be classified and selected into regressions before claiming a fix.

Only this diagnostic source, runner, report and documentation were added.
No production code, geometry, assets or movement limits changed. No live DLL
was rebuilt, no processes stopped, and no Git staging/commit/push occurred.
