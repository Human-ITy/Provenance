# Meadow continuity and native tree trial — 2026-09-07

## Published scope

`Build/x64_Debug/EsotericaEditor.exe`, `data://provenance/provenancesandbox.map`.
This supersedes the inward grass feather described in `MOVEMENT_LANDSCAPE_QOL_V1.md`.

- Remove the inward rectangular blade thinning/shortening. Continue matching-density grass outside the original lab, then smoothly reduce density over seven metres into a lighter meadow.
- Match the intact soil top's edge shading to the flat joining landscape. Preserve the existing omission of fully buried perimeter render triangles. No change to soil occupancy, collision, rock shape, or conserved matter. This does **not** unify the editable soil ledger with the immutable outer landscape.
- Deterministic exterior candidates cover the whole 100 x 100 metre floor. Only nearby 4 m patches are resident: an 11 x 11 requested neighborhood with a one-tile retention band, maximum 169 patches, maximum one 1,024-candidate patch publication per update. Roots use the actual rendered terrain triangle height. Bare creek bed and dirt eligibility come from the landscape cover recipe. Far ground remains its existing textured surface.
- Whole-floor candidate population is 34,070 exterior tufts; this is not the number simultaneously rendered. Live creek observation settled at 13,150 exterior tufts / 121 resident patches. The original lab's editable grass remains separate. Exterior grass sways, receives shadows and follows the F8 casting toggle, but does not yet participate in the lab wear/recovery ledger.
- Grass atlas coordinates are explicit mesh UVs, not inferred from the finite lab cover texture. Exterior blades bypass that texture; original blades still use it.
- Fourteen native v002 trees: one smaller overstory and one young tree in each of seven saved groves, 211,098 triangles total. Preserve the reviewed scene's world transforms and conformed root copies. LOD1 geometry only; visual trial, no trunk collision, chopping, felling, runtime LOD switching, or leaf-litter simulation claimed. Artist masters and the full 117-tree placement scene are unchanged.
- F9 waits for meadow publication to settle before sampling. Shadow-toggle changes discard stale exterior shadow meshes before bounded republishing. Clickable F10 creek/return and F11 30-second capture controls accompany the keyboard shortcuts.

## Native tree import correction

The original 1254-pixel foliage art failed the texture compiler's block-size requirement. An initial 1256-pixel runtime derivative compiled, but the live editor caught a D3D12 `COPYTEXTUREREGION_INVALIDSRCBOX` fatal validation message at its 314-pixel mip. This was diagnosed from a local crash dump, not inferred from a successful resource compile.

Final runtime copies are 1024 x 1024, preserving the full normalized image/UV extent without cropping. The embedded originals remain in the source GLBs and previous derivatives remain preserved. `AlignTreeTextures.py` and `PackageNatureTrial.cjs --trees --install-binaries` document the mechanical conversion and restrict replacement to known prior bytes. `VerifyTreeTrial.cjs` prevents reintroducing this non-power-of-two mip-chain issue. Native foliage retains two-sided alpha testing with its authored ~0.35 cutoff.

## Evidence

- LandscapeGrassTest: 884,315 checks PASS; stable identities, terrain-conformed roots, core exclusion, bare-area exclusion, scale, bounded per-patch candidates, edge normals, excavation normals preserved. 34,070 full-floor tufts, largest patch 779, 704 within 25 cm of the old boundary.
- Existing landscape: 4,028,721 checks PASS, 231 tiles / 694,562 triangles unchanged; sampled mud outside the carve remains zero.
- Grass cover: 32,839 checks each optimized/debug PASS. Movement core: 37 checks each optimized/debug PASS. Shader allocation: 4,108 PASS; assert logging: 16 PASS; grass shadow-receiving source checks: 17 PASS.
- All 70 native tree resources compiled successfully. Corrected two foliage textures compiled again after the mip fix. Live editor subsequently loaded the map and entered play successfully on two launches. Trees render in the editor, and grass renders outside the lab in the playable creek view.
- Computer-use validation caught the initial GPU upload failure and verified the corrected launch and clickable creek bookmark. Full walking inspection around all four lab edges and movement-streaming performance still need user acceptance; this document does not certify that every view is seamless.

### Live stationary ABBA observation

Saved `Artifacts/Landscape/GrassShadowBenchmark_meadow_trees_2026-09-07.csv`.
Embedded preview approximately 1280 x 720, editor visible behind it, Debug x64, RTX 5080. Camera fixed at creek bookmark; all terrain and requested grass patches ready. Four legs, three seconds settling and seven seconds sampled per leg.

| Grass casting | Frames | Mean ms | p95 ms | p99 ms | Max ms |
|---|---:|---:|---:|---:|---:|
| Off | 842 | 16.656 | 17.204 | 17.748 | 19.139 |
| On | 842 | 16.656 | 17.207 | 17.932 | 19.564 |

Within the existing trial's frame envelope, consistent with ~60 FPS limiting. Not an uncapped GPU-headroom measurement, movement-streaming test, full-screen guarantee, or isolated tree cost measurement. Keep grass casting opt-in. Requested user F11 walking capture after this run; do not substitute the earlier pre-tree movement CSV for a current-build movement result.

F11's total frame interval includes meadow streaming. Its existing `upload_cpu_ms` submeasurement does not yet include `PublishMeadow`; use the separate Meadow HUD publication time for that CPU section. Do not interpret that column as complete publication cost.

## Published identities

- Game DLL SHA256: `1F3956AA7E5DDA509870F0538812C29DBBEDB2716990B8239D7817F1DAE2634D`
- Engine DLL SHA256: `3FD71B3B5FEBD1ACB0ACCD2CADB4EEEB62A934CBCB40AD9CF067A942E0420626`
- Sandbox map SHA256: `3407BC8F09CBA1DA7C68C891A35919CEC9CCCC76EC0A6E24C4F68E81252B1B59`

Source assets: `output/placement/world_trees_v002/WorldTreePlacement.blend` and `output/integration/tree_trial_v002/export_report.json`. Native inputs: `Data/Provenance/TreeTrial/v002`. Reproduction and shader sources remain in the checkout; Git status alone is not provenance for the untracked Provenance directories.
