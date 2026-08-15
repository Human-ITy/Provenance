# P5b.2B — Bounded Pore Storage

P5b.2B answers one question: can a bounded amount of water mass move
between a surface-water body and adjacent terrain pore storage without
changing terrain solids, geometry, or hydraulic occupancy?

Contract:

```
P5b.2A: water contact changes terrain state; water mass stays in hydraulic body
P5b.2B: some water mass may transfer into terrain pore storage
water_body_mass loss == terrain_pore_water gain
```

Moisture is no longer purely descriptive — it is tied to stored water grams.

## Freeze

Parent: P5b.2A land `570c7be2` / pin `eae7db9f`. 2A soak control pin
`6c385e2c`. Stage 16F.4 remains frozen at `3d84eac4`.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED / FROZEN
P5b.2B  bounded pore storage              CERTIFIED
P5b.2C  pore occupancy / topology         CERTIFIED
P5b.3   reverse matter/erosion coupling   CLOSED
```

```
P5b.2B off → exact P5b.2A state digest 176ffe1c3845f721
P5b.2B on  → pore digest 224e662e5584a1ea (budget 1 == N == unbounded)
conserved (body + pore + container) = 466363002
```

## Record

```
FTerrainPoreWaterState {
  TerrainCell; MaterialId;
  PoreCapacityGrams; StoredWaterGrams;
  Saturation; PermeabilityClass;
  TerrainRevision; WaterRevision;
}
```

Transfer: source + receiver + material + requested amount + constraints →
admitted conserved transfer. Open only:

```
surface water → adjacent porous terrain
terrain pore storage → surface water
```

Infiltration clamps so a required occupied water cell cannot dry. Debit
uses body surplus above `MinCellUnits`. No 16F.4 topology from pore
transfer (`RewriteBodyLocalHydraulics` only).

## Discriminating fixtures

| Fixture | Result |
|---|---|
| dirt beside lake absorbs a bounded amount | 400 g admitted (high capacity) |
| sandstone absorbs less / slower | 40 g admitted |
| impermeable rock takes zero | 0 g; capacity clamp |
| saturated cell refuses additional pore storage | 0 g after fill |
| contact-loss permits bounded exfiltration | 100 g returned to surface |

This floor's 2A lake-edge bank compiles as sandstone. The high-capacity
"dirt beside lake" receiver is the certified 2A lake-contact cell.

## Certified results

- Disabled P5b.2B: exact P5b.2A state `176ffe1c3845f721`, pore mass 0.
- Enabled pore digest (budget 1 == N == unbounded): `224e662e5584a1ea`.
- Global water conserved: body 456302579 → 456302239, pore 0 → 340,
  container 10060423 unchanged, sum 466363002.
- Surface-water loss == pore-water gain on every admitted receipt.
- Terrain solid grams unchanged: `353566960429`.
- Geometry unchanged. Occupancy mask / 16F.4 topology unchanged.
- Fixtures 5/5 admitted. Locality: max 2 cells, max 1 body.
- Cold == reload == unbounded. Partition-invariant. Stale pore revision refuses.
- P5b.2C now CERTIFIED (successor). P5b.3 / rainfall / groundwater / erosion stay CLOSED.

## Player / traversal (measured)

Visual: `P5B2B_TERRAIN_PORE_PLAYER_VISUAL PASS`. 90 settled frames, 2601
packages, pore_mass=340, nearby water+channel, 2C/3/rainfall/groundwater
CLOSED.

Test A cardinal (EVERY CUT) — PASS. Digest `2396f444f66f1234` origin==return
(2A control was `f6c20f2c4774451b`; different stage, same 2601 / exact
return / movement frames over 16.667 = 0). Water-body / topology /
terrain-state wakes during travel = 0. Created/retired 7803/5202 per leg.

Test B — residency PASS, memory plateau PASS, **frame gate FAIL** (do not
relax 16.667). Latest attributed 300 s receipt includes the 90 s snapshot.
Compared to 2A control (`6c385e2c`):

| Metric | 2A control | 2B |
|---|---|---|
| elapsed / distance | 90.000 s / 2160.1 m | 90.004 s / 2160.1 m |
| created / retired | 19190 / 19190 | 19190 / 19190 |
| max / end resident | 2601 / 2601 | 2601 / 2601 |
| end / max pending | 0 / 101 | 0 / 101 |
| oldest pending / min radius | 0.018 s / 192 m | 0.018 s / 192 m |
| max worker queue | 209 | 210 |
| frames | 20079 | 20904 |
| mean / p95 / p99 / max ms | 4.483 / 6.617 / 9.824 / 60.687 | 4.306 / 6.145 / 9.439 / 42.066 |
| frames >16.667 | **20 FAIL** | **14 FAIL** (pre-attribution); attributed 90 s **6**, 300 s **4** — still FAIL |
| working-set high water | 164 → 2338 MB | pre-cut 167 → 2351 MB; after lookahead bound 167 → 1216 MB @90 s, 1222 MB @300 s (plateau) |
| water / topology / terrain-state wakes | 0 / 0 / 0 | 0 / 0 / 0 |
| mesh / collision publishes | 19190 / 19190 | 19190 / 19190 |

Wake profile unchanged. Frame FAIL remains the honest gate; 2B did not
open a new hitch class. Receipt: `Docs/provenance_p5b2b_streaming_soak_cert.txt`.

## Player runtime

Run `PLAY_P5B2B_TERRAIN_PORE.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-p5b2b-terrain-pore
```

P5b.2B is the latest stable runtime (menu closed). Press `M` for the stage
browser. HUD shows moisture tied to pore grams. P5b.2C and P5b.3 stay closed.

## Re-run

EVERY CUT Test A (required):

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2b
```

or `CERT_TRAVERSAL_EVERY_CUT.cmd`.

Analytical + visual + cardinal: `CERT_P5B2B_TERRAIN_PORE.cmd`.

```text
Build\x64_Release\ProvenanceClient.exe --cert-p5b2b-terrain-pore
Build\x64_Release\ProvenanceClient.exe --cert-p5b2b-terrain-pore-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2b
```

Optional soak vs 2A control (`6c385e2c` / `Docs/provenance_p5b2a_streaming_soak_cert.txt`):

```text
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b2b --soak-duration-s=90 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

or `CERT_STREAMING_SOAK.cmd`. Do not relax 16.667.

Artifacts:

- `Docs/provenance_p5b2b_terrain_pore_cert.txt`
- `Docs/provenance_p5b2b_terrain_pore_receipts.csv`
- `Docs/provenance_p5b2b_terrain_pore_visual_cert.txt`
- `Docs/provenance_p5b2b_terrain_pore_player_view.ppm`
- `Docs/provenance_p5b2b_cardinal_replacement_cert.txt`
- `Docs/provenance_p5b2b_streaming_soak_cert.txt`

## Boundaries (CLOSED)

- **P5b.2C** pore storage affecting water occupancy / topology
- **P5b.3** water→terrain mechanical / matter movement
- deep groundwater, vertical aquifer, long-range subsurface
- rainfall, evaporation, plant uptake
- erosion, sediment, bank collapse
