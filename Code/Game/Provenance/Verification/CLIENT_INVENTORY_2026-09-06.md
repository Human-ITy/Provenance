# Provenance client inventory — 2026-09-06

## Scope and evidence

Repository: `C:/Users/D-Day/ProvenanceWorkspace/Client/ProvenanceClient`.
Branch: `integration/ei3-recovery-byte-integrity-client`.
HEAD at inspection: `6a60dab3ab81e91d9697fe418b7c3a3898205d5e`.

This is a repository-wide first-pass inventory with a deeper file-level map of the playable granite/soil/grass lab. It is not a fresh test certificate, performance baseline, complete dependency graph, authorship audit, or deletion approval. No runtime code, assets, project files, Git index, or existing files were changed. Only this report and its two supporting inventory records were added.

Evidence: filesystem enumeration and sizes; Git index/status/ignore rules; solution/project entries; selected source includes and call sites; material/texture descriptors; authoring scripts; lab records. Historical documentation is distinguished from observed source. No build, test executable, resource import, launcher, or cleanup script was executed.

Supporting records:

- [File manifest](CLIENT_INVENTORY_FILES_2026-09-06.csv): 126 selected files with byte sizes, SHA-256 hashes, Git coverage and proposed disposition. Includes the complete current `Code/Game/Provenance` and `Data/Provenance` trees, the verifier application, and 24 selected authoring/diagnostic files in `Build/x64_Debug`. Excludes these newly added inventory records. This is not a hash manifest of the entire client.
- [Git status snapshot](CLIENT_INVENTORY_GIT_2026-09-06.txt): individual modified/untracked paths before adding the inventory records.

Hashes identify inspected bytes, not their author or whether a test passed. Folder sizes are summed file lengths, not allocated disk space or process memory. Enumeration excluded hidden/system entries by default; `.git` and `.vs` are not included in the area census. It was not a locked filesystem snapshot. Git warned that the user's global ignore file could not be read; repository-local ignore rules were readable and verified.

## Findings that matter before any cleanup

1. The entire `Code/Game/Provenance` tree, `Data/Provenance`, and the three-file `Code/Applications/ProvenanceVerifier` project are untracked. Their files are present and referenced, but not preserved by a commit on this branch.
2. Essential authoring commands and diagnostic records live under ignored `Build/x64_Debug`. The documented workflow depends on them. The directory cannot be treated as disposable wholesale.
3. Conversely, 22 `.exe` and compiler `.obj` artifacts (35.73 MiB) are in the root of `Code/Game/Provenance`. Do not sweep the whole untracked source directory into a commit without reviewing these.
4. The playable lab depends on renderer, shader, input and editor integration outside `Code/Game/Provenance`. A source-slice archive alone is not a reconstruction of the working client.
5. The broader standalone client, the editor-hosted outcrop lab, and the verifier are distinct application paths. They must remain distinguishable during organization.
6. Disk usage is dominated by build intermediates, debugging data and dependencies, not the size of the granite source. Nothing in this census establishes a runtime memory leak.

## Client-wide area census

Counts and MiB are from before adding this inventory. These are directory totals, including nested generated files or archives where present; they are not counts of handwritten source only.

| Area | Files | MiB | Role / proposed disposition |
|---|---:|---:|---|
| `Code` | 2,814 | 135.95 | Applications, game, engine, tools, third-party code, projects; includes misplaced artifacts/archives. Preserve, classify before moving. |
| `Data` | 158 | 162.24 | Resource descriptors, maps, texture inputs and world authority data. Preserve references and byte contracts. |
| `Assets` | 40 | 21.38 | Character OBJ, icons/source sheets, UI mockup and vegetation assets. Separate client asset lane; not assumed redundant with `Data`. |
| `Build` | 2,568 | 6,143.35 | Mixed generated output and authoring/diagnostic workflow. Audit file roles. |
| `Docs` | 532 | 446.82 | Handoffs, images, traces and evidence. Retention review, not blanket deletion. |
| `Tools` | 32 | 0.59 | Integration certificates, launch scripts, worldgen compilers and Python cache. Preserve scripts; cache is a separate class. |
| `External` | 5,318 | 998.24 | Ignored dependency tree. Verify acquisition/version reproducibility before treating as rebuildable. |

