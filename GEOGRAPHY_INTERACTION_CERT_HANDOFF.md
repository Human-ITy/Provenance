# Geography Interaction Cert — Agent Handoff

**Lane:** Esoterica ProvenanceClient only (`C:\Users\D-Day\ProvenanceEsoterica`).  
**Do not:** edit Mygame/Unreal; push to Bobby `origin`; silently “fix” cert FAILs.  
**Pins:** `PROVENANCE_PIN.md`, `SPIKE_CONTRACT.md`.

---

## 1. Purpose

Treat the deterministic **PROVENANCE GEOLOGY RANGE** fixture as a continuous **Horizon-to-Hand certification transect**, not a D2-only unit test.

Headless cannot say “this cliff looks good,” but it can prove what would make it look wrong: wrong surface, bad normals, excessive subdiv, lost openings, material mismatch, packaging seams, support on obsolete HF, virgin D2 work, etc.

Every D2 / HF / dig / place / material / collision / stream change should re-run the same landscape sequence.

---

## 2. Architecture invariants (red if broken)

| Law | Meaning |
|-----|---------|
| **action ≠ D2 dirty ≠ HF aperture** | Bite volume, recon dirty+halo, and mouth stencil are three distinct quantities |
| **Virgin D2 = 0** | No cavity/QEF until a committed dig/pick |
| **SampleSurface ≈ geoCellsCreated** | Geography FBM once per cell create; remesh uses cached cap colors |
| **tip/6 = presentation only** | Adaptive HF refine around mouths; never raises matter resolution |
| **0.85 = work partition, not clip** | `kWorkBudgetR` tiles oversized dirty AABBs; must not truncate excavation |
| **12 cm = new strike mouth only** | Each strike adds ≤12 cm opening; union grows over many strikes |
| **HF / D2 share virgin surface law** | `SampleGroundZBase` / geography grade; no continuous HF↔occupancy rematerialize |
| **EditedRegion remove-only** | Openings never shrink because focus moved; old mouths stay open |

---

## 3. Fixtures — launch

Engine: `voxel_bridge.py` on `127.0.0.1:8765` (Mygame `_engine_truth_lane3/fablescript`).

```bat
REM RANGE play (georange OutDir — fixture world)
Build\x64_Release_georange\ProvenanceClient.exe 127.0.0.1 8765

REM GEO CERT (preferred OutDir from §10 rebuild; georange also OK if that binary has --cert-geo)
Build\x64_Release_geocert\ProvenanceClient.exe 127.0.0.1 8765 --cert-geo
Build\x64_Release_georange\ProvenanceClient.exe 127.0.0.1 8765 --cert-geo
REM aliases: --cert-geography

REM TORTURE (prior extreme FBM — keep intact; not the geo cert world)
Build\x64_Release_georange\ProvenanceClient.exe 127.0.0.1 8765 --geo-fixture=torture
REM or F8 in-client to cycle
```

Also: `RunProvenanceClient.bat --cert-geo` (picks newest `Build\x64_Release*\ProvenanceClient.exe`).
Both `x64_Release_geocert` and `x64_Release_georange` are valid; rebuild geocert via §10 if that OutDir is missing.

| Fixture | Code | Role |
|---------|------|------|
| **RANGE** | `GeoFixture::Range` | Cert transect + default play |
| **TORTURE** | `GeoFixture::Torture` | Edge-case D2 stress; do not delete |

---

## 4. Transect map (contacts A–J near spawn)

**Origin:** `(128, 128)` = `kRangeOriginX/Y`. Walk **+X** (increasing `u = x - 128`). Corridor full strength `|v| < 14 m`, fade by ~22 m.

