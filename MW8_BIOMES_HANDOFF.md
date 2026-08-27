# MW8 — Biomes / Ecological Regime

## RESUMED on canonical orographic context (not GradeToZ, not a bigger page)

```
MW8 RESUMED ON CANONICAL OROGRAPHIC CONTEXT —
ECOLOGY INDEPENDENT OF PAGE/RENDER SCALE
```

Esoterica `adopt_page` consumes canonical Phase 17 pages (see
`ESOTERICA_ADOPT_PAGE_HANDOFF.md`). Adjacent `AdoptContext` consumes
`orographic_ecological_context_v1`. MW8 queries that graph:

- `elevation_grade` (canonical; not GradeToZ metres)
- SystemId / RangeId / MassifId
- ridge / divide / saddle / valley / basin
- slope_grade / aspect
- exposure / windward_factor / lee_factor (ridge/massif vs moisture azimuth 298°)
- accumulation / upstream / downstream drainage

Page `(1,1)` remains `[1024,2048]²`, digest `03f579fed39e4685…`. Influence
radius **4710.4 m** > page **1024 m**. Alpine cells sit on neighboring-page
peaks named by the context graph; render/collision stay on the local page.
`alpine_used_presentation_z=0`, `windward_used_presentation_z=0`.

Do **not** recertify the frozen 64 km microscope at e8155fa3. MW9 CLOSED.

**Cert (2026-08-27):** `CERT_MW8_OROGRAPHIC.cmd` PASS. Receipt
`Docs/provenance_esoterica_adopt_page_cert.txt` (`mw8=PASS`,
`mw8_frozen_until=RESUMED`). Census: alpine barren 74, riparian 379,
basin wetland 149, dry woodland 2, moist forest 1617, dry rocky 335;
windward wet 0.890 vs leeward 0.203; hydro-off contrast 0.000;
page-boundary BiomeId agree 51/51; H2H 289/289; off Δ 0.000000 m;
travel `adopt_rebuilds=0`, `mw8_compiles=0`, `mw8_rebuilds=0`.
64 km Test A / 90 s soak were not re-run (would recertify e8155fa3);
orographic MW8 adds no persistent background workload.

---


The 64 km microscope work below is **kept** (e8155fa3). It is not the
orographic.phase17 resume surface. Do not open MW9.

MW8 remains **ecological regime potential** inferred from MW1–MW7 (climate,
elevation, moisture, soil/regolith, drainage, substrate, exposure). It is not
vegetation placement, not a color mask, and not living-world population.

**Proven boundary (keep):**
- MW8 CERTIFIED / FROZEN @ e8155fa3 (e8155fa38871122cb4e5bb6683f2a97c1caf836d)
- Field digest 3aec4e96adda96f5; input revision bundle e1c2f2c82d66de37
- Kernel CausalRegionalBiome.h last committed at that freeze; working tree
  matches HEAD (no uncommitted MW8 biome-logic WIP)
- Branch provenance/client-spike HEAD 5073baa7 (later WD1.B; MW8 not reopened)
- Dirty Docs/ MW8 visual/cardinal/soak receipts exist on disk — leave them;
  they are not a recert and must not be pushed as a new MW8 CERTIFIED

**Consume path (2026-08-27):** Esoterica `adopt_page` admits canonical
`orographic.phase17` production pages (world hash `b74f957a7fdb429a`, page
`(1,1)` digest `03f579fed39e4685…`) and refuses tectonic-v3 / stale revision.
Adjacent `AdoptContext` admits `orographic_ecological_context_v1`. MW8 is
resumed on that canonical context — never by recertifying the frozen 64 km
microscope at e8155fa3. MW9 CLOSED.

**Gates (orographic.phase17 context — PASS on CERT_MW8_OROGRAPHIC):**
1. Alpine elevation control — canonical elevation_grade; lapse-off changes class
2. Windward vs rain-shadow — ridge/massif exposure across page boundaries
3. Riparian / valley distinct from slope
4. Wet basin — basin/wetness authority
5. H2H BiomeId to 12.5 cm (289/289); page-boundary BiomeId stable
6. MW8-off == exact adopted surface (max |Δ| 0.000000 m)
7. Hydroclimate-off wet/dry contrast collapse; regolith-off substrate/drainage collapse
8. Diagnostic regime colors only — no trees/grass/shrubs/animals/snowpack
9. Ordinary travel does not rebuild context (`adopt_rebuilds=0`, `mw8_compiles=0`, `mw8_rebuilds=0`)

CLOSED remains: MW9 flora/fauna, vegetation placement, live weather, glaciers,
live snowpack, groundwater, ecology simulation, 3C collapse, 16C remobilization.

