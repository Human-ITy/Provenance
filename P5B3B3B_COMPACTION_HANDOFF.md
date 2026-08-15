# P5b.3B.3B — Compaction / Stable Terrain Surface

P5b.3B.3B answers one question: can a **depositional aggregate** compact
under sustained admissible load and then participate in **terrain surface +
collision/support** without becoming host geology?

Compaction is a **state variable** on the same `DepositionalBodyId`. Grams
stay on the DEPOSITIONAL ledger. Same material ID is not the same structural
body.

**Law:** CompactMatter changes packing / compaction / support-capacity on an
already-deposited body. Terrain host solids do not change.

```
depositional aggregate
  → loading / settling / packing
  → compacted depositional body
  → participates in terrain surface + collision/support
NOT depositional sandstone → sandstone formation
NOT terrain fill += grams
```

Pipeline:

```
stable settled aggregate
+ sustained admissible load
+ packing / compaction criteria
+ current support / material / destination revision
  → CompactMatter
  → same DepositionalBodyId
  → BodyPhase SettledAggregate → CompactedDeposit
  → grams unchanged
```

Total matter unchanged. Sufficient hydraulic forcing remobilizes the body
back to loose representation with the same grams and provenance (reversible).

## CLOSED (hard)

```
weld into host geology                         CLOSED
become original sandstone formation            CLOSED
erase LooseMatter / depositional provenance    CLOSED
cement / lithify / chemical bonding            CLOSED
compact arbitrarily (no admissible load)       CLOSED
trigger bank collapse                          CLOSED
enter compiled Stage-16C history               CLOSED
formation-ID merge                             CLOSED
P5b.3C support / bank collapse                 CLOSED
16C remobilization                             CLOSED
general soil mechanics                         CLOSED
general erosion                                CLOSED
rainfall / evaporation / groundwater           CLOSED
plant uptake / ecology                         CLOSED
full WorldOperationScheduler                   CLOSED
```

## Freeze

This cut SHA is stamped after commit on `provenance/client-spike`.
Parent: P5b.3B.3A `160a0842` (`160a0842adcce58e7206cdd10370b94334ccda54`).
3B.2 `eeabfb8c`. 3B `67d5f524`. 3A `6c467fb7`. Long-haul infrastructure
baseline `f7ae29ea`. Gameplay 2B `8bb75265` otherwise frozen.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED / FROZEN
P5b.2C  pore occupancy / topology         CERTIFIED / FROZEN
P5b.3A  hydraulic detachment              CERTIFIED / FROZEN @ 6c467fb7
P5b.3B  hydraulic loose-matter transport  CERTIFIED / FROZEN @ 67d5f524
P5b.3B.2 loose-matter settling            CERTIFIED / FROZEN @ eeabfb8c
P5b.3B.3A depositional aggregate          CERTIFIED / FROZEN @ 160a0842
P5b.3B.3B compaction / terrain surface    CERTIFIED
P5b.3C  bank/support collapse             CLOSED
```

```
P5b.3B.3B off → exact P5b.3B.3A deposition digest 694388e61fa37503
P5b.3B.3B on  → compaction digest ef1ae252489e741a (budget 1 == N == unbounded)
water mass = 466363002 (unchanged)
terrain host solids 353566960349 (unchanged)
loose 0 → 0
depositional matter 80 → 80
held matter unchanged
compiled 16C sediment digest unchanged
cold == reload == unbounded
partitioned == monolithic
```

## Conservation (independent ledgers, exact canonical units)

| Ledger | Before | After | Law |
|---|---:|---:|---|
| TERRAIN HOST SOLIDS | 353566960349 | 353566960349 | unchanged — no host absorb |
| LOOSE | 0 | 0 | already deposited; this cut does not move grams |
| DEPOSITIONAL MATTER | 80 | 80 | same body; compaction is state, not a mass bucket |
| WATER | 466363002 | 466363002 | unchanged |
| HELD | 1000000054337 | 1000000054337 | unchanged |
| TOTAL MATTER | closed | closed | exact |

Compacted deposit = 80 g sandstone, same `DepositionalBodyId`, packing
fraction 850, compaction 800, `IsStableWalkingSurface=true`,
`WeldedToHost=false`, `HostFeatureId` remains distinct. Do **not** do
terrain solids +80.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| stable settled aggregate under sustained admissible load | CompactedDeposit, 80 g exact, same body, walking surface |
| insufficient compaction / load | stays SettledAggregate; not a structural walking surface |
| compacted deposit re-disturbed by sufficient hydraulic forcing | returns to loose, same grams / provenance |
| stale support / material / destination revision | refuse; ledgers unchanged |
| compacted sandstone against sandstone host | separate depositional body; no formation-ID merge |

Receipt: `FCompactMatter` on `FDepositionalMatterBody`. Locality: max 5 cells,
max 0 bodies examined on the admitted path.

## In-client stages / certs table

Press **M** in a worldgen play session. Dark three-column table: checkbox+index,
Name, Status (`CERTIFIED` / `PLAYABLE` / `CLOSED`). Highlight + Enter/click
launches a playable stage. CLOSED row (P5b.3C only) is visible and not
launchable. Esc or M closes. Launch flags still work; the menu does not
rewrite certified world truth.

```
3A      hydraulic detachment                  CERTIFIED
3B      hydraulic transport                   CERTIFIED
3B.2    loose settling                        CERTIFIED
3B.3A   depositional aggregate                CERTIFIED
3B.3B   compaction / stable terrain surface   CERTIFIED
3C      support / bank collapse               CLOSED
```

## Record

Analytical: `Docs/provenance_p5b3b3b_compaction_cert.txt` PASS  
Txn: `Docs/provenance_p5b3b3b_compaction_receipts.csv`  
Visual: `P5B3B3B_COMPACTION_PLAYER_VISUAL PASS` (90 frames, 2601 packages,
loose_mass=0, depositional_mass=80, compacts=1, nearby water+channel,
p5b3c=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 3B.3A:
`9ffe2cb954206be1` → `1d1dd0776f69f322` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b3b3b_streaming_soak_cert.txt`. 0 / 86525 movement frames
>16.667, max 9.277 ms, 2601 resident, pending 0. All P5b.3B.3B travel
counters **0** (compacts, compact wakes, remobilizes, stale refuse,
compaction revision, loose mass, depositional mass). 3B.3A/3B.2/3B/3A/2C
travel physics also idle. 300/900 not run.

**P5b.3C / 16C stay CLOSED.** No bank/support collapse, no host-formation
weld, no cement/lithify, no formation-ID merge, no 16C remobilization.

## Player runtime

```
PLAY_P5B3B3B_COMPACTION.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b3b3b-compaction
```

Cert:

```
CERT_P5B3B3B_COMPACTION.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b3b-compaction
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b3b-compaction-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3b3b
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3b3b --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b3b3b_compaction_cert.txt`  
Txn receipt: `Docs/provenance_p5b3b3b_compaction_receipts.csv`  
Soak receipt: `Docs/provenance_p5b3b3b_streaming_soak_cert.txt`

P5b.3C / rainfall / groundwater / general erosion / cementation /
lithification / formation merge / 16C remobilization stay CLOSED.
