# Provenance Esoterica pin

Upstream: https://github.com/BobbyAnguelov/Esoterica  
Pinned commit: `cf36499f59d0434179b03692986af8aec803d9c4` (2026-07-21)  
Local path: `C:\Users\D-Day\ProvenanceEsoterica`  
Branches:
- `provenance/pin-cf36499` — upstream pin + minimal props patches
- `provenance/client-spike` — **local** ProvenanceClient spike (do not push to Bobby `origin`)

**Spike contract:** `SPIKE_CONTRACT.md` (One Simulation, Two Clients).

## Isolation (hard)

| Do | Don't |
|----|--------|
| Edit ProvenanceClient in this tree | Commit Esoterica work into Mygame / Unreal git |
| Treat Mygame as read-only reference | Push Provenance commits to `origin` (Bobby) |
| Keep Python as authority | Invent client-side matter law |

## Toolchain proven on this machine
- Visual Studio Community 2026 18.8.2 (toolset v145 / MSVC 14.51)
- External deps from upstream Releases/Dependencies (~998 MB) in `External/`

## Fork patches on this pin
- `Code/PropertySheets/RenderDoc.props`: remove trailing `\;` on include path
- `Code/PropertySheets/EA.props`: include `eastl_Esoterica.h` by filename + add `EE_EA_DIR` to includes

## Honest phase status (2026-08-08)

- **P0–P2** done: stock build, handshake, far/streamed heightfield walk.
- **P3** partial: standable + wall capsule; not occupancy collision.
- **P3b voxel-form gate — client land:** dig/pick subtract affect sphere from occupancy → D2 matter-face cavity (VisualMaterial interior); heightfield opens where matter is gone; DigScar cups/chips are flash-only. Authoritative grams still from carve digest + column reconcile.
- **P4 Fablescript authority floor** — see pin block below (`--cert-p4`).
- **P5a water ledger / settle floor** — see pin block below (`--cert-water`).

## Dig / tunnel ruling (locked)

**Tunnel = changing matter, not deforming a heightfield.**  
Owned mesh = projection of authoritative matter face for targeting — never authority.  
End state: engine sphere subtract → settle → **D2 reconstructs boundary**.

### HF / occupancy / D2 ownership (locked 2026-08-08)

HF and D2 must share the **same canonical virgin surface law** (`SampleGroundZBase` / geography grade). They do **not** coexist or continuously cross-materialize.

| Phase | Owner |
|-------|--------|
| Virgin terrain | **HF only** |
| Committed dig/pick | Seed **bounded** local occupancy from that virgin law → apply matter edit → build cavity/D2 **once** → **atomically** hand ownership to the completed patch |
| Aim / look / walk | May refine HF contact. Must **not** trigger D2/QEF rebuilds or volumetric expansion |

Reject: continuous HF↔occupancy rematerialization; peel HF before cavity commit; aim-driven lattice growth; HF aperture from latest tip focus alone.

**EditedRegion (remove-only monotonic):** connected excavation keeps a persistent openings union. Later strikes may enlarge/merge; they must never roof a prior mouth because focus moved. Tip/action disk is transient. Stencil aperture = openings union.

### Interim → P3b
`Main.cpp` carves local/synthetic fill at affect radius (seeded from virgin surface law), rebuilds cavity once, hands HF pads to the completed patch, demands `voxel_column` reconcile. DigScar = flash only.

## Launch

`RunProvenanceClient.bat` → prefers newest `Build\x64_Release*\ProvenanceClient.exe` (LNK1104 spill folders included).  
Engine: `voxel_bridge.py` on `127.0.0.1:8765`.

## Geo fixtures (D2 harness)

Default play/dev: **PROVENANCE GEOLOGY RANGE** (representative local flank transect near spawn `(128,128)`, walk `+X` through flat→slope→mound→drain→hill→bedrock→local cliff).  
Alternate: **D2 TORTURE TERRAIN** (prior extreme FBM) via `--geo-fixture=torture` or **F8**.  
Code: `ProvenanceGeography.h` (`Range*` / `Torture*`); virgin load remains HF-only (D2 dormant until dig).

