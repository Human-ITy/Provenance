# P5b.3B.3A — Depositional Aggregate

Successor: P5b.3B.3B certified — see `P5B3B3B_COMPACTION_HANDOFF.md`.
This file remains the 3A freeze record.

P5b.3B.3A answers one question: can a **settled loose parcel** be admitted as
a **depositional sediment body** without becoming host geology?

Same material ID is not the same structural body. The body may contribute to
terrain **surface representation and collision** while remaining transported /
deposited material.

**Law:** DepositMatter moves grams from the loose ledger onto a depositional
aggregate. Terrain host solids do not change.

```
settled sandstone fragment
  → depositional admission
  → loose/packed sediment body
NOT intact geological host
NOT terrain fill += grams
```

Pipeline:

```
settled loose parcel
+ stable support
+ admissible resting geometry
+ low enough forcing
  → current terrain + deposition revision validation
  → DepositMatter
  → loose ledger −grams
  → depositional body +grams
```

Total matter unchanged. Sufficient forcing remobilizes the body back to loose
representation with the same grams and provenance (reversible).

## CLOSED (hard)

```
weld into host geology                         CLOSED
become original sandstone formation            CLOSED
erase LooseMatter provenance                   CLOSED
cement / lithify                               CLOSED
compact arbitrarily                            CLOSED
trigger bank collapse                          CLOSED
enter compiled Stage-16C history               CLOSED
P5b.3B.3B compaction / terrain integration     CLOSED
P5b.3C support / bank collapse                 CLOSED
16C remobilization                             CLOSED
general erosion                                CLOSED
rainfall / evaporation / groundwater           CLOSED
plant uptake / ecology                         CLOSED
full WorldOperationScheduler                   CLOSED
```

## Freeze

Parent: P5b.3B.2 `eeabfb8c`. 3B `67d5f524`. 3A `6c467fb7`. Long-haul
infrastructure baseline `f7ae29ea`. Gameplay 2B `8bb75265` otherwise frozen.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED / FROZEN
P5b.2C  pore occupancy / topology         CERTIFIED / FROZEN
P5b.3A  hydraulic detachment              CERTIFIED / FROZEN @ 6c467fb7
P5b.3B  hydraulic loose-matter transport  CERTIFIED / FROZEN @ 67d5f524
P5b.3B.2 loose-matter settling            CERTIFIED / FROZEN @ eeabfb8c
P5b.3B.3A depositional aggregate          CERTIFIED
P5b.3B.3B compaction / terrain integration CLOSED
P5b.3C  bank/support collapse             CLOSED
```

```
P5b.3B.3A off → exact P5b.3B.2 settling digest ab46ebdbe3aa6778
P5b.3B.3A on  → deposition digest 694388e61fa37503 (budget 1 == N == unbounded)
water mass = 466363002 (unchanged)
terrain host solids 353566960349 (unchanged)
loose 80 → 0
depositional matter 0 → 80
held matter unchanged
compiled 16C sediment digest unchanged
cold == reload == unbounded
partitioned == monolithic
```

## Conservation (independent ledgers, exact canonical units)

| Ledger | Before | After | Law |
|---|---:|---:|---|
| TERRAIN HOST SOLIDS | 353566960349 | 353566960349 | unchanged |
| LOOSE | 80 | 0 | −80 g onto depositional body |
| DEPOSITIONAL MATTER | 0 | 80 | +80 g aggregate, not host weld |
| WATER | 466363002 | 466363002 | unchanged |
| HELD | 1000000054337 | 1000000054337 | unchanged |
| TOTAL MATTER | 353566960429 | 353566960429 | closed |

Admitted deposit = 80 g sandstone, packing `loose_sediment`, compaction 0,
`WeldedToHost=false`. Do **not** do terrain solids +80.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| settled sandstone parcel on valid riverbed | deposited aggregate, 80 g exact |
| same parcel while hydraulic forcing remains high | stays loose |
| unsupported destination | stays loose |
| stale terrain / deposition revision | refuse; ledgers unchanged |
| deposited parcel re-disturbed by sufficient forcing | returns to loose, same grams / provenance |

Receipt: `FDepositMatter` / `FDepositionalMatterBody`. Locality: max 5 cells,
max 0 bodies examined on the admitted path.

## In-client stages / certs table

Press **M** in a worldgen play session. Dark three-column table: checkbox+index,
Name, Status (`CERTIFIED` / `PLAYABLE` / `CLOSED`). Highlight + Enter/click
launches a playable stage. CLOSED rows (P5b.3B.3B, P5b.3C) are visible and
not launchable. Esc or M closes. Launch flags still work; the menu does not
rewrite certified world truth.

## Record

Analytical: `Docs/provenance_p5b3b3a_depositional_aggregate_cert.txt` PASS  
Txn: `Docs/provenance_p5b3b3a_depositional_aggregate_receipts.csv`  
Visual: `P5B3B3A_DEPOSITIONAL_AGGREGATE_PLAYER_VISUAL PASS` (90 frames, 2601
packages, loose_mass=0, depositional_mass=80, deposits=1, nearby water+channel,
p5b3b3b=closed, p5b3c=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 3B.2:
`0ace5164520e5da0` → `9ffe2cb954206be1` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b3b3a_streaming_soak_cert.txt`. 0 / 86580 movement frames
>16.667, max 7.231 ms, 2601 resident, pending 0. All P5b.3B.3A travel
counters **0** (deposits, deposit wakes, remobilizes, stale refuse,
deposition revision, loose mass, depositional mass). 3B.2/3B/3A/2C travel
physics also idle. 300/900 not run.

**P5b.3C / 3B.3B / 16C stay CLOSED.** No bank/support collapse, no compaction
or host-formation weld, no 16C remobilization.

## Player runtime

```
PLAY_P5B3B3A_DEPOSITIONAL_AGGREGATE.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b3b3a-depositional-aggregate
```

Cert:

```
CERT_P5B3B3A_DEPOSITIONAL_AGGREGATE.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b3a-depositional-aggregate
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b3a-depositional-aggregate-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3b3a
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3b3a --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b3b3a_depositional_aggregate_cert.txt`  
Txn receipt: `Docs/provenance_p5b3b3a_depositional_aggregate_receipts.csv`  
Soak receipt: `Docs/provenance_p5b3b3a_streaming_soak_cert.txt`

P5b.3C / 3B.3B / rainfall / groundwater / general erosion / 16C remobilization
stay CLOSED.
