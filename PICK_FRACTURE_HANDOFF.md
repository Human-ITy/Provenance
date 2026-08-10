# Pick / Surface Gate — Continued Testing Handoff

**Lane:** `C:\Users\D-Day\ProvenanceEsoterica` (ProvenanceClient only).  
**Branch:** `provenance/client-spike`  
**Contract law:** `Docs/P5_PICK_FRACTURE.md`  
**Pins:** `PROVENANCE_PIN.md` · `SPIKE_CONTRACT.md`  
**Do not:** edit Mygame/Unreal; push to Bobby `upstream`; modify `WaterLedger`; open P5b.

---

## Continued testing (resume cold here)

### Current truth

| Item | SHA / path |
|------|------------|
| Tip (pick coverage hard-gate) | feature `fa73dd2` · pin `56260ca` |
| Prior pick fracture floor | feature `434a565` · pin `3d11771` |
| P5a FREEZE | feature `8e09e77` · pin `ee58393` — **do not modify WaterLedger** |
| P5b | **CLOSED** — no terrain–water coupling |
| Preferred build | `Build\x64_Release_pickcov\ProvenanceClient.exe` |

`RunProvenanceClient.bat` selects the **newest-timestamp** `Build\x64_Release*\ProvenanceClient.exe`. Prefer the explicit pickcov path above, or confirm the bat echoed the pickcov binary before trusting live results.

### Hard visual / cert law (user ruling)

1. Sky / clear color **in or around** any strike hole = **FAIL** (`PRESENTATION_COVERAGE_FAIL`).  
   **No** mouth-sky exception. Clear peek is always a defect.
2. Deformation beyond intended strike volume (fracture + minimum recon halo) = **FAIL**.
3. Miss / no-change, punch, surround explosion = **live defects to capture**, not success.

### Expected material expression

| Material | Intended look |
|----------|---------------|
| **Gravel** | Small irregular socket; aggregate-scale roughness; ragged but tight lip; fines/clumps — not a round bowl, not meter plates |
| **Granite** | Compact angular / crystalline wedge; dense tip-scale bite — not a giant flap |
| **Mica schist** | Thin plate bias along foliation; elongate release — not a sphere |
| Soft dig (shovel) | May still use older sphere carve — **pick ≠ sphere** |

### How to test (headless)

Bridge: `voxel_bridge.py` on `127.0.0.1:8765` when the client needs connect (pick-fracture / p5a certs are local-enough; still launch with host/port).

```bat
Build\x64_Release_pickcov\ProvenanceClient.exe 127.0.0.1 8765 --cert-pick-fracture
Build\x64_Release_pickcov\ProvenanceClient.exe 127.0.0.1 8765 --cert-p5a
REM must stay PASS 15/0 — unchanged from P5a FREEZE
Build\x64_Release_pickcov\ProvenanceClient.exe 127.0.0.1 8765 --cert-lsi
Build\x64_Release_pickcov\ProvenanceClient.exe 127.0.0.1 8765 --cert-geo
```

Artifacts:

- `%TEMP%\provenance_pick_fracture_cert.txt` (mirror: `Docs/provenance_pick_fracture_cert.txt`)
- `%TEMP%\provenance_p5a_water_ledger_cert.txt`
- `%TEMP%\provenance_local_surface_intent_cert.txt`
- `%TEMP%\provenance_geography_interaction_cert.txt`
- FAIL blobs under `%TEMP%\provenance_*_fail.txt` when present

### How to test (live)

```bat
Build\x64_Release_pickcov\ProvenanceClient.exe 127.0.0.1 8765
```

1. Hotbar pick · **LMB** strike.
2. Note aim **material** + **slope** (flat / mid / steep / wall).
3. Screenshot fails (sky in/around hole, surround deform, miss, punch).
4. Collect `%TEMP%` probe/cert artifacts.
5. **`[K]`** chip dump if useful for released-piece class.

### What human verifies

For each strike, record material, slope class, and result:

1. **Does anything change?** If no — note aim HUD (material, hit/miss, digest refuse).
2. **Is the recess tip-scale / material-scale**, or does a large HF region warp?
3. **Any sky/clear color in or around the hole?** (any = FAIL — no mouth exception)
4. **Second strike in the same hole** — deepens interior, or remounts virgin crest / tears neighbor?
5. **Chip** matches released piece class (gravel clump vs granite wedge vs schist plate), not random debris.

Ideal pass: two nearby gravel bites = two small sockets, stable lips, no sky in/around, second hit deepens one hole only.  
Ideal fail capture: miss, punch, surround explosion, or sky/clear in the field — keep screenshots + coords for the next cut.

### What next agent should do if live still fails

1. Capture LSI / coverage coords from FAIL blobs + screenshots (world XYZ, material, slope).
2. Fix recon / halo / HF omit so coverage closes **without** relaxing sky law.
3. Keep fracture contact-frame law (pick ≠ sphere); clamp deform to fracture + min recon halo.
4. **No P5b.** **No WaterLedger / water edits.** `--cert-p5a` must remain **15/0**.
5. Re-run `--cert-pick-fracture` + `--cert-p5a` after any play-path change; commit on `provenance/client-spike`.

### Rebuild (if needed)

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
$root = "C:\Users\D-Day\ProvenanceEsoterica\"
$proj = "$root`Code\Applications\ProvenanceClient\Esoterica.Applications.ProvenanceClient.vcxproj"
$out = "$root`Build\x64_Release_pickcov\"
& $msbuild $proj /p:Configuration=Release /p:Platform=x64 "/p:SolutionDir=$root" /p:OutDir=$out /m /v:minimal
```

### Code entry points

| Area | Where |
|------|--------|
| Law / headless cert | `Docs/P5_PICK_FRACTURE.md`, `PickFracture.h` |
| Live carve / coverage | `Main.cpp` — `CarveOccupancyFracture`, mouth coverage / HF omit + D2 cover |
| CLI | `--cert-pick-fracture` / `--cert-p5-pick` |

---

## Related

- Geography transect cert: `GEOGRAPHY_INTERACTION_CERT_HANDOFF.md` (`--cert-geo`, `--cert-lsi`)
- Water freeze: pin block P5a in `PROVENANCE_PIN.md` — do not reopen here
