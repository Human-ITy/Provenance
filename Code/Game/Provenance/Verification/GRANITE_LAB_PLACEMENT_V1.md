# Lab placement V1 — 2026-09-07

## Controls / deliberate scope

Launch RunGraniteLab.cmd, Play Map, then **F2**. This pauses lab walking, strikes,
grass recovery and loose-body simulation. Already-running strike/scoop workers
can finish publication; new actions are not queued. RMB + WASD uses the tools
camera for inspection. Returning to play restores the walking camera position.

- List: the coherent granite outcrop plus existing `NatureTrial_` spatial
  entities in the loaded map. No floor/sky/settings entities are editable here.
- Granite: translation gizmo and XYZ offset sliders; XY +/-2 m, elevation
  -0.25 to +0.60 m relative to the original placement. Satellite connections are
  part of the same authoritative body, not independently movable assets.
- Dragging previews rock rendering only **while authoring simulation is paused**.
  Apply stages translated geometry and refitted pristine soil, then commits them
  on the world thread. Play cannot resume while rock/soil/grass publication is
  pending. Cancel/F2 discards an uncommitted rock preview.
- Granite mass and volume remain unchanged; no mass/scale control is offered in
  this translation-only implementation. Normals, topology, bins and stable render
  group memberships are preserved. Current collision BVH is rebuilt; structural
  basal elevation follows the translated source geometry.
- Apply rejects damaged/mined granite, excavated soil, active workers/pending
  publication and placement overlapping the player's capsule. Return to play,
  press R for an explicit reset, and move away before retrying if necessary.
- Apply creates a **new lab soil fixture** around the rock. It does not translate
  the terrain heightfield, transport soil, credit harvested matter, or simulate
  soil deposition. Grass wear is reset with that fixture. This is authoring,
  not a gameplay relocation mechanic. Original placement previews zero offset;
  Apply confirms it. Undo swaps with the previous applied placement (one level).
- Nature objects: live world position, translate/rotate/uniform-scale gizmos,
  scale 0.1–3, and restore-to-selection-start pose. Decorative only: these changes
  do not grant collision, material authority, harvesting or regrowth.
- All placement changes are **Play-session-only**. The map and editable model
  originals are not overwritten. No new asset instances, palette spawning,
  disk presets, automatic soil snapping, granite rotation/scaling/shape sliders,
  expanded terrain, or mouse-ray object selection are claimed in V1.

## Source separation

- `Geometry/GraniteLabPlacement.h`: engine-independent intact translation and
  refusal checks; source geometry owns collision as well as rendering.
- `Systems/GraniteLabAuthoring.h`: development-only UI/gizmo and existing map
  entity selection; no new reflected layouts or copied engine tools.
- `Systems/GraniteOutcropLab.cpp`: pause/input gating, preview transform,
  staged fixture application, player-overlap refusal, publication and reset.
- `Geometry/GraniteContactCast.h`: deterministic tie choice at shared exterior
  triangle edges. Prefer the strongest entry-facing normal, then source ID,
  for contact distances within 1e-9 m. This avoids translation roundoff choosing
  an exit normal and turning entry into a miss. Normals are evaluated only for
  potentially closest contacts. General collision shape and slope rules unchanged.

The edge case was detected by the new translation regression; it was not fixed
by dropping edge samples or relaxing the expected hit behavior.

## Validation

Optimized and Debug suites passed:

| Suite | Checks per configuration | Failures |
| --- | ---: | ---: |
| New intact placement | 73 | 0 |
| Contact routing | 20 | 0 |
| Tread/movement safety | 99 | 0 |
| Maintained movement integration | 38 | 0 |
| Contact fracture / conservation | 522 | 0 |
| Soil excavation | 203,562 | 0 |

Placement checks cover volume/mass, normals and memberships, basal anchoring,
translated target rays including shared edges, walking BVH, capsule queries,
undo error <1e-12 m, and invalid/edited-body refusal. They do not prove every
possible placement or interactively exercise the ImGui controls.

The focused BuildGraniteLab.ps1 completed all four entry projects and their
shared dependencies. Published `Esoterica.Game.Runtime.dll`: 7,108,608 bytes,
SHA256 `0BD84F510CD64AA7E4E69F8DB5228BFF5487F56148B56DB499F17D227CAB2A20`.
An initial link attempt could not open DebugView_Camera.obj;
the object subsequently existed and the retry succeeded without deleting source
or caches. No causal attribution to earlier cleanup is claimed.
The approved helpers had already exited; the scoped stop operation stopped zero
processes. No unrelated processes were terminated.

Live F2/gizmo appearance and input behavior still require the user's preview
check. The headless tests and successful build are not a visual acceptance claim.
