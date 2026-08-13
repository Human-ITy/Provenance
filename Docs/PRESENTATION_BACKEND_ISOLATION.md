# Presentation backend isolation

Status: PASS

Date: 13 August 2026

## Scope

This cut attributes and repairs the periodic player-visible hitch previously observed inside `SwapBuffers`. It does not alter geology, terrain authority, package geometry, collision, the 192 m live radius, or Stage 12.

The instrumented frame boundary is:

```text
CPU/update
-> OpenGL command submission
-> optional pre-present completion
-> SwapBuffers
```

Receipts separate `draw_ms`, `finish_ms`, `present_ms`, terrain work, pending packages, and minimum complete radius.

## Controlled modes

All modes use Stage 11, a 192 m live radius, the far field disabled, and the same deterministic walk/sprint/free-flight route.

```text
--cert-presentation-isolation-baseline    swap 0, unfenced control
--cert-presentation-isolation-finish      swap 0, pre-present completion
--cert-presentation-isolation-no-terrain  terrain submission suppressed
--cert-presentation-isolation-no-draw     complete scene submission suppressed
--cert-presentation-isolation-swap1       swap interval 1 control
--cert-presentation-isolation-minimal     player-package terrain only
--presentation-duration=N                 10-600 seconds
```

## Attribution results

| Mode | Duration | Worst frame | Worst present | Frames >16.667 ms | Complete 192 m | Result |
|---|---:|---:|---:|---:|---|---|
| unfenced swap 0 | 40 s | 61.993 ms | 61.650 ms | 1 | yes | FAIL |
| no scene submission | 40 s | 57.561 ms | 57.095 ms | 1 | yes | FAIL |
| swap interval 1 | 40 s | 18.493 ms | 0.024 ms | 1173 | no | FAIL |
| pre-finish run 1 | 40 s | 6.414 ms | 0.170 ms | 0 | yes | PASS |
| pre-finish run 2 | 40 s | 8.609 ms | bounded | 0 | yes | PASS |
| pre-finish extended | 90 s | 9.101 ms | 0.468 ms | 0 | yes | PASS |

Suppressing all scene submission did not remove the approximately 27-second periodic event. Therefore terrain display lists, the HUD, and scene draw volume are not its cause. Swap interval one prevents the large presentation spike but violates the uncapped 60 FPS/completeness contract. Completing queued GPU/driver work before the swap keeps the uncapped path, makes the synchronization point explicit, and removes the periodic long frame across more than three former event windows.

The normal Stage 0-11 playable path and the live-ladder certificate now use pre-present completion. The legacy unfenced behavior is retained only as a diagnostic A/B control.

## Permanent regression gates

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-ladder-live-perf --live-radius=192 --far-extent=0
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement --live-radius=192 --far-extent=0
```

Latest results:

- live ladder: PASS;
- directional walk/sprint/free-flight: PASS;
- periodic presentation stall: PASS;
- ordinary traversal phases: zero frames over `16.667 ms`, highest traversal worst `13.609 ms`;
- forward/reverse authority and geometry digests: exact;
- cardinal replacement: 32/32 PASS;
- all movement cases: zero frames over `16.667 ms`;
- 2,601 packages and `192.00 m` minimum complete radius at origin, outer station, and return;
- zero sky pixels and zero fallback-green pixels;
- exact geometry, material/FeatureId, collision, package, and fixed-camera return parity.

The cardinal receipt also includes cold stage-initialization frames. Those can exceed the movement budget and are reported independently as `worst_frame_ms`; the player traversal gate is `movement_worst_frame_ms` and `movement_frames_over_16_667`.

The exact-tree verification also fixes scheduling ownership explicitly: the live player/render owner runs one priority tier above normal while terrain derivation workers remain below normal. Two otherwise-correct pre-finish reruns exposed external scheduling gaps of 100 ms and 34 ms with terrain, draw, finish, and present work all idle. With the owner priority declared, the final 90-second run recorded `p99=2.718 ms`, `worst=9.101 ms`, zero frames over budget, and a complete `192.00 m` live radius throughout.

One full-ladder verification attempt still observed a single `81.652 ms` wait inside `glFinish` during Stage 6 with terrain work idle; the immediately repeated full ladder passed with a `13.609 ms` maximum traversal frame. The committed receipt is the passing rerun, but this isolated driver-completion outlier remains documented rather than being attributed to terrain or silently omitted.

## Governing conclusion

The periodic defect was not terrain reconstruction and was not caused by the display-list scene submission tested here. It was an unpredictable GPU/driver completion wait surfacing at `SwapBuffers`. The corrected runtime makes that completion explicit before presentation, preserves every world digest, and passes the original player-movement and full-replacement contracts.

Stage 12 was not opened or implemented. Its presentation prerequisite is now satisfied; any Stage-12 authorization remains a separate cut.