**Geography interaction cert (Horizon-to-Hand transect):** see `GEOGRAPHY_INTERACTION_CERT_HANDOFF.md` — run with `--cert-geo` / `--cert-geography` (forces RANGE).

**Local Surface Intent / presentation-closure floor:** `--cert-lsi` / `--cert-local-surface-intent` → `%TEMP%\provenance_local_surface_intent_cert.txt`. Dig mouth annulus + place crest/open-skin are expected free boundaries; accidental cross-cell opens FAIL. Narrow D2 seam weld = per-column crest world Z (no occ/ER/SupportBelow/place/chip greenwash). See handoff §Local Surface Intent.

```
P3b D2 CORE FLOOR
SHA: 523e796eba8c3f1f67559caadb27e76957645f4e
```

Freeze D2 contracts/topology (Unknown halo refuse, +max seam ownership, partitioned halo stats, order independence). Must not redesign Hermite/QEF ownership, halo semantics, or HF↔D2 handoff unless a cert exposes a defect. Prior `haloMiss=0` PASS revoked — see handoff §5.

```
P3c OCCUPANCY SUPPORT FLOOR
SHA: 5e9e786d08d2352c4f4efc1c84f9d0ab977d12f9
```

SupportBelow from queryZ through occupancy (not D2 tris, not column crest). Missing authority → refuse/defer. Chips consume this floor — do not invent chip terrain physics.

```
P3d DETACHED MATTER SUPPORT FLOOR
SHA: b1fec59aa637ad69884f92e36419946d0c256f07
```

PHYS chips: gravity → `SupportBelow(x,y,currentZ)` → contact + normal → settle/slide/sleep. Modes `OFF` / `VISUAL` / `PHYS`. Lifecycle `ACTIVE → SETTLED → AGGREGATED (scaffold) / EXPLICIT BODY`. OFF is zero chip cost; mode switch must not remesh D2/HF. **STOP before placement/re-fill.**

```
P3d.1 CHIP QUIESCENCE
SHA: 0a8867affc536b003c3408d9af43bbd8bc4066cd
```

Sleep on quiescence hysteresis — **not** flat `nz ≥ 0.88` gate. Support normal influences slide only. Wake thresholds above sleep; wake on `supportRev` / EditedRegion change under chip. Cert: cavity `CountActiveChips==0` for N frames + steep-static regression. **STOP before placement/re-fill.**

```
P3d.2 CHIP VIBRATE / CYCLE HALT
SHA: 6afec516291bf908c9f88dfae277fada9196bda6
```

Keep P3d.1 hysteresis. Add limit-cycle / orbit detection (position ring + vel sign flips) and tangential KE damp when supported near rest without slide progress → force SETTLED at SupportBelow rest. Wake unchanged (speed / support loss / impulse / supportRev). Cert: cavity quiescence + vibrate/orbit + steep-kinetic regressions.

```
P3e PLACE / RE-FILL
SHA: f302856aa1599531b5e3a2d4ba66d7f7ab9b231b
```

Reverse matter transfer: attached → remove → carried → place → attached again. `PlaceOccupancyFill` debits carried grams into occupancy (bottom-up, supported), bumps EditedRegion/`dirtyRev`, rebuilds D2 from occupancy; `SupportBelow` sees new matter. Never D2-tri authority; never HF grade resurrection; never mint mass/volume beyond hand debit. Cavity re-fill seats on matter floor upward — not virgin HF skin restore.

**Scalability (place):** Placed matter does not automatically require an expensive permanent object. Dump dirt → occupancy/aggregate → cheap settled representation. Meaningful quartz/block may stay explicit MatterBody. `representation changes; matter does not`.

Cert §9: flat mound, slope supported/no float, cavity floor upward, lip no roof, remove→place→remove reconcile.

```
LSI CAPTURE / CERT (read-only)
SHA: 78f9ea0e1e20fc223b944348cbb5910faa6dd079
```

Instrumentation floor only (pre-classification). Superseded by presentation-closure floor below.

```
LSI PRESENTATION-CLOSURE FLOOR
SHA: b3dca76b449d33f796a755c6f899db1096cea612
```