| ID | u (m east of 128) | Approx world XY | Condition |
|----|-------------------|-----------------|-----------|
| A | −10…12 | (128–140, 128) | flat soil / valley pad (grass) |
| B | 10…28 | (138–156, 128) | gentle grassy slope |
| C | ~36 | (164, 128) | rounded mound / convex |
| D | ~51 | (179, 128) | concave drainage (clay/gravel) |
| E | 58…78 | (186–206, 128) | moderate rocky hillside |
| F | ~80–92 | (208–220, 128) | steep diagonal exposed rock |
| G | ~93.4 | (~221.4, 128) | near-vertical local cliff (~4.2 m / ~1.3 m) |
| I | 88…93.6 | (~220, 128) | scree apron at cliff base |
| J | ~102, v≈−3 | (~230, 125) | limestone / mineralized shoulder |
| + | cell edges / multi-cell | scan | packaging contacts |

`--cert-geo` discovers these programmatically (grades/normals/caps along transect + cell boundaries). Nominal table above is for visual teleport when FAIL reports coords.

**Visual reproduce:** launch RANGE client, set feet/cam to FAIL `world=(x,y,z)`, look along reported normal.

---

## 5. Full cert spec (sections 1–15) — summary + red gates

Source: user “Representative Geography — Headless Terrain/Interaction Certification” (2026-08-09).

### §1 Discover and classify
Locate contacts for: flat soil, gentle slope, mound, drainage, moderate rocky slope, steep diagonal rock, near-vertical face, crest/top edge, material boundary, world-cell boundary, multi-cell crossing, ore/mineral if present.  
Record: XYZ, normal, slope°, material, cell, HF subdiv.

### §2 Virgin-world
Assert before interaction:
```
D2 rebuilds = 0
QEF solves = 0          (proxy: D2 rebuilds / qef fallbacks if no separate counter)
EditedRegions = 0
MatterBodies = 0
SampleSurface ~= geoCellsCreated
```
Walk fixture without residency growth → `HF remesh = 0`, `D2 = 0`.  
Virgin verts finite, normals unit, tris non-degenerate, no Z spires.  
`voxel_column` over transect: geography grade before == after (no meter-scale `GradeToZ` jump).

### §3 HF subdivision/refinement
Refine to interaction target **without edit**. Surface/ray/normal unchanged; only resolution. Footprint = tool + collar; neighbor stitch ≠ edited matter.

### §4 Tool-contact matrix
Soft → shovel/dig; hard → pick. Per strike record visual XYZ/N, penetration, action volume, material, grams, occupancy, **D2 dirty bounds**, **HF aperture bounds**, EditedRegion — three volumes stay distinct. Penetration in contact frame, not blind −Z.

### §5 D2/QEF
Production extract: complete halo, finite Hermites/normals, QEF in clamp, deterministic, no degenerates. Byte-identical re-extract. Bite centered / 1-boundary / multi-cell — no cell-sized packaging signature.

**Fail-closed Unknown halo (2026-08-09 correction):**
- Missing neighbor column is **Unknown** — never invent `kFillFull` solid or air in `ColumnFillField::TrySample`.
- `SampleDensity` returns NaN on TrySample fail (`Solid` must not see dens=0 as at-ISO solid).
- `RebuildCavityMesh` aggregates **all** tiled `ExtractStats` (incl. `haloMissing`) via `AccumulateStats`; fetch 3×3 lattices then refuse publication if still incomplete.
- Prior `haloMiss_zero` PASS is **REVOKED** (false confidence). Re-prove with `halo_fail_closed_complete`.

**Cross-cell primal-edge ownership:**
- Each column emits its **+max** face edges (`c=w-1→w`, `r=h-1→h`) exactly once; **-min** face is never emitted (neighbor owns that seam as its +max).
- Cert: `cross_cell_seam_owner_once` + `cross_cell_seam_watertight` (not merely single-ER corridor continuity).

**D2 core is not freeze-ready until both halo fail-closed and seam ownership gates PASS.**