---

MW8 answers one question: given climate, elevation, moisture, soil/regolith,
drainage, substrate, and exposure, what persistent ecological regime does each
place resolve to?

**Law:** A biome is a persistent ecological regime *inferred* from certified
MW1–MW7 authority — not a color mask and not vegetation placement. MW8 publishes
ecological *potential*; MW9 later decides what flora/fauna actually occupy it.

```
    MW1  tectonic provinces
            ↓
    MW2  geology / FormationId
            ↓
    MW3  erosion / exposure
            ↓
    MW4  drainage / valleys
            ↓
    MW5  deposition
            ↓
    MW6  hydroclimate (temperature / moisture / rain-shadow / snow)
            ↓
    MW7  soils / regolith (depth / drainage / substrate)
            ↓
    MW8  biome regime
            ↓
    (closed) MW9 flora / fauna
```

Forbidden: MW9 flora/fauna; vegetation, grass, shrub, or tree placement;
snowpack / glacier geometry; live weather / rainfall; groundwater flow; ecology
simulation; 3C collapse; 16C remobilization; host FormationId merge; new
sediment; terrain geometry mutation; hydroclimate rewrite; regolith rewrite;
live P5b / 16D play ownership.

Production source is **absolute-coordinate 64 km regional authority**
(`[-32000, 32000]²` m). BiomeId is keyed by world identity + absolute quantized
coordinates, not the certification box, so neighboring macro windows agree. No
4096 m wrap.

## This cut

Freeze parents:

```
MW7     soils / regolith                  CERTIFIED / FROZEN @ 307e92fc
        stamp                             0b9421c2
MW6     hydroclimate                      CERTIFIED / FROZEN @ e8704845
MW5     depositional landscape            CERTIFIED / FROZEN @ ba6faf61
MW4     drainage / valleys                CERTIFIED / FROZEN @ cc7fe436
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

MW8-off == exact MW7 present surface (off/on max |ΔZ| **0.000000 m**), with
unchanged HydroclimateId `d523ab620db18280`, RegolithId `b0bc61d730e6085f`, MW5
deposit bodies, and FormationId. MW8-on does not mutate terrain, hydroclimate,
or regolith.

Descriptor: `Data/Worldgen/causal_world_regional_biome_floor.crb`.
Seed `mw8-64km-01`. Field digest `3aec4e96adda96f5`. Input revision bundle
`e1c2f2c82d66de37`.

## Regime vocabulary (nine, physically interpretable)

Ecological *regimes*, not spawned vegetation:

```
alpine barren               alpine tundra potential
subalpine                   cool moist forest potential
dry interior woodland/scrub riparian corridor
wet meadow / marsh          basin wetland
dry rocky slope
```

## Census across the 64 km region

| Regime | Cells |
|---|---:|
| Alpine barren | 9302 |
| Alpine tundra | 1002 |
| Riparian corridor | 16509 |
| Basin wetland | 7078 |
| Dry interior woodland | 3554 |
| Cool moist forest | 19003 |
| Dry rocky slope | 5675 |
| Wrap RMS vs +4096 m | 187.631 m |

## Six fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Alpine elevation control | **PASS** — high cold + thin regolith → alpine/barren; lapse-off changes classification |
| 2 Windward vs rain-shadow | **PASS** — windward wetter-frac **1.000** vs leeward **0.019** |
| 3 Riparian / valley control | **PASS** — riparian distinct from slope **0.669**; 16509 riparian cells |
| 4 Wet basin | **PASS** — 7078 basin wetland; `basin_wetland` / `saturated` |
| 5 H2H identity | **PASS** — 64 km → 192 m → 12.5 cm agree; biome `7f913de36bdf66e3`; 289/289 at 12.5 cm |
| 6 MW8-off == exact MW7 | **PASS** — max \|ΔZ\| **0.000000 m**; hydroclimate / regolith / bodies / FormationId identity |

## Mandatory counterfactuals (proves MW8 is caused, not another noise field)

| Counterfactual | Result |
|---|---|
| Hydroclimate neutralized → wet/dry contrast collapses | **PASS** — `hydro_off_contrast` **0.000** (windward/leeward split disappears) |
| Regolith neutralized → substrate/drainage contrast collapses | **PASS** — riparian/wetland substrate contrast disappears |

## Terminal-branch coverage guard

`check.terminal_moisture_split_expresses_hydroclimate` — **PASS**. The ordinary
temperate moisture split (`TerminalMoistureRegime`) is exercised directly: a
temperate interior cell with real hydroclimate, not windward and not wet, must
resolve to the drier regime (`dry_interior_woodland`); windward, wet, and
no-hydroclimate inputs stay `cool_moist_forest`. No cell in the 64 km fixture
reaches the dry arm, so this guard is what proves the MW6 wet/dry terminal
decision cannot silently collapse to the wet regime on both sides. The guard
fails against the prior no-op ternary and passes after the one-line correction;
the region census is byte-identical either way (branch unreachable in the
normal fixture).

## Horizon-to-Hand ecological ladder

| Rung | Result |
|---|---|
| 64 km biome map | PASS — 3969 samples |
| Watershed → hydroclimate → RegolithId → BiomeId | PASS — biome `7f913de36bdf66e3` |
| 192 m local region | PASS — 81 / absolute BiomeId |
| 12.5 cm sample | PASS — 289/289 same biome + deposit provenance (`D05_BASIN_FILL` ≠ host `S01_BASIN_FILL`) |

## Visual readability

Diagnostic ecological-regime visualization only (no trees, grass, shrubs,
snowpack, or animals): cold high ridges → subalpine → moist windward slopes →
riparian valley corridors → wet basin floor, versus dry leeward slopes / basin
margins. Zones track ridges, drainage, rain shadow, and soils.

Player visual: `MW8_BIOMES_PLAYER_VISUAL PASS`. Image
`Docs/provenance_mw8_biomes_player_view.ppm`. Receipt
`Docs/provenance_mw8_biomes_visual_cert.txt`.

## Analytical / Test A / soak

Analytical (`--cert-mw8-biomes`) — **PASS 62/62**. Receipt
`Docs/provenance_mw8_biomes_cert.txt`. Determinism budget 1 == N; MW8-off ==
exact MW7; ordinary travel does not rebuild MW8; absolute-coordinate source (no
4096 m wrap); every parent off-path identity preserved.

Test A (`--cert-worldgen-cardinal-replacement-mw8`) — **PASS 4/4**. Receipt
`Docs/provenance_mw8_cardinal_replacement_cert.txt`. Resident-package digest
`38afaf06ee7edb2b` origin==return, 2601 packages, N/E/S/W exact return,
`min_complete_radius` 192 m at origin/outer/return. Movement frames over
16.667 = **0** (worst movement ~5.9 ms). Sky 0, fallback 0. `wake_grass_loads`
/ `wake_tree_loads` = 0.

Test B 90 s NE fly (`--cert-streaming-soak-mw8`) — **PASS**. Receipt
`Docs/provenance_mw8_streaming_soak_cert.txt`. 0 frames >16.667, max ~6.1 ms,
2601 resident, pending drains to 0. `mw8_rebuilds` = 0, `mw8_compiles` = 0,
MW1–MW8 travel physics **idle**. 300/900 not run — MW8 adds no persistent
background workload.

## CLOSED (hard)

```
LOCAL CAUSAL CHAIN
3A–3B.3B                         CERTIFIED
16C.1 compiled classification    CERTIFIED

