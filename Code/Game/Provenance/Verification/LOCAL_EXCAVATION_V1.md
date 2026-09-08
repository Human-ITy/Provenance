# Local cumulative Granite excavation certificate — V2 (V1 history retained)

## Scope

This is an additive authority prototype, not a change to the four-bridge
release fixture, FIX6, world generation, rendering contracts, or the 16 frozen
geometry fingerprints. A separate playable cube now exercises this model;
it is not yet adapted to arbitrary procedural Granite meshes, support
recomputation, or other materials.

`GraniteExcavation::Specimen` starts as one coherent 32 cm Granite cube.
The material-owned state is 32 x 32 x 32 occupied 1 cm cells, each with its own
stored damage energy. There is no whole-object health or strike counter.

## Contact and response

`Apply` raycasts the current remaining volume, records contact position,
normal, cell identity and distance, and returns an outcome even for a valid
zero-removal contact. A circular flat implement distributes finite transferred
energy over visible cell centers in its shallow contact footprint. Normal
incidence transfers more energy than a grazing angle. Grade gates
deconstruction; insufficient grade dissipates transferred energy without
changing occupancy or stored damage. No surface-mark rendering is claimed.

The specimen uses a fixed 2700 kg/m3 density. The default removal-work setting
of 1,000,000 J/m3 and required grade 2 are certificate calibration parameters,
not measured Granite fracture physics or final gameplay balance.

Stored damage is distinct from excavation. Cells retain damage until their
local removal-work threshold is reached. Removed cells clear their damage and
emit source-identified cube chips. Selection is made before mutation, so one
strike cannot reuse its energy on successively exposed layers. Excess energy
is dissipated in V1, not propagated; subsequent strikes raycast deeper.

Invalid/nonfinite requests do not mutate the specimen. Rays originating inside
occupied matter do not mine hidden cells. Reach is bounded and enforced.

## Accounting and discretization

- Each removed 1 cm cube has volume 0.000001 m3 and mass exactly 2700 mg (2.7 g).
- Milligrams retain sub-gram matter; this prototype does not round each chip
  into the existing integer-gram parcel/spall chain. That adapter remains future work.
- All removal is represented as explicit cell chips in V1; no fines split is fabricated.
- Per-strike energy: transferred = stored-damage change + fracture work + dissipated.
- Per-strike matter: lost occupied cells = unique emitted chip identities.
- The verifier integrates signed triangle volume from the exposed boundary,
  verifies paired oppositely oriented edges, and checks volume against actual
  chip mass/density and receipt aggregates with absolute tolerance 1e-10 m3.
  This is numerical consistency tolerance, not geometric accuracy.
- Spatial resolution is 1 cm; cell-center footprint boundaries have at most
  one cell diagonal (about 1.733 cm) sampling uncertainty. Sub-cell cracks,
  smooth gouges, material-plane propagation and arbitrary-mesh fidelity are not claimed.

The boundary builder emits canonical grid vertices and removes internal faces.
The certificate validates closure for its tested excavation sequences; it is
not a proof that all arbitrary voxel removal patterns remain manifold.

## Executable checks

The existing `RunGraniteAuthorityVerificationCorpus` invokes the separate
`GraniteLocalExcavationV1` certificate. Failures use that receipt name and fail
the process in Debug and Release. `m_numExcavationChecks` counts its checks.

Coverage includes:

- weak implement and zero-energy response receipts;
- invalid energy, ray miss, inside-origin protection and reach limit;
- damage accumulation, third-hit removal, and repeated deepening;
- exact named-field receipt/state replay (no struct padding comparisons);
- an independent location 20 cm away without advancing the first site;
- contact/excavation on all six specimen faces;
- footprint, material resistance and incidence response;
- mesh/matter/energy reconciliation and terminal column exhaustion;
- dropped, duplicated, wrong-mass and wrong-location chip negative controls.

The canonical exact replay checks here cover one toolchain's ordered execution.
They are not a portable cross-platform determinism guarantee or a new frozen
excavation fingerprint corpus. AlgorithmVersion=1 identifies this initial
cell-based law. Any deliberate law change must bump it and document the reason.

## Next boundary

Confirm the separate player test with live contact/footprint feedback and
visible cavity updates. Then design the conservative adapter to
procedural Granite geometry and the existing gram-based matter chain. Only
after those agree should changed support be recomputed and detachment invoked.

## Validation — 2026-09-03

- Debug runtime build: succeeded.
- Standalone verifier Debug and optimized Release: both exit 0, PASS.
- 45 local-excavation checks; overall 1113 gate checks and 27 negative controls.
- Existing four strike-input cases / 268 checks remain passing.
- All 16 original geometry goldens and 16 surface goldens remain unchanged.
- Console results match between configurations. New excavation named-field
  replay is tested within each configuration; no cross-configuration raw
  excavation fingerprint comparison is claimed.
- Default repeated-hit example: six strikes remove two cells, deepen the
  contacted column by 2 cm, and emit 5400 mg (5.4 g) of chips.
- No build or verifier error encountered. No new interactive excavation test
  was performed: this feature is not connected to Play yet.

Changed source files (relative to the project root):

