# Accepted movement checkpoint and focused lab entry

## Preserved before adding the entry point

Local recovery directory:
`C:\Users\D-Day\ProvenanceWorkspace\Checkpoints\GraniteLab-20260907-091956`

- `checkout.zip`: 2,839,168,884 bytes; 9,966 archived files.
- Source bytes: 5,830,608,575; all entries verified by length and SHA256.
- Archive SHA256: `3EC1D96E41E3E9DA54809D592CE6A36DBC942859893345AA62CD0396336D4240`.
- `manifest.csv`, `git-status.txt`, `CHECKPOINT.md` accompany the ZIP.
- Base Git HEAD: `6a60dab3ab81e91d9697fe418b7c3a3898205d5e`.
- This includes mixed worktree edits, not a claim of lab authorship over Phase18
  or nature-asset work. No Git commit, branch change or push performed.
- Editable output/models originals and local External dependencies retained.
  See receipt/script for exclusions. This is an on-disk checkpoint, not a saved
  live gameplay session or independently verified clean-machine restoration.

The accepted shared runtime DLL was unchanged by the entry-point work:
7,041,536 bytes; SHA256
`E04C6E1B5941818F2429CC7FF8E78990AC846A6E4675A69572E0A6A868D0C772`.
Movement evidence: MOVEMENT_SUPPORT_FIX5.md and user's live pass confirmation.

## New entry files (added after that recovery snapshot)

- Root `GraniteLab.slnx`: launcher plus nine shared production projects.
- `Code/Applications/GraniteLab/GraniteLab.vcxproj`: build/debug front door,
  not a separate application binary or copied gameplay implementation.
- Root `RunGraniteLab.cmd`: named-map launcher, with read-only `check` option.
- `Verification/Scripts/BuildGraniteLab.ps1`: safe shared build, no process kill.
- `Verification/Scripts/TestGraniteLabWorkspace.ps1`: read-only workspace checks.
- `Verification/GRANITE_LAB_WORKSPACE.md`: usage and proposed authoring stages.

MSBuild validated Debug/x64 solution configuration and evaluated the launcher
debugger executable, map argument, working directory and build command correctly.
RunGraniteLab.cmd check found the editor and sandbox. Build preflight found the
installed MSBuild and four entry projects. Actual build invocation was safely
refused with the editor, resource server and seven compiler helpers running;
none was stopped. No full rebuild or new F5/live-play test is claimed this turn.

No production C++, terrain dimensions, map, material, texture or authored mesh
was edited. The proposed gizmo, palette, sliders and expansion are not yet built.
