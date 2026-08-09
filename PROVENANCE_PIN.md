# Provenance Esoterica pin

Upstream: https://github.com/BobbyAnguelov/Esoterica  
Pinned commit: `cf36499f59d0434179b03692986af8aec803d9c4` (2026-07-21)  
Local path: `C:\Users\D-Day\ProvenanceEsoterica`  
Branches:
- `provenance/pin-cf36499` — upstream pin + minimal props patches
- `provenance/client-spike` — **local** ProvenanceClient spike (do not push to Bobby `origin`)

**Spike contract:** `SPIKE_CONTRACT.md` (One Simulation, Two Clients).

## Isolation (hard)

| Do | Don't |
|----|--------|
| Edit ProvenanceClient in this tree | Commit Esoterica work into Mygame / Unreal git |
| Treat Mygame as read-only reference | Push Provenance commits to `origin` (Bobby) |
| Keep Python as authority | Invent client-side matter law |

## Toolchain proven on this machine
- Visual Studio Community 2026 18.8.2 (toolset v145 / MSVC 14.51)
- External deps from upstream Releases/Dependencies (~998 MB) in `External/`

## Fork patches on this pin
- `Code/PropertySheets/RenderDoc.props`: remove trailing `\;` on include path
- `Code/PropertySheets/EA.props`: include `eastl_Esoterica.h` by filename + add `EE_EA_DIR` to includes

## Honest phase status (2026-08-08)

- **P0–P2** done: stock build, handshake, far/streamed heightfield walk.
- **P3** partial: standable + wall capsule; not occupancy collision.
- **P3b voxel-form gate — client land:** dig/pick subtract affect sphere from occupancy → D2 matter-face cavity (VisualMaterial interior); heightfield opens where matter is gone; DigScar cups/chips are flash-only. Authoritative grams still from carve digest + column reconcile.
- **P4 parity digests** next (headless = Unreal = Esoterica receipts).
- **P5 water** deferred.

## Dig / tunnel ruling (locked)

**Tunnel = changing matter, not deforming a heightfield.**  
Owned mesh = projection of authoritative matter face for targeting — never authority.  
End state: engine sphere subtract → settle → **D2 reconstructs boundary**.

### HF / occupancy / D2 ownership (locked 2026-08-08)

HF and D2 must share the **same canonical virgin surface law** (`SampleGroundZBase` / geography grade). They do **not** coexist or continuously cross-materialize.

| Phase | Owner |
|-------|--------|
| Virgin terrain | **HF only** |
| Committed dig/pick | Seed **bounded** local occupancy from that virgin law → apply matter edit → build cavity/D2 **once** → **atomically** hand ownership to the completed patch |
| Aim / look / walk | May refine HF contact. Must **not** trigger D2/QEF rebuilds or volumetric expansion |

Reject: continuous HF↔occupancy rematerialization; peel HF before cavity commit; aim-driven lattice growth; HF aperture from latest tip focus alone.

**EditedRegion (remove-only monotonic):** connected excavation keeps a persistent openings union. Later strikes may enlarge/merge; they must never roof a prior mouth because focus moved. Tip/action disk is transient. Stencil aperture = openings union.

### Interim → P3b
`Main.cpp` carves local/synthetic fill at affect radius (seeded from virgin surface law), rebuilds cavity once, hands HF pads to the completed patch, demands `voxel_column` reconcile. DigScar = flash only.

## Launch

`RunProvenanceClient.bat` → prefers newest `Build\x64_Release*\ProvenanceClient.exe` (LNK1104 spill folders included).  
Engine: `voxel_bridge.py` on `127.0.0.1:8765`.

## Geo fixtures (D2 harness)

Default play/dev: **PROVENANCE GEOLOGY RANGE** (representative local flank transect near spawn `(128,128)`, walk `+X` through flat→slope→mound→drain→hill→bedrock→local cliff).  
Alternate: **D2 TORTURE TERRAIN** (prior extreme FBM) via `--geo-fixture=torture` or **F8**.  
Code: `ProvenanceGeography.h` (`Range*` / `Torture*`); virgin load remains HF-only (D2 dormant until dig).

**Geography interaction cert (Horizon-to-Hand transect):** see `GEOGRAPHY_INTERACTION_CERT_HANDOFF.md` — run with `--cert-geo` / `--cert-geography` (forces RANGE).

```
P3b D2 CORE FLOOR
SHA: 523e796eba8c3f1f67559caadb27e76957645f4e
```

Freeze D2 contracts/topology (Unknown halo refuse, +max seam ownership, partitioned halo stats, order independence). Support/chips frozen — may consume occupancy/ER/D2 boundary later; must not redesign Hermite/QEF ownership, halo semantics, or HF↔D2 handoff unless a cert exposes a defect. Prior `haloMiss=0` PASS revoked — see handoff §5.

## Material / appearance docs for review (keeper)

Review-only refs (not geo-cert / dig-cert authority; do **not** overwrite when merging):

- https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-distribution-v1/fablescript/docs
- https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-set-appearance/fablescript/docs

**Heightfield object palette = keeper** — its own presentation/worldbuilding lane. Separate from `--cert-geo` / occupancy / D2. Do not replace it when folding material-distribution or appearance docs from other branches. Exact palette filenames TBD when those trees are locally fetchable (see handoff §Related docs).
