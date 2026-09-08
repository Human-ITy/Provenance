# Movement contact clearance, fix 2 — 2026-09-07

Follow-up: `MOVEMENT_SLOPE_FIX3.md` resolves the remaining sampled slope-route
flags. Results below retain the historical fix-2 policy and observations.

## Scope and implementation

Production change: `Geometry/GraniteLabMovement.h`. A grounded capsule blocked
on a nearby upward-facing tread may perform a small contact-clearance move
even if the tread height is below the rounded capsule's feet datum. Previously
this was rejected because it did not qualify as a positive stair rise.

The new path runs only when the existing tread callback supplies a height from
no more than half a capsule radius below the feet, with treadRise <= Skin.
It does not bypass the tread callback's normal cutoff. Maximum lift is the
lesser of 6 mm and one quarter of the requested axis travel. The final clearance
margin also stays within that bound. It checks the raised starting position,
midpoint and destination, then refines destination clearance with four bisection
probes. Existing support/fall handling follows. This remains the lab's discrete
collision model, not a claim of mathematically continuous swept collision.

Step height (.22 m), slope cutoff (.65 normal Z), player dimensions, speed,
gravity, authored granite and all material assets remain unchanged. Full stair
searches still require a positive tread rise. This is not permission to lift
through arbitrary walls or to climb while unsupported.

## Regression results

Before the change, authored routes 11 and 16 advanced only .053333 m and .160000 m,
with 43 and 66 consecutive catch frames. New regression cases failed in both
optimized and Debug builds before implementation.

After the change:

- Tread suite: 24 checks, zero failures in both configurations. The two routes
  have zero sustained catch frames at 30, 60, 120 and 240 updates/second. Their
  achieved distances at 60 Hz are 1.176569 m and 1.946667 m.
- New shape-independent core suite: 37 checks, zero failures in both builds.
  It preserves 24 existing generic assertions from the original movement test
  and adds 13 clearance checks: tiny supported obstacles, overhead obstruction,
  tall-wall refusal at four rates, and unsupported-body refusal. Original
  integration source/assertions remain intact; this extraction prevents its
  deferred seam failure from hiding generic movement results.
- Slow blocked-frame core case uses 8 collision probes, within its existing
  limit of 24. Full route observations reach 35 probes in a sampled frame.
  No new wall-clock performance guarantee is claimed.
- Existing soil suite: 203,562 checks, zero failures in optimized and Debug.
- Full fresh/edited diagnostic: 32 routes, 6 flagged (three per specimen),
  zero diagnostic safety flags. Flags were 10 after fix 1 and 14 originally.
- Three tested steep side/rear approaches stop at unchanged positions. Some
  walkable front paths shift slightly; exact overall trajectories are not
  expected to remain identical following a movement fix.
- Normalized optimized/Debug reports are identical. Top-route fingerprint:
  `522dbe1a8beb272d`. Timing samples are excluded. Snapshot is in
  `Fixtures/MovementClearanceFix2_2026-09-07.txt`.
- Isolated runtime compile/link succeeded after the final cap refinement.
  Existing engine dependency products were reused. No live DLL was replaced
  and no editor or helper process was stopped.

New maintained runner: `Scripts/RunGraniteMovementCoreTest.cmd`.
The existing tread runner now includes the expanded multi-rate route checks.

## Remaining work / limits

Routes 06, 07 and 12 still catch at the existing slope cutoff. Those flags remain
active and the full diagnostic intentionally exits 1. Address slope policy
separately, retaining the exterior-wall checks; do not relax these tests merely
to produce a green report.

The original soil/apron assertion still uses the old environment setup. It was
not changed or rerun as a whole suite here. The generic core assertions were
extracted and executed, but that does not certify every remaining original
authored integration case. No interactive smoke test, live runtime publication,
full clean build, or broad dynamic-debris/undercut coverage is claimed.

No shape edits, cleanup deletion, Git staging, commit or push occurred.
