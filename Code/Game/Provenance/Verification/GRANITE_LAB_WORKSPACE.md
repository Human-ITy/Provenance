# Granite Lab workspace and authoring direction — 2026-09-07

## Separate entry point

Open `GraniteLab.slnx` at the checkout root. Set **Granite Lab** as the startup
project once if Visual Studio chooses another project. Select Debug / x64.
F5 launches the existing Esoterica editor with
`-map data://provenance/provenancesandbox.map`; click **Play Map** for the
playable lab. `RunGraniteLab.cmd` launches the same map without Visual Studio.
Neither entry automatically starts Play Map or creates a second runtime.

The workspace contains one build/debug launcher and the nine existing production
projects needed by the editor, resource tools and ProvenanceVerifier. Shared
projects are visible for navigation/debugging but excluded from solution Build;
the launcher explicitly builds their dependency closure using the existing
projects. No geometry, terrain, assets, movement or material settings changed.

Build the **Granite Lab** launcher (or the solution), not an individual shared
project. Its maintained BuildGraniteLab.ps1 disables the broad pre-build kill
event and refuses to build while checkout executables are running. Close the
editor and its resource tools first. It never terminates processes. Individual
shared projects retain their original build events in both solutions.
Rebuild is deliberately incremental and Clean is a no-op, because outputs are
shared with Esoterica.slnx. Building the verifier is not running its certificates.

## Recovery and baseline

NewGraniteLabCheckpoint.ps1 creates an external byte-verified ZIP and manifest,
including untracked source, editable model originals, installed local External
dependencies, runnable binaries and test evidence. It is not a Git commit or
push, and does not attribute unrelated changes to lab work. See the checkpoint
receipt for exact exclusions and restore instructions. The installed compiler
and SDKs, Git object store, and inputs outside this checkout are not bundled.

The accepted movement baseline is MOVEMENT_SUPPORT_FIX5.md, plus the user's live
confirmation that the reported catches pass. This does not certify every route.
No tests are relaxed to make a new authored shape pass.

## Placement progress

The first session-only placement pass is now implemented; see
`GRANITE_LAB_PLACEMENT_V1.md` for F2 controls, checks and limitations. It covers
existing-asset selection, granite translation/elevation preview and application,
and decorative entity transforms. The larger roadmap below is not all complete.

## Authoring roadmap

Catalog placement progress is tracked in `GRANITE_LAB_ASSET_PLACEMENT_V2.md`;
its validation status distinguishes headless checks from live acceptance.

**Direction clarification:** `GRANITE_LAB_BUILD_SCULPT_DIRECTION.md` records the
newer requirement: existing and new instances share build tools, with a manual
radius/strength/falloff sculpt brush for granite. The numbered roadmap below
describes the earlier staged plan, not a restriction on that requested workflow.

1. Explicit lab authoring mode and selectable instance list. Keep the validated
   specimen/reset preset. Label decorative nature assets separately from granite
   with collision, excavation, support and conserved matter; those capabilities
   do not appear just because a decorative model is placed.
2. Reuse the engine's ImGui translate/rotate/scale gizmos. Add elevation entry,
   optional soil snap, controlled burial, below-ground inspection, undo and
   save/load presets. A shared instance transform must drive rendering, contact,
   walking collision and support consistently. Moving rock preserves its mass;
   soil does not silently move with it or appear from nowhere.
3. Live XYZ/proportion preview, then actual shape parameters (crest, facets,
   shoulder/ridge), not merely scaling a rounded mesh. Keep density fixed. In
   ordinary scale mode derive mass from volume; in locked-mass proportion mode
   preserve volume. A target-mass control drives volume rather than overriding
   the ledger. Show total, exposed and buried dimensions/volume distinctly.
   Preview changes are provisional; Apply replaces an intact authoring specimen
   with geometry/collision/support/accounting committed together. Do not rescale
   a mined body with outstanding recovered chips in the first implementation.
4. Add room outside the validated terrain patch for multiple assets. Preserve
   the current outcrop fixture and its movement routes. Measure grass, soil,
   collision and publication costs before choosing the expansion size. Placement
   and soil-contact queries must stop assuming one origin-centered outcrop.

These are lab-only authoring operations, not free matter creation during normal
gameplay. Interactive regeneration should coalesce slider changes and discard
superseded previews; keep timings outside deterministic fingerprints. Multi-asset
support and scalable terrain are real implementation work, not launcher features.
