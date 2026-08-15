# P5b.3A — Hydraulic Detachment

P5b.3A answers one question: can certified hydraulic conditions cause one
bounded parcel of susceptible terrain matter to detach, while both water and
terrain conservation remain exact?

**Choice A:** detached matter remains a local loose body/aggregate. It is
**that matter** (material identity, grams, source cell, ancestry, formation
provenance, cause, transaction id) — not anonymous `sediment`. Do **not**
send through Stage 16C.

**Law:** `terrain solid loss == new detached/sediment matter`. Water does
not delete terrain merely because an erosion condition is true.

Publication is an atomic coherent pair via P5b.1 machinery:

```
old terrain R + water W  →  new terrain R+1 + water W+1
```

No mixed-revision frame.

## CLOSED (hard)

```
P5b.3B  detached sediment transport     CLOSED
P5b.3C  bank/support collapse           CLOSED
general erosion / 16B active erosion    CLOSED
16C remobilization                      CLOSED
rainfall / evaporation / groundwater    CLOSED
plant uptake / ecology                  CLOSED
generic SimulationDomain extract        CLOSED
```

## Freeze

Parent: P5b.2C `e4c9da99` + soak harness `a28ed5c5`. Long-haul infrastructure
baseline `f7ae29ea`. Gameplay 2B `8bb75265` otherwise frozen.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED / FROZEN
P5b.2C  pore occupancy / topology         CERTIFIED / FROZEN
P5b.3A  hydraulic detachment              CERTIFIED
P5b.3B  detached sediment transport       CLOSED
P5b.3C  bank/support collapse             CLOSED
```

```
P5b.3A off → exact P5b.2C occupancy digest 7102e45c92f6545d
P5b.3A on  → detachment digest e3ba9b3af77265cb (budget 1 == N == unbounded)
water mass = 466363002 (unchanged)
terrain solids 353566960429 → 353566960349  (−80)
loose/sediment 0 → 80                       (+80)
held matter unchanged
compiled 16C sediment digest unchanged
```

Eligibility reads certified **P5b.2A material state** (wetting saturation +
cohesion). 2B overlays pore fill onto `QueryTerrainState` saturation; that
overlay is not the 3A gate.

## Conservation (independent ledgers, exact canonical units)

| Ledger | Before | After | Law |
|---|---:|---:|---|
| TERRAIN SOLIDS | 353566960429 | 353566960349 | before = after + detached |
| LOOSE/SEDIMENT | 0 | 80 | before + detached = after |
| WATER | 466363002 | 466363002 | unchanged (no water txn) |
| HELD | 1000000054337 | 1000000054337 | unchanged |
| TOTAL MATTER | 353566960429 | 353566960429 | closed |

Admitted parcel = 80 g. No rounding residual.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| saturated susceptible bank (wetland sandstone, sat=1.000, cohesion=0.550, water contact, exposed face) | solid −80, loose +80, water unchanged |
| same material dry control (sat=0.050) | 0 g |
| saturated resistant host (shale, sat override 1.000) | 0 g admitted |
| stale terrain/water revision | refuse, zero mass change |

Receipt: `FHydraulicTerrainDetachment`. Locality: max 5 cells, max 0 bodies
examined on the admitted path. Cold == reload == unbounded. Partition-invariant.

## Record

Analytical: `Docs/provenance_p5b3a_hydraulic_detachment_cert.txt` PASS  
Txn: `Docs/provenance_p5b3a_hydraulic_detachment_receipts.csv`  
Visual: `P5B3A_HYDRAULIC_DETACHMENT_PLAYER_VISUAL PASS` (90 frames, 2601
packages, loose_mass=80, nearby water+channel, p5b3b/p5b3c=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 2C:
`f743150420e22175` → `8b0f3cca9ccb1bb6` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b3a_streaming_soak_cert.txt`. 0 / 86035 movement frames
>16.667, max 6.965 ms, 2601 resident, pending 0. All P5b.3A travel counters
**0** (detachments, admitted mass, stale refuse, water responses, 16F.4
rebuilds, terrain/water revision, loose mass). 2C travel physics also idle.
300/900 not run.

**P5b.3B and P5b.3C stay CLOSED.** No sediment transport, no bank/support
collapse, no general erosion, no 16C remobilization.

## Player runtime

```
PLAY_P5B3A_HYDRAULIC_DETACHMENT.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b3a-hydraulic-detachment
```

Cert:

```
CERT_P5B3A_HYDRAULIC_DETACHMENT.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b3a-hydraulic-detachment
Build\x64_Release\ProvenanceClient.exe --cert-p5b3a-hydraulic-detachment-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3a
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3a --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b3a_hydraulic_detachment_cert.txt`  
Txn receipt: `Docs/provenance_p5b3a_hydraulic_detachment_receipts.csv`  
Soak receipt: `Docs/provenance_p5b3a_streaming_soak_cert.txt`

P5b.3B / P5b.3C / rainfall / groundwater / general erosion / 16C remobilization
stay CLOSED.
