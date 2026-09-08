# Catalog placement V2 — 2026-09-07

## Scope

This implements the first new-instance placement slice of
`GRANITE_LAB_BUILD_SCULPT_DIRECTION.md`. It does not implement the entire build
or sculpt workflow. Original scene entities remain selectable and editable.

- Expand **Add new asset** in F2. Click a nature catalog entry, then press/release
  over a valid soil/rock/landscape surface, or drag the entry out and release.
- A provisional visual follows the pointer. The entry's initial click/release
  does not commit a placement. Invalid drops leave it provisional; Escape,
  Cancel new asset, or returning to play cancels it.
- A valid drop creates one persistent-map entity in the current Play world,
  preserving the original. It remains selected for the existing move/rotate/scale
  gizmo. Mouse release ends movement without requiring a separate Apply button
  for these decorative entities.
- The scene list includes originals and new additions. World clicks also select
  nature objects through their world bounding boxes, with solid-surface occlusion;
  granite uses its current boundary ray. Gizmo handles get first refusal.
- **Undo last added asset** removes the most recent added instance. This is not
  general transform undo/redo; the existing selection-start pose restore remains.
- All edits are session-only. No map or editable source asset is overwritten.

Only initialized, single-static-mesh NatureTrial entities enter this catalog.
Their mesh resource, material overrides, local nonuniform scale, and root
rotation/uniform scale are copied. This is not arbitrary entity/prefab cloning.
The mesh's authored pivot is placed at the surface; automatic bounding-bottom
alignment, surface-normal rotation, snapping, and burial controls remain future
work. Decorative selection is bounds-based, not alpha-tested triangle selection.

New granite instances are intentionally not offered as decorative copies. The
single interactive granite still uses its existing translation/Apply pathway.
Reusable per-instance granite state, local sculpt brushes, and saved presets are
still required before the broader requested workflow is complete.

## Separation

- `Systems/GraniteLabAssetPlacement.h`: draft/copy/session entity ownership.
- `Systems/GraniteLabAuthoring.h`: catalog, selection, pointer and gizmo routing.
- `Geometry/GraniteBuildPlacement.h`: gesture release policy, selection ray-box
  interval, and outer-landscape triangle picking.
- `Systems/GraniteOutcropLab.cpp`: supplies current granite/soil/world-space
  surface hits. Outer landscape is eligible after its publication finishes.

Landscape picking walks the grid intervals crossed by the ray and intersects the
same triangle diagonal used by rendering, rather than placing against an
approximate analytic height. The existing soil and granite geometry remain
authoritative in the central lab patch. No movement/material thresholds changed.

## Validation status

The integrated translation unit compiles. New headless tests pass **115 checks,
zero failures** in both optimized and Debug builds. They cover fresh press vs
catalog release, drag validity, cancel, duplicate-release protection, bounded
selection rays, and vertical/oblique/reach-limited outer-terrain rays. Existing
granite placement tests also pass **73 checks, zero failures** in both builds.

These tests do not exercise entity resource loading, live mouse/gizmo behavior,
or UI visibility. The full shared Debug build was published and launched. The
user has accepted the corrected granite-handle interaction below. New-asset
placement/cancel/selection checks remain pending; they are not covered by that
acceptance.

## Granite handle crash correction

The user reported the editor exiting while grabbing the granite's green gizmo
handle. Source inspection found an unconditional per-frame `Gizmo::SetMode`
call for granite. Engine `Gizmo::SetMode` asserts `!IsManipulating()` BEFORE
checking whether the requested mode already matches. Thus the first frame after
a drag starts violates its contract, even with unchanged translation mode.

`Systems/GraniteLabGizmoMode.h` now gates configuration on an idle gizmo and an
actual mode change. Granite's automatic translation selection and the decorative
move/rotate/scale buttons use this helper. Engine assertions remain intact.
Five added contract tests cover unchanged mode, 120 held-drag frames, rejecting
a mode change during a drag, and switching/restoring translation after release.

Corrected Game.Runtime SHA256:
`8255BA04EEF1ED93D759BB5D3AC228648184D121DE547EFEADF286A2FC75FB8E`.
The normal launcher opened the scene and F2 tools. Automated green/vertical-handle
drag attempts did not close the editor, but did not show clear displacement.
On 2026-09-07 the user answered **"good to go"** to the explicit request to drag
the green handle and confirm that granite moves and the editor stays open.
That establishes user-confirmed live acceptance of this crash correction.
No current matching crash dump was available; older dumps were not attributed
to this run. Scene inputs, material rules, and gameplay geometry were not changed
by this correction.
