# Stage 16C.1 — Compiled-Deposit Classification

Stage 16C.1 answers one question: given sediment Stage 16C already compiled
into the present landscape, classify it into the 3A–3B.3B depositional
vocabulary.

This is a **worldgen-only semantic mapping**. It cannot erode, transport,
remobilize, rewrite 16C mass routing, weld into host geology, open 3C
collapse, or invent live physics. Classification is not a ledger transfer.

**Law:** Off == frozen 16C/16D present world. On assigns deposit class /
body identity only. Host geology grams do not absorb classified deposit
grams. Same sandstone material + different history = different body.

```
recent/mobile compiled deposit        → loose
stable supported compiled deposit     → settled aggregate
old/stable/high-load compiled deposit → compacted deposit
bedrock exposure                      → not depositional (host geology)
```

## Causal chain

```text
frozen Stage 16C compiled sediment
    → 16C.1 read compiled mass / facies / grain / depth / drainage
    → deterministic DepositClass + depositBodyId
    → present geometry / materials / water unchanged
```

Play kernel is **16C-weight**: classification owns compiled sediment only.
Analytical cert loads 16D once to prove water digest unchanged.

## Freeze parents

```
3A–3B.3B  runtime deposition chain   worldgen shaping primitive (ready)
P5b.3B.3A depositional aggregate          CERTIFIED / FROZEN @ 160a0842
P5b.3B.3B compaction / terrain surface    CERTIFIED / FROZEN @ 2f722735
16C     compiled sediment routing         CERTIFIED / FROZEN
16C.1   compiled-deposit classification   CERTIFIED
P5b.3C  structural collapse               CLOSED
```

16C sediment `1367584c41aedcdf`, geometry `7ae7aa62c50489e2`, 16D water
`636d01ba00d3dffe`.

## Conservation

| Ledger | Off | On | Law |
|---|---:|---:|---|
| 16C source grams | 24518286038425.840 | 24518286038425.840 | unchanged |
| 16C deposited grams | 23174494269683.570 | 23174494269683.570 | unchanged |
| 16C mobile grams | 986017855622.387 | 986017855622.387 | unchanged |
| 16C exported grams | 357773913120.076 | 357773913120.076 | unchanged |
| 16C sediment digest | 1367584c41aedcdf | 1367584c41aedcdf | frozen |
| 16C geometry digest | 7ae7aa62c50489e2 | 7ae7aa62c50489e2 | frozen |
| 16D water digest | 636d01ba00d3dffe | 636d01ba00d3dffe | frozen |
| present digest | 2eb519c42ca5bd6b | 2eb519c42ca5bd6b | off == on == frozen |

Classification digest off `f66e8d6115624a93` (no classes assigned).
Classification digest on `4075ca07bfa05881` (budget 1 == N).

## Discriminating fixtures

| Class | Cells | Mapping |
|---|---:|---|
| loose | 6157 | residual mobile, Bar, or thin Wash |
| settled aggregate | 37551 | supported deposit that is not high-load/old |
| compacted deposit | 12264 | BasinFill / thick / Lag-Gravel / high accumulation |
| not depositional | 9564 | no compiled deposit mass (bedrock / host) |

Sandstone host cells 7835 vs sandstone deposit cells 43682; distinct
`depositBodyId` vs `hostFeatureId`; `WeldedToHost=false`.

Stale / missing compiled inputs fail-closed. Runtime classify and
remobilize stay 0.

## CLOSED (hard)

```
3C structural collapse                         CLOSED
cement / lithify                               CLOSED
formation-ID merge / host weld                 CLOSED
soil mechanics / bank collapse                 CLOSED
rainfall / ecology                             CLOSED
16C remobilization / 16C mass-routing rewrite  CLOSED
new erosion / transport                        CLOSED
macro provinces / mountain belts / basins      CLOSED
regional drainage / hydroclimate / soils / biomes CLOSED
```

16C.1 is the last local-chain bridge before macro-scale worldgen. Do not
add PLAYABLE macro-province rows.

## Record

Analytical: `Docs/provenance_16c1_compiled_deposit_classification_cert.txt` PASS  
Visual: `STAGE16C1_COMPILED_DEPOSIT_CLASSIFICATION_PLAYER_VISUAL PASS`
(90 frames, 2601 packages, 0 sky, runtime classifies=0, remobilizes=0).

Test A cardinal (EVERY CUT) — PASS. Receipt
`Docs/provenance_16c1_cardinal_replacement_cert.txt`.
Digest `e7bb12b2b2ec12e3` origin==return. 2601 packages. Movement frames
over 16.667 = 0 on N/E/S/W (worst movement ~4.8 ms).

Test B 90 s NE fly — **PASS**. Receipt
`Docs/provenance_16c1_streaming_soak_cert.txt`. 0 / 84161 movement frames
>16.667, max 6.178 ms, 2601 resident, pending 0. 16C.1 travel physics
**idle** (runtime classifies / remobilizes / stale / revision delta = 0;
compile classifies = 1). P5b 2C–3B.3B travel also idle. 300/900 not run.

## Player runtime

```
PLAY_16C1_COMPILED_DEPOSIT_CLASSIFICATION.cmd
Build\x64_Release\ProvenanceClient.exe --play-16c1-compiled-deposit-classification
```

Gate: `--classify-compiled-deposits=0|1` (off freezes the 16C/16D world).

Cert:

```
CERT_16C1_COMPILED_DEPOSIT_CLASSIFICATION.cmd
Build\x64_Release\ProvenanceClient.exe --cert-16c1-compiled-deposit-classification
Build\x64_Release\ProvenanceClient.exe --cert-16c1-compiled-deposit-classification-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-16c1
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-16c1 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

In-client stages table: **M** → `GEOMORPH.COMPILED_DEPOSIT_CLASSIFICATION`
/ “Stage 16C.1 - Compiled-Deposit Classification”.
