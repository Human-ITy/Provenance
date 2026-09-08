# Build consolidation pass 2: preserve runnable recipes

The user supplied `codex-clipboard-a25f02c1-8bfa-43cb-8cbe-c92a11c8b0af.png` after pass 1. It shows the V5.9 playable preview loaded with granite, soil and grass. Record this as user-supplied visual confirmation, not a complete movement/excavation or automated rendering test.

## Changes

- Added 17 maintained `.cmd` entry points in `Verification/Scripts`, outside the ignored Build tree.
- Each resolves the existing Build output directory from its own location, restores the caller's directory, preserves exit status, and contains the original compile/test recipe. Static comparison verified all 17 recipe bodies after the intentional path substitutions.
- Authoring calls its maintained sibling scripts. It does not accidentally invoke the old Build copies.
- Preserved the generated mouse harness in `Verification/Fixtures/EditorMouseStateRegression.cpp` and pointed the maintained mouse wrapper at it. Text matches the old harness after line-ending normalization; no test logic was edited.
- Updated the authoring guide and added script usage/prerequisite documentation.
- Original Build scripts remain unchanged for compatibility and rollback. Output files remain in Build; moving products into a dedicated verification output tree is deferred.

## Actual execution results

Scripts were launched from `C:/Users/D-Day/AppData/Local/Temp`, not their script/output directories, to exercise location-independent entry.

| Recipe | Result |
|---|---|
| Maintained Outcrop V2 | Optimized: 8,684 checks, 0 failures. Debug: 8,684 checks, 0 failures. Exit 0. |
| Maintained mouse regression | Debug and optimized: 784 checks, 0 failures each. Exit 0. |
| Maintained movement | Optimized exits 1: `continuous soil reaches the buried granite apron without a wall seam`. Debug stage correctly not reached. |
| Untouched original movement, from Build directory | Same assertion and exit 1; same printed approach coordinates `2.100 -0.093 0.300`. Establishes that the script migration does not cause this failure, not its underlying cause. |
| Maintained authoring, nonexistent OBJ input | Reports `Could not bake ...ProvenanceMissingInput_7b1c8e5f.obj`, exits 1; live authored header unchanged. No successful apply/import was attempted. |

Both outcrop runs reported initial granite volume 9.000000000000 m3 / 24,300 kg, four visible islands, and excavated boundary plus removed volume summing to 9.000000000000 m3 within reported floating-point residuals. Timings from these ad hoc runs are not a controlled performance baseline.

Compared the original 126-file inventory hashes after execution: only `GRANITE_OUTCROP_AUTHORING.md` changed, as intended. The original scripts, runtime/gameplay source, assets, live authored header and preserved inputs in that manifest remain unchanged. Generated test binaries/objects were rebuilt in Build as part of executing the recipes; those are not an editor rebuild. No file deletion, Git staging, commit or push occurred.

## Boundaries / open work

The movement seam assertion must be diagnosed before claiming a fully passing baseline. Do not weaken the test or modify the artist shape merely to make script migration green. Other recipes were statically preserved but not all executed.

The new Scripts/Fixtures files are still untracked along with their parent Provenance tree; outside ignored Build is not the same as committed/backed up. Editable sculpts, candidate headers and import backups have not yet been relocated. Do not delete Build wholesale. The old entry points can later become forwarding wrappers after the remaining recipes are validated.
