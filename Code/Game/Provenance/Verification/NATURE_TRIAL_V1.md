# Native nature asset trial — 2026-09-07

## Installed scope

Eight render-only entities named `NatureTrial_*` were added to
`Data/Provenance/ProvenanceSandbox.map`:

- One ripe red-berry bush on the front-left outer grass slope.
- One ripe blue-berry bush on the right outer grass slope.
- Chestnut, bell, ivory and ochre mushroom groups around different soil margins.
- A cyan pair and a violet lantern mushroom toward the rear margins.

There are 17 mushroom meshes and two bushes containing 54 berry meshes. All
models retain meter scale; ordinary mushroom heights are about 4–16 cm, not
scaled to match the tall grass. The map uses one entity per bush/group, not a
dense scatter. Small mushrooms can still be partially concealed by grass.

Native resources are isolated under `Data/Provenance/NatureTrial/v001`.
No goblin, floor, studio camera or optional glow light was imported. No new
runtime C++ behavior, harvesting, health/stamina effect, inventory, regrowth,
collision or conserved material authority was added. These are decorative
static meshes; the player and tools can pass through them. The original
granite/soil/grass systems remain authoritative for the playable lab.

## Conversion and appearance

The artist sources in `output/models` remain unchanged. Editable .blend files,
original GLBs, build scripts, texture sources, references and berry sockets are
retained. Separate ripe/harvested/pickup exports remain available for a future
interaction implementation; only ripe bushes are placed in this visual trial.

`Scripts/PrepareNatureTrial.py` creates transform-baked static GLB derivatives
under `output/integration/nature_trial_v001`, without saving any source scene.
Ordinary mushroom display-group translations are removed. Baking node
transforms avoids the current importer discrepancy between vertex-axis
conversion and node transforms, especially important for fruit socket offsets.
No mesh decimation or shape redesign was performed. Exported totals match
the supplied source-set counts: 228,836 triangles.

`Scripts/PackageNatureTrial.cjs` extracts embedded PNG bytes unchanged, derives
constant numeric PBR texels from glTF factors, and emits native texture,
material and static-mesh descriptors. It uses existing `ComplexSurfacePBR`,
with named submesh/material mappings, normal maps where supplied and the
double-sided material flag. No new shader or engine importer was added.
Descriptors were installed through reviewed patches. `--install-binaries`
installs only binary derivatives and refuses to replace differing files.

Emission maps/factors are mapped to the existing renderer. Exposed strength is
limited to 1 in this trial: cyan filaments authored at 1.65 and warm gill edges
at 1.8 are therefore reduced. No point lights or environmental lighting changes
were added; emissive surface color does not imply nearby-ground illumination,
bloom or automatic day/night behavior.

Blender reported that its user extension cache was not writable in the sandbox;
all eight asset exports nevertheless completed. The original source hashes
were checked after export and still match.

## Placement and preservation

`NatureTrialPlacementTest.cpp` samples the current initial soil and granite
with the lab's default specimen, searches near planned locations and favors
existing sparse cover for mushrooms. It does not mutate grass, soil or rock.
Root centers and surrounding samples must be outside exposed granite. The
mushroom groups share a planting plane at the lowest sampled root support,
slightly burying some stems on slopes rather than suspending downhill roots.
Bushes use soil height at the trunk. Placement offsets match the playable
adapter's (-1.4f, 35, .12f) world origin. Recorded in
`Fixtures/NatureTrialPlacements.json`.

These are sampled root checks, not exact mesh/terrain collision certification.
Several sites retain full initial grass cover. A live close-up pass is still
needed to judge visibility, cap clearance and root seating. If the user digs
away supporting soil, static decorative entities will not settle or uproot;
dynamic support belongs to future integration, not this art trial.

The three original map entities and their settings were verified text-identical
after removing only the eight added entity blocks. The pre-trial map is kept
at `Fixtures/ProvenanceSandbox_before_nature_trial.map`. To undo this trial,
remove only entities named `NatureTrial_*`; do not overwrite later unrelated
map edits with the snapshot. Resource files can remain for future use.

## Checks completed

- Eight derivative models exported, with source GLB SHA-256 checks unchanged.
- All 108 native textures/materials/meshes compiled without warnings.
- All 108 compiled resource files verified present.
- Sandbox map compiled successfully with the eight new static entities.
- Package totals: 30 materials, 168 submeshes, 228,836 triangles; 186 source-side
  files totaling 20,977,827 bytes. This is a modest-count, full-detail trial;
  no LOD or broad scattering/performance certification is claimed.
- Game Runtime DLL SHA-256 is unchanged from movement fix 4:
  `33B42ECF2D61C7E6BFBE78198D5EB855885F789DCA1565803320E62D56B29021`.
  No runtime rebuild or movement-policy change was needed or performed.

Maintained resource list/count evidence: `Fixtures/NatureTrialResources.json`.
Runner: `Scripts/CompileNatureTrial.ps1`; the engine's successful compile exit
code is 1, while 2 denotes warnings. This script explicitly normalizes those
semantics and refuses warnings. Raw results/logs are in
`Build/Verification/MovementBaseline/nature-resource-results.json` and
`nature-resources.log`. Placement runner: `Scripts/ProbeNatureTrialPlacement.cmd`.

## Live acceptance still needed

Reopen the sandbox map and Play Map. Inspect bushes from both sides, mushroom
caps/undersides, base seating and grass occlusion. Compare frame cost with the
pre-trial scene before increasing density. This turn verified native resource
and map compilation, not the final image in the running editor.

The two user-reported granite movement catches remain a separate open item;
adding visual assets does not resolve them. No process was stopped, no source
or asset deleted, and no Git staging, commit or push occurred in this turn.
