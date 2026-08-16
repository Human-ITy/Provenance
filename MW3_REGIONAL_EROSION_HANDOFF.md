# MW3 — Regional Erosion

MW3 answers one question: given certified MW1 relief and MW2 persistent 3D
geology, can compiled denudation remove and redistribute material according
to relief, drainage potential, lithology, structure, and exposure — without
inventing the underlying mountain shape?

**Law:** Erosion changes the intersection surface. It does not rewrite
FormationId, merge host bodies, or certify a river network.

```
    MW1  uplift / subsidence / basin structure
            ↓
    MW2  persistent 3D formations / faults / intrusions
            ↓
    MW3  compiled denudation acting on that geology
            ↓
    (closed) MW4 drainage → MW5 deposition
```

Forbidden: noise ridges; independent mountain sculptors; “erosion” that
ignores FormationId/lithology; mass-deletion carvers; certified watersheds /
tributaries / floodplains (MW4); final basin-fill (MW5).

Production source is **absolute-coordinate 64 km regional authority**.
The 4096 m Stage-15 wrap is not used here. Off / no-MW3 leaves the exact
MW2 present surface (and frozen local 16C.1) untouched.

## This cut

SHA: `PENDING_STAMP` (filled after the certify commit).

Freeze parents:

```
MW2     regional 3D geology               CERTIFIED / FROZEN @ 6ada7260
        stamp                             581438fe
MW1     provinces / belts / basins        CERTIFIED / FROZEN @ 569269aa
16C.1   compiled-deposit classification   CERTIFIED / FROZEN @ 047d4304
3B.3B   compaction / terrain surface      CERTIFIED / FROZEN @ 2f722735
```

Off / no-MW3 == exact MW2 present surface (max |ΔZ| **0.000 m**; off digest
`8a75a65a66088ad9` == MW2 surface digest). Frozen 16C.1 local present
`2eb519c42ca5bd6b`. 16C sediment `1367584c41aedcdf` / geometry
`7ae7aa62c50489e2`. 16D water truth unchanged. MW1 uplift-off still
flattens the shape field (relief **0.000 m**).

Descriptor: `Data/Worldgen/causal_world_regional_erosion_floor.cre`.
Seed `mw3-64km-01`. Field digest `7bf66c6b9a32ea90`. Denuded surface
digest `c7dbeb3b66483e7f`.

## Mass account (16B/16C conservation)

Not a mass-deletion carver. HOST geology identity unchanged.

| Account | Grams |
|---|---:|
| Upland removed | 87936795832068944.000 |
| Exported / routed erosion mass | 87936795832068944.000 |
| Residual | 0.000000 |

`removed == exported`. Final deposition remains MW5.

## Five fixtures (fail-closed)

| Fixture | Result |
|---|---|
| 1 Resistant vs weak lithology | **PASS** — shale denude **44.600 m** vs sandstone **21.783 m**; lithology-off equalizes to **28.720 / 28.911 m** |
| 2 Structural grain | **PASS** — structure-on anisotropy **27.771°** (score **0.9986**); structure-off score **0.6806** (orientation signal collapses) |
| 3 Intrusion control | **PASS** — I01_PLUTON on Z **1542.574 m** vs off **1501.613 m**; MW2 intrusion-off geology identity preserved |
| 4 Basinward mass accommodation | **PASS** — removed == exported, residual **0** |
| 5 Erosion-off == exact MW2 present | **PASS** — max \|ΔZ\| **0.000000 m**; digest identity |

## Horizon-to-Hand FormationId ladder

MW3 changes the intersection, not geological identity.

| Rung | Result |
|---|---|
| 64 km map | PASS — 3969 formation samples |
| Eroded ridge | PASS |
| Exposed formation | **B02_HOST_MIDDLE** |
| Buried continuation (same XY, −25 m; along-strike same local Z) | **B02_HOST_MIDDLE** same FeatureId / chronology |
| 12.5 cm materialization | PASS — 289/289 same FormationId |

## Visual readability

64 km metric pack (not a beauty pass):

- 17 major ridges, 34 subsidiary ridges
- Belt relief **1655 m** (mountain belt, not one swollen dome)
- Basin transition: belt mean Z well above basin
- Lithology expresses (shale incises deeper than sandstone)
- Anisotropy **27.771°** vs MW1 azimuth 28°
- Wrap RMS vs +4096 m: **188 m** (not a 4 km tile)
- Slope hierarchy **7.287**
- Mean / max denudation **10.2 / 95.8 m**

Player visual: `MW3_REGIONAL_EROSION_PLAYER_VISUAL PASS` — 90 settled
frames, 2601 packages, 0 lower-frame sky, rebuilds 1, wrap 0. Exposed
`B03_HOST_UPPER` at the camera. Image
`Docs/provenance_mw3_regional_erosion_player_view.ppm`.

## Additional fixtures

| Fixture | Result |
|---|---|
| Determinism budget 1 == N | **PASS** |
| Off / no-MW3 exact MW2 present | **PASS** |
| Ordinary travel does not rebuild MW3 | **PASS** — rebuilds stay 1 after load |
| No wrap (not 4096 m tile) | **PASS** |
| MW1 uplift-off still relief 0 | **PASS** — **0.000 m** |
| MW2 deformation-off keeps MW1 relief | **PASS** |
| Frozen 16C.1 local present on off path | **PASS** |

## Visual / Test A / soak

Visual: `MW3_REGIONAL_EROSION_PLAYER_VISUAL PASS`. Receipt
`Docs/provenance_mw3_regional_erosion_visual_cert.txt`.

Test A (`--cert-worldgen-cardinal-replacement-mw3`) — **PASS**. Receipt
`Docs/provenance_mw3_cardinal_replacement_cert.txt`.
Digest `3af80264fc1a619e` origin==return. 2601 packages. N/E/S/W exact
return. Movement frames over 16.667 = **0** (worst movement ~6.0 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_mw3_streaming_soak_cert.txt`. 0 / 87926 frames >16.667,
max 5.318 ms, 2601 resident, pending 0. MW1/MW2/MW3 travel physics
**idle** (rebuilds = 0, compiles = 0). P5b 2C–3B.3B and 16C.1 also idle.
300/900 not run — no persistent MW3 background workload.

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
MW3 regional erosion             CERTIFIED
MW4 valley/drainage morphology   CLOSED
MW5 depositional landscape       CLOSED
MW6 hydroclimate                 CLOSED
MW7 soils                        CLOSED
MW8 biomes                       CLOSED
```

Also closed: Voronoi/splat biome painter, cementation / lithification /
formation merge, 16C mass-routing rewrite, 16D water-truth rewrite,
certified river network, final basin-fill deposition.

## Player runtime

```
PLAY_MW3_REGIONAL_EROSION.cmd
Build\x64_Release\ProvenanceClient.exe --play-mw3-regional-erosion
```

Cert:

```
CERT_MW3_REGIONAL_EROSION.cmd
Build\x64_Release\ProvenanceClient.exe --cert-mw3-regional-erosion
Build\x64_Release\ProvenanceClient.exe --cert-mw3-regional-erosion-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-mw3
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-mw3 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client M board: `MACRO.REGIONAL_EROSION` /
“MW3 - Regional Erosion”.
Caption: `MACRO WORLDGEN  MW1 CERTIFIED  MW2 CERTIFIED  MW3 CERTIFIED  MW4-MW8 CLOSED`.
CLOSED rows visible, not launchable.
