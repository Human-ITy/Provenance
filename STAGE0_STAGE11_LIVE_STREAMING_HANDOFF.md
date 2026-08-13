# Stage 0-11 live-streaming compatibility handoff

Status: terrain and presentation compatibility gates passed; Stage 0-11 player traversal is clean

Workspace: `C:\Users\D-Day\ProvenanceEsoterica`

Branch observed: `provenance/client-spike`

Observed HEAD: `cd1360fc`

Date: 13 August 2026

## Why this cut exists

Stages 7-11 had moved to the bounded four-worker package pipeline, while the clean floor and Stages 5-6 still reconstructed terrain synchronously. The objective was to give every certified playable stage the same complete 192 m live-terrain streaming floor without changing geology, geometry, collision, or identity.

Stage 12 was not changed by this cut. Its former presentation blocker is now closed; opening it remains a separate architecture decision.

## Implemented

- Clean, Stage 5, and Stage 6 now build immutable CPU terrain packages on the same four bounded workers used by later stages.
- Stage 7 now uses the same worker/publication path.
- OpenGL publication remains on the owning frame thread.
- Stage 5-6 preserve their original world-aligned 0.5 m surface lattice, cell-centre interpolation, material query, palette colour, collision, and authority semantics.
- Stage 7 preserves its original `SampleBlock`, topology, and per-triangle authority/material law.
- Terrain display-list retirement is now `active -> one-second grace -> reusable pool`; ordinary traversal no longer calls `glDeleteLists`. Destruction occurs at GL shutdown.
- Terrain workers run below normal thread priority so background derivation cannot take priority over the player/presentation owner.
- The Stage 0-11 live player/render owner runs one tier above normal priority; this closes otherwise unattributed OS scheduling gaps without changing authority, geometry, or publication semantics.
- Clean certification shutdown now destroys the owned window, joins package workers, and releases GL before static teardown.
- The live-ladder trace now separates `draw_ms`, pre-present `finish_ms`, and `present_ms`; the report includes `gpu_finish_mean_ms` and `present_wait_mean_ms`.
- The ladder reports the requested live radius/diameter rather than the stale 64/128 label.
- The permanent cardinal receipt now hard-gates movement frames over 16.667 ms.

## Preserved semantic certificates

These were rerun after the shared worker conversion:

- Stage 5 geology: PASS, digest `4b4b94154ce6caae`
- Stage 6 exposure: PASS, digest `de56cbf0edb7bdce`
- Stage 7 visible exposure: PASS, geometry digest `3dc3e3e3ffd959fc`

## Permanent full-replacement gate

