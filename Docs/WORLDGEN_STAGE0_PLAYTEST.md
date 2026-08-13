# Provenance Worldgen Certified Runtime Playtest

Status: playable certified-runtime lane. Stage 0 remains available as the clean control.

## Launch

```text
Build\x64_Release\ProvenanceClient.exe --play-worldgen-baseline --live-radius=192 --far-extent=0
```

An argument-free launch of `ProvenanceClient.exe` now resolves to this same
latest-certified player entry. The older Phase-4 client is compatibility-only
and requires the explicit `--play-phase4` or `--legacy-phase4` flag. This keeps
an ordinary executable launch from entering a runtime where the certification
browser is unavailable.

The executable also resolves its owning `Data\Worldgen` root from its own
location. Opening the copy under `Build\x64_Release` directly is supported; it
does not depend on Windows supplying the repository as the working directory.

The included `PLAY_WORLDGEN_STAGE0.cmd` launcher uses this command despite its
legacy filename. It now opens the latest numbered certified runtime, Stage 11,
with a complete 192 m live radius / 384 m diameter. The far-field path remains
disabled while its presentation backend is isolated. The clean Stage-0 fixture
is still selectable as the permanent control.

The playtest does not own the camera, route, duration, or exit. It begins in walk
mode after stage selection and keeps normal traversal controls:

- `WASD`: move
- `Shift`: sprint; in free camera, begin a distance-tiered speed chain
- `F`: toggle walk and free camera
- `Q` or `Ctrl`: descend in free camera
- `E` or `Space`: ascend in free camera
- mouse: look and turn rapidly
- `Tab`: release or recapture the cursor
- `R`: toggle the diagnostic residency overlay directly
- `M`: open the journal on the Certified Runtimes page
- `T`: flip the journal to the Diagnostics page
- `L`: toggle the distance ruler directly
- `P`: summon or relocate the terrain palette and its calibration tools
- `X`: toggle the aim-local geology x-ray in Stages 5-10
- mouse wheel: change x-ray depth; Shift+wheel changes width
- `V`: cycle x-ray material, identity, and history modes
- `F9`: save the current x-ray frame and terminal geology receipt
- `Esc`: release mouse look; press again to exit

The journal does not open automatically. Press `M` when you want to change
runtimes. Its left pane declares runtime categories; its right pane contains
only the stages in the selected category. Use Left/Right for categories,
Up/Down or the mouse wheel for stages, and Enter to load. The stage pane pages
around the current selection automatically when future stages exceed its height.
The
`CERTIFIED RUNTIMES` and `DIAGNOSTICS` tabs flip the same journal between pages;
number keys remain optional runtime shortcuts:

```text
[1] PERF.CLEAN_WORLD                     Clean performance floor
[2] CAL.COMBINED                         Combined calibration
[3] GEO.KERNEL                           Stage 5 geology kernel
[4] GEO.EXPOSURE                         Stage 6 geologic exposure
[5] GEO.VISIBLE_EXPOSURE                 Stage 7 visible geologic exposure
[6] GEOMORPH.DIFFERENTIAL_EROSION        Stage 8 differential erosion
[7] GEO.GRANITE_INTRUSION                Stage 9 granite intrusion
[8] GEO.CONTACT_MINERALIZATION           Stage 10 contact mineralization
[9] GEO.FAULT_DISPLACEMENT                Stage 11 fault displacement
[0] CUT.C.OCCUPANCY_PARITY                Cut C integration control
```

The clean choice is the closest playable counterpart to the permanent headless
control. The ruler and terrain/material palette are global tools, not separate
worlds. `CAL.COMBINED` loads the clean calibration certificate and enables both
tools as a convenient preset. Stage 5 is a flat
walkable datum read directly from the certified 3D geology kernel. Stage 6 is
the compiled present erosion surface, with height, material, feature identity,
bedding frame, and contact information supplied by the certified exposure
kernel.

Stage 7 retains that exact authority but reconstructs it through a globally
aligned 0.5 m dual surface, with complete 8 m packages and collision derived
from the same triangles. It is the first watertight player-visible exposure
runtime; it does not add differential erosion or new geology.

Stage 8 applies a compiled erosion-work field to the same folded geology. Its
equal and differential controls vary only sandstone/shale resistance. Runtime
stage 8 displays the differential result; its permanent certificate retains
both controls and proves that the landform disappears when resistance is
equalized.

