# P5b.3B.2 — Loose-Matter Settling

P5b.3B.2 answers one question: can a transported parcel come to rest at a
physically admissible location while remaining the same conserved loose
matter?

Settling changes **where** loose matter rests, not **what** it is. The
parcel stays loose. It is never turned back into terrain.

**Law:** Settling commits authoritative location + motion/rest state only.

```
same LooseMatterId
same material
same grams
same source formation/provenance
same detachment transaction ancestry
authoritative location + Settled motion
NOT terrain fill += grams
```

Pipeline:

```
LooseMatterId
+ current position
+ momentum / hydraulic forcing
+ candidate support
  → resting query
  → current terrain + loose-matter revision validation
  → commit resting location
  → parcel remains loose
```

## CLOSED (hard)

```
P5b.3C  bank/support collapse           CLOSED
P5b.3B.3A depositional aggregate        (next cut; not this freeze)
P5b.3B.3B compaction / terrain integration CLOSED
16C remobilization                      CLOSED
general erosion / 16B active erosion    CLOSED
rainfall / evaporation / groundwater    CLOSED
plant uptake / ecology                  CLOSED
full WorldOperationScheduler            CLOSED
```

Unavailable / unsupported destination retains the parcel and a pending
settle obligation. Section truth-ready later revalidates revision. No
fallback surface, deletion, or teleport.

## Freeze

Parent: P5b.3B `67d5f524`. 3A `6c467fb7`. Long-haul infrastructure
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
P5b.3B.3A depositional aggregate          (not this freeze)
P5b.3B.3B compaction / terrain integration CLOSED
P5b.3C  bank/support collapse             CLOSED
```

```
P5b.3B.2 off → exact P5b.3B transport digest 876ac027936dce35
P5b.3B.2 on  → settling digest ab46ebdbe3aa6778 (budget 1 == N == unbounded)
water mass = 466363002 (unchanged)
terrain solids 353566960349 (unchanged)
loose/sediment 80 → 80 (rest state changed, mass closed)
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

Admitted settle = 80 g sandstone, cell 289, InTransport → Settled. No
rounding residual. Not terrain fill.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| transported parcel reaches supported low-energy cell | settles; same LooseMatterId, grams, sandstone, provenance; remains loose |
| forcing still above transport threshold (800 ≥ 200) | does not settle; stays InTransport |
| destination unsupported / occupied incompatibly | retain loose + pending settle obligation; never delete/teleport/fallback |
| destination revision goes stale | refuse; parcel remains where it was, still InTransport |
| two compatible tiny parcels share a resting region | co-location admitted; exact grams and constituent IDs retained; merge not required this cut |

Receipt: `FHydraulicLooseMatterSettle`. Locality: max 5 cells, max 0 bodies
examined on the admitted path. Cold == reload == unbounded. Partition-invariant.

## Record

Analytical: `Docs/provenance_p5b3b2_loose_matter_settling_cert.txt` PASS  
Txn: `Docs/provenance_p5b3b2_loose_matter_settling_receipts.csv`  
Visual: `P5B3B2_LOOSE_MATTER_SETTLING_PLAYER_VISUAL PASS` (90 frames, 2601
packages, loose_mass=80, settles=1, nearby water+channel, p5b3c=closed,
p5b3b3=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 3B:
`59a2725b6b89fd77` → `0ace5164520e5da0` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b3b2_streaming_soak_cert.txt`. 0 / 76169 movement frames
>16.667, max 7.280 ms, 2601 resident, pending 0. All P5b.3B.2 travel
counters **0** (settles, settle wakes, pending retained, stale refuse,
settling revision, loose mass). 3B/3A/2C travel physics also idle. 300/900
not run.

**P5b.3C / 3B.3B / 16C stay CLOSED.** No bank/support collapse, no compaction
or host-formation weld. See `P5B3B3A_DEPOSITIONAL_AGGREGATE_HANDOFF.md` for
the next certified cut (depositional aggregate, not host weld).

## Player runtime

```
PLAY_P5B3B2_LOOSE_MATTER_SETTLING.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b3b2-loose-matter-settling
```

Cert:

```
CERT_P5B3B2_LOOSE_MATTER_SETTLING.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b2-loose-matter-settling
Build\x64_Release\ProvenanceClient.exe --cert-p5b3b2-loose-matter-settling-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3b2
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3b2 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b3b2_loose_matter_settling_cert.txt`  
Txn receipt: `Docs/provenance_p5b3b2_loose_matter_settling_receipts.csv`  
Soak receipt: `Docs/provenance_p5b3b2_streaming_soak_cert.txt`

P5b.3C / 3B.3B / rainfall / groundwater / general erosion / 16C remobilization
stay CLOSED.
