# MW6 — Hydroclimate

MW6 answers one question: given the certified mountain belt, valleys, basins,
and depositional landscape, what long-term temperature and moisture regime
does each place experience, and why?

**Law:** Hydroclimate is compiled from absolute position, solar/latitude
forcing, elevation lapse, prevailing moisture transport, topographic
exposure, orographic uplift, rain shadow, and basin/valley geometry. It does
not paint climate zones, mutate terrain, or simulate weather.

```
    MW1  tectonic provinces / relief forcing
            ↓
    MW2  regional 3D geology
            ↓
    MW3  differential erosion
            ↓
    MW4  watershed / valley morphology
            ↓
    MW5  depositional landscape
            ↓
    MW6  hydroclimate
            ↓
    (closed) MW7 soils / regolith
    (closed) MW8 biomes
```

Forbidden: MW7 soils; MW8 biomes; MW9 flora/fauna; glacier geometry; live
snowpack; live rainfall/weather; groundwater; ecology; 3C collapse; 16C
runtime remobilization; host formation changes; new sediment; terrain
mutation; live P5b / 16D play ownership.

Production source is **absolute-coordinate 64 km regional authority**
(`[-32000, 32000]²` m). HydroclimateId is keyed by world identity +
absolute quantized coordinates, not the certification box, so neighboring
macro windows can agree. Prevailing moisture azimuth is a larger-system
transport direction (298° toward the tectonic foreland), not a 64 km box
edge.

## This cut

SHA: `e8704845` (`e8704845a00c9945423b791b79f73c3fa6e5a48b`).

Freeze parents:

```
MW5     depositional landscape            CERTIFIED / FROZEN @ ba6faf61
        full                              ba6faf61b0159a670191b05e1a68363645341b8a
        stamp                             30a7b89c
MW4     drainage / valleys                CERTIFIED / FROZEN @ cc7fe436
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

MW6-off == exact MW5 present surface (max |ΔZ| **0.000000 m**; on |ΔZ|
**0.000000 m**). Frozen 16C.1 local present `2eb519c42ca5bd6b`. MW5 mass
closure remains exact. MW5-off still == exact MW4. MW4-off still == exact
MW3. MW3-off still == exact MW2. MW1 uplift-off still flattens the shape
field (relief **0.000 m**).

Descriptor: `Data/Worldgen/causal_world_regional_hydroclimate_floor.chc`.
Seed `mw6-64km-01`. Field digest `d523ab620db18280`. Parent surface
`deaf8426c4679e7e`. Input revision `1bb9f0daa5fa8690`.

## What hydroclimate means across the 64 km region

Compiled long-term climatic state on the certified MW5 surface — not weather
events and not a biome paint:

```
windward mountain flank   wetter, orographic gain
alpine high terrain       colder, snow-persistence potential
leeward / rain shadow     drier after the barrier
foreland basin / valleys  sheltered cold-pool tendency
```

| Inventory | Count |
|---|---:|
| Alpine-cold cells | 10300 |
| Windward-wet cells | 1055 |
| Leeward-dry cells | 10998 |
| Basin cold-pool cells | 9825 |
| Valley-sheltered cells | 1505 |
| High / low mean temp | 0.125 / 8.368 °C |
| High / low snow potential | 0.661 / 0.000 |
| Windward / leeward wetness | 2.254 / 0.575 |
| Ridge / basin cold-pool | 0.038 / 0.659 |
| Wrap RMS vs +4096 m | 187.631 m |

MW4 drainage is not rewritten. MW6 publishes runoff *potential* onto the
certified watershed tree.

## Six fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Elevation lapse | **PASS** — high **0.125 °C** vs lowland **8.368 °C**; snow **0.661** vs **0.000**; lapse-off contrast collapses to **0.071 °C** |
| 2 Windward orographic | **PASS** — windward wetness **2.254** vs lowland/leeward **0.575** |
| 3 Leeward rain shadow | **PASS** — leeward drier; barrier-off contrast collapses to **0** |
| 4 Basin / valley | **PASS** — basin cold-pool **0.659** vs ridge **0.038**; 9825 basin + 1505 valley cells |
| 5 H2H provenance | **PASS** — 3969 / 81 / 289; hydroclimate `8c01f85ae179b713`; watershed `af35c28e0c54b26c` |
| 6 MW6-off == exact MW5 | **PASS** — max \|ΔZ\| **0.000000 m**; digest identity (`deaf8426c4679e7e`) |

## Horizon-to-Hand hydroclimate ladder

| Rung | Result |
|---|---|
| 64 km hydroclimate map | PASS — 3969 samples |
| MW1 province / MW4 watershed | PASS — `af35c28e0c54b26c` |
| MW5 landform context | PASS |
| Hydroclimate regime | PASS — `alpine_cold` / windward |
| 192 m local region | PASS — 81 / absolute identity |
| 12.5 cm sample | PASS — 289/289 same HydroclimateId |

## Visual readability

Diagnostic climate visualization only (not biomes / snowpacks / glaciers):

- High terrain colder (`climate_cold`)
- Windward wetter (`climate_wet`)
- Leeward drier (`climate_dry`)
- Basin / valley distinct (`climate_pool`)

Player visual: `MW6_HYDROCLIMATE_PLAYER_VISUAL PASS` — 90 settled frames,
2601 packages, 0 lower-frame sky, rebuilds 1, wrap 0. Image
`Docs/provenance_mw6_hydroclimate_player_view.ppm`.

## Additional fixtures

| Fixture | Result |
|---|---|
| Determinism budget 1 == N | **PASS** |
| MW6-off == exact MW5 | **PASS** |
| Ordinary travel does not rebuild MW6 | **PASS** — rebuilds stay 1 after load |
| No wrap (not 4096 m tile) | **PASS** |
| MW5-off still exact MW4 | **PASS** |
| MW4-off still exact MW3 | **PASS** |
| MW3-off still exact MW2 | **PASS** |
| MW1 uplift-off still relief 0 | **PASS** — **0.000 m** |
| Frozen 16C.1 local present on off path | **PASS** |

## Visual / Test A / soak

Visual: `MW6_HYDROCLIMATE_PLAYER_VISUAL PASS`. Receipt
`Docs/provenance_mw6_hydroclimate_visual_cert.txt`.

Test A (`--cert-worldgen-cardinal-replacement-mw6`) — **PASS**. Receipt
`Docs/provenance_mw6_cardinal_replacement_cert.txt`.
Digest `0720123f96a82229` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~4.7 ms).
Hydroclimate identity returns exactly after eviction/reload.

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw6_streaming_soak_cert.txt`. 0 / 84699 frames >16.667,
max 8.999 ms, 2601 resident, pending 0. MW1–MW6 travel physics **idle**
(rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW6 background workload.

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
MW7 soils / regolith             CLOSED
MW8 biomes                       CLOSED
```

Also closed: live weather / rainfall, glaciers, live snowpack, groundwater,
ecology, live P5b remobilization, 16D play ownership, Voronoi climate paint.

## Player runtime

```
PLAY_MW6_HYDROCLIMATE.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw6-hydroclimate
```

Cert:

```
CERT_MW6_HYDROCLIMATE.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw6-hydroclimate
Build\x64_Release\ProvenanceClient.exe --cert-mw6-hydroclimate-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw6
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw6 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.HYDROCLIMATE` / “MW6 - Hydroclimate”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4 CERTIFIED  MW5 CERTIFIED  MW6 CERTIFIED  MW7-MW8 CLOSED`.
CLOSED rows visible, not launchable.
