# Grass cast shadows — opt-in trial, 2026-09-07

## Scope

The existing grass, not extra grass density, gains an opt-in ShadowMap view
layer. No additional geometry, shadow texture, light, terrain edits, collision
changes, biomass, or conservation changes. The existing 64 cover patches are
rebuilt within their ordinary two-patch update budget when quality is toggled;
this is not a per-frame rebuild. GPU shadow rendering still has a real cost.

The material compiler already emits a depth-only alpha-test pixel permutation
and uses the material's same mesh shader in forward and shadow views. Therefore
the corrected shadow path uses the atlas alpha, spill rejection, cover visibility, wind
deformation and 6 cm culling padding already used by the visible grass.

At the initial casting-trial publication, grass RECEIVING shadows and SSAO remained disabled to preserve the earlier
root-band correction. The misleadingly named material flag
`RENDERER_GLOBAL_FLAG_DISABLE_CAST_SHADOWS` controls receiving directional shadows
inside MaterialShaderPBR; actual casting is controlled by the mesh view layer.
This trial adds casting onto terrain and other receivers; it does not claim
grass-to-grass self-shadowing or rock shadows received by grass. The follow-up
below enables receiving in source; its publication/validation status is separate.

## Playable controls

- The top-right **Grass shadow trial** panel provides clickable toggle/test
  buttons as alternatives to the shortcuts (automated short key taps were
  unreliable in this editor).
- F8 toggles grass cast shadows, initially OFF. Cancels an active comparison.
- F9 starts an OFF/ON/ON/OFF comparison; F9 again cancels it.
- Each leg waits for grass publication, then settles for 3 seconds and samples
  frame intervals for 7 seconds. Movement, camera-look, crouch, jump, harvest or
  reset input cancels the comparison rather than mixing camera positions.
- Keep the preview focused, camera and resolution unchanged, no sculpting or
  other GPU work during a comparison. Run at overview and close grass views.
- The test reports mean, p95 and p99 in milliseconds. The provisional added-cost
  envelope is mean +1 ms, p95 +2 ms, p99 +4 ms, with >=120 samples per state.
- Completion restores shadows OFF even if within budget. Enable with F8 after
  inspecting the shadows and verifying uncapped performance.
- Raw samples overwrite `GrassShadowBenchmark.csv` beside the editor executable
  (`Build/x64_Debug`), not in a launcher-dependent working directory.
  This is frame pacing, not GPU pass timing. Vsync/frame caps may hide
  added GPU cost; a passing capped run is not proof of available GPU headroom.

## Validation status

- Benchmark statistics/control tests: 11 checks, zero failures. Their generated
  CSV in `Build/Verification/GrassShadows` is SYNTHETIC, not a live FPS capture.
- Shared wind tests: 1,420,038 checks, zero failures; maximum displacement
  0.050797 m stays inside the existing 0.060 m bounds used by both render views.
- Existing grass behavior: 32,839 optimized and 32,839 debug checks pass.
- Game and Engine Runtime DLLs built and published successfully. Playable
  sandbox opened successfully with the new controls.
- Live testing exposed solid rectangular card shadows. In
  `Code/Engine/Render/RenderPasses/RenderPass_CascadedShadow.cpp`, the alpha-test
  bucket was incorrectly bound to `m_pDepthOnlyPipeline`. It now binds
  `m_pDepthOnlyAlphaTestPipeline`, matching the forward depth prepass. The opaque
  bucket is unchanged. A rebuilt live toggle confirmed the rectangle on the
  granite disappeared and the grass cast cutout shadows instead.
- One complete stationary overview ABBA run, RTX 5080, Debug editor,
  1280 x 720 playable viewport, VSync active, default editor limiter:
  HUD OFF/ON mean **16.66 / 16.66 ms**, p95 **17.16 / 17.14 ms**,
  p99 **17.36 / 17.47 ms**. The provisional frame-pacing envelope passed.
  These are rounded HUD observations, NOT GPU timings or an uncapped result.
- That run reported CSV not written. Output now appends the filename to
  `FileSystem::GetCurrentProcessPath()` (which returns a directory). An interim
  incorrect `ReplaceFilename` call was corrected and rebuilt. The subsequent
  trial started successfully but player/camera movement cancelled it, restoring
  shadows OFF; no completed raw CSV from that repeat is claimed.
