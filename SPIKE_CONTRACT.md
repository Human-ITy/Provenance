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
| **P3b Voxel form gate** | Interaction geometry from **occupancy / D2**, not heightfield cups | **Not done — blocks correct dig/pick** |
| **P4** Interaction parity digests | Headless = Unreal = Esoterica material receipts | Blocked on P3b |
| **P5** Water | Deferred until hydrology contract certified | Deferred |

### The voxel-form gate (user law)

> Dig and pick will not be correct until the world represents its forms by our voxel definitions.

That means:

- Aim / carve / scar / matter return must come from **authoritative subvoxel occupancy** (and material identity), not from grade cups or sealed face overlays alone.
- Heightfield + DigScar cups are **bring-up projection** — useful for walk/look, insufficient for Horizon-to-Hand truth.
- Next concrete work after isolation: consume `voxel_column` / occupancy for contact solid, removed volume, and cavity presentation (D2 reconstruct), still via existing wire receipts.

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

## Reject / defer

Blank-slate engine · client-side material authority · Phase-1-style terrain fakery as dig law · editor parity · water before contract · Unreal abandonment without metrics · pushing to upstream Esoterica.