DEFERRED
3C structural collapse           CLOSED
16C runtime remobilization       CLOSED

MACRO WORLDGEN
MW1 provinces / belts / basins   CERTIFIED / FROZEN @ 569269aa
MW2 regional 3D geology          CERTIFIED / FROZEN @ 6ada7260
MW3 regional erosion             CERTIFIED / FROZEN @ 0274e22e
MW4 drainage / valleys           CERTIFIED / FROZEN @ cc7fe436
MW5 depositional landscape       CERTIFIED / FROZEN @ ba6faf61
MW6 hydroclimate                 CERTIFIED / FROZEN @ e8704845
MW7 soils / regolith             CERTIFIED / FROZEN @ 307e92fc
MW8 biomes                       CERTIFIED / FROZEN @ e8155fa3
MW9 flora / fauna                CLOSED
```

**Stop before MW9.** MW8 is the boundary between environmental authority and
living-world population; MW9 is a deliberate separate green-light. Also closed:
live weather / rainfall, glaciers, live snowpack, groundwater, ecology
simulation, vegetation placement, live P5b remobilization, 16D play ownership.

## Player runtime

```
PLAY_MW8_BIOMES.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw8-biomes
```

Cert:

```
CERT_MW8_BIOMES.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw8-biomes
Build\x64_Release\ProvenanceClient.exe --cert-mw8-biomes-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw8
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw8 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.BIOMES` / "MW8 - Biomes".
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4 CERTIFIED  MW5 CERTIFIED  MW6 CERTIFIED  MW7 CERTIFIED  MW8 CERTIFIED  MW9 CLOSED`.
