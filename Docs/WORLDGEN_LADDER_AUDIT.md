# Worldgen Ladder Audit

Status: permanent Stage 0-11 regression and performance-attribution gate.

## Scope

The audit runs the clean performance floor and certified Stages 5 through 11
through the same three workloads:

- settled resident;
- deterministic 48 m traversal;
- repeated 96 m residency-boundary stress.

It records frame-time distribution, CPU categories, collision, residency,
generation, mesh work, memory, resident cells, triangles, packages, admissions,
evictions, and rebuild activity. Marginal receipts subtract each stage from its
predecessor for every workload.

Every phase also publishes the player-facing performance receipt: average and
median FPS, 1% and 0.1% lows, mean/median/p95/p99/p99.9/worst frame time, and
hitch counts above 16.667, 33.333, 50, and 100 ms. Traversal and residency
receipts include distance, admitted cells, built/evicted terrain packages, and
normalized elapsed cost per metre, terrain work per new package, and terrain
work per 1,000 admitted cells. The CSV carries workload, instantaneous FPS,
position, and per-frame package work so the summary remains traceable.

Each phase also records separate authority, geometry, material/FeatureId,
collision, and resident-package digests. A reverse settled sweep from Stage 11
back to clean must reproduce the forward answers. The existing runtime
independence certificate remains the cold-start and irregular transition-order
companion.

Performance values are attributed references, not shipping thresholds. A PASS
requires isolation, coverage, and digest invariance; it does not bless an
arbitrary FPS target from the lightweight renderer.

The original `CERT_WORLDGEN_LADDER_AUDIT.cmd` remains deliberately timer-driven
and is classified as a correctness plus synchronous load-latency audit. Its
outer approximately 29 ms cadence must not be interpreted as gameplay FPS.

`CERT_WORLDGEN_LADDER_LIVE_PERF.cmd` runs the same ladder through the continuous
uncapped player loop with swap interval zero. Settled phases sample two real
seconds. Local traversal covers all eight compass bearings over 48 m and mixes
18 m walking, 18 m sprinting, and 12 m collision-free diagnostic flight using
live high-resolution frame time. A live PASS permits no traversal frame over
16.667 ms. This is a local frame-time test; it is not, by itself, proof that an
entire prior residency footprint unloaded. The cardinal streaming companion
must additionally travel at least 192 m outward and back on +X, -X, +Y, and -Y;
at every outer station, no origin live package may remain resident.

Residency stress performs six 96 m crossings over three seconds. These are the
authoritative live FPS, lows, and player-experienced frame distributions.
The measured render path includes the normal playable HUD, stage/menu overlay
calls, tool drawer call, and surface-snapped ruler/palette presentation call;
their current visibility settings determine their actual draw work.

## Cardinal streaming and visual return gate

Every certified playable map must additionally prove the following on each
cardinal plane:

- incremental travel from origin to at least 192 m, not a destination rebuild;
- complete eviction of the original live terrain-package footprint;
- complete current coverage with bounded cells/packages and no empty result;
- procedural geology/material/resource samples at the newly admitted region;
- endpoint images with no sky tears, fallback green, cracks, or handoff seams;
- return travel that re-admits the origin deterministically;
- identical origin geometry, material/FeatureId, collision, and package digests;
- a fixed return camera with no representation pop relative to the cold origin;
- walk, sprint, and free-flight segments, with free flight never receiving body
  collision.

Static distant rebuilds and short loops inside one residency diameter are useful
diagnostics, but cannot satisfy this gate.

## Run

```text
CERT_WORLDGEN_LADDER_AUDIT.cmd
```

Outputs:

```text
Docs/provenance_worldgen_ladder_audit.txt
Docs/provenance_worldgen_ladder_audit_trace.csv
```

Live-runtime outputs:

```text
Docs/provenance_worldgen_ladder_live_perf.txt
Docs/provenance_worldgen_ladder_live_perf_trace.csv
```

Complete world-replacement companion:

```text
CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd
Docs/provenance_worldgen_cardinal_replacement_cert.txt
Docs/provenance_worldgen_cardinal_replacement_trace.csv
```

The cardinal certificate is the authoritative proof that a 64 m live
neighborhood can be completely evicted, replaced 192 m away, and regenerated
exactly on return. See `Docs/WORLDGEN_CARDINAL_REPLACEMENT.md`.

## Geology cutaway

The global Playtest Toolkit now includes an aim-local `Geology x-ray flashlight`
for Stages 5-11. `X` toggles it directly. The authoritative terrain hit and its
normal establish a local inspection frame. A 10-20 ft wide presentation prism
follows the aim point; the wheel changes inspection depth and Shift+wheel changes
width. The prism masks terrain presentation only and publishes one terminal
scan face at the selected depth. `V` cycles
material, FeatureId, and chronology views. Colors and identities come from the
same 3D geology query used by the active stage, including granite and contact
quartz.

The x-ray is diagnostic presentation only: it creates no occupancy, D2,
collision, support, matter bodies, edits, or geology.

## Residency-aware diagnostics

The calibration ruler and palette follow the active 64 m terrain footprint.
The ruler is regenerated from active package keys as the player travels; no
global 2 km display list or traveled-history geometry remains. Press `P` to
destroy any current palette and summon exactly one new, surface-fitted palette
beside the player's current ground contact. If its host package leaves active
residency, it retires completely and does not resurrect automatically.

`CERT_PLAYABLE_RUNTIME_INDEPENDENCE.cmd` certifies palette replacement,
host-residency retirement, ruler movement, deterministic return, and zero ruler
geometry when disabled. The governing invariant is: **Diagnostic presentation
follows residency, not history.**