### §6 Accumulated excavation
≥20 adjoining strikes on flat, moderate rock, steep face → one coherent EditedRegion when connected. Prior carve stays removed; no HF roof because focus moved. Outside dirty+halo bit-identical. Extend past 1 cell, several cells, and **0.85 work budget** (partition OK, clip FAIL).

### §7 HF ↔ D2 ownership
Track refinement / action / ownership masks. D2 patch before HF retire. No uncovered void. Prior openings: occupancy carved? HF drawn? D2 owner? Unrelated strikes must not flicker HF cover.

### §8 Material correctness
Presented vs authoritative vs removed. Flag `AUTH_MATERIAL_MISMATCH` (do not remap). Soil, gravel/clay, host rocks, ores in fixture.

### §9 Placement
Place on flat, slope, cavity lip/floor, adjacent excavation. Must mutate matter, not presentation blob. remove→place→remove conserves mass; no HF resurrection.

### §10 Support/collision
Outside EditedRegions: HF support OK. Inside: **SupportBelow(x,y,queryZ)** through occupancy (same fill D2 consumes) — not D2 tris, not column crest/topmost. Tunnel: floor below queryZ (roof ignored). Missing authority → refuse/defer, never invent HF. Support query → zero D2/HF remesh.

### §11 Chips
Modes `OFF` / `VISUAL` / `PHYS`. Geometry cert normally OFF. PHYS: gravity → `SupportBelow(x,y,currentZ)` → contact normal → settle/slide; no crest teleport; cavity/tunnel entry; missing occ defer; ACTIVE→SETTLED sleep. Lifecycle also `AGGREGATED` (fines scaffold) / `EXPLICIT BODY` (meaningful plates).

**Scalability:** material vocabulary is cheap; instantiated representation is what costs — wake geometry/physics only on exposure, detachment, proximity, or gameplay meaning.

### §12 Streaming / async
Permute 3×3 `voxel_column` fan-in → identical final occupancy/materials/ER/D2/HF. `fill=keepLocalCarve`, grade preserved; no cavity rebuild unless auth edited.

### §13 Performance budgets
Separate timings (geo / HF / occ / QEF / D2 / support / chips). Preserve virgin invariants + coarse 2×2 vista; tip/6 presentation only.

### §14 Determinism
Normal / reversed async / different pacing / fresh process → matching auth + canonical D2 hashes.

### §15 Output artifact
One file with per-scenario rows + red-gate summary (see §6 below).

**Global red gates (any → FAIL):**
spire/runaway Z · lost carve · HF resurrection · uncovered void outside legitimate cavity · packaging/cell signature · incomplete halo · non-deterministic D2 · wrong material contributor · support over excavated void · virgin D2 work · excessive remesh · mass/conservation discrepancy.

**Do not auto-fix during cert.** Capture first FAIL + reproduce blob; quit non-zero.

---

## 6. Output artifact

