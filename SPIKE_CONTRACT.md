# One Simulation, Two Clients — Esoterica Runtime Spike

**Lane home:** `C:\Users\D-Day\ProvenanceEsoterica` (this tree only)  
**Do not:** commit here into Mygame / `_engine_truth_lane3` / Unreal.  
**Do not:** push Provenance work to Bobby Anguelov’s `origin` without an explicit fork remote.

## Framing (adopted)

> You already created the unique engine.

```
Authoritative Python simulation (voxel_bridge + ledger)
        ├── headless validation
        ├── Unreal reference client
        └── Esoterica experimental client  ← this lane
```

Clients may differ in render, residency, input, tooling.  
They may **not** disagree about subvoxel occupancy, grams, terrain mutation, water volume, inventory, structure validity, or revisions.

## Five laws (hard)

1. Python remains the only world-state authority.
2. Chunks / meshes are packaging — never world truth.
3. Every client-visible mutation derives from an authoritative receipt.
4. Disconnect / reload / client swap cannot fork matter history.
5. Continuation depends on measured advantage over Unreal — not feature inventory.

## Refined sequence (simpler than a full engine port)

| Gate | Proof | Status (2026-08-08) |
|------|--------|---------------------|
| **P0** Stock Esoterica builds + pin receipt | Reflector/Engine/Compiler | Done @ `cf36499` |
| **P1** Native window + protocol handshake + identity/HUD | Caps / revisions / reconnect | Done (wire live) |
| **P2** Far projection only (no dig authority) | Heightfield + sky + camera | Done (analytic + streamed surface) |
| **P3** Standable projection + collision | Walk / wall capsule | Partial |
| **P3b Voxel form gate** | Interaction geometry from **occupancy / D2**, not heightfield cups | **Client land** — affect-sphere carve → D2 cavity + skin open; column reconcile |
| **P3c Occupancy support** | `SupportBelow(x,y,queryZ)` through occupancy | **Floor** @ `5e9e786` |
| **P3d Detached matter support** | PHYS chips on SupportBelow + ACTIVE/SETTLED lifecycle | **Floor** @ `b1fec59`; **stop before place** |
| **P3d.1 Chip quiescence** | Hysteretic ACTIVE→SETTLED; steep-static sleep; wake on supportRev | **Floor** @ `0a8867a`; **stop before place** |
| **P3d.2 Chip vibrate/cycle halt** | Limit-cycle / orbit detect + tangential damp → SETTLED | **Floor** @ `6afec51`; **stop before place** |
| **P4** Fablescript authority floor | Receipt-driven dig/place; no invent on refuse; `--cert-p4` | Done @ `5d66975` |
| **P4.3** Terrain residency / invalidation | Terrain wakes only on terrain-relevant affect; `--cert-residency` | See `PROVENANCE_PIN.md` |
| **P4.4** Upstream body id wire | Production `terrain_mutate` emits body/agg ids; `--cert-p4` vs committed bridge | See `PROVENANCE_PIN.md` |
| **P4.5** Gameplay-scale stress floor | History must not scale recurring frame cost; `--cert-stress` | See `PROVENANCE_PIN.md` |
| **P5** Water | P5a ledger freeze; **P5b.1 FROZEN** one-way; **P5b.2A FROZEN** state-only; **P5b.2B CERTIFIED** bounded pore. P5b.2C/P5b.3 CLOSED | See `PROVENANCE_PIN.md` / `P5B2B_TERRAIN_PORE_HANDOFF.md` |
| **Traversal** | Standing matrix: Test A cardinal ≠ Test B soak. 2A control: Test A PASS; Test B residency PASS / frame FAIL (do not relax 16.667) | `TRAVERSAL_STREAMING_SOAK_HANDOFF.md` |

### The voxel-form gate (user law)

> Dig and pick will not be correct until the world represents its forms by our voxel definitions.

**P3b client land (2026-08-08):**

- **Virgin = HF only.** HF and cavity share one virgin surface law; they are not continuously cross-materialized.
- **Commit only:** dig/pick seeds a bounded local occupancy from that law → sphere subtract → cavity rebuild once → atomic HF→patch handoff. Aim/look must not expand volume or rebuild D2/QEF.
- Cavity presentation = solid|air boundary painted with `VisualMaterialDef` (exterior material becomes viewable interior).
- DigScar is flash-only then retired. Engine carve digest + column reply remain authority for grams / reconcile — client mesh is never world truth.
- Geo fixtures: default **RANGE** (representative flank transect); **TORTURE** keeps prior extreme FBM (`--geo-fixture=` / F8). See `PROVENANCE_PIN.md`.
- Geography interaction cert (RANGE as H2H transect, not D2-only): `GEOGRAPHY_INTERACTION_CERT_HANDOFF.md` — `--cert-geo`.
- Local Surface Intent / presentation-closure: `--cert-lsi` → `%TEMP%\provenance_local_surface_intent_cert.txt` (mouth annulus + place open-skin expected; cross-cell accidental opens FAIL; canonical per-column-crest seam).
- Async + determinism (includes LSI hashes/closure): `--cert-async` → `%TEMP%\provenance_async_determinism_cert.txt`.
- **Pick / surface continued testing:** `PICK_FRACTURE_HANDOFF.md` — `--cert-pick-fracture`; sky/clear in|around hole = FAIL (no mouth exception); P5a freeze; P5b.1 FROZEN one-way; P5b.2A FROZEN state-only; P5b.2B CERTIFIED bounded pore (P5b.2C/P5b.3 closed).
- **Scalability:** material vocabulary is cheap; instantiated representation is what costs (wake geometry/physics/optics on exposure, detachment, proximity, or gameplay meaning).
- Material distribution / appearance docs for **review** (not cert authority): plaintxt-decoded branches `claude/terrain-material-distribution-v1` and `claude/terrain-material-set-appearance` under `fablescript/docs`. **Heightfield object palette is a keeper** — do not overwrite when merging; separate from dig/geo cert. See `PROVENANCE_PIN.md`.

## Isolation rules

| Path | Role |
|------|------|
| `C:\Users\D-Day\ProvenanceEsoterica\` | **Only** edit surface for this spike |
| `Code/Applications/ProvenanceClient/` | Client app + H2H presentation |
| Mygame / Unreal / fablescript | **Read-only** protocol + parity reference |
| `origin` (BobbyAnguelov/Esoterica) | Upstream pin only — **never push** Provenance commits here |
| Local branch `provenance/client-spike` | Provenance client commits (local until fork remote exists) |

## Platform

Windows 11 + VS 2026 + this OpenGL ProvenanceClient spike.  
Not a mobile claim. Separate study required for Android/iOS.

## Continuation criteria

Keep going only if shared-route benchmarks beat Unreal on residency / latency / memory **without** dropping collision, receipts, grams, or persistence.

A worldgen stage is not certified just because its math is correct. It is
certified only if the player can keep moving through newly generated world
indefinitely, within bounded residency and frame-time limits. Cardinal
replacement (exact return) is not a soak. See `TRAVERSAL_STREAMING_SOAK_HANDOFF.md`.

## Reject / defer

Blank-slate engine · client-side material authority · Phase-1-style terrain fakery as dig law · editor parity · water before contract · Unreal abandonment without metrics · pushing to upstream Esoterica.