`--cert-lsi` green: dig mouth annulus + place crest/open-skin classified as expected LSI free boundaries; cross-cell accidental opens = 0 via canonical per-column-crest seam verts; coverage + outside identity green. Freeze: do not weaken boundary classification or reopen broad D2 topology.

```
ASYNC + DETERMINISM FLOOR
SHA: faa8b9fcefddf1037f467cd7458a7eb2b32908dd
```

`--cert-async` / `--cert-async-determinism` → `%TEMP%\provenance_async_determinism_cert.txt`. Equal matter + equal LSI closure (unexplainedOpen=0, occ/D2/outside hashes) under 3×3 `voxel_column` forward/reverse/checkerboard pacing and D2 rebuild order. `keepLocalCarve` preserved.

```
P4 FABLESCRIPT AUTHORITY FLOOR
SHA: 5d669758f1c853a88a543c341f9377a7c689b417
```

**Law:** Client owns intent, prediction, cache and presentation. Fablescript owns matter, mutation, inventory transfer, fracture/separation identity and world revision.

`--cert-p4` / `--cert-p4-authority` → `%TEMP%\provenance_p4_authority_cert.txt`. Headless probe: `cert_p4_headless.py`. Hard kills: `nothing_to_dig` must not synthesize a scoop; visualCap≠auth must not silently remap (flag/telemetry only). Dig/place held credit/debit and terrain rev follow the authoritative receipt; StrikePick prediction rolls back on refuse.

```
P4.1 PREDICTION ROLLBACK FLOOR
SHA: 67790cf7b08acd845644e34b72d8b600a4d7c34d
```

Adversarial: capture pre-intent → optimistic dig/place prediction → REFUSE / `nothing_to_dig` → exact restore of occupancy bytes, EditedRegion membership/bounds/revision, local D2 publication, HF ownership/openings, SupportBelow, held grams, and no surviving predicted chip/body. Delay permutations (0/4/18 frames) must still reverse.

```
P4.2 AUTHORITATIVE BODY IDENTITY FLOOR
SHA: 248cbcd2cd1eb8937d8449b3a0e2997eaab7d5cf
```

Wire contract: `Docs/P4_BODY_IDENTITY_WIRE.md`. Fablescript issues `body_id` / `aggregate_id` (+ form/fracture seeds, parent, provenance, revision). Clients present the receipt id (e.g. `body 18472`); never invent from local H2H.

```
P4 FULLY CLOSED
SHA: 248cbcd2cd1eb8937d8449b3a0e2997eaab7d5cf
```

P4 authority holes closed: receipt-driven dig/place (no invent), prediction rollback floor (P4.1), authoritative body/aggregate identity on the wire (P4.2). No known exceptions on `--cert-p4`.

```
P4.3 TERRAIN RESIDENCY / INVALIDATION FLOOR
SHA: 3a53fdde3fda0f8b720d928b3f4d0f13ee48cfb0
```

**Law:** World activity does not imply terrain activity. Terrain presentation wakes only when terrain-relevant state changes (`affect` / classifier). Contract: `Docs/P4_3_TERRAIN_RESIDENCY.md`.

`--cert-residency` / `--cert-terrain-residency` → `%TEMP%\provenance_residency_invalidation_cert.txt`. Idle unrelated burst: D2/HF/occ/refetch/ER = 0. One mutation: scoped D2 dirty + bounded work; remainder cached. Continuity-before-cull: invalidation must not open voids.

```
P4.4 UPSTREAM BODY ID WIRE
SHA: 784b77a4a8e7ac6164f76b60f3a5ea2218d81a23
FABLESCRIPT_SHA: 5a69afb3bc65690041f7c2fab69182df8db3bbda
```

**Law:** Same receipt keys as P4.2, issued by committed Fablescript `terrain_mutate.py` (not a local bridge patch). Esoterica consumes `body_id` / `aggregate_id` from production authority.

Upstream land: `plaintxt-decoded` `client-terrain-residency` @ `5a69afb` — carve success → `body_id`; place success → `aggregate_id`; refuse → no identity. Contract: `Docs/P4_BODY_IDENTITY_WIRE.md`.