- Final published Game DLL SHA256:
  `6F3D3CB12874CC215B8E2F93E87E98B4A1425549529F930EC97EAB69C01AB3DC`.
  Engine Runtime DLL SHA256:
  `5B926620BAB467BEB0075A252BEC16869DC1005EEA4867487E5E2614B5CED019`.
- Ground shadows are visually strong; lighting/contrast acceptance remains
  pending. Default remains OFF. No general FPS assurance is claimed.

Before enabling by default: confirm cutout (not rectangular-card) shadows,
movement with sway, no culling flicker, no resurrected blade-tip artifacts,
shadows disappearing with worn/suppressed grass, and compare uncapped runs at
identical views. If cost is excessive, prefer a near-camera shadow radius with
stable patch selection/hysteresis, then validate its transitions. Do not reduce
the ecological grass density or alter movement just to improve this benchmark.

## Shadow receiving follow-up — 2026-09-07

User accepted casting and requested shadows on blade surfaces. The grass-only
material branch now retains DISABLE_SSAO but no longer sets the directional
shadow receiver-disable flag. Existing PBR samples the cascades at the actual
wind-deformed pixel position and attenuates direct sunlight, leaving IBL intact.
F8 still controls casting only; receiving does not require F8 to be ON.

An additional source finding explains a likely contributor to the previous
root bands: grass normals encode cover-cell identity in a downward-pointing
cone. The common shadow sampler used that normal for receiver bias, offsetting
samples into the ground. A shader-local `m_shadowNormal` now defaults to the
original geometric normal for every other material; grass overrides it with
world up. This bias normal is not flipped for back faces. No reflected custom
parameters, packed buffer offsets, CPU types, or allocation sizes changed.
This is a source-supported diagnosis, not yet a live proof of band removal.

Validation completed for this follow-up:

- EsotericaReflector shader parsing, input reflection, compilation and generation
  succeeded. Log: `Build/Verification/MovementBaseline/grass-receive-shaders.log`.
- Engine ClCompile succeeded for all seven generated material shader units.
  Log: `Build/Verification/MovementBaseline/grass-receive-engine-compile.log`.
- `Scripts/TestGrassShadowReceiving.cjs`: 17 source-contract checks passed.
  These guard routing/bias/SSAO/alpha/wind, not rendered pixels or GPU timings.
- Shader parameter layout: 4,108 checks passed; assertion formatting: 16 passed.
- Wind: 1,420,038 checks passed, max displacement 0.050797 m (<0.060 m).

Published after the user's readiness confirmation: editor was closed; only the
eight remaining resource helpers in this client build were stopped. Engine
Runtime Build/link succeeded, and grasscover.material force-compilation reported
success (standalone compiler exit 1 means CompilationResult::Success, not failure).
Logs: `grass-receive-engine-build.log` and `grass-receive-material.log` beside the
shader log above. Published Engine Runtime SHA256:
`6AF9CCA043D1343676D60A16D173469AA79E84BF359F4754A6BF1FD69AD08409`.

Computer-use startup check: the first preview launch emptied the map/viewport;
stopping preview and explicitly reopening ProvenanceSandbox.map restored the
map and a working playable preview. No source change was made for that recovery.
Settled overview displayed about 60 FPS with 7,834 tufts, zero pending patches,
and receiving enabled. Detailed shadow-boundary and performance validation is
still required; this observation alone is not a GPU cost certificate.

Completed stationary 1280 x 720 overview ABBA capture with receiving enabled
throughout, existing VSync/capped setup unchanged. Preserved raw samples:
`Verification/GrassShadowReceiving-20260907-122751.csv` (not synthetic).
Casting OFF/ON: 844/845 samples; mean 16.629987/16.629387 ms;
p95 17.447/17.292 ms; p99 18.809/17.975999 ms; max 27.987/19.750001 ms.
The configured added-casting frame-pacing envelope passed. This does NOT measure
isolated receiving cost or uncapped GPU headroom. Casting was then enabled for
handoff; the playable overview remained visible, with stronger shadowing through
the grass. Rock-shadow boundaries on individual blades were not close-up certified.

Check rock-shadow boundaries across roots and tips with F8
OFF, then grass self-shadows with F8 ON; inspect both card faces, swaying blades,
root bands and cascade transitions. Compare the same stationary overview and
close view before/after receiving; the existing F9 test compares CASTING only
and cannot isolate the incremental receiving cost. Keep VSync limitations clear.
