# Stage 15 — Bare-Earth Causal Geography Floor

## Frozen base

- Stage 14 baseline: `ba948ed8`
- Stage 15 adds a separate regional geography descriptor and playable/certified runtime.
- Stage 14's one-strike fixture remains a read-only control. Stage 15 does not broaden mining.

## Authority chain

```text
Stage 12 faulted surface-breach geology
→ Stage 15 regional uplift and deformation
→ material-resistance compiled erosion
→ final geology re-query at the exposed surface
→ world-aligned 8 m terrain packages
→ shared render/collision reconstruction
```

The analytical region is 4,096 m × 4,096 m. The player runtime keeps the certified
192 m live radius (384 m diameter) and uses no far field. Grass, trees, water, fog,
scatter, props, active erosion, sediment transport, and P5b remain closed in the
Stage 15 fixture.

## Implemented landform proof

- regional massif and folded sedimentary ridge system;
- granite intrusive shoulder;
- shale recess;
- Stage 11 fault-aligned scarp;
- saddle/pass;
- valley floor and drainage-cut ravine;
- bedrock footslope receiving zone (no sediment is minted);
- basin floor.

The descriptor is [causal_world_bare_earth_geography_floor.cbg](Data/Worldgen/causal_world_bare_earth_geography_floor.cbg).
The kernel is [CausalBareEarthGeography.h](Code/Applications/ProvenanceClient/CausalBareEarthGeography.h).

## Permanent receipts

- `Docs/provenance_stage15_bare_earth_geography_cert.txt`
  - PASS; 16,641 regional samples; 125.237252 m relief; zero geology misses.
  - semantic digest `4f61f8d7b9de8760`.
  - geometry digest `e62633fd61dde6a9`.
- `Docs/provenance_stage15_cardinal_replacement_cert.txt`
  - PASS for N/E/S/W 408 m outbound and return.
  - origin fully evicted at every outer station.
  - 2,601/2,601 packages and 192.00 m complete radius at origin, outer, and return.
  - zero holes, fallback, authority, material, collision, or return-image mismatch.
  - zero movement frames over 16.667 ms; worst movement frame 10.386 ms.
- `Docs/provenance_stage15_bare_earth_visual_cert.txt`
  - PASS; 2,601 resident packages; zero lower-frame sky pixels.
  - fixed capture: `Docs/provenance_stage15_bare_earth_player_view.png`.

Inherited controls rerun against the Stage 15 executable:

- Stage 14 single-pick proof: PASS, conserved, approximately 2.05 ms total strike.
- L1 grass-card sentinel: PASS, worst movement 10.747 ms, zero frames over budget.
- L6 representative game-load sentinel: PASS, worst movement 8.981 ms, zero frames over budget.
- playable runtime independence: PASS for cold/transition-order Stage 10–15.
- boundary/toolkit audit: PASS in all Stage 0–15 bearings, including free flight,
  surface-snapped ruler/palette, x-ray parity, and zero boundary pop.

## Run it

From the repository root:

```powershell
.\Build\x64_Release\ProvenanceClient.exe --play-stage15-bare-earth-geography
```

Stage 15 is also selectable from the in-game certified-runtime menu. It starts in
walk mode on bare ground with beauty/diagnostic layers off. Toggle free flight with
`F`; normal walk/sprint/free-flight streaming uses the same 192 m package path as
the certificate.

## Reproduce the gates

```powershell
.\Build\x64_Release\ProvenanceClient.exe --cert-stage15-bare-earth-geography
.\Build\x64_Release\ProvenanceClient.exe --cert-stage15-bare-earth-visual
.\Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-stage15
.\Build\x64_Release\ProvenanceClient.exe --cert-stage14-single-pick
.\Build\x64_Release\ProvenanceClient.exe --cert-living-world-load-1
.\Build\x64_Release\ProvenanceClient.exe --cert-living-world-load-6
.\Build\x64_Release\ProvenanceClient.exe --cert-playable-runtime-independence
```

## Continue from here

Keep Stage 15 as the bare-earth control. Do not hide weak geography with ecology or
textures. The next geography iteration should improve regional landform composition
against this exact authority/streaming floor. Hydrology, sediment transport, soils,
biomes, vegetation authority, generalized mining, and Stage 16 remain closed until
explicitly opened.