Live proof (clean committed bridge, no patch apply): `--cert-p4` PASS 36/0; headless PASS (refuse no id, place aggregate); `--cert-async` PASS 9/0; `--cert-lsi` PASS 16/0. Save/reconnect identity fetch: SKIP (path absent).

```
P4.5 GAMEPLAY-SCALE STRESS FLOOR
SHA: ceddd13697c7936852f2398cd0f12be6057daac6
```

**Law:** Increasing historical world complexity must not proportionally increase recurring frame cost. Contract: `Docs/P4_5_GAMEPLAY_STRESS.md`.

`--cert-stress` / `--cert-gameplay-stress` → `%TEMP%\provenance_gameplay_stress_cert.txt`. Seeds 200 historical ERs + 500 sleeping chips + 8 active; walk + unrelated ticks under that history must keep D2/HF/occ/refetch/ER/wakes = 0; burst 50 unrelated → no wake; 1 scoped explosion bounded; 10 simultaneous scoped → dirty ∪ coalesced (one wake).

```
P5a WATER LEDGER / SETTLE FLOOR (REPAIR — foundation-ready)
SHA: 8e09e77249fb5ff7d3afb897d6c5bb320445a60e
```

**Law:** Water should wake from causality, not from time passing. Conserved occupancy-aware body; settle → dormant; unrelated receipts → zero water work. Contract: `Docs/P5A_WATER_LEDGER.md`.

**P5a REPAIR (closes implementation-floor defects on tip `9da3534` / pin `830c046`):**
1. Deterministic hydraulic head/level settle (not greedy fill) — equal-floor share `34/33/33`
2. No water in solid occupancy under mutation — displace to neighbors or `spillOutOfScopeUnits`
3. Derived equal-level ≠ equal-units (and equal-units ≠ equal-level) from basin geometry — not assigned surfaces
4. Honest `capacity_units` + volume/mass conversion contract (not fake “grams”)

`--cert-water` / `--cert-p5a` / `--cert-water-ledger` → `%TEMP%\provenance_p5a_water_ledger_cert.txt`  
Committed artifact: `Docs/provenance_p5a_water_ledger_cert.txt` — **PASS 15/0** (`exit_code=0`).

Cert summary: `level_equal_floor_share`, `height_neq_grams` (derived sA=sB=0.34, uA=100≠uB=34), `amount_eq_height_neq`, `no_water_in_solid_mutation`, `units_volume_mass_contract`, `settle_order_determinism` digest `96f06506b2fb6329`, plus prior causality/dormant rows.

**Authority note:** Esoterica-local `WaterLedger.h` for this floor. Fablescript `engine/water.py` remains the intended long-term ledger authority; P5a may stay client-local until that wire is consumed here.

**P5a status:** foundation-ready for later gates (settle/units/solid/determinism green).

```
P5b.1 TERRAIN → WATER RESPONSE (ONE-WAY) — FULLY DONE
SHA: 5f4c075d7ef0b6af192a3afe7e088cd493e842aa
LAND: 4e6db8433fb136ab2068bab114a04d4ab265e935
```

**Law:** A terrain mutation does not directly manipulate water. It changes
world truth, issues a bounded causal receipt, and the hydraulic domain
responds. Authoritative terrain mutation wakes a local hydraulic neighborhood
only. Water mass is conserved independently of terrain matter. Occupancy,
surface, and topology may change; water is never deleted. Publish terrain +
water as one coherent revision pair. Disabled P5b.1 == exact 16F.4 field
`43068558cd0b4a8e`.

```
16F.4   dynamic hydraulic topology       CERTIFIED
P5b.1   terrain → water coupling          CERTIFIED / FROZEN @ e64a4df3
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED
P5b.2C  pore occupancy / topology         CLOSED
P5b.3   reverse matter/erosion coupling   CLOSED
```

`--cert-p5b1` / `--cert-p5b1-terrain-water` → `Docs/provenance_p5b1_terrain_water_cert.txt`  
Play: `PLAY_P5B1_TERRAIN_WATER.cmd` / `--play-p5b1-terrain-water`  
Handoff: `P5B1_TERRAIN_WATER_HANDOFF.md`

