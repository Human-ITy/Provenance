# MW7 — Soils / Regolith

MW7 answers one question: given host geology, depositional history, terrain
position, drainage, and hydroclimate, what physically distinct near-surface
material profile develops at each place?

**Law:** Soils / regolith is a derived near-surface state/profile on certified
MW2–MW6 bodies. It is not host geology, not a depositional body, not a
terrain carver, and not vegetation.

```
    MW1  tectonic provinces
            ↓
    MW2  parent material / FormationId
            ↓
    MW3  exposure / erosional position
            ↓
    MW4  slope / drainage / watershed position
            ↓
    MW5  depositional body + depositional class
            ↓
    MW6  temperature / moisture / wetness / aridity / runoff
            ↓
    MW7  soils / regolith
            ↓
    (closed) MW8 biomes
```

Forbidden: MW8 biomes; MW9 flora/fauna; vegetation placement; biome color
paint; glaciers / live snowpack; live weather; groundwater flow; ecology;
3C collapse; 16C remobilization; host FormationId merge; new sediment;
terrain geometry mutation; hydroclimate rewrite; live P5b / 16D play
ownership.

Production source is **absolute-coordinate 64 km regional authority**
(`[-32000, 32000]²` m). RegolithId is keyed by world identity + absolute
quantized coordinates, not the certification box, so neighboring macro
windows can agree.

## This cut

SHA: `307e92fc` (`307e92fc41d9654f6994ea3ec4daf27f4ddab797`).

Freeze parents:

