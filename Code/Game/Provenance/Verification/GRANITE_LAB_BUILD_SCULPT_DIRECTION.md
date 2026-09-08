# Build and sculpt workflow — user clarification, 2026-09-07

## Status and authority

This records the user's corrected requirements, not completed features. It
supersedes the existing-items-only placement direction in
GRANITE_LAB_PLACEMENT_V1.md. The playable lab remains the behavioral baseline;
the old 32 cm regression cube is not its implementation.

The original scene is editable in build mode. Do not make original entities
unselectable merely to protect the baseline. Preserve a restorable preset and
undo history instead. New catalog placements must be real new instances.

## Interaction contract

- F2 enters/leaves build mode. A world click must not dismiss or obscure its UI.
  Provide explicit minimize/expand. Panel clicks must not move the camera or
  accidentally place an object. The panel must fit and scroll within the viewport.
- Catalog: choose/drag an asset, see a provisional surface placement, click/drop
  to place. Releasing the mouse leaves the object in position and selected.
- Existing and newly placed instances share selection and translate/rotate/scale
  gizmos. Separate object selection from catalog selection. Keep optional snapping,
  fine adjustment, elevation/burial, cancel, and undo discoverable.
- Save/load scene presets must be explicit. Session-only edits must be labeled;
  do not claim persistent placement until save/reload is tested.
- Building interactions follow the useful patterns in Enshrouded, not a literal
  copy of all its keybindings or unsupported assumptions about its internals.

## Granite sculpting: actual local brush, not XYZ sliders alone

The user explicitly wants manual Blender-like manipulation: pulling and
shrinking contours with an adjustable brush footprint.

- Grab/Pull: a mouse drag moves a local region around the picked surface point.
- Inflate/Shrink: expand/contract that region along its surface normals.
- Radius changes the affected area: broad crown/ridge shaping versus fine edges.
- Strength controls displacement; falloff blends the center into untouched rock.
- Show the brush footprint and affected region. Capture one undo record per
  stroke, not per frame. Minimize/selection/camera input cannot start a stroke.
- Topology changes are not prohibited by principle. If tessellation is needed,
  preserve the authored shape and rebuild its dependent representations. Do not
  force correspondence with the old OBJ's vertex count or smooth away the shape.
- Granite is authorable like clay in build mode; this does not redefine granite
  as soft material during play. Sculpting is not a pickaxe strike, and must not
  emit chips or silently add recovered gameplay parcels.

## Required architecture before multiple interactive granite placements

GraniteOutcropLab.cpp currently holds one static State with a single body,
terrain adapter, fracture worker, chip set, and receipts. The current Authoring
UI translates that single intact body. It is not an instance-based sculpt tool.

Separate the reusable granite material/strike implementation from an instance's
geometry, transform, bounds, damage, collision, support, debris, and accounting.
Each placed granite instance needs stable identity and its own state. Mouse
picking, strikes, movement, soil contacts and publication must route to the
appropriate instance. A second visual mesh alone does not provide these behaviors.

Keep accepted playable geometry separate from provisional authored geometry.
Build mode previews should coalesce stale work and preserve the exact edited
shape. On leaving build mode, validate and publish the accepted geometry,
collision, support, material mapping and accounting together. If invalid, keep
the draft and explain the problem; do not exit into broken collision or silently
reshape it. Store a rollback snapshot before replacing accepted state.

Rigid movement preserves volume/mass. Sculpting or scaling changes volume and
therefore authored mass at the configured density. The user did not request
constant-volume sculpting. Do not stretch an existing recovered-matter ledger to
fit a new volume. Editing a previously mined instance needs an explicit state
transition/revision policy rather than an invisible reset or a blanket ban on
editing. Capture this policy before implementing that transition.

## Verification targets

Test interaction and material invariants, not one outcrop's exact silhouette:
panel survives world/panel clicks; minimize preserves selection; drag release
places one instance; cancel creates none; originals and copies can be selected;
gizmo changes survive release; undo restores a stroke/placement; radius/strength
affect the intended neighborhood; untouched vertices stay unchanged; invalid
solids are not committed; accepted visual/collision geometry agrees; mass matches
accepted volume; new instances run the same strike/support/movement logic.

No existing assertions have been removed or weakened by this direction record.
Obsolete UI/shape-specific expectations may be replaced with these requirements;
finite geometry, ownership, concurrency and matter checks remain meaningful.

## Reference actually reviewed

Keen Games, *Pact of the Flame Update* (January 28, 2025), Building Improvements:
https://enshrouded.com/en-US/news/enshrouded-pact-of-the-flame-update

It documents first-person building and full-axis rotation for decorative props,
with different rotation limits for interactive props and blocks. User-supplied
screenshots demonstrate placement/rotation fine-tuning and snapping controls.
This is a targeted reference review, not a claim to have read all developer
discussions or to know Enshrouded's internal implementation.

## Immediate panel patch published — 2026-09-07

GraniteLabAuthoring.h: keep active placement/gizmo windows above the editor's
preview at RenderPre without forcing focus; add minimize/expand; bound panel
size and make long content scroll; move slider labels above their full-width
controls; permit Cancel even when Apply is locked. This patch alone does not
implement catalog spawning, multi-granite authority, sculpting, or persistence.

BuildGraniteLab.ps1 completed the four shared Debug entry projects and their
dependencies. RunGraniteLabPlacementTest.cmd passed 73 checks with zero failures
in each of optimized and Debug configurations. Those are geometry placement
regressions, not tests of the UI or the requested sculpt workflow.

Launched the normal RunGraniteLab.cmd, opened Play Map, and observed the playable
scene remaining responsive beyond four minutes. The user opened F2 after the
automation's rapid F2 press did not toggle it. Live computer-use checks confirmed
that a world click leaves the panel visible; minimize preserves the selected
granite and its gizmo; another world click leaves the compact panel visible;
Expand restores the same granite selection. No placement was applied or saved.

That check exposed clipping of the Expand button in the fixed-height compact
bar. A follow-up uses current font/frame metrics and a 120 px minimum. The final
rebuild succeeded; that final sizing adjustment still needs a live recheck.
Published Game.Runtime DLL: 7,158,784 bytes, SHA256
`97A88411B650E618BB28B8EA1D98D02A0D0EFAA1F5323AC9C019D9E87777E8BD`.
Only the specifically approved checkout editor/resource helpers were stopped.
Existing shared grass changes were preserved; the grass-shadow controls were
not modified during the live check. New asset placement, multi-instance granite,
sculpting, and save/reload remain unimplemented requirements above.