Player path (pick/shovel legal receipt, not only synthetic fixtures) is inside
`--cert-p5b1-terrain-water`: cut retaining edge → water grows; fill into
occupied water with no admissible conserved resolution → refuse.

```
P5b.2A WATER → TERRAIN MATERIAL STATE — REVERSE STATE ONLY
SHA: 570c7be27260289eb26a3608d7aed5416c929449
PARENT: e64a4df3 / 5f4c075d / 4e6db843
```

**Law:** Water may change how terrain matter behaves, but not how much
terrain matter exists. Contact revision → neighborhood → wetting query →
`FTerrainMaterialStateDelta` receipt. No infiltration, no terrain mass
movement. Disabled P5b.1 == 16F.4 `43068558cd0b4a8e`. Disabled P5b.2A ==
exact P5b.1 field `b1340afeef311fd8` / state `d3bd4455d64ca895`.
Enabled state digest `176ffe1c3845f721` (budget 1 == N == unbounded).

`--cert-p5b2a` / `--cert-p5b2a-terrain-state` → `Docs/provenance_p5b2a_terrain_state_cert.txt`  
Play: `PLAY_P5B2A_TERRAIN_STATE.cmd` / `--play-p5b2a-terrain-state`  
Handoff: `P5B2A_TERRAIN_STATE_HANDOFF.md`

**P5b.2B CERTIFIED** — bounded pore storage. Handoff:
`P5B2B_TERRAIN_PORE_HANDOFF.md`. Disabled == exact P5b.2A state
`176ffe1c3845f721`. Enabled pore digest `224e662e5584a1ea`. Conserved
body+pore+container `466363002`. Play: `PLAY_P5B2B_TERRAIN_PORE.cmd` /
`--play-p5b2b-terrain-pore`. Cert: `CERT_P5B2B_TERRAIN_PORE.cmd`.

**P5b.2C CLOSED** — pore storage affecting water occupancy / topology.  
**P5b.3 CLOSED** — water→terrain mechanical / matter movement.  
Also closed: water erosion, sediment transport, bank collapse, rainfall,
groundwater, active 16B erosion, 16C remobilization, ecology.

```
TRAVERSAL + STREAMING SOAK (standing matrix)
```

Cardinal replacement ≠ soak. **Law:** Travel distance may grow; active
residency, memory, pending work, and wake backlog must remain bounded.
Contract: `TRAVERSAL_STREAMING_SOAK_HANDOFF.md`.

- Test S: `--cert-semantic-distance` (teleport+settle across/beyond 4.096 km
  Stage-15 domain; independent of Test B). `CERT_SEMANTIC_DISTANCE.cmd`.
- Test A: `--cert-worldgen-cardinal-replacement-p5b2b` (EVERY CUT, latest)
- Test B: `--cert-streaming-soak-p5b2b` (90 s first landing; 300 / 900 s)
- 2A control remains `--cert-streaming-soak-p5b2a`: residency PASS, frame
  gate FAIL (20 / 20079 frames >16.667, max 60.687 ms). Do not relax 16.667.
- Test S `--cert-semantic-distance` PASS (8/8 stations) at `6b14d8fd`.
  Origin through ±2048 m boundary and out to 8 km / 12 km NE. Same
  generator `provenance_causal_world` / `36d381f2ab7bb953`. Resident-mesh
  relief 13–56 m (not flat). 16A–16D 441/441. Visual sky 0.
  **Caution:** wrapping the 4096 m Stage-15 tile is a valid continuity
  fix only if we explicitly accept **repeated macro geography** as the
  current production shortcut. It solves “flat fallback.” It does **not**
  yet solve “indefinitely novel macro geography.”
- 2B persistent-water soak: `--soak-water-backend=persistent` (default).
  90 s frame **PASS** (0 / 79961, max 10.091 ms). First live occupied
  water did not stall. 300 s / 900 s frame PASS (max 11.942 / 11.505).
  Water GPU 2 batches / 170 KB, travel growth 0. Process private still
  `FAIL_scales_with_distance` (1.32 → 2.62 → 4.19 GB); logical residency
  114 MB flat. 16.667 not relaxed. P5b.2C / P5b.3 CLOSED.
