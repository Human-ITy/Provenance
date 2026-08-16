# MW5 — Depositional Landscape

MW5 answers one question: where does the material removed from the mountain
actually accumulate, and how does that accumulation reshape valleys, basin
margins, fans, floodplains, and lowlands?

**Law:** Deposition consumes certified MW3/MW4 export mass. It does not invent
sediment, merge into host FormationId, or overwrite the MW4 surface when off.

```
    MW1  province / uplift / basin
            ↓
    MW2  3D geology
            ↓
    MW3  differential erosion
            ↓
    MW4  drainage / valley hierarchy
            ↓
    MW5  depositional landscape
            ↓
    (closed) MW6 hydroclimate
```

Forbidden: hydroclimate / rainfall / snow / glacier (MW6); soils / biomes /
flora (MW7–MW9); live P5b remobilization; 16D play ownership; painted
floodplains without mass; FormationId merge; invented sediment; 3C collapse;
16C runtime remobilization; 4096 m tile as a sediment source.

Production source is **absolute-coordinate 64 km regional authority**
(`[-32000, 32000]²` m). Deposit body IDs are scoped by world identity +
facies + watershed (or quantized cluster for colluvium) so a fan, basin-fill
body, or trunk-valley package can accumulate across neighboring windows.

## This cut

SHA: `ba6faf61` (`ba6faf61b0159a670191b05e1a68363645341b8a`).

Freeze parents:

```
MW4     drainage / valleys                CERTIFIED / FROZEN @ cc7fe436
        full                              cc7fe436df53f5d85111f7e913bb3203951d4944
        stamp                             61e09506
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

MW5-off == exact MW4 present surface (max |ΔZ| **0.000000 m**; off digest
`0be36c3223a3dd00` == MW4 parent digest). Frozen 16C.1 local present
`2eb519c42ca5bd6b`. 16C sediment `1367584c41aedcdf` / geometry
`7ae7aa62c50489e2`. 16D water truth unchanged. MW4-off still == exact MW3.
MW3-off still == exact MW2. MW1 uplift-off still flattens the shape field
(relief **0.000 m**).

Descriptor: `Data/Worldgen/causal_world_regional_deposition_floor.cdp`.
Seed `mw5-64km-01`. Field digest `7aaa9ca7ecef8029`. Parent surface
`652f49850d59c5e7`. Deposit surface `7014bcb7b4a1e59c`.

## What the 64 km landscape reads as vs MW4

MW4 left organized erosional valleys and exported the extra incision. MW5
places that mass (plus MW3 denudation export) as depositional bodies:

```
steep erosional uplands
├─ confined upper valleys
├─ widening lower-valley fill / floodplain
├─ mountain-front fans where gradient collapses
├─ terraces on abandoned higher surfaces
└─ smoother compacted basin floors
```

| Inventory | Count |
|---|---:|
| Deposit bodies | 354 |
| Fan cells | 94 |
| Valley-fill cells | 384 |
| Floodplain cells | 17694 |
| Bar cells | 40 |
| Basin-fill cells | 10727 |
| Terrace cells | 456 |
| Colluvial cells | 1354 |
| Loose / settled / compacted | 16661 / 6741 / 7347 |
| Wrap RMS vs +4096 m | 187.631 m |

Max / RMS raise vs MW4 **72.000 / 11.732 m**. Fan max **62.726 m** (slope-break
off **0.000 m**). Basin mean fill **16.795 m** vs belt **4.118 m**. Valley
mean **1.928 m** vs low-supply **0.734 m**.

Deposits use 16C.1 / 3B.3B vocabulary and stay distinct from host geology.
A basin-floor package is `D05_BASIN_FILL` / compacted deposit sourced from
upstream `B03_HOST_UPPER` (and other FormationIds) — not `S01_BASIN_FILL`
host sandstone.

## Mass account

Upland mass loss becomes accounted-for depositional or exported mass.
HOST geology grams do not absorb deposit grams.

| Account | Grams |
|---|---:|
| MW3 export | 87936795832068944.000 |
| MW4 export | 29749478927319940.000 |
| Source (MW3+MW4) | 117686274759388880.000 |
| Deposited | 50300761198638112.000 |
| Boundary export | 67385513560750768.000 |
| Residual | 0.000000 |
| Host geology grams | 0.000 |

`source == deposited + export`. Residual **0**. No invented sediment.

## Six fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Mountain-front fan | **PASS** — confined high-gradient exit dumps a fan (max **62.726 m**); slope-break-off fan max **0.000 m** |
| 2 Valley-floor aggradation | **PASS** — excess-supply trunk floor **1.928 m**; low-supply control **0.734 m** (stays more incised) |
| 3 Basin fill | **PASS** — foreland basin mean **16.795 m** vs belt **4.118 m**; tectonic accommodation + belt erosion |
| 4 Provenance H2H | **PASS** — watershed `af35c28e0c54b26c` → body `4456c17b822849c3` → 12.5 cm **289/289**; class compacted; source `B03_HOST_UPPER`; host `S01_BASIN_FILL`; not a FormationId merge |
| 5 Mass closure | **PASS** — source == deposit + export; residual **0**; host grams **0** |
| 6 MW5-off == exact MW4 | **PASS** — max \|ΔZ\| **0.000000 m**; digest identity (`0be36c3223a3dd00`) |

## Horizon-to-Hand provenance ladder

| Rung | Result |
|---|---|
| 64 km source / deposit map | PASS — 3969 samples |
| Specific watershed | PASS — `af35c28e0c54b26c` |
| Source FormationId(s) | PASS — mask `000000000000001f` |
| Transport path → body | PASS — `4456c17b822849c3` |
| 12.5 cm sample | PASS — 289/289 same body + class + source; `D05_BASIN_FILL` ≠ host `S01_BASIN_FILL` |

## Visual readability

64 km metric pack (not a beauty pass):

- Steep erosional uplands vs depositional lowlands (belt Z well above basin)
- Confined upper valleys; widening lower-valley / floodplain fill
- Fans where gradient collapses (94 cells, max **62.7 m**)
- Smoother compacted basin floors (mean **16.8 m**)
- Terraces on older abandoned surfaces (456 cells)
- Lithology/provenance on deposited material (shale-rich fan at the camera)

Player visual: `MW5_DEPOSITIONAL_LANDSCAPE_PLAYER_VISUAL PASS` — 90 settled
frames, 2601 packages, 0 lower-frame sky, rebuilds 1, wrap 0. Camera on
`D05_FAN` (shale-derived). Image
`Docs/provenance_mw5_depositional_landscape_player_view.ppm`.
Silhouette vs MW4: max |ΔZ| **72 m**, RMS raise **11.7 m**.

## Additional fixtures

| Fixture | Result |
|---|---|
| Determinism budget 1 == N | **PASS** |
| MW5-off == exact MW4 | **PASS** |
| Ordinary travel does not rebuild MW5 | **PASS** — rebuilds stay 1 after load |
| No wrap (not 4096 m tile) | **PASS** |
| MW4-off still exact MW3 | **PASS** |
| MW3-off still exact MW2 | **PASS** |
| MW1 uplift-off still relief 0 | **PASS** — **0.000 m** |
| Frozen 16C.1 local present on off path | **PASS** |

## Visual / Test A / soak

Visual: `MW5_DEPOSITIONAL_LANDSCAPE_PLAYER_VISUAL PASS`. Receipt
`Docs/provenance_mw5_depositional_landscape_visual_cert.txt`.

Test A (`--cert-worldgen-cardinal-replacement-mw5`) — **PASS**. Receipt
`Docs/provenance_mw5_cardinal_replacement_cert.txt`.
Digest `bdef220660da3b68` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~9.7 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw5_streaming_soak_cert.txt`. 0 / 83025 frames >16.667,
max 6.750 ms, 2601 resident, pending 0. MW1–MW5 travel physics **idle**
(rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW5 background workload.

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
MW6 hydroclimate                 CLOSED
MW7 soils / regolith             CLOSED
MW8 biomes                       CLOSED
```

Also closed: live P5b remobilization, 16D play ownership, formation-ID merge,
invented sediment, painted floodplains, certified rainfall / flora.

## Player runtime

```
PLAY_MW5_DEPOSITIONAL_LANDSCAPE.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw5-depositional-landscape
```

Cert:

```
CERT_MW5_DEPOSITIONAL_LANDSCAPE.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw5-depositional-landscape
Build\x64_Release\ProvenanceClient.exe --cert-mw5-depositional-landscape-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw5
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw5 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.DEPOSITIONAL_LANDSCAPE` /
“MW5 - Depositional Landscape”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4 CERTIFIED  MW5 CERTIFIED  MW6-MW8 CLOSED`.
CLOSED rows visible, not launchable.