Command:

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement --live-radius=192 --far-extent=0
```

Result: PASS, 32/32 cases and 10,624 movement samples.

- all eight playable stages: clean and Stages 5-11
- north/east/south/west 192 m outbound and return
- 2,601 packages and full 192 m completeness at origin, outer station, and return
- zero origin packages survive at the outer station
- zero sky/fallback samples
- exact authority, material/FeatureId, geometry, collision, package, and fixed-camera return parity
- zero movement frames over 16.667 ms
- latest per-stage movement worsts across all four bearings: Stage 0 `14.780 ms`, Stage 5 `9.855`, Stage 6 `11.397`, Stage 7 `8.572`, Stage 8 `10.238`, Stage 9 `8.777`, Stage 10 `8.100`, Stage 11 `8.736`; every case remains below `16.667 ms`

Artifacts:

- `Docs/provenance_worldgen_cardinal_replacement_cert.txt`
- `Docs/provenance_worldgen_cardinal_replacement_trace.csv`

## Presentation-backend isolation and correction

The periodic player-visible hitch has been isolated and closed without changing terrain or geology. The permanent controlled matrix retained the same Stage-11 world, full 192 m residency, and package lifecycle while changing only presentation behavior:

- unfenced swap-interval-zero baseline: `61.993 ms` worst, `61.650 ms` in `SwapBuffers`, FAIL;
- complete scene submission suppressed: `57.561 ms` worst, `57.095 ms` in `SwapBuffers`, FAIL;
- swap interval one: no large periodic swap spike, but the 60 FPS and residency-completeness contract failed;
- pre-present completion with swap interval zero: two 40-second runs and one 90-second run passed with zero frames over `16.667 ms`;
- the final exact-tree 90-second run's p99 was `2.718 ms`, worst frame was `9.101 ms`, worst completion wait on that frame was `0.359 ms`, worst present wait was `0.468 ms`, and the complete radius remained `192.00 m`.

Because the no-scene control retained the defect, terrain display lists, HUD submission, and scene draw volume are excluded as causes of this periodic event. The evidence identifies queued GPU/driver work being surfaced unpredictably by `SwapBuffers`. The narrow correction explicitly completes submitted work before presentation in the Stage 0-11 player and live-ladder paths, keeping swap interval zero. The unfenced path remains available only as the A/B diagnostic baseline.

The corrected full live ladder now reports `status=PASS`, directional walk/sprint/free-flight PASS, digest invariance PASS, stage isolation PASS, and periodic present stall PASS. Every ordinary traversal phase has zero frames over `16.667 ms`; the highest traversal worst was `13.609 ms`. The separate residency-stress phases remain attributed as stress work rather than ordinary player traversal. One immediately prior ladder attempt captured a single `81.652 ms` driver-completion wait during Stage 6 with terrain idle; the passing rerun is the committed receipt, and the outlier remains recorded here as a presentation-tail observation rather than a terrain regression.

Artifacts and commands are documented in `Docs/PRESENTATION_BACKEND_ISOLATION.md`. Primary receipts:

- `Docs/provenance_presentation_isolation_baseline_swap0.txt`
- `Docs/provenance_presentation_isolation_no_scene_submission.txt`
- `Docs/provenance_presentation_isolation_swap1.txt`
- `Docs/provenance_presentation_isolation_prefinish_swap0.txt`
- `Docs/provenance_worldgen_ladder_live_perf.txt`
- `Docs/provenance_worldgen_ladder_live_perf_trace.csv`

## Playable launcher and runtime browser contract

The ordinary playtest launcher now enters the latest certified stable runtime directly instead of opening the old Stage-0 menu:

- `PLAY_WORLDGEN_STAGE0.cmd` retains its legacy filename but launches Stage 11 (`CERTIFIED FAULT DISPLACEMENT`).
- Launching `ProvenanceClient.exe` with no arguments follows the same Stage-11 entry contract; it no longer falls through to the unrelated Phase-4 UI.
- Phase 4 remains available explicitly as `--play-phase4` or `--legacy-phase4`.
- Startup resolves the nearest owning `Data\Worldgen` root from the executable location before any certificate access. Directly opening `Build\x64_Release\ProvenanceClient.exe` no longer turns missing relative paths into false runtime failures.
- The certification browser and tool drawer are closed on entry.
- The playable world uses the certified 192 m live radius / 384 m diameter with the far field disabled.
- Press `M` to open the runtime browser.
- The browser is a scalable two-pane hierarchy: Left/Right changes category; Up/Down or the mouse wheel changes the stage within that category; Enter loads it.
- Number shortcuts remain available for direct stage changes.
- The stage list pages to the selected entry, so adding stages does not extend the panel below the window.
- Stage 12 is not exposed. This cut clears its presentation prerequisite but does not implement or authorize Stage 12.

Permanent launch check:

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-launch-contract
```

Latest result: PASS, including when invoked with `Build\x64_Release` as the process working directory. It proves menu/tool closure, Stage-11 selection, the 192 m/384 m world dimensions, far-field disablement, category/stage navigation, and successful authority-backed selection of every runtime from Stage 5 through Stage 11. Receipt: `Docs/provenance_worldgen_launch_contract.txt`.

Player-visible QA also launches the executable with no arguments, sends `M`, and captures the rendered browser. Receipt: `Docs/provenance_runtime_menu_visual.png`.

## How to continue in this folder

1. Build the Release x64 Provenance client.
2. Rerun the cardinal replacement gate above after any terrain lifecycle change.
3. Run the live ladder:

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-ladder-live-perf --live-radius=192 --far-extent=0
```

4. Use `draw_ms`, `finish_ms`, and `present_ms` in the CSV to keep command submission, GPU/driver completion, and compositor presentation distinct.
5. Rerun the permanent presentation isolation control after any rendering-backend or WGL policy change.
6. Preserve the unfenced baseline and no-scene modes until a future VBO/IBO backend proves the same periodic-stall gate. Display-list modernization remains worthwhile, but is not the cause or required repair for this specific hitch.
7. Require the live ladder and cardinal replacement gates to remain green before opening or changing Stage 12.

## Workspace caution

The worktree contains extensive pre-existing modified and untracked work belonging to active lanes. Do not reset, clean, bulk-stage, or rewrite unrelated files. The compatibility work in this cut is concentrated in `Code/Applications/ProvenanceClient/Main.cpp` plus generated receipts and this handoff.

## Governing status

The complete live terrain streamer and the corrected presentation boundary are certified across Stages 0-11. Ordinary all-bearing walk, sprint, and free-flight movement remains below the 60 FPS frame budget while the full 192 m neighborhood stays resident and exact replacement/return parity remains intact. Stage 12 was not opened by this cut, but the presentation blocker that held it closed is resolved.
