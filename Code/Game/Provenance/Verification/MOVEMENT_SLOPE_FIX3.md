# Movement slope policy, fix 3 — 2026-09-07

Follow-up: `MOVEMENT_INTEGRATION_FIX4.md` corrects the maintained integration
fixtures and records publication status. The results below retain fix-3 scope.

## Scope and implementation

The current authored outcrop's upper contours should be traversable without
persistent catches, while nearly vertical ground approaches remain blocked.
No granite, soil, grass, authoring input or material asset was reshaped here.

`Geometry/GraniteLabContact.h` now names the lab's rock tread policy as
`RockTreadMinNormalZ = .5`: up to 60 degrees from horizontal is eligible.
The previous cutoff was .65. A 1e-12 numerical tolerance includes the boundary.
This is a gameplay policy, not a measured granite friction coefficient, and
not an exemption granted only after reaching the top. Exterior slopes between
the old and new cutoffs can also become eligible. Soil's slope policy was not
globally changed. Eligibility alone does not bypass capsule clearance or the
22 cm maximum step height; higher steep faces still occlude lower treads.

`Geometry/GraniteLabMovement.h` also changes horizontal sampling spacing from
Radius * .75 to Radius * .125 (2.875 cm with the current radius). The existing
maximum of two horizontal substeps per frame remains. Walking at 60 Hz uses
one; longer travel, including sprinting, can use two. This is a bounded work
increase in those cases, not a universal spacing guarantee or swept collision.
Player dimensions, speeds, gravity and step height are unchanged.

The sampling change was necessary: with only the new slope cutoff, route 06
still caught at 30 Hz. The recorded feet height was .752037232 m and leading
tread height .971236574 m, normal Z .547324234. Including skin, the requested
rise was about .222199 m, just over the .22 m step limit. Shorter movement
samples traverse it without increasing that limit.

## Verification

- New slope cases against the old policy produced 14 failures in the expanded
  tread suite in each configuration. Changing the cutoff alone left one;
  the final bounded sampling change leaves 44 checks / zero failures in both
  optimized and Debug builds.
- Synthetic treads cover 0, 49, 55, 59.9, 60, 60.1, 70 and 85 degrees, including
  refusal beyond the new limit. Authored routes 06, 07 and 12 progress without
  sustained catches or overlap at 30, 60, 120 and 240 updates per second.
  The earlier tread and small-clearance cases remain active.
- Full headless diagnostic: 32 sampled routes (16 fresh, 16 edited), zero route
  flags and zero diagnostic safety flags in both configurations. Its acceptance
  thresholds were not relaxed. Both runs exit 0.
- Normalized optimized/Debug reports are identical. Top-route trajectory
  fingerprint: `a0f9a6c19b350125`. Timings are excluded. Preserved report:
  `Fixtures/MovementSlopeFix3_2026-09-07.txt`.
- The three sampled steep side/rear approaches still stop at their previous
  positions without embedding. The front u=.75 approach advances farther and
  stops at a face with normal Z .490070, below the new cutoff. Not all ground
  approaches retain the same trajectory; that follows from the changed policy.
- Shape-independent movement core: 37 checks / zero failures in both builds.
- Contact routing: 20 checks / zero failures in both builds.
- Soil suite: 203,562 checks / zero failures in both builds.
- Isolated playable runtime compile/link succeeded, reusing existing engine
  dependency products. No live DLL was replaced; no editor or helper was stopped.

No wall-clock performance budget or authority-fingerprint comparison is claimed
by this pass. Parallel test timing observations are not a calibrated benchmark.

## Remaining baseline work

The original `GraniteLabMovementTest.cpp` integration assertions are unchanged.
Its old soil/apron setup remains a known failure: it starts inside current soil
and omits the rock tread callback. The diagnostic's control with that callback
passes the original approach assertion, but this does not make the untouched
original runner green. It was not rerun as a complete suite in this pass.

The next baseline step is to align the maintained integration setup with the
playable collision environment, preserving explicit seam and wall checks,
then rebuild/publish the runtime when the editor can safely be closed and
perform a manual walk, run, jump and crouch check. These headless changes are
not yet evidence of the behavior in the user's running editor.

Coverage here is sampled, not every possible angle or edited shape. The full
fresh/edited route diagnostic uses 60 Hz walking and a limited strike/scoop
fixture. Targeted multi-rate cases do not certify all routes while sprinting,
10 Hz/variable-frame traversal, moving debris, or arbitrary undercuts.

No cleanup deletion, Git staging, commit or push occurred.