Stage 9 preserves Stage-8 geometry and adds a younger irregular granite pluton
that cuts the folded host as one persistent 3D `FeatureId`. Surface granite is
the final world query at the reconstructed height, not an elevation rule.

Stage 10 preserves Stage-9 geometry and adds a host-valid quartz deposit body
inside a bounded exterior granite-contact band. Admission depends on host
material and structural permeability. Disabling the mineralizing event produces
zero deposits; the surface is re-queried rather than painted.

Stages 5 through 11 execute their permanent headless certificate once on first
selection and load the same hash-linked authority descriptors. Invalid
authority refuses the selection; there is no fallback causal geology.

Every browser entry has a stable identifier, runtime status, dependency, and
provided capability. `CURRENT`, `CERTIFIED`, `AVAILABLE`, and `FAILED` describe
runtime state rather than decorative progress. Hydrology entries remain visibly
locked by P5b instead of appearing selectable.

## Journal Diagnostics

The Diagnostics page is available in every playable certificate. Each ready row
has a clickable `ON` / `OFF` switch; clicking anywhere on its row toggles the
same state. Up/Down plus Enter or Space remains available from the keyboard.
Ready diagnostics currently include:

- distance ruler and material palette;
- geology identity, formation contacts, and bedding data;
- residency rings;
- performance and mutation/revision HUDs;
- provenance trace details.
- an aim-local, residency-scoped geology x-ray flashlight for Stages 5-10.

Occupancy and D2 remain locked by Stage-0 isolation. Water authority remains
locked by P5b. Collision, package-boundary, and structural-face views are marked
planned and cannot be falsely enabled.

## Stage-0 boundary

The mode admits traversal and a selected read-only terrain stream only. It does not connect
to a server or run the bridge stream, D2, occupancy mutation, edits, support,
water settling, loose bodies, gallery content, vegetation, geography noise,
digging, placing, gripping, or fixture cycling. The palette pickaxe, axe, and
shovel remain non-authoritative calibration props.

The HUD reports isolation counters for D2, occupancy, edits, support, and
mutations. A valid Stage-0 session keeps all five at zero. P5b remains closed.

## Instrumentation

The live HUD and per-frame CSV report:

- FPS, frame time, rolling 300-frame p99, and worst frame
- resident and newly generated cells, generation rate, pending work, and eviction
- full heightfield rebuild count/rate/time and local-update count
- generation request time and worst observed request
- process working-set memory
- movement mode, position, speed, and isolation counters

The session writes:

```text
Docs/provenance_worldgen_playtest_trace.csv
Docs/provenance_worldgen_playtest_summary.txt
```

The trace is replaced when a new playtest begins and flushed on normal exit.
The summary is written on normal window shutdown.

## Performance architecture

The foundation, Stage-5, and Stage-6 runtimes use persistent 8 by 8 m presentation blocks. Residency expansion rebuilds
only blocks whose quads reference newly admitted vertices. It does not rebuild a
single 128 m-wide heightfield.

Stage 7 uses a separate globally aligned half-metre dual reconstruction. Its
8 m packages are all-or-nothing: every block contains all 512 triangles before
publication, neighboring packages share canonical edge vertices, and eviction
removes only complete packages. This is the certified repair for the partial
Stage-6 presentation blocks that appeared as sky tears.

Stage 8 retains the same half-metre, complete-package reconstruction. Surface
height comes from erosion work spent through the actual geological column, and
visible material is re-queried at the final reconstructed surface.

The supplied playable launcher requests a 192 m active radius. A bounded cache
margin prevents boundary churn.
Virgin deterministic cells and presentation blocks wholly outside that margin
are evicted. Cells carrying authoritative edit, occupancy, cavity, or crest state
are retained rather than silently discarded.

The playable lane drains window/input messages and renders continuously instead
of using the low-resolution 16 ms Win32 timer. It also requests swap interval
zero through `WGL_EXT_swap_control`. The HUD reports whether that request was
accepted as `swap=uncapped` or remained under driver control.