- `Code/Game/Provenance/Geometry/GraniteExcavation.h` (new API/types)
- `Code/Game/Provenance/Geometry/GraniteExcavation.inl` (new implementation)
- `Code/Game/Provenance/Geometry/GraniteGeometry.cpp` (compile the additive module)
- `Code/Game/Provenance/Verification/GraniteExcavationCertificate.inl` (new checks)
- `Code/Game/Provenance/Verification/GraniteAuthorityVerifier.cpp` (invoke checks)
- `Code/Game/Provenance/Verification/GraniteAuthorityVerifier.h` (check counter)
- `Code/Applications/ProvenanceVerifier/Main.cpp` (print counter)
- `Code/Game/Provenance/Verification/LOCAL_EXCAVATION_V1.md` (this contract)

## Play adapter — 2026-09-03

The Play world now starts in **local excavation mode**. A 32 cm cube at world
minimum (-1.4, 27.0, 0.12) sits beside the old bridge lab at (-3, 27).
No map asset or old geometry law was changed.

- `G`: switch between the excavation specimen and old bridge regression lab.
- `F`: one strike per press, along the camera-center ray. Release and press again.
- `R`: reset only the selected lab; switching modes preserves both states.
- `T` in excavation: cycle chisel (15 mm radius, 4 J), hand (grade 0),
  point (4 mm radius, 0.4 J). These are test calibrations, not real tool physics.
- RMB remains the editor's ordinary camera control. No new cursor capture,
  player collision, or grounded locomotion is introduced.

The excavation reach is 2 m. Green reticle means a reachable specimen contact
with a capable implement; yellow means contact with insufficient grade; white
means no reachable specimen hit. A second read-only query reports out-of-reach
contacts up to 10 m. This query is specific to the cube, not scene-wide physics
occlusion or collection of the display chips.

HUD: live distance, contacted cell damage, attempts/contacts/removing strikes,
last response, removed volume/grams, cumulative recovered grams and ledger
status. Yellow cell outlines show retained local damage. A surface-tangent
circle is a footprint guide, not a sub-cell fracture boundary prediction.

Removed cells rebuild the material mesh; zero-removal damage does not rebuild
it. A dedicated tiny-face-safe builder preserves 1 cm faces without changing
the old preview builder's area threshold. The first 128 actual ledger chips
are displayed beside the cube; all chips remain accounted beyond that display
limit. The tray is presentation, not pickup/rigid-body simulation.

The input adapter owns only Play-world state, samples held-key edges without
the broken keyboard-connectivity flag, and releases its mesh on world teardown.
F/R cannot dispatch to both labs. No structural release occurs in cube mode.

Adapter files: `Systems/GraniteExcavationLab.inl` (new) and
`Systems/WorldSystem_Provenance_Debug.cpp` (integration), plus seven added
chisel checks in `Verification/GraniteExcavationCertificate.inl`.

Adapter validation: Debug runtime builds successfully. Debug and optimized
Release verifiers both exit 0 with 52 local-excavation checks, 1120 total gate
checks, 27 negative controls, and zero failures. The original goldens remain
unchanged. No build error encountered; live editor acceptance is pending.

Manual acceptance still required: start Play, approach the labeled cube, check
live HIT distance, repeat F at one location until a cavity appears, move 20 cm
and confirm independent damage, try HAND zero-removal, use R reset, and switch
G to confirm the bridge fixture retains its own progress. The standalone
verifier does not validate keyboard focus, GPU rendering, or teardown in a
running editor.

## V2 recessed-contact repair and performance diagnostics — 2026-09-03

Reproduction: a narrow open recess allows an oblique ray to strike the edge of
a cell, while a parallel ray through that cell's center is blocked by the lip.
Twelve cases span all three axes, both opening sides, and both tilt directions.
Before the repair all twelve `RecessedEdgeMustDamageContactedMatter` checks
failed, while contact and conservation checks passed. After the repair all pass.

The direct hit now survives footprint selection unconditionally after valid
contact/grade/energy checks. Neighbor cells still require the original bounded
footprint and visibility tests. This deliberately changes only the additive
excavation law, so its AlgorithmVersion is now 2. Neither the original granite
algorithm version nor its 16 goldens was changed. V2 has 88 excavation checks.

The Play HUD is labeled `LOCAL CUMULATIVE EXCAVATION v2`. It reports CPU wall
times for aiming, the previous whole lab tick and peak, the last strike's
Apply and ledger stages, mesh construction, mesh registration, and old-mesh
unregistration. These measure only this lab's CPU work, not the rest of the
editor or GPU execution; they are not whole-frame or GPU profiling. The
whole lab tick includes diagnostic I/O on action/heartbeat frames.

Safe adapter-only optimizations: one long-range aim query followed by a 2 m
reach classification; reuse that result only while camera origin/direction and
specimen state are unchanged; refresh damage-outline IDs on strikes/resets
instead of scanning every cell each frame. No authority query algorithm or
fracture rule was optimized in this pass. FPS improvement is not yet measured.

`GraniteExcavationTrace.log` is appended beside the executable. It flushes
request coordinates/direction/tool parameters, response summaries, reset and
session markers, and begin/end stages for Apply, ledger, boundary construction,
mesh registration/unregistration and teardown. One-second tick heartbeats help
locate whether execution stopped inside the lab or later in the engine. A
missing end marker is a clue, not proof of a specific deadlock. If tracing
cannot open the file the HUD reports FILE ERROR. The log contains local test
geometry/input diagnostics only; no background uploads.

Retest: record HUD timings while idle, moving/aiming, striking without removal,
and removing matter. Compare G/bridge mode. If the full editor hangs, capture
Visual Studio Break All / call stacks and preserve this log. The earlier hang
has not been reproduced or claimed fixed by this contact repair.
