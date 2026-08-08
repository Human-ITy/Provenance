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

### Interim → P3b
`Main.cpp` carves local/synthetic fill at affect radius, rebuilds interior D2 shell, retires DigScar when cavity owns the hole, demands `voxel_column` reconcile. Heightfield remains far/targeting underlay and opens pads via `SurfaceOpenedByOccupancy`.

## Launch

`RunProvenanceClient.bat` → prefers `Build\x64_Release_new` if present (when primary exe is locked).  
Engine: `voxel_bridge.py` on `127.0.0.1:8765`.