Additional root archives: `External.zip` 283.58 MiB and `External (2).zip` 287.39 MiB. Their different sizes do not establish which is authoritative or that either is redundant. Untracked `Code/Engine.zip` and `Code/Game/Provenance.zip` also exist; contents/equivalence were not audited.

`Data` breakdown: Demo 63.18 MiB, Editor 26.49 MiB, Provenance 55.23 MiB, Worldgen 16.56 MiB, Render 0.78 MiB, Physics below 0.01 MiB. These other areas cannot be discarded just because the immediate work is granite.

`Assets` breakdown: Characters 16.47 MiB, Icons 4.69 MiB, UI 0.18 MiB, Vegetation 0.04 MiB. The small `Assets/Vegetation/grass_tuft.png` is not the grass atlas resource referenced by the outcrop material.

## Git and preservation map

At inspection: 3,312 tracked paths, 125 individually reported untracked paths, 43 tracked paths reported modified by porcelain status, and no staged changes. Counts precede this report. Some working changes may involve line endings; status is not a claim that every modification is a semantic change.

- Untracked paths by area: Code 85, Data 33, Tools 5, and two root ZIPs.
- `Code/Game/Provenance`: 54 files across Components (1), Geometry (20), Systems (8), Verification (25), plus the 22 root-level test artifacts noted above.
- `Data/Provenance`: 23 files, including the sandbox map and all granite, soil and grass material inputs.
- Three untracked material shaders: `Code/Engine/Render/Shaders/Materials/ProvenanceGraniteTriplanarPBR.esf`, `ProvenanceGrassSoilPBR.esf`, `ProvenanceSoilTriplanarPBR.esf`.
- Other untracked work includes `Phase18MacroPageAuthority.h`, `Data/Worldgen/MacroAuthorityPhase18`, worldgen Python scripts and `__pycache__`.
- Tracked working changes extend through Engine rendering, Base memory, camera/editor UI, Player input, project/property files, the standalone client and documentation. See the exact status snapshot. Do not attribute all of this work to granite or to one author.

Repository `.gitignore` ignores `.vs/`, `build/`, `/External/`, `**/_AutoGenerated/*`, and `*.vcxproj.user`. `git check-ignore -v` confirms that the authoring command, authoring OBJ and generated mouse-regression C++ harness are ignored by the build rule.

`.gitattributes` marks `Data/Worldgen/**`, `*.mcp`, and `*.mcm` as `-text` to preserve canonical byte-addressed data. Do not run a broad line-ending or formatting cleanup across these files.

Recommended preservation sequence (not executed): classify intended source/assets and required engine integration; identify originals and necessary ignored tools; establish a reviewed backup/version-control checkpoint; only then move files. Do not blanket-add `Build`, source-root binaries or ZIP archives. Git coverage is a preservation issue, not proof of past authorship.

## Application and source responsibility map

