# MW1 — Provinces / Mountain Belts / Basins

MW1 is the first cut of the macro program. It is the **geologic forcing
field**, not final planet terrain.

**Law:** Elevation is a consequence of regional structure, not the source
of structure.

```
province history
    → uplift / subsidence / deformation
    → persistent geology
    → (observed) drainage tendency
    → present topography
```

Forbidden: `noise → mountains → label geology afterward`.

Production source is **absolute-coordinate 64 km regional authority**.
The 4096 m Stage-15 wrap is a semantic continuity shortcut only; it is
not MW1 macro geography.

## This cut

SHA: pending (filled after commit on `provenance/client-spike`).

Freeze parents:

```
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

Off / no-MW1 reproduces frozen 16C.1 local present `2eb519c42ca5bd6b`,
16C sediment `1367584c41aedcdf`, geometry `7ae7aa62c50489e2`. 16C mass
routing and 16D water truth are unchanged.

## 64 km region

Analytical region `[-32000, 32000]²` m. Three compiled provinces:

| Id | Type | Role |
|---:|---|---|
| 1 | mountain_belt | elongated highland, azimuth 28°, uplift ~1100 m, grain 3200 m |
| 2 | foreland_basin | south of the mountain front, subsidence ~380 m |
| 3 | hinterland | north of the belt, modest uplift ~140 m |

NeighborRelations: `1↔2 mountain_front`, `1↔3 hinterland_front`.

Derived present Z = datum(160) + uplift − subsidence + structural grain
when forcing is on. No wrap. Along-strike orogen modulation so
Z(x) ≠ Z(x+4096).

Scaffold digest `8c17d992f259042d`. Field digest `0d05fff271a43aa5`.
Counterfactual digest `f8238e0926298dcc`. Seed `mw1-64km-01`.
Descriptor: `Data/Worldgen/causal_world_macro_provinces_floor.cmp`.

## Four fixtures

| Fixture | Result |
|---|---|
| 1 Mountain belt | **PASS** — coherent elongated highland (mean Z 1004 m, relief 1683 m, aspect 3.70, 4 ridges, long-axis continuity, multiple watersheds) |
| 2 Basin | **PASS** — coherent low (mean Z −126 m), 6/8 inward-drainage edges, sediment accommodation |
| 3 Boundary | **PASS** — mountain→foreland follows NeighborRelations / uplift–subsidence contrast (uplift drop 1100 m, subsidence rise 231 m), not a noise fade |
| 4 Counterfactual | **PASS** — disable uplift/deformation, same seed: relief **0.000 m**; mountain system disappears |

Determinism: budget 1 == N. Ordinary sample queries do not rebuild the
64 km scaffold (rebuilds stay 1 after load).

## Wrap retirement

Production MW1 source is absolute-coordinate, not `4096 m tile → wrap`.

- vs +4096 m: RMS 200 m, max |Δ| 367 m
- same structural domain 8 km apart: RMS 181 m — not a tiled copy of the
  same 4096 m heightfield

Existing local/semantic tests may still wrap. MW1 production must not.

## Horizon-to-Hand scale ladder

Forced through existing residency / sample-height / digest machinery:

| Rung | Samples | Result |
|---|---:|---|
| 64 km regional view | 15876 | PASS |
| 10 km valley view | 24649 | PASS |
| 1 km watershed view | 15876 | PASS |
| 192 m live residency | 625 | PASS |
| 12.5 cm authoritative matter | 289 | PASS + continuity |

## Visual / Test A / soak

Visual: `MW1_PROVINCES_PLAYER_VISUAL PASS` — 90 settled frames, 2601
packages, 0 lower-frame sky, rebuilds 1, wrap 0.

Test A (`--cert-worldgen-cardinal-replacement-mw1`) — **PASS**. Receipt
`Docs/provenance_mw1_cardinal_replacement_cert.txt`.
Digest `017a2368097d219c` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~4.5 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw1_streaming_soak_cert.txt`. 0 / 88171 frames >16.667,
max 5.917 ms, 2601 resident, pending 0. MW1 travel physics **idle**
(rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW1 background workload.

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
MW2 regional 3D geology          CLOSED
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
PLAY_MW1_PROVINCES.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw1-provinces
```

Cert:

```
CERT_MW1_PROVINCES.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw1-provinces
Build\x64_Release\ProvenanceClient.exe --cert-mw1-provinces-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw1
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw1 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.PROVINCES_BELTS_BASINS` /
“MW1 - Provinces / Mountain Belts / Basins”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2-MW8 CLOSED`.
CLOSED rows visible, not launchable.
