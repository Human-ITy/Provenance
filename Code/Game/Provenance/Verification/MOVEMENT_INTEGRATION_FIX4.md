# Movement integration baseline, fix 4 — 2026-09-07

## Test setup corrected

`GraniteLabMovementTest.cpp` now initializes the apron approach by settling a
capsule from clear space above the authored terrain. It uses the playable
static collision environment: current granite plus `SoilScoop::Body` capsule
queries, the editor-floor fallback `-double(.12f)`, and combined rock/soil tread
selection with loose soil excluded. This matches the movement callbacks in
`Systems/GraniteOutcropLab.cpp`; it does not include the complete rendering or
dynamic debris update loop.

The original apron assertion remains: Y must exceed .05 and feet must remain
at or above the ground height. Additional checks require a supported, clear
start and no sampled overlap throughout the approach. A negative control starts
at the same settled location but omits the tread callback; it must not cross
the seam threshold. This control passes, demonstrating the setup difference
is consequential rather than simply moving the start past the obstruction.

Fixing that approach exposed two further stale fixture assumptions:

- The old front wall-slide test inherited the approach endpoint, now on an
  upper tread. It advanced only .236 m and stalled for 81 frames. The test now
  independently approaches the steep left exterior and then moves along +Y
  with inward +X pressure. Progress >2 m, zero stalled frames and the 40-probe
  bound are unchanged; no-overlap and a blocked-ground-approach precondition
  were added. This is an explicit route replacement, not a claim that the old
  upper-surface trajectory was repaired in this pass.
- The old central-crown refusal test treated total height as a wall, despite
  legal treads on that route. It is replaced by right and rear steep-face
  refusal checks, in addition to the left-face precondition. These require
  stopping outside the summit, below .4 m, grounded and without sampled overlap.

Rear-shoulder integration routes now also settle onto the terrain and use the
playable static collision/tread callbacks. The existing at-least-one-crossing
criterion remains unchanged. These are versioned authored-fixture checks, not
universal assertions about every future sculpt.

Generic movement, rock-only landing, finite-patch floor behavior, tunnel work
limits and the artificial elevated-floor cavity-lip regression remain in the
same maintained suite. Their isolated setups are deliberate and are not all
claimed to model the complete live environment.

No production movement, geometry, material, map or authored-input changes were
made in this pass. Runtime publication incorporates the earlier movement fixes.

## Results

`Scripts/RunGraniteLabMovementTest.cmd` passes 38 checks in optimized and 38 in
Debug, exit 0. The original maintained runner now reaches both configurations
and the later tunnel/cavity tests instead of stopping at the old seam failure.

- Apron endpoint: (2.100, .600, .714) m; original threshold passes.
- Left exterior slide: 2.354 m, zero stalled frames, maximum 20 probes.
- Fresh-rock slow frame: 13 probes, within its unchanged 32-probe bound.
- Right/rear stops: (3.680, 1.440, .344) and (1.050, 2.173, .300) m.
- Dug-tunnel case: 16 chips, 9 probes, within its unchanged 24-probe bound.
- Elevated cavity-lip slide: 1.828 m, zero stalls, maximum 39 probes within 40.

Normalized transcript: `Fixtures/MovementIntegrationFix4_2026-09-07.txt`.
Timings remain instrumentation, not enforced budgets or determinism inputs.
The separate 32-route and 44-check tread results are recorded in fix 3; those
were not rerun in this test-only pass. This closes the known maintained seam
runner failure, not every possible movement scenario or future authored shape.

## Publication and remaining live check

The closed editor left a resource server and seven compiler helpers holding
the Game Runtime DLL. After explicit user permission, only those eight
processes were stopped, with each process ID, name and executable path checked.
No other process was targeted. These are restartable resource helpers; no
source, asset or build artifact was deleted.

The actual Game Runtime Debug/x64 build succeeded with zero warnings and zero
errors, updating `Build/x64_Debug/Esoterica.Game.Runtime.dll` at 02:10:40 local
time on 2026-09-07. Published size: 7,037,952 bytes; SHA-256:
`33B42ECF2D61C7E6BFBE78198D5EB855885F789DCA1565803320E62D56B29021`.
Build log: `Build/Verification/MovementBaseline/runtime-fix4.log` (repository
relative). No Esoterica/Provenance process remained at the post-build check.

The initial standalone project invocation lacked SolutionDir and failed before
compilation; it was rerun with the actual repository root supplied.
`BuildProjectReferences=false` reused existing engine dependency products and
`PreBuildEventUseInBuild=false` disabled the broad process-killing pre-build
event. This was a real runtime publication, not just the isolated link check.

The remaining user check is to reopen the normal editor, play the sandbox,
walk over the upper contours and around the soil edge, press into the steep
exterior from ground level, then check sprinting, jump/landing and crouching.
No interactive acceptance or full clean-checkout rebuild is implied by these
headless results. No Git staging, commit or push occurred.
