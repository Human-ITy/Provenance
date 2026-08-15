# P5b.3B — Hydraulic Transport of Already-Detached Loose Matter

P5b.3B answers one question: can water move an existing detached parcel from
one physically valid location to another while preserving its mass, material
identity, provenance, and revision history?

Detachment, transport, and deposition stay separate. Fixtures start with
**already-detached** matter (P5b.3A parent). Transport is its own receipt.
Destination remains loose matter — not auto-deposited terrain.

**Law:** Transport changes location, not identity.

```
same LooseMatterId
same material
same grams
same source formation/provenance
same detachment transaction ancestry
different authoritative location
```

Pipeline:

```
existing loose body
  → water contact / hydraulic forcing
  → transport eligibility
  → candidate downstream/resting location
  → authoritative destination validation
  → move same loose matter
  → water/topology responds if necessary
  → sleep
```

## CLOSED (hard)

```
P5b.3C  bank/support collapse           CLOSED
P5b.3B.2 loose-matter settling          CERTIFIED (child; location + rest only)
P5b.3B.3 terrain reincorporation        CLOSED
16C remobilization                      CLOSED
general erosion / 16B active erosion    CLOSED
rainfall / evaporation / groundwater    CLOSED
plant uptake / ecology                  CLOSED
full WorldOperationScheduler            CLOSED
```

Unavailable destination retains the parcel and a pending transfer obligation.
That establishes the behavior. It is not a generic scheduler.

## Freeze

Parent: P5b.3A `6c467fb7` + soak harness `a28ed5c5`. Long-haul infrastructure
baseline `f7ae29ea`. Gameplay 2B `8bb75265` otherwise frozen.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED / FROZEN
P5b.2C  pore occupancy / topology         CERTIFIED / FROZEN
P5b.3A  hydraulic detachment              CERTIFIED / FROZEN @ 6c467fb7
P5b.3B  hydraulic loose-matter transport  CERTIFIED / FROZEN @ 67d5f524
P5b.3B.2 loose-matter settling            CERTIFIED (child)
P5b.3B.3 terrain reincorporation          CLOSED
P5b.3C  bank/support collapse             CLOSED
```

```
P5b.3B off → exact P5b.3A detachment digest e3ba9b3af77265cb
P5b.3B on  → transport digest 876ac027936dce35 (budget 1 == N == unbounded)
water mass = 466363002 (unchanged)
terrain solids 353566960349 (unchanged)
loose/sediment 80 → 80 (location changed, mass closed)
held matter unchanged
compiled 16C sediment digest unchanged
```

## Conservation (independent ledgers, exact canonical units)

| Ledger | Before | After | Law |
|---|---:|---:|---|
| TERRAIN SOLIDS | 353566960349 | 353566960349 | unchanged |
| LOOSE/SEDIMENT | 80 | 80 | before == after; same LooseMatterId |
| WATER | 466363002 | 466363002 | unchanged (no water txn) |
| HELD | 1000000054337 | 1000000054337 | unchanged |
| TOTAL MATTER | 353566960429 | 353566960429 | closed |

Admitted hop = 80 g sandstone, cell 33 → 289. No rounding residual.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| water moves susceptible loose parcel one bounded hop (drainage receiver) | source loses parcel, dest gains same parcel, grams exact |
| insufficient hydraulic forcing (force 50 < resistance 200) | same parcel, zero movement |
| heavy/resistant parcel (resistance 2500 > force 400) | zero movement |
| stale destination revision (eval then dest changes) | refuse, parcel stays at source |
| unavailable destination section | retain parcel, retain pending transfer obligation, never delete/teleport/fallback-place |

Receipt: `FHydraulicLooseMatterTransfer`. Locality: max 5 cells, max 0 bodies
examined on the admitted path. Cold == reload == unbounded. Partition-invariant.

## Record

Analytical: `Docs/provenance_p5b3b_hydraulic_transport_cert.txt` PASS  
Txn: `Docs/provenance_p5b3b_hydraulic_transport_receipts.csv`  
Visual: `P5B3B_HYDRAULIC_TRANSPORT_PLAYER_VISUAL PASS` (90 frames, 2601
packages, loose_mass=80, transfers=1, nearby water+channel, p5b3c=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 3A:
`8b0f3cca9ccb1bb6` → `59a2725b6b89fd77` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b3b_streaming_soak_cert.txt`. 0 / 85720 movement frames
>16.667, max 6.881 ms, 2601 resident, pending 0. All P5b.3B travel counters
**0** (transfers, transport wakes, pending retained, stale refuse, transport
revision, loose mass). 3A/2C travel physics also idle. 300/900 not run.

**P5b.3C stays CLOSED.** No bank/support collapse, no 16C remobilization,
no deposition of transported loose matter back into structural terrain.

## Player runtime

```
PLAY_P5B3B_HYDRAULIC_TRANSPORT.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b3b-hydraulic-transport
```

Cert:

```
CERT_P5B3B_HYDRAULIC_TRANSPORT.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b-hydraulic-transport
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b-hydraulic-transport-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3b
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3b --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b3b_hydraulic_transport_cert.txt`  
Txn receipt: `Docs/provenance_p5b3b_hydraulic_transport_receipts.csv`  
Soak receipt: `Docs/provenance_p5b3b_streaming_soak_cert.txt`

Child: P5b.3B.2 loose-matter settling is CERTIFIED — see
`P5B3B2_LOOSE_MATTER_SETTLING_HANDOFF.md`. Settling changes location + rest
state; the parcel remains loose. **P5b.3B.3 terrain reincorporation stays
CLOSED.** P5b.3C / rainfall / groundwater / general erosion / 16C
remobilization stay CLOSED.
