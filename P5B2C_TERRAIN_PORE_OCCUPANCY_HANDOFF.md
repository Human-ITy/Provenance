# P5b.2C — Pore Occupancy / Topology

P5b.2C answers one question: can pore-water transfer change surface-water
occupancy and hydraulic topology while terrain solid matter stays fixed?

Contract:

```
2B: infiltration may not dry a required occupied water cell → clamp
2C: pore transfer MAY change occupancy (dry/grow/split/merge) via 16F.4
    topology, then 16F.1 / 16F.2
```

Terrain solid grams unchanged. Water mass conserved: body + pore + container.
P5b.3 stays CLOSED (no water-induced terrain matter movement, no erosion,
no sediment remobilization).

## Freeze

Parent: long-haul infrastructure baseline `f7ae29ea` / pin `156df62d` on
`provenance/client-spike`. Gameplay 2B `8bb75265` otherwise frozen.
Stage 16F.4 remains frozen. P5b.1 revision coherence reused.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED / FROZEN
P5b.2C  pore occupancy / topology         CERTIFIED
P5b.3   reverse matter/erosion coupling   CLOSED
```

```
P5b.2C off → exact P5b.2B pore digest 224e662e5584a1ea
P5b.2C on  → occupancy digest 7102e45c92f6545d (budget 1 == N == unbounded)
conserved (body + pore + container) = 466363002
terrain solid grams = 353566960429 (unchanged)
```

## Record

Fixtures 5/5 admitted. Infiltration dried a thin occupied cell (shrink).
Exfiltration wet an adjacent dry cell (grow). Saturated refuse 0 g.
Impermeable rock 0 g. Far-from-water occupancy unchanged. Locality: max 5
cells, max 1 body. Cold == reload == unbounded. Partition-invariant.
Stale pore/topology revision refuses. Disabled 2C == exact 2B state.

Analytical: `Docs/provenance_p5b2c_terrain_pore_occupancy_cert.txt` PASS  
Txn: `Docs/provenance_p5b2c_terrain_pore_occupancy_receipts.csv`  
Visual: `P5B2C_TERRAIN_PORE_OCCUPANCY_PLAYER_VISUAL PASS` (90 frames, 2601
packages, pore_mass=426, nearby water+channel, p5b3=closed)

Test A cardinal (EVERY CUT) — PASS. Stage identity changed vs 2B:
`2396f444f66f1234` → `f743150420e22175` (origin==return). Movement frames
over 16.667 = 0 on N/E/S/W. Wake water-body / topology / terrain-state = 0.

Test B 90 s NE fly (this cut) — **PASS**. Receipt
`Docs/provenance_p5b2c_streaming_soak_cert.txt`. 0 / 86317 movement frames
>16.667, max 7.233 ms, 2601 resident, pending 0. All 2C travel counters 0
(pore transfers, wet→dry, dry→wet, 16F.4 rebuilds, body split/merge/grow/shrink).
300/900 not run. P5b.3 stays CLOSED.

**P5b.3 stays CLOSED.** No water-induced terrain matter movement, no
erosion, no sediment remobilization.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| infiltration dries a thin occupied cell | occupancy shrink / possible split |
| exfiltration wets adjacent dry cell | occupancy grow / possible merge |
| saturated cell refuses extra pore | 0 g; occupancy unchanged |
| impermeable rock | 0 bulk |
| far from water | no occupancy change |

## Hard gates

- terrain solid grams unchanged
- global water (body+pore+container) conserved
- topology lineage deterministic (16F.4 rules)
- budget 1 == N == unbounded
- cold/reload + partition
- 2C disabled → exact 2B state
- Test A digest if stage identity changes, document it
- 0 movement frames >16.667 on EVERY-CUT cardinal
- Test B 90 s: 0 movement frames >16.667; 2C travel counters idle or bounded

## Player runtime

```
PLAY_P5B2C_TERRAIN_PORE_OCCUPANCY.cmd
Build\x64_Release\ProvenanceClient.exe --play-p5b2c-terrain-pore-occupancy
```

Cert:

```
CERT_P5B2C_TERRAIN_PORE_OCCUPANCY.cmd
Build\x64_Release\ProvenanceClient.exe --cert-p5b2c-terrain-pore-occupancy
Build\x64_Release\ProvenanceClient.exe --cert-p5b2c-terrain-pore-occupancy-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2c
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b2c --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

Analytical receipt: `Docs/provenance_p5b2c_terrain_pore_occupancy_cert.txt`  
Txn receipt: `Docs/provenance_p5b2c_terrain_pore_occupancy_receipts.csv`
Soak receipt: `Docs/provenance_p5b2c_streaming_soak_cert.txt`

P5b.3 / rainfall / groundwater / erosion / sediment remobilization stay CLOSED.
