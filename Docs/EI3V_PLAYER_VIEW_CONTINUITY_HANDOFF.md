# EI3.V player-view traversal continuity — handoff

Status: **CERTIFIED**

```text
EI3.A — Authority/coverage mechanics          CERTIFIED
EI3.V — Player-view traversal continuity      CERTIFIED
EI3.Q — Detailed visual richness / quality    CLOSED
```

This cut certifies the playable handoff during movement. It does not change
WorldGenesis laws, WorldSubstrate matter, water, material semantics, terrain
output, or the visual-quality target reserved for EI3.Q.

## Source state

- Client branch: `integration/ei3-playable-launch-mode-fix-client`
- Client source SHA before this cut: `93f55017331a188d864d6ea7fcf957f7abce1b58`
- Engine branch: `integration/ei3-recovery-byte-integrity`
- Engine source SHA before this cut: `140720bf62b3c86eb98d635b168c19d1b06812ef`
- Engine EI3.V commit: `0ccee15ace0f39316b227a4b61ad34839bd447de`
- Certified world UUID: `b3b56645-085c-4572-b2da-3ddcd9d805bb`
- Genesis digest: `9dca0db5344baf0cf709dd654fb80fce38589f0f7fcf01ed8fe086d74818d711`

## Authority and presentation contract

- FableScript publishes macro WorldGenesis and detailed WorldSubstrate matter.
- Provenance retains the coarse carrier until a complete detailed replacement
  is published; a pending or obsolete request cannot create an empty frame.
- Interactive detailed requests can overtake queued speculative prewarm work.
- Normal play uses the matter-backed renderer. The EI3 diagnostic lattice and
  local physics fixtures are fail-closed.
- Landing, gravity, and walking sample the same published matter surface used
  to construct the visible near mesh.
- Static engine ancestry (`MacroFeatureId`, `MacroLandformId`, `ParentRangeId`,
  `MacroPeakId`, and lithology) crosses the detailed snapshot contract without
  the client recreating it.

## Deterministic traversal

The EI3.V run used an isolated authority session on control/bulk ports 8875/8876.
It flew four exact 256 m legs at 30 m/s: north, south back to origin, east, west
back to origin. Every endpoint required published detailed matter. Both returns
required the original engine-issued `MacroFeatureId` at no more than 1 m error.
The run then descended, grounded, disabled flight, and walked 48 m immediately.

The spawn feature has `MacroFeatureId=2a6efd8baccad211`; it does not have a
`MacroPeakId`. The certificate therefore calls this a stable landmark ancestry
round trip and does not falsely claim a named peak.

## Receipts

`Docs/provenance_ei3v_traversal_cert.txt`:

```text
EI3_V=PASS
renderer=NORMAL_PLAYABLE
diagnostic_lattice=OFF
flight_speed_mps=30
flight_legs=4
flight_distance_m=1024.039
landing=PASS
walking=PASS
visible_lower_sky_pixels=0
macro_empty_frames=0
source_revision_changes=0
revision_mismatch_delta=0
ahead_detail_checkpoints=4
endpoint_detail_checkpoints=4
landmark_macro_feature_id=2a6efd8baccad211
landmark_returns=2
landmark_max_return_error_m=0.000000
walk_detail_miss_frames=0
ground_mismatch_frames=0
ungrounded_walk_frames=0
support_hold_delta=0
snapshots_rejected=0
```

Additional exact receipts:

- client release build: PASS (39 pre-existing warnings, 0 errors);
- client executable SHA-256:
  `6D64C1781D4B4DF3659522EA8B2F324EA5B764BC7BC51F0F78C3D4AEA638D42F`;
- FableScript detailed-projection tests: 7/7 PASS;
- client snapshot/delta contract: PASS;
- static ancestry transfer: PASS;
- predictive residency at 24/60/120/240 m/s: PASS;
- landing P0 and course-change planning: PASS;
- canonical launch mode and descriptor authority: PASS;
- launcher authority session and clean owned shutdown: PASS.

Visual evidence:

- `Docs/provenance_ei3v_leg_1.png`
- `Docs/provenance_ei3v_leg_2.png`
- `Docs/provenance_ei3v_leg_3.png`
- `Docs/provenance_ei3v_leg_4.png`
- `Docs/provenance_ei3v_final.png`

## Files changed

Engine:

- `fablescript/worldgen/detailed_projection.py`
- `fablescript/engine_tests/test_ei3_detailed_projection.py`

Client:

- `Code/Applications/ProvenanceClient/Ei3DetailedProjection.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Code/Applications/ProvenanceClient/VisualMaterial.h`
- `Tools/Integration/Ei3DetailedProjectionClientCert.cpp`
- `Tools/Integration/Start-Ei3CanonicalPlayable.ps1`
- `Docs/EI3_CANONICAL_PLAYABLE.md`
- compact text/PNG receipts listed above

## Explicit non-claims

EI3.V does not certify finished terrain beauty. The 8 m near reconstruction may
still look coarse or washed, and the current seed/origin is not a rich named
mountain target. Mountain morphology diversity, rocks and boulders, finer strata
display, vegetation, trails, waterfalls, and hiking-scale readability remain
EI3.Q work. No new world-authority stage is opened by this handoff.
