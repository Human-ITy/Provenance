# Stage 16B — Compiled Fluvial Erosion

Stage 16B is a derived compiled-history layer above the frozen Stage 15 and
Stage 16A controls. It does not simulate or render water.

## Causal chain

```text
Stage 15 bare-earth geography
→ Stage 16A deterministic drainage graph
→ erosion potential from accumulation, slope, lithology, and structure
→ bounded incision, widening, and headward extension
→ existing geology re-queried at the new surface
```

The forcing-disabled control returns the exact Stage 15 surface. Equalizing
material resistance removes the lithology multiplier. Missing parent authority
is never interpreted as air, and client-side material relabelling is forbidden.

## Certified results

- Stage 16A channels before/after: 3,308 / 3,308.
- Mean / p95 / maximum incision: about 0.59 m / 2.40 m / 12.0 m.
- Removed compiled-history volume: about 9.92 million m³, recorded by material
  and watershed. This is an erosion receipt, not yet transported sediment.
- All 2,408 Stage 16A depressions remain present and are classified as micro,
  closed geomorphic, through-with-spill, channel-connected, or structural.
- 192 m live radius: 2,601 packages, zero holes, zero fallback, zero authority
  or collision mismatches.
- North/east/south/west 408 m replacement and exact return: PASS.
- Movement remains inside 16.667 ms in all four cardinal sweeps.
- Cold start and Stage 16A ↔ Stage 16B transition order produce the same digest.

## Player runtime

Run `PLAY_STAGE16B_FLUVIAL_EROSION.cmd`. The client now opens on Stage 16B as
the latest stable runtime with the menu closed. Press `M` for the stage browser.

The orange/cream arrows are presentation-only drainage diagnostics. They read
the compiled cell cache and do not alter terrain authority.

## Boundaries

Still closed: sediment transport, deposition, actual water occupancy/rendering,
runtime erosion, hydraulic solving, and P5b. Stage 16C should account for where
the Stage 16B removal receipt goes; it must not retroactively alter this control.

## Re-run

Run `CERT_STAGE16B_FLUVIAL_EROSION.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.
