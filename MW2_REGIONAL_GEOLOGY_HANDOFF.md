# MW2 — Regional 3D Geology

MW2 answers one question: given the certified MW1 mountain belt / foreland
basin / hinterland scaffold, can we generate persistent 3D formations,
intrusions, faults, bedding, and chronology that remain coherent from
regional scale down to 12.5 cm matter?

**Law:** Surface exposure is the intersection of the existing MW1 present
surface with those bodies. MW2 does not carve valleys, rewrite drainage,
or open a compiled erosion solver.

```
    MW1 province / belt / basin
            ↓
    MW2 regional 3D geology
            ↓
    (closed) MW3 erosion → MW4 drainage → MW5 deposition
```

Forbidden: noise → paint geology afterward; host-formation merge;
Voronoi biome painter; 3C collapse; 16C remobilization.

Production source is **absolute-coordinate 64 km regional authority**.
The 4096 m Stage-15 wrap is not used here. Off / no-MW2 leaves the
MW1-only (or frozen local 16C.1) path untouched.

## This cut

SHA: `6ada7260` (`6ada7260511f2144f7fc9de4f23fb8fc2f877f82`).

Freeze parents:

```
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

Off / no-MW2 reproduces frozen 16C.1 local present `2eb519c42ca5bd6b`.
16C sediment `1367584c41aedcdf` / geometry `7ae7aa62c50489e2`. 16D water
truth unchanged. MW1 uplift-off still flattens the shape field (relief
**0.000 m**).

## Body inventory

Keyed to MW1 provinces. Query is world XY + Z → FormationId / chronology /
structure.

| Id | Role | Province | Material | Chronology |
|---|---|---|---|---:|
| H01_BASEMENT | older coherent basement | hinterland (and deep belt) | granite | 5 |
| B01_HOST_LOWER | deformed layered host | mountain belt | shale | 10 |
| B02_HOST_MIDDLE | deformed layered host | mountain belt | sandstone | 12 |
| B03_HOST_UPPER | deformed layered host | mountain belt | shale | 16 |
| S01_BASIN_FILL | thickening sedimentary package / unconformity fill | foreland basin | sandstone | 25 |
| I01_PLUTON | intrusion cutting host | mountain belt | granite | 40 |

Regional fault event chronology 30 displaces older bodies without rewriting
identity. Intrusion is younger than host; the contact is a cut, not a merge.
Bedding strike follows MW1 structural azimuth **28°**.

Body digest `d97694fdfad43f7a`. Field digest `20de896183c028c5`.
Seed `mw2-64km-01`. Descriptor:
`Data/Worldgen/causal_world_regional_geology_floor.crg`.

## Counterfactuals (fail-closed)

| Fixture | Result |
|---|---|
| Deformation off | **PASS** — bedding tilt 0; fault structural expression collapses; MW1 shape relief stays **1631 m** (not flattened) |
| Intrusion off | **PASS** — I01_PLUTON and contact effects disappear |
| Fault displacement off | **PASS** — formation continuity restores across the fault |
| MW1 uplift/deformation off | **PASS** — shape-field relief **0.000 m** (MW1 certification intact) |

## Horizon-to-Hand FormationId ladder

Forced through existing occupancy / sample / digest machinery:

| Rung | Result |
|---|---|
| 64 km map | PASS — 3969 formation samples |
| Mountain belt → specific ridge | PASS |
| Exposed formation | **B02_HOST_MIDDLE** |
| Buried continuation (same XY, −25 m; along-strike same local Z) | **B02_HOST_MIDDLE** same FeatureId / chronology |
| 12.5 cm materialization | PASS — 289/289 same FormationId |

## Additional fixtures

| Fixture | Result |
|---|---|
| Layered host follows MW1 azimuth 28° | **PASS** — bedding azimuth 28.000° |
| Regional fault displaces, identity preserved | **PASS** |
| Intrusion cuts host, younger chronology | **PASS** |
| Basin package + unconformity, distinct from belt/hinterland | **PASS** |
| Hinterland basement coherent through depth | **PASS** |
| Determinism budget 1 == N | **PASS** |
| Off / no-MW2 frozen 16C.1 present | **PASS** |
| No wrap (not 4096 m tile) | **PASS** |
| Ordinary sample does not rebuild volumes | **PASS** — rebuilds stay 1 after load |

## Visual / Test A / soak

Visual: `MW2_REGIONAL_GEOLOGY_PLAYER_VISUAL PASS` — 90 settled frames, 2601
packages, 0 lower-frame sky, rebuilds 1, wrap 0. Exposed
`B03_HOST_UPPER`.

Test A (`--cert-worldgen-cardinal-replacement-mw2`) — **PASS**. Receipt
`Docs/provenance_mw2_cardinal_replacement_cert.txt`.
Digest `09b4bccd6f04f59d` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~5.1 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw2_streaming_soak_cert.txt`. 0 / 88066 frames >16.667,
max 5.153 ms, 2601 resident, pending 0. MW1/MW2 travel physics **idle**
(rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW2 background workload.

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
MW3 regional erosion             CLOSED
MW4 valley/drainage morphology   CLOSED
MW5 depositional landscape       CLOSED
MW6 hydroclimate                 CLOSED
MW7 soils                        CLOSED
MW8 biomes                       CLOSED
```

Also closed: Voronoi/splat biome painter, cementation / lithification /
formation merge, 16C mass-routing rewrite, 16D water-truth rewrite.

## Player runtime

```
PLAY_MW2_REGIONAL_GEOLOGY.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw2-regional-geology
```

Cert:

```
CERT_MW2_REGIONAL_GEOLOGY.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw2-regional-geology
Build\x64_Release\ProvenanceClient.exe --cert-mw2-regional-geology-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw2
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw2 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.REGIONAL_3D_GEOLOGY` /
“MW2 - Regional 3D Geology”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3-MW8 CLOSED`.
CLOSED rows visible, not launchable.