The CSV decomposes every frame into CPU frame, simulation, residency,
generation, HF build, HF upload, draw submission, pre-present GPU/driver
completion, present wait, and pacing wait. The Stage 0-11 playable path keeps
swap interval zero and explicitly completes queued OpenGL work before
`SwapBuffers`; controlled isolation proved this prevents the periodic
approximately 53-62 ms present hitch while preserving the complete 192 m world.
The player/render owner is explicitly one priority tier above normal while
bounded terrain workers remain below normal, preventing background derivation or
ordinary OS scheduling from displacing the frame owner during certified travel.
The unfenced behavior remains available only through the presentation-isolation
A/B certificate documented in `Docs/PRESENTATION_BACKEND_ISOLATION.md`.
The legacy display-list backend does not expose a distinct upload phase, so its
driver compile/upload work is included in `hf_build_ms`. GPU timer queries are
not yet available and are truthfully reported as `-1` / `n/a`.

The permanent headless certificate retains its historical timer cadence so its
route and phase comparisons remain stable. Use the playable lane for uncapped
headroom and perceptual traversal testing.

## Calibration lanes

The distance ruler follows world +Y from the fixed Stage-0 origin and extends to
2,000 m. It uses near-origin 1 m ticks, standard bands every 10 m, emphasized
bands every 100 m, and landmark bands every 1,000 m. Standard bands are based on
the requested approximately 0.91×1.83 m scale. The complete ruler is one display
list and has no collision, occupancy, support, body, or interaction state. Each
complete 6 by 3 ft band and every narrow dash is clipped to the active runtime's
canonical 0.5 m terrain-triangle lattice. Its resulting vertices are evaluated
on those same triangle planes, then receive an 8 mm offset along the local
surface normal. The measured coordinate remains the unmodified terrain anchor.
This preserves the full rectangular footprint while preventing a marker from
bridging across HF/D2 triangles. The list is rebuilt when the certificate
changes, so the ruler conforms to relief without becoming coplanar with it.

The terrain palette is summoned beside the player and contains deterministic 4×4 m
walk-over swatches for dirt, disturbed dirt, gravel, clay, and sand. Its first
cut demonstrates base HF-scale form, fixed-function normal/color breakup, and
limited meso presentation. Gravel includes sixteen small batched protrusions;
they are not loose bodies. The entire palette is one display list. Every grid
vertex, gravel protrusion, dropped calibration tool, and strike post is grounded
against the active reconstructed surface. Palette relief follows the surface
normal; gravel uses a local tangent frame. Runtime changes invalidate and rebuild
the palette surface. Every swatch base is tessellated against the same canonical
terrain-triangle lattice as the ruler; material-only relief is layered on that
fitted base rather than replacing its surface shape. Summoning the palette also
resets and surface-fits the pickaxe, axe, shovel, and strike post beside its
current anchor, so none of those probes remain behind at the old Stage-0 origin.

Free-camera sprint begins at 48 m/s. While `Shift` remains held and movement is
continuous, each 50 m traveled adds another 24 m/s tier with no temporary cap.
Releasing `Shift` resets the chain. The HUD reports current speed, tier, chained
distance, and distance to the next tier.

The permanent windowed runtime-independence certificate proves direct Stage-10
cold start and the Stage 5-10 transition matrix. It also certifies toolkit
surface attachment, normal offset, zero penetration, bounded visual clearance,
and calibration-list reconstruction.

The geology x-ray is a global inspection overlay, not a certification stage. It
starts at the live authoritative terrain hit, builds its prism in the hit-local
frame, masks only presentation between the contact and selected depth, and draws
one half-metre sampled terminal scan face. Sandstone, shale,
granite, and contact quartz therefore remain the same named bodies seen by
surface and provenance tools. The overlay has no collision, occupancy, D2,
support, mutation, or material authority, and retires as soon as its host
terrain is no longer resident.

`F9` writes `Docs/provenance_geology_xray_snapshot.ppm` plus a terminal-query
receipt at `Docs/provenance_geology_xray_snapshot.txt`. The permanent toolkit
certificate finds a Stage-10 quartz deposit, scans to its authoritative depth,
and requires both the image and matching deposit identity receipt.

The HUD and CSV report calibration stage, CPU submission time, triangles,
vertices, build time, meso-detail count, and body count. GPU timer queries remain
unavailable, so calibration draw timing is CPU submission—not claimed GPU cost.
