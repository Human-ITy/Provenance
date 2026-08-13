# Stage 16A — Dry Watershed Authority Handoff

Base: `da1d3d0101c473027078083ce5fdd0aa7c3fa8ce` (certified Stage 15 bare-earth geography)

## Result

Stage 16A derives one deterministic, region-wide drainage graph from the frozen Stage 15 surface. It adds no water occupancy, water rendering, fluid solve, active erosion, sediment transport, terrain displacement, or P5b behavior.

```text
Stage 15 bare-earth surface
        ↓ exact samples
priority-flood depression accounting
        ↓
D8 receivers + accumulation
        ↓
watersheds + outlets + basins + spills
        ↓
channel network + channel order + confluences
        ↓
read-only live diagnostics over unchanged Stage 15 terrain
```

The topology is compiled over the complete 4.096 km region. Player packages and tiled consumers read that single graph; they do not recompute drainage independently.

## Frozen authority values

- Grid: 256 × 256 cells at 16 m (65,536 cells).
- Topology digest: `d05dfeb3d06362ab`.
- Stage 15 semantic digest: `4f61f8d7b9de8760`.
- Watersheds/outlets: 1,020 / 1,020.
- Detected closed depressions/spills: 2,408 / 2,408.
- Channel cells: 3,308.
- True channel confluences: 39.
- Maximum upstream area: 2.324480 km².
- Invalid receivers: 0.
- Non-basin uphill edges: 0.
- Channel crossings of watershed divides: 0.

The 2,408 depression count deliberately includes small faceting-scale closed depressions at this first 16 m dry-authority resolution. Every one has an explicit spill. A later morphology calibration may classify significant basins separately, but must not silently delete or disguise these receipts.

## Player runtime

Run [PLAY_STAGE16_DRY_HYDROLOGY.cmd](PLAY_STAGE16_DRY_HYDROLOGY.cmd), or launch the normal worldgen playtest and choose `Stage 16A — Dry Watershed Authority` from the in-game stage journal.

The runtime starts with the menu closed. Stage 16A is the latest certified stable runtime. The diagnostic overlay uses cream flow arrows, orange channel paths, and magenta spill markers. It follows the live terrain surface and is presentation-only; no blue water is drawn.

HUD diagnostics include watershed ID, basin ID, spill elevation, receiver direction, local slope, upstream area, channel order, and explicit `sediment=CLOSED` state.

## Permanent receipts

- `Docs/provenance_stage16_dry_hydrology_cert.txt`
- `Docs/provenance_stage16_dry_hydrology_visual_cert.txt`
- `Docs/provenance_stage16_dry_hydrology_player_view.ppm`
- `Docs/provenance_stage16_cardinal_replacement_cert.txt`
- `Docs/provenance_stage16_cardinal_replacement_trace.csv`
- `Docs/provenance_playable_runtime_independence_cert.txt`
- `Docs/provenance_worldgen_boundary_continuity_cert.txt`

Run the complete Stage 16A gate with [CERT_STAGE16_DRY_HYDROLOGY.cmd](CERT_STAGE16_DRY_HYDROLOGY.cmd).

## Passed gates

- Stage 15 remains an exact negative control; its terrain is not rewritten.
- Receivers are adjacent, acyclic, and downhill in the depression-resolved head field.
- Raw-surface uphill routing exists only inside explicitly identified closed basins.
- Every closed basin publishes a deterministic spill elevation.
- Channels derive from upstream accumulation and never cross watershed identity.
- Cold/reload and stage transition order preserve topology and terrain digests.
- Stage 16A has zero boundary sky/fallback in all four cardinal views.
- 408 m cardinal replacement fully evicts origin packages and returns exactly.
- Walk, sprint, and free-flight segments in all directions have zero movement frames over 16.667 ms; worst measured movement frame was 13.706 ms.
- Stage 14 single-pick, Stage 15 geography, L1, and L6 remain controls.

## Closed scope

Stage 16B and later remain closed here. Do not add channel incision, erosion, sediment movement, water bodies, water rendering, fluids, wetlands, or P5b under this certificate. The next cut may consume this graph for compiled fluvial history only after Stage 16A is preserved as an exact control.
