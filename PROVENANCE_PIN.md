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
P5b.2C  pore occupancy / topology         CERTIFIED / FROZEN
3A–3B.3B  runtime deposition chain   worldgen shaping primitive (ready)
P5b.3A  hydraulic detachment              CERTIFIED / FROZEN
P5b.3B  hydraulic loose-matter transport  CERTIFIED / FROZEN @ 67d5f524
P5b.3B.2 loose-matter settling            CERTIFIED / FROZEN @ eeabfb8c
P5b.3B.3A depositional aggregate          CERTIFIED / FROZEN @ 160a0842
P5b.3B.3B compaction / terrain surface    CERTIFIED / FROZEN @ 2f722735
P5b.3C  structural collapse               CLOSED
16C     compiled sediment routing         CERTIFIED / FROZEN
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
MW4     drainage / valleys                CERTIFIED / FROZEN @ cc7fe436
MW5     depositional landscape            CERTIFIED / FROZEN @ ba6faf61
MW6     hydroclimate                      CERTIFIED / FROZEN @ e8704845
MW7     soils / regolith                  CERTIFIED / FROZEN @ 307e92fc
MW8     biomes                            CERTIFIED / FROZEN @ e8155fa3
MV1     32 km terrain derivation          CERTIFIED @ 77aa7767 (geometry/residency/fidelity/identity)
MV1.C   real 32 km raster visibility      CERTIFIED @ 331f94ff (two-pass depth split; VBO ownership)
MV1.G   full-raster GPU presentation      CERTIFIED @ 0d39ffcd (revalidated after MV1.C; ~0.47 ms, no stalls)
PX1     present-pacing attribution        CERTIFIED @ dae52665 (owner = OS-scheduling inter-frame gap; engine production always < 16.667 ms)
PX2     two-contract present gate         CERTIFIED @ 3a9a9e74 (engine_cpu 0-over + presented cadence owner-classified; wall-clock kept as telemetry)
MV1.D   distance / depth readability      CERTIFIED @ fa9c048f (continuous aerial perspective; far pass only; --mv1d-off == frozen MV1.C)
PX3     frozen-cut perf revalidation      CERTIFIED @ 4cd5af4b (worldgen ladder GREEN under PX2 gameplay gate; wall-clock merely superseded, no historical regression; 240 m/s kept as a named STRESS CEILING)
MV2     extended 100+ km horizon          DESIGN LOCKED (MV2_HORIZON_ARCHITECTURE_DESIGN.md); NEXT cut = MV2.A regional macro authority (Python), renderer MV2.B after
MV2.A   regional macro authority          CERTIFIED @ d23f3810; MV2.A2 @ 7c344ca1 extends to ±2 ring = 25 pages (full 128 km radial authority; continuity across ±96 km seams; cardinal ±2 reaches 160 km; same field, more pages)
MV2.B   horizon renderer                  CERTIFIED @ 40272156 (real 128 km raster; three-pass depth split; seam 5.3 m; non-repeating; bounded 25-page ring; 0 MV2-domain holes; recalibrated 128 km aerial; off==frozen; MV2.A2 fixture: cardinal N/E/S/W 100-128 km coverage)
MV2 full 128 km radial                    CERTIFIED (MV2.A2 authority + MV2.B cardinal proof) @ 7c344ca1
MV2.C   distant-terrain presence          CERTIFIED @ 48ccf496 (presentation only; far ranges read as terrain ~7x more separated from sky; gentler aerial + land-cover palette + hillshade; authority/relief/step UNTOUCHED; off==frozen MV2.B)
MV1     live-terrain coverage             FIXED+CERTIFIED @ 779611bc (band-boundary LOD-ring underlap -> AABB desired-set; rotation-invariant resident set; 0 below-terrain holes; skirts no longer exposed; frozen certs + Test B green)
WORLD   seed-deterministic + UNBOUNDED     DOCTRINE (128 km = player horizon, NOT world extent; pages=cache units; new seed=new world; 4 coupled causal grammars landform/surface/water/living)
MV3     macro morphology grammar          DESIGN LOCKED (MV3_MACRO_MORPHOLOGY_DESIGN.md)
MV3.A   morphology control fields + ops    CERTIFIED @ 4f804588 (independent control fields; style weights belt/plateau/volcanic/cratonic; maturity/sharpness affecting relationships; plateau/escarpment; volcanic cones; gen_version=2; morphology-diversity + seed-diversity certs green; renderer green). NEXT = MV3.B drainage/canyons/volcanics/buttes
MW9     flora / fauna                     CLOSED
```

**MV1.C CERTIFIED @ `331f94ff`** — real 32 km raster visibility. A projection
audit found the render far plane was 600 m, so MV1's regional/horizon bands were
built and submitted but **clipped before framebuffer contribution**. MV1.C adds a
**two-pass depth split** (far pass near≈128 m / far≈33 km → depth-only clear →
frozen 0.03–600 m near pass painted over the top) so the certified near-depth path
is unchanged. Proof is far-pass depth-buffer pixel binning by real distance:
`agg_regional_px=328769`, `agg_horizon_px=7331` (both were 0/clipped before).
Far tiles moved from display lists to **persistent pooled VBOs** (explicit GPU
byte accounting ~4.7 MB, bounded, no display-list finalization lifecycle); build
cap + behind-camera cull kept. **MV1.G revalidated** after MV1.C — still ~0.47 ms
async, no stalls, with the full 32 km now rasterized. Test A correctness PASS 4/4
(exact return digests/geometry/material/collision/image). Handoff:
`MV1C_RASTER_VISIBILITY_HANDOFF.md`. Cert: `CERT_MV1C_RASTER_VISIBILITY.cmd`.

**PX1 CERTIFIED @ `dae52665`** — baseline present-pacing attribution
(instrumentation only; no production behavior change; 16.667 ms standard not
weakened). Per-frame QPC decomposition (gap / engine_cpu / gpu_finish / swap /
engine_work / presented) over an MV1-**off** MW8 baseline. Findings: **frame
production (engine_cpu) is always ~0.5–0.7 ms (max ~7 ms), 0 over 16.667 in every
config including vsync** — the engine never takes 20–80 ms to produce a frame.
The intermittent 20–82 ms spikes are **100 % inter-frame `gap`** (OS thread
scheduling / message pump between SwapBuffers return and the next tick; the
worldgen loop is a tight `PeekMessage`+`TickFrame` busy loop with no Sleep), with
engine_cpu / gpu_finish / swap all tiny on every spike. vsync pacing (when on)
lands in glFinish or SwapBuffers and pins frames to a steady 16.67 ms 60 Hz
cadence (smooth `presented`), which a naive wall-clock gate would misread as an
overrun; the pre-present glFinish is protective. **Proposed standing gate:**
(1) engine_cpu 0-over-16.667 as the engine production contract (always green,
uncontaminated); (2) presented_frame cadence with an environmental allowance for
rare gap-dominated OS-scheduling outliers (a sustained/frequent presented miss is
still a real FAIL); never gate on wall-clock-including-glFinish under vsync.
Test A/B failures are the same present-pacing class. Optional future hardening
(separate cut): clean frame pacing / thread priority to reduce OS-scheduling gaps.
Handoff: `PX1_PRESENT_PACING_HANDOFF.md`. Cert: `CERT_PX1_PRESENT_PACING.cmd`.

**PX2 CERTIFIED @ `3a9a9e74`** — two-contract present-pacing gate
(measurement/classification only; no production behavior change; 16.667 ms
standard not weakened). Replaces the ambiguous wall-clock movement gate with:
**(1) engine production gate** — `engine_cpu_ms` (pure CPU frame production,
already contains all stage work) must have **0 travel frames > 16.667 ms**
(hard); **(2) player presentation cadence** — `presented_frame_ms` (swap-to-swap)
with every miss recorded and **owner-classified**: `gpu_finish`/`swap` large →
stage-owned (hard FAIL); gap-dominated or unattributed-boundary → OS-gap
(reported, not charged); plus a **non-regression rule** (OS-gap misses may not
materially exceed the PX1 baseline — >10/route or worst >250 ms fails). Raw
wall-clock frames-over kept in telemetry only. Validated: PX1 classifier tags a
known 44 ms gap spike as `os_gap` with engine gate PASS; **Test A cardinal PASS
3/3** (engine 0-over, 0 stage-owned, 4 os-gap capture-boundary frames reported);
**Test B soak PASS** reliably where the old wall-clock gate was ~50 % flaky.
MW1–MW8 are **not** reopened — only the weak wall-clock movement clause is
superseded; a retrospective sweep under the two-contract gate can follow later.
Handoff: `PX2_GATE_DEFINITION_HANDOFF.md`.

**MV1.D CERTIFIED @ `fa9c048f`** — distance / depth readability via
continuous aerial perspective. Presentation-only: every **far-pass** fragment
attenuates toward the exact horizon sky colour by a deterministic transmittance
of the **actual camera-to-surface distance**, `T(d)=exp(-(rho·d)²)`,
`rho=4.30e-5/m` (`GL_FOG`/`GL_EXP2`). Because `T` depends on distance alone there
is **no colour switch at the 192 m / 2 km / 20 km LOD edges** — an analytical 4 m
scan proves `T` strictly non-increasing (max step jump `1.5e-4`, max LOD-edge
jump `3.5e-5`). Applied **only** around the MV1 far draw; the frozen 0.03–600 m
near authoritative world is **never fogged**, so MV1.C geometry, depth precision,
and persistent VBO ownership are byte-identical and **`--mv1d-off` reproduces the
frozen MV1.C presentation byte-for-byte** (`cmp -l`=0). Framebuffer bands binned
by real distance over the five frozen stations: **contrast falls** (meso 0.066 →
regional 0.049 → horizon 0.022) and **atmospheric blend rises** (0.44 → 0.55 →
0.57) while regional/horizon stay rasterised. Distant alpine **white remains the
diagnostic material, not snow**; this is **not causal weather** (a later cut may
drive `rho` from MW6). Full gauntlet green: PX2 self-test PASS, MV1.C band-pixel
PASS (fog on), MV1.G full-raster GPU PASS, **Test A 4/4** (engine 0-over, 0
stage-owned), **Test B 90 s PASS** (0 presented/stage-owned misses, cadence
no-regression). Handoff: `MV1D_DISTANCE_READABILITY_HANDOFF.md`. Cert:
`CERT_MV1D_DISTANCE_READABILITY.cmd`.

**MV2.C @ `48ccf496`** — distant-terrain **presence** (presentation ONLY; authority,
macro relief, region cadence, and 1 km page step all untouched). The far MV2 ranges were
technically visible but washed to near-sky, reading as a thin floating strip above the
legitimate negative-space band. MV2.C fixes the *presentation*: (1) gentler 32–128 km
aerial tail (`GL_EXP` 1.85e-5 → 0.98e-5 on both far passes; still one continuous
distance curve, no band switch; T(128 km) 0.09→0.32); (2) coarse **land-cover** palette
(vegetated / dry-substrate / exposed-rock by elevation+slope, all darker than sky; bare
high ground is rock, **never snow**) replacing the diagnostic green→tan→white ramp;
(3) stronger silhouette-preserving hillshade. Colours baked into macro VBOs at build,
fog at render; `--mv2c-off` == exact frozen MV2.B. **A/B:** far 50–128 km separation
from sky **0.007 → 0.049 (~7×)** (fixture 13, gates MV2.C only; MV2.B baseline reports
it but is not held to it); distance-class pixels **identical** (depth-based, coverage
unchanged); 0 MV2 coverage holes preserved (negative space intact); no 32 km seam /
palette switch; Test B 24 m/s green. **Honest limit:** MV2.C makes far ranges legible
and separated from sky but not *taller* — low profile at central viewpoints is macro
**relief** (geometry), the reserved question, deliberately NOT touched. Tool:
`Code/.../Main.cpp` (Mv2ShadeAt land-cover + Mv2ActiveAerialDensity + `--cert-mv2c`/
`--mv2c-off`); receipt `Docs/provenance_mv2c_presence_cert.txt`; A/B visual
`Docs/provenance_mv2c_ab_comparison.png`; handoff `MV2C_DISTANT_PRESENCE_HANDOFF.md`;
script `CERT_MV2C_PRESENCE.cmd`. MV2.A/MV2.B/MW1–MW8/MV1 not reopened.

**MV2.A2 @ `7c344ca1`** — full 128 km **radial** authority. The initial MV2.A/MV2.B
cut proved 128 km on the 3×3 diagonal only (±96 km square). MV2.A2 extends the same
continuous deterministic field to the **±2 ring = 25 pages** (no new generation
semantics), so neighbouring authority reaches 128 km+ in every direction. Authority
cert (25 pages): all IDs/digests deterministic + unique; continuity across the new
±96 km ring seams (0.0 m anomalous step); no square-world signature over ±160 km;
cardinal ±2 reaches 160 km with real relief (N 4544 m); features cross the ±2 ring;
cheap (1.3 s). Renderer loads all 25 pages, bounded (25 high-water, 0 runtime allocs
after warmup); horizon **fixture 12** proves 100-128 km framebuffer pixels in N/E/S/W
(N=4929 E=7813 S=6389 W=1871) — full radial 128 km, not just diagonal. Test B 24 m/s
hard gate green with the 25-page ring (engine 0-over, 0 stage-owned, cadence
no-regression). `Tools/Worldgen/cert_macro_authority.py`,
`Data/Worldgen/MacroAuthority/*.mcp` (25), receipt
`Docs/provenance_mv2a_macro_authority_cert.txt`. Deferred (by decision): coarse
long-distance semantic presentation (rock/vegetated/desert/wetland/snow) across MV1+MV2.

**MV2.B CERTIFIED @ `40272156`** — 100–128 km Horizon Renderer (presentation
only; consumes MV2.A `.mcp` macro pages). Renders the 32–128 km horizon behind the
frozen 0–32 km MV1 world via a **three-pass depth split** (macro 24–130 km → clear →
MV1 far 0.128–33 km → clear → near 0.03–600 m); MV1 stays authoritative at the 32 km
seam (MV2.A central anchoring → seam ≤ 5.3 m vs `ReconstructedZ`, sub-pixel).
Per-page persistent VBOs (sync-safe; no shared-slab in-flight mutation), bounded
±2-cell ring (9 pages, 0 runtime allocs after warmup), never samples fine
`ReconstructedZ` for the horizon. Recalibrated 128 km aerial curve (`GL_EXP`
ρ=1.85e-5) on both far passes → one continuous distance attenuation, no band switch;
`--mv2b-off` reproduces frozen MV1.C/D. **All fixtures PASS:** real 128 km raster
(distance-class px 44053/53316/10200/4926 for 32-50/50-80/80-100/100-128 km),
non-repetition (9 unique pages), seam continuous, no fine authority, bounded
residency, and **0 MV2-domain (≥32 km) coverage holes** — a mask+authority-raymarch
classifier proved the mid-distance sky band is geographic **negative space**, not
missing terrain (the only terrain/sky discrepancies are thin <32 km slivers in frozen
MV1's own LOD domain). Gameplay PX2 hard gate held at 24 m/s with the horizon on.
Receipt `Docs/provenance_mv2b_horizon_cert.txt`; visual
`Docs/provenance_mv2b_horizon_contact_sheet.png`; handoff
`MV2B_HORIZON_RENDERER_HANDOFF.md`; scripts `PLAY_/CERT_MV2B_HORIZON_RENDERER.cmd`.
MW1–MW8/MV1/MV2.A not reopened.

**MV2.A CERTIFIED @ `d23f3810`** — Regional Macro Authority (Python
world-authority lane; NO renderer). Deterministic non-repeating boundary-continuous
macro pages beyond the frozen ±32 km centre. `macro_z = central_envelope +
anchor_window·regional_field`: `central_envelope` replicates
`CausalMacroProvinces::SampleForcing().surfaceZ` (centre IS frozen authority);
`anchor_window=smoothstep(32,64 km, max|x|,|y|)` is 0 with zero slope on the centre
boundary → **position+slope continuity by construction**; `regional_field` is a
continuous absolute-coordinate field (tectonic base + 6 belts 400–900 km + basins +
massifs) seeded/hashed from the world seed, wavelengths **independent of the 64 km
cell** (a belt runs 874 km across ~13 cells). 9-page set (center+N/S/E/W+NE/NW/SE/SW),
each carrying region_id/world_seed/generator_version/bounds/source_digest/8-way
neighbor lineage/no-wrap. **All 8 fixtures + cheap-source PASS:** determinism
(0 bytes cross-process), non-repetition (period-64 RMS 675 m), boundary continuity
(0.046 m / slope 1e-4), cross-page crest continuity, central anchor (exact 0),
no-square-signature, feature >64 km (874 km belt), no `WrapIntoRegion`/`ReconstructedZ`,
cheap (9 pages / 0.48 s). Tool: `Tools/Worldgen/{macro_authority,cert_macro_authority}.py`;
pages `Data/Worldgen/MacroAuthority/*.mcp`; receipt
`Docs/provenance_mv2a_macro_authority_cert.txt`; visual
`Docs/provenance_mv2a_macro_footprint.png`; handoff
`MV2A_REGIONAL_MACRO_AUTHORITY_HANDOFF.md`. MW1–MW8/MV1 not reopened; frozen centre
untouched. Next = MV2.B horizon renderer (consume pages; no new world authority).

**MV2 DESIGN LOCKED** (design only; no code) — `MV2_HORIZON_ARCHITECTURE_DESIGN.md`.
Two grounded findings decide the architecture: (A1) the MW authority is a **single
64 km region** (`kRegionHalfM=32000`) tiled by `WrapIntoRegion`; MV1's 32 km horizon
reaches exactly its edge, so sampling at 100 km **repeats** the 64 km world — MV2
needs a **new non-repeating macro-source**, not "MV1 farther"; (A2) the ~4–8 ms/tile
is fine `ReconstructedZ` erosion/geology, while the macro silhouette (massif/belt/
basin/structural envelope) is **cheap analytic** — MV2 samples that coarsely and
caches it. Locked §G: (1) deterministic **continuous absolute-coordinate** macro
field (seeded control lattice, interpolated → boundary continuity by construction;
no per-region random programs); (2) **Python** macro compiler is authority, Esoterica
consumes compiled pages (carry WorldSeed/GeneratorVersion/MacroRegionId/bounds/
revision/neighbor-lineage), renderer derives presentation only; (3) **64 km = cache/
ownership cell, NOT feature cadence** — feature wavelengths independent and much
larger; (4) authority conceptually **unbounded**, resident ring **bounded**, cert
target **100–128 km**; (5) the certified central 64 km region is an **anchored**
constraint the new field must continue across (four-boundary continuity cert; no
`WrapIntoRegion` beyond it); (6) no deep `ReconstructedZ`/`QueryMaterial` resampling,
cacheable pages, build-then-promote/fenced GPU, never write GPU-in-flight memory.
Extra hard reqs: **no square-world signature** (region boundary ≠ terrain boundary).
Sequenced as two cuts: **MV2.A regional macro authority** (Python + client consume,
certified non-repeating + anchored + boundary-continuous, NO horizon render) FIRST,
then **MV2.B horizon renderer** (bounded ring, widened two-pass to ~128 km, MV1.D
recalibrated, seam-free join). MW1–MW8 not rebuilt.

**PX3 CERTIFIED @ `4cd5af4b`** — retrospective frozen-cut performance
revalidation under the PX2 two-contract gate (semantics/determinism NOT reopened).
Every frozen cut re-run through its existing harness: Test A cardinal N/E/S/W +
Test B 90 s NE fly at 24 m/s gameplay speed. **Worldgen ladder (P5b.3B.3B, 16C.1,
MW1–MW8) GREEN** — engine production 3–7 ms, 0 over, 0 stage-owned, digests exact;
the historical `wall_clock>16.667` receipts were **merely superseded** by PX2, **no
historical regression**. MV1.C/MV1.D **green at gameplay speed**. The **240 m/s
cardinal free-fly** (10× gameplay) is kept exactly as-is and reported as a separate
**STRESS CEILING** (worst ~16.85 ms vs 16.667 on one case). Its investigation:
(1) per-tile draw submission → fixed by a **horizontal frustum cull** (kept,
image-identical); (2) a **shared-slab batching experiment** cut CPU submit 9 ms →
0.1 ms but was **REJECTED** — `glBufferSubData` into in-flight slabs caused 50–98 ms
gameplay-frame stalls (4/5 Test B fail) — reverted to per-tile VBO ownership;
(3) remaining owner = **MW-authority tile construction** (~4–8 ms/tile, unpreemptable),
an **MV2 scaling constraint**. Also kept: **canonical draw order**
(band→Y→X→key) with the one deterministic z-tie pixel at `ridge_shoulder`
re-baselined once (MV1.C+MV1.D station2). **Two contracts now explicit:** gameplay
PX2 gate hard-green at 24 m/s; 240 m/s a permanently-reported stress ceiling.
**MV2 requirements:** address tile-construction scaling and use only
synchronization-safe batching (persistent-mapped/fenced/build-then-promote), never
CPU writes over GPU-in-flight storage. Receipt:
`Docs/provenance_px3_retrospective_sweep.txt`. Handoff:
`PX3_RETROSPECTIVE_PERFORMANCE_SWEEP_HANDOFF.md`.

**GLOBAL Test-B present pacing — attributed by PX1 (not MV1).** A controlled 4-way experiment
(MV1 on/off × glFinish on/off, plus per-outlier component logging) proved the
90 s soak / movement-frame gate is **baseline-flaky on this hardware, independent
of MV1**: the stock MW8 travel gate fails ~3/5 runs with MV1 **disabled**
(worst 61 ms), vs 2/5 with MV1 on (worst 51 ms); every spike frame does zero MV1
work; the pre-present `glFinish` is *protective* (removing it worsens SwapBuffers
pileups to 58–98 ms). So the historical zero-overrun 90 s gate is not
reproducibly met even by the frozen baseline here — a present-path / vsync /
driver-pacing / frame-time-definition infrastructure issue that must be repaired
as its own item before it is used to gate MV1.D and later cuts. **The 16.667 ms
standard is not weakened** — the baseline gate is what needs repair. Test A's
movement sub-gate inherits the same jitter (correctness always exact).

**MV1 correction (projection audit).** MV1 certified construction, bounded
residency, adaptive geometric fidelity, geographic identity, and 32 km derived
terrain *extent*. A subsequent projection audit found the active render far
plane remained **600 m**, so the earlier claim of 32 km raster-visible terrain
was overstated: regional/horizon MV1 geometry was built and submitted but
**clipped before framebuffer contribution**. Empirically confirmed — opening the
far plane surfaces distant ranges (e.g. the foreland-basin horizon) that 600 m
clips. **MV1.C** is opened to restore actual 32 km visibility via a two-pass
depth split (far terrain pass at near≈64–128 m / far≈33 km, depth-only clear,
then the frozen 0.03–600 m near pass painted over the top) so the certified
near-depth path is unchanged. **MV1.G qualification:** its display-list
lifecycle, bounded resources, query mechanism, first-use behavior, and
submission measurements remain valid; its claim that fully rasterized 32 km
terrain is GPU-safe **requires revalidation after MV1.C**, because most
regional/horizon fragments were clipped by the 600 m projection during the
original test (the 0.53 ms number was real for the path that executed, but does
not yet certify full 32 km raster cost). Sequence: **MV1.C raster visibility →
rerun MV1.G → then MV1.D.**

**MV1 forward plan (small cuts, not MW-sized stages):**
- **MV1.G — GPU visibility performance. CERTIFIED @ `0d39ffcd`.** Async
  `ARB_timer_query` around the MV1 draw span (never blocks to measure). Worst GPU
  draw across all scenarios (five stations, worst-orientation sweep, 360°
  rotation, movement band churn, first-visible cold, warm repeat) **0.53 ms** vs
  an 8 ms stall bound; every scenario p99 < 0.03 ms; no first-use stall (cold
  0.022 ms, warm 0.013 ms); 626 draw calls / 65 k tris bounded; no
  travel-history resource growth. VRAM bytes and display-list upload time honestly
  `UNAVAILABLE` (no reliable extension; driver-defined). Standing gates held:
  Test A 4/4 (0 movement frames >16.667), Test B 90 s 0 frames >16.667. Handoff:
  `MV1G_GPU_PRESENTATION_HANDOFF.md`. Cert: `CERT_MV1G_GPU_PRESENTATION.cmd`.
- **MV1.D — distance / depth readability. CERTIFIED @ `fa9c048f`.**
  Continuous aerial perspective `T(d)=exp(-(rho·d)²)` (`rho=4.30e-5/m`) toward the
  horizon sky colour, by actual camera distance, in the far pass only. No colour
  switch at the LOD edges (analytical continuity proof); near/meso crisp,
  regional half-blended, horizon strongly atmospheric but still rasterised;
  `--mv1d-off` == frozen MV1.C byte-for-byte. A later, separate cut may drive
  `rho` causally from MW6 hydroclimate (dry interior = long visibility, humid
  valley = stronger attenuation).
- **MV2** extends the certified 32 km hierarchy toward the Alaska-scale target
  only after MV1.G (GPU-safe) and MV1.D (depth-readable) pass.

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

**P5b.2C CERTIFIED** — pore transfer may change surface-water occupancy /
topology via 16F.4, then 16F.1 / 16F.2. Terrain solid grams unchanged.
Handoff: `P5B2C_TERRAIN_PORE_OCCUPANCY_HANDOFF.md`. Disabled == exact P5b.2B
pore digest `224e662e5584a1ea`. Enabled occupancy digest `7102e45c92f6545d`
(budget 1 == N == unbounded). Conserved body+pore+container `466363002`.
Play: `PLAY_P5B2C_TERRAIN_PORE_OCCUPANCY.cmd` /
`--play-p5b2c-terrain-pore-occupancy`. Cert: `CERT_P5B2C_TERRAIN_PORE_OCCUPANCY.cmd`.
Parent freeze: long-haul infrastructure `f7ae29ea` / pin `156df62d`.
Gameplay 2B `8bb75265` otherwise frozen.

**P5b.3A CERTIFIED / FROZEN** @ `6c467fb7` — one bounded parcel of susceptible
terrain may detach under certified hydraulic conditions. Choice A: local
loose body/aggregate (not 16C). Law: terrain solid loss == new loose matter.
Water mass unchanged. Disabled == exact 2C occupancy `7102e45c92f6545d`.
Enabled detachment digest `e3ba9b3af77265cb` (budget 1 == N == unbounded).
Solids −80 / loose +80. Handoff: `P5B3A_HYDRAULIC_DETACHMENT_HANDOFF.md`.

**P5b.3B CERTIFIED / FROZEN** @ `67d5f524` — water may move an already-detached
loose parcel one bounded hop without changing identity. Destination stays
loose matter. Disabled == exact 3A detachment `e3ba9b3af77265cb`. Enabled
transport digest `876ac027936dce35` (budget 1 == N == unbounded). Solids
unchanged, loose 80 at dest. Handoff: `P5B3B_HYDRAULIC_TRANSPORT_HANDOFF.md`.
Play: `PLAY_P5B3B_HYDRAULIC_TRANSPORT.cmd` / `--play-p5b3b-hydraulic-transport`.
Cert: `CERT_P5B3B_HYDRAULIC_TRANSPORT.cmd`.

**P5b.3B.2 CERTIFIED / FROZEN** @ `eeabfb8c` — a transported parcel may come to rest at a physically
admissible location while remaining the same conserved loose matter. Settling
changes location + motion/rest only — not identity, not terrain fill.
Disabled == exact 3B transport `876ac027936dce35`. Enabled settling digest
`ab46ebdbe3aa6778` (budget 1 == N == unbounded). Solids/water/loose grams
unchanged (80 → 80 Settled). Handoff: `P5B3B2_LOOSE_MATTER_SETTLING_HANDOFF.md`.
Play: `PLAY_P5B3B2_LOOSE_MATTER_SETTLING.cmd` / `--play-p5b3b2-loose-matter-settling`.
Cert: `CERT_P5B3B2_LOOSE_MATTER_SETTLING.cmd`.

**P5b.3B.3A CERTIFIED / FROZEN** @ `160a0842` — settled loose matter may be admitted as a depositional
sediment body. Same material ID ≠ host formation. Terrain host solids unchanged.
Disabled == exact 3B.2 settling `ab46ebdbe3aa6778`. Enabled deposition digest
`694388e61fa37503` (budget 1 == N == unbounded). Loose 80 → 0, depositional
0 → 80. Handoff: `P5B3B3A_DEPOSITIONAL_AGGREGATE_HANDOFF.md`.
Play: `PLAY_P5B3B3A_DEPOSITIONAL_AGGREGATE.cmd` / `--play-p5b3b3a-depositional-aggregate`.
Cert: `CERT_P5B3B3A_DEPOSITIONAL_AGGREGATE.cmd`. In-client stages table: **M**.

**P5b.3B.3B CERTIFIED / FROZEN** @ `2f722735` — a deposited aggregate may compact under admissible
load and participate in terrain surface + collision/support while remaining
the same depositional body. Compaction is state, not a mass transfer.
Disabled == exact 3B.3A deposition `694388e61fa37503`. Enabled compaction
digest `ef1ae252489e741a` (budget 1 == N == unbounded). Depositional 80 → 80.
Host solids unchanged. Handoff: `P5B3B3B_COMPACTION_HANDOFF.md`.
Play: `PLAY_P5B3B3B_COMPACTION.cmd` / `--play-p5b3b3b-compaction`.
Cert: `CERT_P5B3B3B_COMPACTION.cmd`.

**3A–3B.3B chain closed** — ready as a **worldgen shaping primitive**, not
merely a runtime water feature. Host geology ≠ loose ≠ transported ≠
settled aggregate ≠ compacted deposited ground; same MaterialId allowed.
Runtime still only wakes locally when a player/event disturbs it.
M-table caption:
`3A-3B.3B  runtime deposition chain   worldgen shaping primitive (ready)`

**P5b.3C CLOSED** — structural collapse (connectivity / multi-body;
different problem class). Visible on the M board; not launchable.
Also closed: cement/lithify, formation-ID merge, general soil mechanics,
general erosion, rainfall, groundwater, active 16B erosion, 16C remobilization,
ecology.

**16C CERTIFIED / FROZEN** — compiled sediment routing unchanged
(`1367584c41aedcdf` / geometry `7ae7aa62c50489e2`).

**16C.1 CERTIFIED / FROZEN** @ `047d4304` — last local-chain bridge before
macro-scale worldgen.
Already-compiled 16C sediment is labeled loose / settled aggregate /
compacted deposit / not depositional. Classification is not a ledger
transfer. Off == frozen 16C/16D present (`2eb519c42ca5bd6b`). On
classification digest `4075ca07bfa05881`. 16D water `636d01ba00d3dffe`
unchanged. Play kernel is 16C-weight (16D loaded only in analytical cert).
Handoff: `P16C1_COMPILED_DEPOSIT_CLASSIFICATION_HANDOFF.md`.
Play: `PLAY_16C1_COMPILED_DEPOSIT_CLASSIFICATION.cmd`.
Cert: `CERT_16C1_COMPILED_DEPOSIT_CLASSIFICATION.cmd`.

**MW1 CERTIFIED / FROZEN** @ `569269aa` — provinces / mountain belts / basins. Absolute-coordinate
64 km geologic forcing field (not final planet terrain). Off / no-MW1 ==
frozen 16C.1 local present. Production source is not the 4096 m wrap.
Handoff: `MW1_PROVINCES_HANDOFF.md`.
Play: `PLAY_MW1_PROVINCES.cmd` / `--play-mw1-provinces`.
Cert: `CERT_MW1_PROVINCES.cmd`.

**MW2 CERTIFIED / FROZEN** @ `6ada7260` — regional 3D geology keyed to MW1 provinces. Persistent
formations, faults, intrusions, bedding, and chronology from 64 km to
12.5 cm. Surface is the existing MW1 present surface (no new carver).
Off / no-MW2 == frozen 16C.1 local present. Handoff:
`MW2_REGIONAL_GEOLOGY_HANDOFF.md`.
Play: `PLAY_MW2_REGIONAL_GEOLOGY.cmd` / `--play-mw2-regional-geology`.
Cert: `CERT_MW2_REGIONAL_GEOLOGY.cmd`.

**MW3 CERTIFIED / FROZEN** @ `0274e22e` — compiled denudation on MW2 geology + MW1 relief. Changes
the intersection surface, not FormationId. Off / no-MW3 == exact MW2 present
(max |ΔZ| 0). Mass conserved: removed == exported. Provisional flow only;
river network is MW4. Handoff: `MW3_REGIONAL_EROSION_HANDOFF.md`.
Play: `PLAY_MW3_REGIONAL_EROSION.cmd` / `--play-mw3-regional-erosion`.
Cert: `CERT_MW3_REGIONAL_EROSION.cmd`.
CLOSED: MW8 biomes, 3C structural collapse, 16C runtime remobilization.
Do not add PLAYABLE biome/flora rows.

**MW4 CERTIFIED / FROZEN** @ `cc7fe436` — watershed + valley organization on the MW3 surface,
steered by MW2 structure. Not a river-noise layer. Off / no-MW4 == exact
MW3 present (max |ΔZ| 0). Mass conserved: removed == exported. H2H
drainage identity from 64 km through 12.5 cm bank/bed (host MW2
FormationId on the MW4-off / no-MW5 path). Handoff: `MW4_DRAINAGE_VALLEYS_HANDOFF.md`.
Play: `PLAY_MW4_DRAINAGE_VALLEYS.cmd` / `--play-mw4-drainage-valleys`.
Cert: `CERT_MW4_DRAINAGE_VALLEYS.cmd`.
CLOSED: MW8 biomes, 3C structural collapse, 16C runtime remobilization,
live P5b / 16D play ownership.

**MW5 CERTIFIED / FROZEN** @ `ba6faf61` — depositional landscape consuming MW3/MW4 export mass.
Fans, valley fill, floodplains, bars, basin fill, terraces, and colluvial
aprons as 16C.1 / 3B.3B bodies (loose / settled / compacted), distinct from
host FormationId. Off / no-MW5 == exact MW4 present (max |ΔZ| 0). Mass
conserved: source == deposited + boundary export. Handoff:
`MW5_DEPOSITIONAL_LANDSCAPE_HANDOFF.md`.
Play: `PLAY_MW5_DEPOSITIONAL_LANDSCAPE.cmd` / `--play-mw5-depositional-landscape`.
Cert: `CERT_MW5_DEPOSITIONAL_LANDSCAPE.cmd`.
CLOSED: MW8 biomes, 3C structural collapse, 16C runtime remobilization,
live P5b / 16D play ownership.

**MW6 CERTIFIED / FROZEN** @ `e8704845` — compiled regional hydroclimate on the certified MW1–MW5
landscape. Long-term temperature, moisture, orographic/rain-shadow, wetness,
runoff potential, snow-persistence, exposure, and basin/valley tendency.
Does not mutate terrain (off == exact MW5 present, max |ΔZ| 0). Does not
paint biomes or simulate weather. HydroclimateId is absolute-coordinate,
not scoped to the 64 km cert box. Prevailing moisture azimuth is regional
(298° toward the tectonic foreland), not a box-edge. Handoff:
`MW6_HYDROCLIMATE_HANDOFF.md`.
Play: `PLAY_MW6_HYDROCLIMATE.cmd` / `--play-mw6-hydroclimate`.
Cert: `CERT_MW6_HYDROCLIMATE.cmd`.
CLOSED: MW8 biomes, 3C structural collapse, 16C runtime remobilization,
live P5b / 16D play ownership, live weather, glaciers.

**MW7 CERTIFIED / FROZEN** @ `307e92fc` — derived near-surface soils / regolith profile on certified
MW1–MW6. Not host geology, not a depositional body, not a terrain carver, and
not vegetation. Off / no-MW7 == exact MW6 present (max |ΔZ| 0) with unchanged
HydroclimateId, deposit bodies, and FormationId. A 12.5 cm sample in an MW5
deposit retains the body's source ancestry (`D05_BASIN_FILL` / `B03_HOST_UPPER`).
Handoff: `MW7_SOILS_REGOLITH_HANDOFF.md`.
Play: `PLAY_MW7_SOILS_REGOLITH.cmd` / `--play-mw7-soils-regolith`.
Cert: `CERT_MW7_SOILS_REGOLITH.cmd`.
CLOSED: MW9 flora/fauna, 3C structural collapse, 16C runtime
remobilization, live P5b / 16D play ownership, live weather, glaciers,
groundwater, vegetation.

**MW8 CERTIFIED / FROZEN** @ `e8155fa3` — compiled ecological regime /
biome potential inferred from certified MW1–MW7 (climate, elevation, moisture,
soils/regolith, drainage, substrate, exposure). A regime, not a color mask and
not vegetation placement; MW8 publishes ecological potential, MW9 later decides
occupancy. Off / no-MW8 == exact MW7 present (max |ΔZ| 0) with unchanged
HydroclimateId, RegolithId, deposit bodies, and FormationId. Mandatory
counterfactuals bite: neutralizing hydroclimate collapses the wet/dry contrast
(`hydro_off_contrast` 0.000); neutralizing regolith collapses the
substrate/drainage contrast. BiomeId is absolute-coordinate (no 4096 m wrap).
Terminal-branch coverage guard proves the MW6 wet/dry decision cannot silently
collapse. Handoff: `MW8_BIOMES_HANDOFF.md`.
Play: `PLAY_MW8_BIOMES.cmd` / `--play-mw8-biomes`.
Cert: `CERT_MW8_BIOMES.cmd`.
CLOSED: MW9 flora/fauna, 3C structural collapse, 16C runtime remobilization,
live P5b / 16D play ownership, live weather, glaciers, groundwater,
vegetation, ecology simulation.

**MV1 CERTIFIED** @ `77aa7767` — multi-scale terrain **visibility** to 32 km.
Presentation / scale layer, not new worldgen: derived visible terrain beyond the
192 m full-interactive residency, sampling the SAME absolute-coordinate MW1–MW8
composite authority (`ReconstructedZ`). Representation coarsens with distance,
geographic truth does not. Bands: meso (128 m–2 km), regional (2–20 km), horizon
(20–32 km), each **terrain-aware adaptive** (fine 8/64/128 m sampling emitted at
the coarsest resolution within a ≤5/18/45 m error target, so ridges/valleys/
saddles keep detail and only genuinely smooth ground uses large faces).
Absolute-world retained tiles, incremental frame-budgeted build (never a
main-thread compiler), band-ring residency bounded by the view working set (no
travel-history growth), source-revision lineage, perimeter skirts so no seam
shows sky. Geometric fidelity vs authority: max 44.93 m (horizon), mean 0.68 m.
Five true player stations (trunk valley, mountain flank, ridge shoulder, foreland
basin, TRUE high divide) readable from neutral cameras; six hard fixtures +
geometric-fidelity + H2H (32 km massif == 12.5 cm sample) all PASS. `--mv1-off`
== exact MW8 near path (presentation-only; no authority/geometry/material/
collision/mass/worldgen mutation). Near 192 m residency and standing gates
unchanged: Test A 4/4, 2601 packages, 0 movement frames >16.667; Test B 90 s
0 frames >16.667, MW1–MW8 idle. Handoff: `MV1_MULTI_SCALE_TERRAIN_HANDOFF.md`.
Play: `PLAY_MV1_MULTI_SCALE_TERRAIN.cmd` / `--play-mv1-multi-scale-terrain`.
Cert: `CERT_MV1_MULTI_SCALE_TERRAIN.cmd`.
CLOSED: MV2 100+ km horizon; MW9 flora/fauna; all MW8-closed items.

```
TRAVERSAL + STREAMING SOAK (standing matrix)
```

Cardinal replacement ≠ soak. **Law:** Travel distance may grow; active
residency, memory, pending work, and wake backlog must remain bounded.
Contract: `TRAVERSAL_STREAMING_SOAK_HANDOFF.md`.

- Test S: `--cert-semantic-distance` (teleport+settle across/beyond 4.096 km
  Stage-15 domain; independent of Test B). `CERT_SEMANTIC_DISTANCE.cmd`.
- Test A: `--cert-worldgen-cardinal-replacement-mw7` (EVERY CUT, latest).
  Digest `983e0d8f7014d5ea` origin==return. 2601 packages. Movement frames
  over 16.667 = 0 on N/E/S/W (worst movement ~5.8 ms). MW6 control remains
  `--cert-worldgen-cardinal-replacement-mw6` digest `0720123f96a82229`.
  MW5 control remains
  `--cert-worldgen-cardinal-replacement-mw5` digest `bdef220660da3b68`.
  MW4 control remains
  `--cert-worldgen-cardinal-replacement-mw4` digest `e7d0f813783e2e9f`.
  MW3 control remains
  `--cert-worldgen-cardinal-replacement-mw3` digest `3af80264fc1a619e`.
  MW2 control remains
  `--cert-worldgen-cardinal-replacement-mw2` digest `09b4bccd6f04f59d`.
  MW1 control remains `--cert-worldgen-cardinal-replacement-mw1` digest
  `017a2368097d219c`. 16C.1 control remains
  `--cert-worldgen-cardinal-replacement-16c1` digest `e7bb12b2b2ec12e3`.
  3B.3B control remains `--cert-worldgen-cardinal-replacement-p5b3b3b`
  digest `1d1dd0776f69f322`.
- Test B: `--cert-streaming-soak-mw7` (90 s NE fly **PASS** this cut:
  0 / 84043 frames >16.667, max 11.158 ms, 2601, pending 0.
  MW1/MW2/MW3/MW4/MW5/MW6/MW7 travel physics **idle**: rebuilds = 0, compiles = 0. P5b 2C–3B.3B
  and 16C.1 also idle. 300/900 not run — no persistent MW7 workload.
  Long-haul 300/900 remain the 2B baseline receipts.)
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
  90 s frame **PASS** (0 / 84138 >16.667, max 8.688 ms). 12 km / 500 s
  frame **PASS** (0 / 482125, max 11.282). Water GPU 2 batches / 170 KB,
  travel growth 0. FollowStream scratch growth 0, CRT segment 0.
  Package-worker scratch **PASS**: 4 × 12562 B, fallback CRT 0, overflow 0,
  event/chronology high-water 0. SampleBlock temps closed. Worker Stage-12
  dual-surface cache **bounded**: 4 workers, entries warmup 0.93 M → plateau
  1.3–1.5 M (8→12 km 1.50 M → 1.34 M), bytes ~70 MB plateau, 116 M evictions.
  CRT 8→12 km **+1.5 MB** (197 → 199 MB), `PASS_plateau`. Return CRT 208 MB.
  `erosion_cache_entries` now reads the four worker kernels. Golden:
  cache enabled == forced-cold recompute. 16.667 not relaxed.
  P5b.3B.3B 90 s Test B **PASS** (idle 3B.3B compact count = 0).
  16C.1 90 s Test B **PASS** (idle runtime classifies = 0).
  MW1 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW2 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW3 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW4 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW5 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW6 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  MW7 90 s Test B **PASS** (idle rebuilds/compiles = 0).
  **P5b.3C / 16C remobilization / MW8 biomes stay CLOSED.**
- Ownership-class private accounting + checkpoint drain at 0/1/2/4/8/12 km
  + return-origin receipt are required on every soak receipt.
- P5b.2B gameplay frozen at `8bb75265`. **P5b.3C** / rainfall / erosion
  remain CLOSED.

```
LONG-HAUL INFRASTRUCTURE BASELINE
SHA: f7ae29ea8e8e689196e829fbac6cb098e0cda05b
PARENT: 800cfaef
```

**Pin:** Stage-12 worker dual-cache lifetime is the long-haul infrastructure
baseline. Cache-lifetime correction, not worldgen. Stage-12 digest
`d2f1c29c2fcaad9d`. Test S green. Test A `2396f444f66f1234`. Forced-cold ==
cache-enabled; eviction/recompute identical. 900 s 0 frames >16.667. CRT
8→12 km ~+1.5 MB. Gameplay remains P5b.2B `8bb75265`; P5b.2C / P5b.3 CLOSED.

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
**P5b.2B CERTIFIED** (bounded pore). **P5b.2C FROZEN** (pore occupancy /
topology). **P5b.3A FROZEN** (hydraulic detachment, choice A local loose).
**P5b.3B FROZEN** (hydraulic transport of already-detached loose matter).
**P5b.3B.2 CERTIFIED / FROZEN** (loose-matter settling: location + rest, not identity).
**P5b.3B.3A CERTIFIED / FROZEN** (depositional aggregate: settled loose → sediment body, not host weld).
**P5b.3B.3B CERTIFIED / FROZEN** @ `2f722735` (compaction: deposited body → stable deposited ground, not host weld).
**3A–3B.3B** = worldgen shaping primitive (ready). **16C** CERTIFIED
unchanged. **16C.1** CERTIFIED / FROZEN @ `047d4304` (last local bridge before macro).
**MW1 CERTIFIED / FROZEN** @ `569269aa` (provinces / belts / basins; 64 km forcing field).
**MW2 CERTIFIED / FROZEN** @ `6ada7260` (regional 3D geology; persistent bodies through depth).
**MW3 CERTIFIED / FROZEN** @ `0274e22e` (compiled denudation on MW2 geology; intersection surface only).
**MW4 CERTIFIED / FROZEN** @ `cc7fe436` (watershed + valley organization on the MW3 surface; no river-noise layer).
**MW5 CERTIFIED / FROZEN** @ `ba6faf61` (depositional landscape from MW3/MW4 export; 16C.1 bodies, not host weld).
**MW6 CERTIFIED / FROZEN** @ `e8704845` (compiled regional hydroclimate on MW1–MW5; forcing only, no terrain carve).
**MW7 CERTIFIED / FROZEN** @ `307e92fc` (soils / regolith: derived near-surface profile on MW1–MW6; not vegetation).
**MW8 CERTIFIED / FROZEN** @ `e8155fa3` (biomes / ecological regime inferred from MW1–MW7; not a color mask, not vegetation).
**MV1 CERTIFIED** @ `77aa7767` (multi-scale terrain visibility to 32 km; derived presentation from the MW1–MW8 composite authority — representation coarsens with distance, geographic truth does not; adaptive error-bounded tessellation, not a new terrain generator).
**MV1.G CERTIFIED** @ `0d39ffcd` (GPU presentation: async ARB_timer_query proves the 32 km draw path is GPU-safe — worst 0.53 ms vs 8 ms bound, no first-use stall, bounded draw calls/resources; VRAM/upload UNAVAILABLE by design).
**P5b.3C CLOSED** (structural collapse). **MW9 flora / fauna CLOSED**. **MV2 100+ km horizon CLOSED**.
Certified rainfall / flora / deep groundwater CLOSED.

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