```
MW6     hydroclimate                      CERTIFIED / FROZEN @ e8704845
        full                              e8704845a00c9945423b791b79f73c3fa6e5a48b
        stamp                             457df7a7
MW5     depositional landscape            CERTIFIED / FROZEN @ ba6faf61
        stamp                             30a7b89c
MW4     drainage / valleys                CERTIFIED / FROZEN @ cc7fe436
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

MW7-off == exact MW6 present surface (max |ΔZ| **0.000000 m**; on |ΔZ|
**0.000000 m**). Hydroclimate digest identity `d523ab620db18280`. MW5 mass
closure remains exact. MW6-off still == exact MW5. MW5-off still == exact
MW4. MW4-off still == exact MW3. MW3-off still == exact MW2. MW1 uplift-off
still flattens the shape field (relief **0.000 m**). Frozen 16C.1 local
present `2eb519c42ca5bd6b`.

Descriptor: `Data/Worldgen/causal_world_regional_regolith_floor.crr`.
Seed `mw7-64km-01`. Field digest `b0bc61d730e6085f`. Parent surface
`27639dc50eb099ec`. Input revision `1880c761ed17fbf6`.

## What profiles mean across the 64 km region

Compiled near-surface material profile on the certified MW1–MW6 landscape —
not dirt labels and not a biome paint:

```
alpine ridge / exposed belt   thin regolith / frequent bedrock
valley floor / floodplain     deeper alluvial / floodplain sediment
windward flank                deeper / higher water-holding
rain-shadow slope             thinner / drier than windward control
wet enclosed basin            poorly drained / saturated mineral
```

A 12.5 cm sample in an MW5 deposit retains the depositional body's source
ancestry. Profile class may be basin fill / alluvium / floodplain sediment.
It does not become generic soil and does not overwrite host FormationId.

| Inventory | Count |
|---|---:|
| Bedrock / weathered | 15251 |
| Alluvium cells | 474 |
| Floodplain cells | 17694 |
| Basin-fill profile cells | 2802 |
| Waterlogged mineral cells | 7969 |
| Poorly drained cells | 9173 |
| Saturated cells | 8832 |
| Alpine / valley mean depth | 0.232 / 1.446 m |
| Windward / leeward depth | 0.314 / 0.154 m |
| Windward / leeward water-hold | 0.255 / 0.091 |
| Basin poorly-drained fraction | 0.876 |
| Wrap RMS vs +4096 m | 187.631 m |

## Six fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Alpine ridge | **PASS** — alpine/ridge mean depth **0.232 m** vs valley **1.446 m**; 15251 bedrock/weathered cells |
| 2 Valley floor / floodplain | **PASS** — 17694 floodplain + 474 alluvium; deeper than alpine by **1.214 m** |
| 3 Rain-shadow slope | **PASS** — leeward depth **0.154 m** / hold **0.091** vs windward **0.314 m** / **0.255** |
| 4 Wet basin | **PASS** — poorly drained **9173** + saturated **8832**; basin poor-frac **0.876** |
| 5 H2H deposit provenance | **PASS** — body `4456c17b822849c3`; 12.5 cm **289/289**; class `basin_fill`; source `B03_HOST_UPPER`; host `S01_BASIN_FILL`; deposit `D05_BASIN_FILL` |
| 6 MW7-off == exact MW6 | **PASS** — max \|ΔZ\| **0.000000 m**; hydroclimate / bodies / FormationId identity |

## Horizon-to-Hand deposit provenance ladder

| Rung | Result |
|---|---|
| 64 km source / profile map | PASS — 3969 samples |
| MW4 watershed / MW5 body | PASS — body `4456c17b822849c3` |
| Source FormationId(s) | PASS — `B03_HOST_UPPER` / mask `000000000000001f` |
| MW7 profile on that body | PASS — `basin_fill` / poorly drained |
| 192 m local region | PASS — 81 / absolute RegolithId |
| 12.5 cm sample | PASS — 289/289 same DepositBodyId + source + `D05_BASIN_FILL` ≠ host `S01_BASIN_FILL` |

## Visual readability

Diagnostic profile visualization only (not biomes / vegetation):

- Alpine ridges thin / bedrock-exposed (`regolith_bedrock` / `regolith_thin`)
- Valley floors deeper alluvium / floodplain (`regolith_alluvium` / `regolith_floodplain`)
- Windward vs rain-shadow profile contrast
- Wet basin poorly drained (`regolith_waterlogged` / `regolith_basin`)

Player visual: `MW7_SOILS_REGOLITH_PLAYER_VISUAL PASS` — 90 settled frames,
2601 packages, 0 lower-frame sky, rebuilds 1, wrap 0. Image
`Docs/provenance_mw7_soils_regolith_player_view.ppm`.

## Additional fixtures

| Fixture | Result |
|---|---|
| Determinism budget 1 == N | **PASS** |
| MW7-off == exact MW6 | **PASS** |
| Ordinary travel does not rebuild MW7 | **PASS** — rebuilds stay 1 after load |
| No wrap (not 4096 m tile) | **PASS** |
| MW6-off still exact MW5 | **PASS** |
| MW5-off still exact MW4 | **PASS** |
| MW4-off still exact MW3 | **PASS** |
| MW3-off still exact MW2 | **PASS** |
| MW1 uplift-off still relief 0 | **PASS** — **0.000 m** |
| Frozen 16C.1 local present on off path | **PASS** |

## Visual / Test A / soak

Visual: `MW7_SOILS_REGOLITH_PLAYER_VISUAL PASS`. Receipt
`Docs/provenance_mw7_soils_regolith_visual_cert.txt`.

Test A (`--cert-worldgen-cardinal-replacement-mw7`) — **PASS**. Receipt
`Docs/provenance_mw7_cardinal_replacement_cert.txt`.
Digest `983e0d8f7014d5ea` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~5.8 ms).
Regolith query identity returns exactly after eviction/reload.

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw7_streaming_soak_cert.txt`. 0 / 84043 frames >16.667,
max 11.158 ms, 2601 resident, pending 0. MW1–MW7 travel physics **idle**
(rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW7 background workload.

## CLOSED (hard)

```
LOCAL CAUSAL CHAIN
3A–3B.3B                         CERTIFIED
16C.1 compiled classification    CERTIFIED

DEFERRED
3C structural collapse           CLOSED
16C runtime remobilization       CLOSED

MACRO WORLDGEN
MW1 provinces / belts / basins   CERTIFIED
MW2 regional 3D geology          CERTIFIED / FROZEN @ 6ada7260
MW3 regional erosion             CERTIFIED / FROZEN @ 0274e22e
MW4 drainage / valleys           CERTIFIED / FROZEN @ cc7fe436
MW5 depositional landscape       CERTIFIED / FROZEN @ ba6faf61
MW6 hydroclimate                 CERTIFIED / FROZEN @ e8704845
MW7 soils / regolith             CERTIFIED / FROZEN @ 307e92fc
MW8 biomes                       CLOSED
```

Also closed: live weather / rainfall, glaciers, live snowpack, groundwater,
ecology, vegetation placement, live P5b remobilization, 16D play ownership.

## Player runtime

```
PLAY_MW7_SOILS_REGOLITH.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw7-soils-regolith
```

Cert:

```
CERT_MW7_SOILS_REGOLITH.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw7-soils-regolith
Build\x64_Release\ProvenanceClient.exe --cert-mw7-soils-regolith-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw7
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw7 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.SOILS_REGOLITH` / “MW7 - Soils / Regolith”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4 CERTIFIED  MW5 CERTIFIED  MW6 CERTIFIED  MW7 CERTIFIED  MW8 CLOSED`.
CLOSED rows visible, not launchable.
