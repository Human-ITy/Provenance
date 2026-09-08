# Granite Lab startup validation — 2026-09-07

## Cause and correction ownership

The grass-sway work's `GRASS_SWAY_LAYOUT_FIX.md` records the startup assertion:
grass reflection ended at byte 62 while HLSL storage occupied 64 bytes. The
renderer incorrectly treated the last field end as the aligned allocation size.
Logging then interpreted the assertion's literal `%` as a format directive,
masking the original assertion with the CRT invalid-parameter failure.

Windows Application events confirm editor exception 0xc0000409 at 10:06–10:07
and 10:18, with a resource-server breakpoint exception at 10:07. Those events
alone did not identify the source defect; the grass-sway investigation supplied
the source-level diagnosis and correction.

Both fixes were already published during the separate grass-sway work:
Base DLL at 10:25:15 and Engine Runtime DLL at 10:26:06. This startup-validation
pass did not rewrite those fixes, rebuild the runtime, alter assets, or change
the launcher. All six source/binary hashes in
`Fixtures/GrassSwayLayoutFixBuild.json` matched the current files.

## Independent checks

- Reran `Scripts/RunShaderParameterLayoutTest.cmd`: 4,108 allocation checks and
  16 literal assertion-format checks, zero failures.
- Reran `Scripts/RunLogAssertDllSmokeTest.cmd`: four calls into the published
  Base DLL returned without CRT failure.
- Diagnostic editor launch reached the editor; a repeat with assertion tracing
  enabled captured no exceptions or application breakpoints during the bounded
  45-second startup observation. Diagnostic code did not patch runtime memory.
- Launched the actual root `RunGraniteLab.cmd` without a debugger, keeping the
  observing shell alive. PID 38036 reached the editor, loaded
  `provenancesandbox.map`, and Play Map visibly loaded the granite, soil, grass,
  and nature assets. This is a live startup/preview observation, not just a
  successful build. Computer-use inspection verified the visible result.
- An earlier detached shell-launch observation was inconclusive: the editor
  remained at its splash and later disappeared without a new Application crash
  event. No cause is assigned to that observation; the held-open normal-launch
  test above is the controlled verification.

## Boundaries

F2 key injection in the earlier diagnostic preview did not expose the placement
panel. Placement interaction, Apply/Undo, complete movement routes, and long-run
stability are not certified by this startup pass. No existing tests were relaxed.
No gameplay geometry, terrain, shader, or asset inputs were changed here.

Only approved test editor/resource processes were stopped between launches.
The final normal-launch playable preview was left open for the user.
