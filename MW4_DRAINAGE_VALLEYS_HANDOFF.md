# MW4 — Drainage / Valleys

MW4 answers one question: given the certified MW3 denuded surface and MW2
structure, can compiled watersheds and valley systems organize that surface
without inventing a second river-noise layer?

**Law:** Drainage follows the MW3 surface and MW2 structure. It does not
overwrite them. Off / no-MW4 leaves the exact MW3 present surface untouched.

```
    MW1  regional uplift / basin structure
            ↓
    MW2  persistent 3D geology
            ↓
    MW3  differential erosion
            ↓
    MW4  watershed + valley organization
            ↓
    (closed) MW5 depositional landscape
```

Forbidden: random grooves; independent river-noise; baking premature
channels that silently rewrite parent Z when MW4 is off; live P5b water
rewrite; 16D play ownership; MW5 basin fill; MW6–MW8; 3C collapse;
16C remobilization.

Production source is **absolute-coordinate 64 km regional authority**
(`[-32000, 32000]²` m). The 4096 m Stage-15 wrap is not a drainage source.

## This cut

SHA: `cc7fe436` (`cc7fe436df53f5d85111f7e913bb3203951d4944`).

Freeze parents:

```
MW3     regional erosion                  CERTIFIED / FROZEN @ 0274e22e
        full                              0274e22e784f3c70a7b5af70dff5123a1644a089
        stamp                             b268025a
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

MW4-off == exact MW3 present surface (max |ΔZ| **0.000000 m**; off digest
`e4e59137d6b1ba16` == MW3 parent digest). Frozen 16C.1 local present
`2eb519c42ca5bd6b`. 16C sediment `1367584c41aedcdf` / geometry
`7ae7aa62c50489e2`. 16D water truth unchanged. MW3-off still == exact MW2.
MW1 uplift-off still flattens the shape field (relief **0.000 m**).

Descriptor: `Data/Worldgen/causal_world_regional_drainage_floor.crd`.
Seed `mw4-64km-01`. Field digest `bb660ea1a4439a06`. Parent surface
`cd182208fc449dbf`. Valley surface `b5e6ca2da40cc018`.

## What the 64 km landscape reads as

Organized watershed tree on the MW3 denuded mass — not grooves everywhere:

```
ridge
├─ first-order hollows
├─ tributary valleys
│   └─ confluences
└─ trunk valley
    └─ basin / outlet
```

| Inventory | Count |
|---|---:|
| Watersheds / outlets | 1000 |
| Channels | 6983 |
| Confluences | 159 |
| Closed basins / spills | 33 / 33 |
| Max Strahler order | 4 |
| Divide cells | 7166 |
| Trunks | 511 |
| Channel fraction | 0.1108 |
| Valley widen ratio | 3.117 |
| Wrap RMS vs +4096 m | 191.089 m |

Mean / max extra incision **2.719 / 40.938 m**. MW4-on max |ΔZ| vs MW3
**38.351 m**. Structural steering rewires the receiver graph (structure-off
digest `55925a3c5a015ee3`).

## Mass account

Organized incision on the MW3 surface — not a second mass-deletion carver.

| Account | Grams |
|---|---:|
| Valley removed | 29749478927319940.000 |
| Exported | 29749478927319940.000 |
| Residual | 0.000000 |

`removed == exported`. Final deposition remains MW5.

## Six fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Divide causality | **PASS** — opposite sides of a certified ridge route to different watersheds; flatten/remove that divide (2400 m punch) changes the drainage relationship |
| 2 Tributary hierarchy | **PASS** — Strahler-like order; max order **4**; small headwaters combine into fewer larger channels; deterministic |
| 3 Confluence geometry | **PASS** — 159 joins of `upstream A + upstream B → downstream trunk`; no unexplained channel birth/death |
| 4 Longitudinal consistency | **PASS** — trunks descend toward outlets; local steps only with explicit spill / closed-basin semantics (33 basins == 33 spills) |
| 5 Structural control | **PASS** — MW2 faults / bedding / contacts / basin margins steer receivers; StructureOff rewires the graph |
| 6 MW4-off == exact MW3 | **PASS** — max \|ΔZ\| **0.000000 m**; digest identity (`e4e59137d6b1ba16`) |

## Horizon-to-Hand drainage-identity ladder

All rungs agree on the same watershed `af35c28e0c54b26c`. Bank / bed
FormationId is host MW2 (`H01_BASEMENT`) — there is no MW5 deposit yet.

| Rung | Result |
|---|---|
| 64 km regional watershed | PASS — 3969 drainage samples |
| Major trunk valley | PASS |
| Tributary on the same watershed | PASS |
| Specific confluence | PASS |
| 192 m local terrain | PASS — 81 / same identity |
| 12.5 cm bank / bed | PASS — 289/289; FormationId **H01_BASEMENT** |

## Visual readability

64 km metric pack (not a beauty pass):

- Main divide + multiple drainage basins (1000 watersheds, 7166 divide cells)
- Dendritic / structurally controlled tributaries (order 1→4, not grooves)
- Trunk valleys with believable confluences (511 trunks, 159 joins)
- Valley widening downstream (ratio **3.117**)
- Basinward outlets (belt mean Z well above basin; wrap RMS **191 m**)

Player visual: `MW4_DRAINAGE_VALLEYS_PLAYER_VISUAL PASS` — 90 settled
frames, 2601 packages, 0 lower-frame sky, rebuilds 1, wrap 0. Interior
confluence camera on host granite (`H01_BASEMENT`). Image
`Docs/provenance_mw4_drainage_valleys_player_view.ppm`.

## Additional fixtures

| Fixture | Result |
|---|---|
| Determinism budget 1 == N | **PASS** |
| MW4-off == exact MW3 | **PASS** |
| Ordinary travel does not rebuild MW4 | **PASS** — rebuilds stay 1 after load |
| No wrap (not 4096 m tile) | **PASS** |
| MW3-off still exact MW2 | **PASS** |
| MW1 uplift-off still relief 0 | **PASS** — **0.000 m** |
| Frozen 16C.1 local present on off path | **PASS** |

## Visual / Test A / soak

Visual: `MW4_DRAINAGE_VALLEYS_PLAYER_VISUAL PASS`. Receipt
`Docs/provenance_mw4_drainage_valleys_visual_cert.txt`.

Test A (`--cert-worldgen-cardinal-replacement-mw4`) — **PASS**. Receipt
`Docs/provenance_mw4_cardinal_replacement_cert.txt`.
Digest `e7d0f813783e2e9f` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~4.9 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw4_streaming_soak_cert.txt`. 0 / 87851 frames >16.667,
max 5.083 ms, 2601 resident, pending 0. MW1/MW2/MW3/MW4 travel physics
**idle** (rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW4 background workload.

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
MW5 depositional landscape       CLOSED
MW6 hydroclimate                 CLOSED
MW7 soils / regolith             CLOSED
MW8 biomes                       CLOSED
```

Also closed: live P5b water rewrite, 16D play ownership, river-noise Z
overwrite, Voronoi/splat biome painter, certified rainfall / flora.

## Player runtime

```
PLAY_MW4_DRAINAGE_VALLEYS.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw4-drainage-valleys
```

Cert:

```
CERT_MW4_DRAINAGE_VALLEYS.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw4-drainage-valleys
Build\x64_Release\ProvenanceClient.exe --cert-mw4-drainage-valleys-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw4
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw4 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.VALLEY_DRAINAGE` /
“MW4 - Drainage / Valleys”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4 CERTIFIED  MW5-MW8 CLOSED`.
CLOSED rows visible, not launchable.