| Path relative to client root | Observed role / dependency |
|---|---|
| `Esoterica.slnx` | Includes Editor, Engine, tooling, Game and ProvenanceVerifier projects. No dedicated Granite Lab project. The standalone ProvenanceClient project is not listed in this solution. |
| `Code/Applications/ProvenanceClient` | Separate client project with `Main.cpp`, Causal world/geology/water headers, EI integration and terrain presentation. Contains 63 files recursively, including a nested `Build` directory. Do not conflate it with the editor-hosted outcrop. |
| `RunProvenanceClient.bat` | Searches release output folders for the newest standalone client and supplies the historical local bridge address. This is not the granite sandbox launcher. |
| `PLAY_EI3_CANONICAL.cmd` / `Tools/Integration/Start-Ei3CanonicalPlayable.ps1` | Separate canonical EI3 launch path. Preserve independently of the granite authoring workflow. |
| `Code/Applications/ProvenanceVerifier` | Headless authority verifier project. Directly compiles shared granite/material/surface sources and both verifier implementations. Its `Main.cpp` invokes granite and surface corpora. |
| `Code/Game/Provenance/Components/Component_ProvenanceWorldSettings.h` | World settings and default granite/soil/grass material resource references. |
| `.../Systems/WorldSystem_Provenance.h/.cpp` | Reflected world-system contract and runtime orchestration/package state. |
| `.../Systems/WorldSystem_Provenance_Workbench.cpp` | Deterministic matter workbench fixtures. |
| `.../Systems/ProvenanceSurfaceEvaluation.h/.cpp` | Shared absolute-coordinate surface evaluation. |
| `.../Systems/WorldSystem_Provenance_Debug.cpp` | Debug UI, visualization and integration host; 12,214 physical lines. Includes both lab `.inl` files at lines 6754–6755. Calls `OutcropLab::Tick` at line 11672 and releases it at line 7235. |
| `.../Systems/GraniteOutcropLab.inl` | Playable outcrop controller, worker/publication coordination, rendering, grass and diagnostics; 504 lines. Textually included, not its own compilation unit. |
| `.../Systems/GraniteExcavationLab.inl` | Separate local-excavation lab, also textually included. |
| `.../Geometry/GraniteGeometry.h/.cpp` | Broader granite authority/formation/fracture implementation. `.cpp` has 20,162 physical lines and includes `GraniteExcavation.inl`. Not synonymous with the playable outcrop implementation. |
| `.../Geometry/GraniteExcavation.h/.inl` | Separate local-excavation model; retain the regression path independently of outcrop interaction. |
| `.../Geometry/GraniteOutcrop.h`, `GraniteOutcropV2.h` | Outcrop geometry/partition foundations; V2 is consumed by the current contact-cast model. |
| `.../Geometry/GraniteOutcropAuthoredShape.h` | Current baked authored top surface; `GraniteOutcropV2::TopHeight` samples it. Preserve as an active shape input. |
| `.../Geometry/GraniteContactCast.h` | Current outcrop contact/cut geometry and matter path. Includes V2. |
| `.../Geometry/GraniteStructuralSupport.h` | Outcrop support preparation; depends on ContactCast. |
| `.../Geometry/GraniteContactDebris.h` | Debris movement/contact, depending on ContactCast, ground and SoilScoop. |
| `.../Geometry/GraniteLabMovement.h`, `GraniteLabContact.h` | Playable movement/contact routing. |
| `.../Geometry/GraniteOutcropGround.h` | Seeded ground/soil relationship to outcrop geometry; includes V2. |
| `.../Geometry/SoilScoop.h` | Current mutable soil path; includes ContactCast and OutcropGround. |
| `.../Geometry/GrassCover.h` | Grass-cover state and terrain interaction; includes SoilScoop. |
| `.../Geometry/SoilHeightfieldV26.h` | Superseded soil implementation retained for reference according to lab record; repository search found no external code consumer. Preserve pending historical-reference policy. |
| `.../Geometry/MaterialGeometry.h/.cpp`, `SoilGeometry.h/.cpp` | Broader material/soil authority implementation, distinct from lab presentation. |

This include structure is a warning against physically reorganizing by filename alone: current grass and soil explicitly depend on the outcrop types. A later material-general tool needs deliberate interfaces, not merely renaming `Granite` to `Material`.

The Game runtime project explicitly compiles the Provenance `.cpp` sources. The verifier separately compiles a subset. Moves must update both project consumers, filters, includes and any generating workflow. Whether every custom project addition survives BuildGenerator regeneration remains unverified; do not regenerate the solution as an inventory operation.

## Engine/editor dependencies outside the lab folder

Modified tracked paths relevant to reviewing the complete integration include:

- Rendering: `Code/Engine/Render/Device/DeviceRenderWorld.h/.cpp`, `RenderGeometryBuilder.h/.cpp`, `RenderMaterial.h`, `RenderSystem.cpp`, `Systems/WorldSystem_Render.h/.cpp`.
- Shader integration: `Code/Engine/Render/Shaders/EngineShader.h`, `CommonFlags.esh`, `Renderer/MaterialShaderPBR.esh`, plus the three untracked material shaders listed above.
- Memory diagnostics: `Code/Base/Memory/Memory.cpp`, `Code/Engine/Debug/Widgets/PerformanceStatsWidget.cpp`.
- Camera/UI: `Code/Engine/Camera/Components/Component_ToolsCamera.h/.cpp`, `Code/EngineTools/Core/EditorTool.cpp`, `Code/Applications/Editor/EditorUI.cpp`.
- Input: `Code/Game/Player/PlayerInputState.h/.cpp`, `Code/Game/Player/Systems/EntitySystem_Player.cpp`.

These are observed modified paths, not a file-by-file authorship assertion. `Verification/EDITOR_MOUSE_LOCKOUT.md` specifically records camera/editor fixes and the historical regression run. It also warns that rebuilt PDBs do not match the original saved dump. Preserve that distinction if archiving crash evidence.

## Active resource chains

Read source descriptors and checked 34 explicit `data://` references found in Provenance map/material/texture files: all resolved to existing files on this Windows filesystem. This does not certify compiled resources, actual GPU loading or case-sensitive portability.

| Material | Referenced texture inputs |
|---|---|
| Granite | `T_Granite_A_1K_RUNTIME.png` and `T_Granite_N_1K_RUNTIME.png` through GraniteAlbedo/GraniteNormal descriptors. |
| Soil | `t_soil_a_2k_master.png` and `t_soil_n_2k.png`. |
| Grass/soil cover | Soil inputs plus `t_grass_a_2k_master_v006.png`, `t_grass_n_2k_v006.png`, and both `t_grass_blade_atlas_a.png` / `_b.png`. |

The grass atlas descriptors specify the uncompressed four-channel texture group. Their presence and wiring are verified; alpha correctness and the old rectangular-strip rendering issue were not re-tested during inventory.

Granite 2K master albedo/normal and the development height image also exist. They are not the source paths selected by the current granite texture descriptors. This is not permission to delete them: they may be the authoring masters for runtime derivatives. Conversely, grass and soil files named `master` are directly referenced inputs, so moving all `master` files out of `Data` would break these resource chains unless references/import handling change.

`ProvenanceSandbox.map` explicitly references granite and soil resources; the world settings component supplies a default grass-cover resource, and the lab obtains it through `GetGrassCoverMaterial`. A map-only text search would miss this default dependency.

## Build contents and preservation hazards

| Build area | Files | MiB | Treatment proposed |
|---|---:|---:|---|
| `_Temp` | 2,243 | 2,878.74 | Predominantly compiler/linker intermediates. Verify build recipes before later cleanup. |
| `x64_Debug` | 273 | 2,609.86 | Mixed runtime files, test generations, scripts, sculpts, imports and diagnostic evidence. File-level review required. |
| `x64_Release` | 49 | 500.57 | Runtime/tool dependencies and symbols; not interchangeable with Debug. |

Largest individual item: `Build/x64_Debug/EsotericaMouseLockout.dmp`, 1,643.10 MiB. Other large items include `Build/x64_Release.zip` (153.70 MiB), an Engine.Runtime incremental-link file (131.09 MiB), `libclang.dll` (121.59 MiB in each Debug/Release output), and large PDBs. The complete Build directory is about 6.00 GiB, while its Debug subdirectory is about 2.55 GiB. These are disk figures only.

Files to preserve/review before deleting anything under Build:

- `ApplyGraniteOutcropAuthoring.cmd`, `RunGraniteOutcropShapeTool.cmd`, and `RunGraniteOutcropCandidateTest.cmd`: actual import/bake/certify workflow. The apply command copies the accepted candidate into the live authored header. It was inspected, not executed.
- Other `Run*Test.cmd` and `RunEditorMouseRegression.cmd` wrappers: test build recipes. Several rely on being launched from `Build/x64_Debug` and a hardcoded Visual Studio installation. A move needs path fixes, not just relocation.
- `GraniteOutcropAuthoring.obj`: verified Wavefront text, not a compiler object. A blanket `*.obj` deletion would erase an editable mesh.
- `GraniteOutcropAuthoredShape.last-pass.h`, `GraniteOutcropCandidate.h`, `GraniteOutcropCandidate3.h`: rollback/candidate state; establish which artist source produced each before retirement.
- `EditorMouseStateRegression.cpp`: generated test source; its generator exists in Verification, but the exact reproduction command and surrounding recipe must be retained.
- `MouseLockoutDiagnosis.md`, `inspect_mouse_dump.py`, the dump and relevant traces: historical diagnosis, subject to explicit retention policy. Dump handling also deserves privacy care because memory dumps may contain sensitive process data.
- `GrassCoverImport`: staged engine sources, project/material/shader files and `before-grass-runtime` backups. Not merely a texture cache.
- `SoilMaterialImport`: staged descriptors/shader/project files and earlier master texture backups. Not yet proven redundant or reproducible.

The authoring OBJ still contains an older comment requiring vertex order and fixed XY, while the current authoring guide describes arbitrary topology with highest-surface XY baking. Preserve both; reconcile this documentation mismatch during tool organization.

## Tests, records and historical evidence

`Code/Game/Provenance/Verification` contains authority/surface verifiers, outcrop and contact tests, support/local-update/debris tests, movement/contact tests, grass and soil tests, allocator/render regression tests, authoring/export utilities and four Markdown records. Full filenames and hashes are in the manifest.

The broader client also has root-level `CERT_*.cmd` and `PLAY_*.cmd`, `Tools/Integration`, `Tools/Worldgen`, extensive root handoffs and `Docs` traces/images. Keep those broader systems separate from the narrow outcrop certificate suite. Existing reports are historical evidence, not fresh passes obtained by this inventory.

`Docs` contains sizable CSV performance traces and PPM captures; its largest observed file is `provenance_worldgen_ladder_live_perf_trace.csv` at 21.52 MiB. Decide which records are long-term evidence versus replaceable runs before any archival policy. A source cleanup must not quietly discard the only evidence of a regression or accepted build.

## Proposed disposition and next decisions

1. **Preserve/checkpoint first:** intended untracked source, resource inputs, shaders and required modified engine/editor integration. Keep unrelated Phase 18/EI work identifiable. No commit or push was made.
2. **Extract workflow from ignored output:** propose stable homes for authoring scripts, test runners and editable sculpts; retain their existing copies until new paths work. Determine source-of-truth OBJ/preset and candidate lineage.
3. **Separate actual build artifacts:** propose moving test products out of the source root and giving tests dedicated output locations. Old `_new`, versioned test builds, link intermediates and caches are review candidates, not yet approved deletions.
4. **Reorganize source by responsibility:** first lab host/UI boundaries, then authority implementation boundaries, with the future baseline guarding each move. Keep the 32 cm certificate, playable outcrop and broader authority systems explicit.
5. **Preserve shared resource wiring:** proposed source/artifact folders must account for direct master-texture references and component-provided default resources.

Open checks before an actual cleanup: compare archive contents; verify import backup uniqueness; identify original authored OBJ lineage (including the user-supplied Dropbox files, which were not scanned here); validate all wrapper/build-generation recipes; decide historical trace/dump retention; resolve intended Git checkpoint scope. The whole external machine/Dropbox is outside this client inventory.

Next step recommended: review the preservation/checkpoint and relocation map, then capture the separate correctness/performance baseline before changing source organization. Inventory is now recorded; no cleanup or runtime restructuring has begun.