- 250 m ledger + per-overrun receipts + stop/drain/optional return
  are required on every soak receipt.
- P5b.2B gameplay frozen at `8bb75265`. P5b.2C / P5b.3 / rainfall /
  erosion remain CLOSED.

```
P5 SIDE-GATE — MATERIAL-TRUE PICK FRACTURE + CLOSED LOCAL SURFACE
SHA: fa73dd240214ce48659937fa5df757d05d10c15f
```

**Law:** Pick is not sphere/cup/cell deform. Contact frame `(T,B,N)` at hit owns tip/penetration/pry/fracture envelope; material structure (aggregate / compact angular / foliation) determines releasable connected volume; occupancy loses exactly that material; HF/D2 reconstruct the remaining local boundary (canonical mouth). Four distinct radii: contact/query ≠ fracture ≠ D2 recon halo ≠ HF refine. Outside physical changed + min recon halo → pre-strike surface bit-identical.

**Hard presentation gates (user ruling — corrects prior “sky through mouth OK” confusion):**
1. Any color **inside or around** a strike hole matching clear/sky (`glClear` ≈ RGB(114,158,224) / sky classifiers) = **`PRESENTATION_COVERAGE_FAIL`**. **No exception** for “legitimate cavity mouth showing sky.”
2. Any deformation beyond fracture volume + minimum recon halo = **`HF_CHANGED_OUTSIDE_RECON_HALO`** / fold explosion FAIL.

Contract: `Docs/P5_PICK_FRACTURE.md`.  
`--cert-pick-fracture` / `--cert-p5-pick` → `%TEMP%\provenance_pick_fracture_cert.txt`  
Committed artifact: `Docs/provenance_pick_fracture_cert.txt` — **PASS 9/0**.

Cert summary: `contact_frame_orthonormal`, `radius_separation`, `material_morphology`, `rotation_equivalence` (gravel/granite/mica_schist 0°…90°), `two_strike_gravel`, `outside_recon_halo_identity`, `no_world_up_fracture_law`, `presentation_coverage_closed`, `outside_strike_volume_deform`.

**P5a FREEZE held** — `--cert-p5a` re-run unchanged **PASS 15/0**.  
**P5b.1 FULLY DONE / FROZEN** (one-way terrain→water, including pick/shovel
legal receipt) @ `e64a4df3`. **P5b.2A FROZEN** (state only).
**P5b.2B CERTIFIED** (bounded pore). **P5b.2C / P5b.3 CLOSED.**
Erosion / rainfall / deep groundwater CLOSED.

**Continued human + agent testing (cold resume):** `PICK_FRACTURE_HANDOFF.md`  
Preferred binary: `Build\x64_Release_pickcov\ProvenanceClient.exe` (do not rely on `RunProvenanceClient.bat` newest-timestamp alone). Live: LMB pick; sky/clear in|around hole = FAIL; deform past fracture+min recon halo = FAIL; miss/punch/surround explosion = capture, not pass.

### Standing scalability rule

> **Material vocabulary is cheap; instantiated representation is what costs.**
> Buried/irrelevant stone, ore, mineral occurrences stay compact identity/structure data. Geometry, collision, physics, expensive optics wake only on exposure, detachment, proximity, or gameplay meaning.

Apply to chips: not every fragment = permanent active rigid body forever.  
Apply to place: dump dirt → occupancy aggregate; not a permanent rigid body per scoop.

## Material / appearance docs for review (keeper)

Review-only refs (not geo-cert / dig-cert authority; do **not** overwrite when merging):

- https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-distribution-v1/fablescript/docs
- https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-set-appearance/fablescript/docs

**Heightfield object palette = keeper** — its own presentation/worldbuilding lane. Separate from `--cert-geo` / occupancy / D2. Do not replace it when folding material-distribution or appearance docs from other branches. Exact palette filenames TBD when those trees are locally fetchable (see handoff §Related docs).
