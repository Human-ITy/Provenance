# C++ playable checkpoint — 2026-09-07

This is an integration work-in-progress checkpoint, not a completed world release.
The playable authority is the C++ preview of `data://provenance/provenancesandbox.map`.
The editor's certificate workbench and the frozen 32 cm excavation certificate are
distinct paths. Preserve their regression evidence without substituting them for
validation of the playable outcrop.

## Included scope

- Provenance components, geometry and world systems; the shared renderer, shader,
  input, camera, logging and build integration changes required by this checkout.
- GraniteLab.slnx, RunGraniteLab.cmd, the verifier and maintained build/test scripts.
- Data/Provenance maps, descriptors and editable native GLB/PNG resource inputs.
- Current nature/tool source cohorts, export reports and runtime derivatives;
  soil/mud authoring inputs; the saved editable v002 tree placement scene.
- Lab history, migration receipts, fixtures and small measurement evidence.

The exact initial selection is `CPP_PLAYABLE_CHECKPOINT_FILES_2026-09-07.txt`, with
paths relative to the repository root. This selection records a content snapshot,
not individual authorship: these source/data directories were previously untracked.
The selection helper is a preparation tool, not a post-commit ownership detector.

The tree placement .blend was opened headlessly and checked: no linked libraries
and no unpacked file images. It preserves the editable 117-tree scene used to
derive the 14-tree runtime trial. The full separate v001/v002 model library is
not included; its v002 README/manifest provide context only. Regenerating that
entire library from its original builder is outside this checkpoint. Current
runtime assets and the saved scene can be edited independently of that library.
Some offline export reports contain original absolute workspace paths: repoint
those paths or regenerate a report before using the packaging scripts elsewhere.

## Excluded, not deleted

Unrelated Phase 18 client/Worldgen changes and their independent evidence remain
local and unstaged. Goblins, other unused artist-library folders, Blender backups,
duplicate archives, generated executables/objects, caches and crash dumps are not
part of this checkpoint. Exclusion is not proof of an external backup.

The accepted granite shape is preserved in Geometry/GraniteOutcropAuthoredShape.h;
the shape tool can export it for mesh editing. An OBJ left in ignored Build is
not automatically treated as the accepted artist source. Compiler .obj files
must not be confused with editable Wavefront meshes.

## Reproduction and limits

1. Restore the checkout, obtain the external engine dependencies described in the
   root README, and install the matching Visual Studio 2026 C++ toolchain/SDK.
   External is deliberately not vendored here. Exact clean-machine restoration
   of the locally installed External package has not been certified.
2. Follow GRANITE_LAB_WORKSPACE.md. Open GraniteLab.slnx, Debug/x64; build using
   the Granite Lab launcher rather than invoking broad shared pre-build events.
3. Launch the editor with the sandbox map and select Play Map. Compiled resources
   and binaries are rebuilt locally, not stored in this source checkpoint.
4. Follow MEADOW_SEAM_TREE_TRIAL_V1.md for published binary/map hashes, tests and
   the current stationary performance observation. No gameplay code was changed
   merely to make this Git checkpoint.

Remaining work is explicit: walking/seam acceptance and current-build movement
streaming measurements; outer terrain is still separate from editable lab soil;
outer grass has no wear/recovery ledger; tree trial has no chopping, felling,
trunk collision or runtime LOD switching. Stationary capped ~60 FPS is not proof
of uncapped GPU headroom or movement performance. Grass casting remains opt-in.

Do not copy the unrelated modified ProvenanceClient/Main.cpp into this lab
checkpoint. This checkpoint uses the Esoterica editor/runtime entry instead.

## Checkpoint preparation checks

- 954 selected paths, approximately 658 MiB before Git compression/deduplication;
  largest input is the 88.4 MiB saved Blender scene. No LFS conversion or paid
  storage configuration was introduced.
- Native Data/Provenance descriptor references resolve to selected or already
  tracked inputs (zero missing references in the descriptor-path scan).
- 701 staged asset/fixture blobs match working bytes exactly. Explicit -text
  attributes preserve these initial inputs without renormalizing old history.
- Workspace configuration/dependency preflight passes. Native tree checks pass
  for 15 power-of-two textures, 14 foliage materials, 14 mapped instances and
  211,098 triangles. Grass shadow receiving passes 17 source checks.
- A targeted credential-pattern scan of selected source/metadata returned no
  matches; this is not a comprehensive security audit of all repository history.
- Existing whitespace and CRLF in byte-preserved asset reports/fixtures are
  intentionally retained; diff whitespace warnings are not silently corrected.
- A zero-byte Git index.lock last written September 2 blocked staging. No Git
  processes were present. It was moved to the local recovery filename
  .git/index.lock.stale-20260902-preserved-20260907; no source file was deleted.
- No new gameplay build or live movement benchmark was run for this commit.
  Existing published validation remains bounded by the receipt cited above.