Primary:
```
%TEMP%\provenance_geography_interaction_cert.txt
```
Optional mirror: `Build\cert\` or beside exe when writable.

Also related:
- `%TEMP%\provenance_startup_perf.txt` (virgin stream snap)
- `%TEMP%\provenance_geography_interaction_fail.txt` (first hard FAIL reproduce blob)
- Existing dig tour: `--cert-dig` → `provenance_cert_*.ppm` / tour summary

---

## 7. FAIL format (no auto-compensate)

```text
FAIL:
fixture=steep_schist_02
action=pick_07
world=(221.40,128.00,…)
normal=(nx,ny,nz)
reason=HF_RESURRECTION
beforeMeshHash=...
afterMeshHash=...
occupancyHash unchanged
```

Then teleport the visual client to that XYZ and inspect.

---

## 8. Code entry points

| Area | Where |
|------|--------|
| CLI `--cert-geo` / `--cert-geography` | `Main.cpp` `wWinMain` argv |
| Cert tick | `Main.cpp` `CertGeoTick()` (called from `TickFrame`) |
| Dig tour (separate) | `CertDigTick()`, `--cert-dig` |
| Fixtures | `ProvenanceGeography.h` (`Range*` / `Torture*`, `SetFixture`) |
| Virgin surface | `SampleGroundZBase`, `EnsureGeoCell` / `SampleSurface` |
| D2 rebuild | `RebuildCavityMesh` (`kWorkBudgetR = 0.85f`) |
| HF aperture / stencil | `CrestMouthStencilAt`, `NearOpeningMouthAt`, openings union |
| Support | `SupportBelow` (Z-aware); `SupportAt` / `SampleGroundZ` adapters |
| Perf counters | `perfVirginD2Rebuilds`, `perfSampleSurfaceCalls`, `perfGeoCellsCreated`, `perfHfRebuilds`, `perfD2Rebuilds`, … |
| Adapt HF | `EmitTerrainQuadAdaptive` (tip/6), vista 2×2 |

---

## 9. Implementation status / suggested order

| Step | Status |
|------|--------|
| RANGE + TORTURE fixtures | **Done** |
| Adapt shell (cached cap, tip/6, virgin D2=0) | **Done** |
| Handoff + pin pointers | **Done** (this file) |
| `--cert-geo` harness + artifact writer | **Done (incremental)** |
| §1 discover contacts | **Done** |
| §2 virgin + walk | **Done** — cert freezes `FollowStreamCenter` + 8-cell vista recenter during walk so remesh=0 is measurable; play path unchanged |
| §3 HF refine no-edit | **Done** — aim/look: D2=0, no mouth collar, vista div=2, surface Z/N identity, no HF remesh (tip/6 dormant pre-edit) |
| §4 dig matrix soft/hard (record action/D2/HF distinct) | **Minimal runnable** |
| §5 D2/QEF halo + re-extract | **Done (corrected)** — fail-closed Unknown halo + tile stats agg + cross-cell seam owner/watertight + order independence; prior `haloMiss=0` PASS revoked (SKIP); explicit `UNKNOWN_HALO_REFUSED` |
| §6 accumulated excavation | **Done** — flat + **moderate_rock** + **steep_face** 20-strike corridors (same ER/lip/cross-cell/0.85 gates; steep uses into-normal carve) |
| §7 HF/D2 ownership masks | **Done** — action≠dirty≠HF aperture; prior opening triple owner; no uncovered void; D2 re-extract does not mutate HF ownership; far mouth=0 |
| §8 material correctness | **Done (instrumented gates)** — `MaterialSlumpsOpen` soft/hard; presented cap vs `SampleSurface`; soft-roof omit/sink vs hard CrestMouth; no invented remapper |
| §9 Placement | **Frozen / deferred** — after P3d (stash: `Build/_d2_floor_stash/`) |
| §10 Support/collision | **Done (P3c)** — SupportBelow occupancy floor; see live run below |
| §11 chips | **Done (P3d)** — PHYS on SupportBelow + lifecycle; see live run below |
| §12 / §14 | **Scaffold SKIP** |
| §13 performance budgets | **Partial** — virgin invariants + timing snapshot (header counters) |
| §15 artifact | **Done** |
| Async permutation / chips PHYS | **TODO** (after freeze) |
| Visual teleport helper CLI | **TODO** (manual feet set from FAIL blob) |

### P3b D2 CORE FLOOR — pending SHA

Checkpoint after green pre-commit D2 closure. Pin records exact SHA in a follow-up commit.

```
P3b D2 CORE FLOOR
SHA: 523e796eba8c3f1f67559caadb27e76957645f4e
```

Meaning: freeze D2 **contracts and topology**; bugfixes ok later; support/chips may consume occupancy / EditedRegion / D2 boundary but must not redesign Hermite/QEF ownership, halo semantics, or HF↔D2 handoff unless a cert exposes a defect.

### Live run (2026-08-09) — D2 halo/seam / refusal / order closure

`Build\x64_Release_geocert\ProvenanceClient.exe 127.0.0.1 8765 --cert-geo`  
→ `exit_code=0`, `PASS_rows=77 FAIL_rows=0 SKIP_rows=7` (D2-only; §9/§10 frozen out).

| Row | Required |
|-----|----------|
| `haloMiss_zero_PRIOR_REVOKED` | **SKIP** (documentary revoke of false invent-solid PASS) |
| `UNKNOWN_HALO_REFUSED` | **PASS** — deliberate missing neighbor → refuse, empty cavity, miss recorded |
| `UNKNOWN_HALO_REFUSED_partitioned` | **PASS** — partitioned `AccumulateStats` propagates halo/refusal |
| `halo_fail_closed_complete` | **PASS** — haloMiss=0 after Ensure+full tile agg; no invent |
| `cross_cell_seam_owner_once` | **PASS** — Lbound>0, Rbound=0 on shared +X |
| `cross_cell_seam_watertight` | **PASS** — owner seam tris present |
| `order_independence_LR_RL` | **PASS** — L→R and R→L hashes match |
| `order_independence_partitioned` | **PASS** — partitioned rebuild hash-stable |
| §6 20-strike corridors | **PASS** — coherent ER; no packaging / HF resurrection / spires |

**Chips frozen** for P3b — support resumed as P3c below.

### P3c OCCUPANCY SUPPORT FLOOR

```
P3c OCCUPANCY SUPPORT FLOOR
SHA: 5e9e786d08d2352c4f4efc1c84f9d0ab977d12f9
```

Live `--cert-geo` after support floor: `exit_code=0`, `PASS_rows=87 FAIL_rows=0 SKIP_rows=6` (§10 all PASS; §9/§11 still frozen SKIP).

§10 probes (RANGE): virgin flat/slope HF unchanged; cavity floor ≠ virgin HF; wall no false ledge; tunnel floor below queryZ (roof ignored); lip inside/outside; cross-cell continuity; neighbor-strike far identity; missing authority refuse; support query zero remesh.

### P3d DETACHED MATTER SUPPORT FLOOR

```
P3d DETACHED MATTER SUPPORT FLOOR
SHA: b1fec59aa637ad69884f92e36419946d0c256f07
```

**Scalability (standing):** material vocabulary is cheap; instantiated representation is what costs. Chips: ACTIVE→SETTLED sleep; AGGREGATED fines scaffold OK; EXPLICIT BODY only for meaningful plates — not permanent active rigid bodies for every fragment.

Live `--cert-geo` after chip floor: `exit_code=0`, `PASS_rows=97 FAIL_rows=0 SKIP_rows=5` (§11 all PASS; §9 placement still frozen SKIP).

§11 probes (RANGE): falls to HF; into dig hole; tunnel ignores roof; incline slide + support normal; cross-cell no hop; missing occ defer; OFF→VISUAL→PHYS zero D2/HF remesh; determinism; ACTIVE→SETTLED sleep; AggregatePatch scaffold.

### P3d.1 CHIP QUIESCENCE

```
P3d.1 CHIP QUIESCENCE
SHA: 0a8867affc536b003c3408d9af43bbd8bc4066cd
```

Sleep = supported + speed < sleepSpeed + `|z-restZ|` < sleepPosEps + contact stable for N frames → SETTLED. **No flat-nz sleep gate** (steep-static / rough cavity floors must sleep). Wake thresholds clearly larger; wake on support move/loss, meaningful impulse, or `supportRev` / EditedRegion change under chip. HUD/cert probe: id, life, state, speed, support nz, z, restZ, `|z-restZ|`, quietFrames, supportRev.

§11 added: `chip_cavity_quiescence`, `chip_steep_static_sleep`, `chip_wake_on_supportRev`. §9 placement still frozen.

Live `--cert-geo` after quiescence: `exit_code=0`, `PASS_rows=100 FAIL_rows=0 SKIP_rows=5` (rows=105; §9 placement still frozen SKIP).

### P3d.2 CHIP VIBRATE / CYCLE HALT

```
P3d.2 CHIP VIBRATE / CYCLE HALT
SHA: (see PROVENANCE_PIN.md)
```

Keep P3d.1 quiescence hysteresis (no flat-nz sleep gate). Add position-ring limit-cycle / orbit detection + tangential KE damp when supported near rest without slide progress → force SETTLED at SupportBelow rest / mean contact. Wake unchanged (speed / support loss / impulse / supportRev).

§11 added: `chip_vibrate_orbit_halt`, `chip_steep_kinetic_settle`. §9 placement still frozen.

Live `--cert-geo` after vibrate/cycle halt: `exit_code=0`, `PASS_rows=102 FAIL_rows=0 SKIP_rows=5` (§9 placement still frozen SKIP).

**Order for next agents:** PLACE/RE-FILL only after user asks; then ASYNC+DETERMINISM → P4. Never redesign D2 halo/seam ownership or SupportBelow without a cert defect. Do not start placement in the same cut as P3d.2.

---

## 10. How to run (operator)

1. Start bridge: `PYTHONPATH=. python voxel_bridge.py` from fablescript.  
2. Build (if needed):
   ```powershell
   $msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
   $root = "C:\Users\D-Day\ProvenanceEsoterica\"
   $proj = "$root`Code\Applications\ProvenanceClient\Esoterica.Applications.ProvenanceClient.vcxproj"
   $out = "$root`Build\x64_Release_geocert\"
   & $msbuild $proj /p:Configuration=Release /p:Platform=x64 "/p:SolutionDir=$root" /p:OutDir=$out /m /v:minimal
   ```
3. Run:  
   `Build\x64_Release_geocert\ProvenanceClient.exe 127.0.0.1 8765 --cert-geo`  
4. Read `%TEMP%\provenance_geography_interaction_cert.txt`. Exit `0` = no hard FAIL; non-zero = first FAIL captured.

If bridge is down, client may sit in connect retry — still compile/ship harness; document that live cert needs `:8765`.

---

## Constraints (do not violate)

- No adapt-architecture redesign  
- No silent visual/D2 “fixes” inside the cert run  
- Keep TORTURE fixture intact  
- Capture FAIL with coords/hashes; quit — do not compensate  

---

## Related docs for review (do not overwrite)

Gameplay / worldbuilding + material **appearance** refs live outside this cert lane. Review when folding material correctness (§8) / presentation polish — **do not merge them into geo cert or dig-cert law**, and **do not overwrite the heightfield object palette**.

| Tree (GitHub) | Role |
|---------------|------|
| [plaintxt-decoded `claude/terrain-material-distribution-v1` / `fablescript/docs`](https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-distribution-v1/fablescript/docs) | Terrain material distribution (review) |
| [plaintxt-decoded `claude/terrain-material-set-appearance` / `fablescript/docs`](https://github.com/Human-ITy/plaintxt-decoded/tree/claude/terrain-material-set-appearance/fablescript/docs) | Material set appearance (review) |

**Keeper:** the **heightfield object palette** is its own thing (presentation / worldbuilding). Separate from `--cert-geo` / `--cert-dig` ownership, occupancy, and D2 gates. When copying from other branches or merging docs, leave that palette intact.

Local skim (2026-08-09): those GitHub trees were not fetchable here (private / unauth); `_engine_truth_lane3/fablescript/docs` has related material doctrine notes (`MATERIAL_GEOMETRY_DOCTRINE.md`, `DIG_MATERIAL_IDENTITY_FINDING.md`, …) but **not** the heightfield object palette pack — cite exact palette filenames once the branches are available.  
Also see `PROVENANCE_PIN.md` / `SPIKE_CONTRACT.md` pointers.  

