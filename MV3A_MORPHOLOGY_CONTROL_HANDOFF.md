# MV3.A — Morphology Control Fields + Maturity/Sharpness + Plateau/Escarpment

First implementation cut of the MV3 macro morphology grammar (design
`MV3_MACRO_MORPHOLOGY_DESIGN.md`, `4108de63`). **Python world-authority only**
(`Tools/Worldgen/macro_authority.py`); the renderer is unchanged and consumes the same
`.mcp` pages. `generator_version` 1 → 2; the 25-page ±2 ring re-emitted.

## What it adds (extends MV2.A's narrow smooth vocabulary)

MV2.A produced one rounded terrain type at different amplitudes (every feature a smooth
Gaussian/sinusoid). MV3.A gives the same continuous field a **morphology grammar** — by
type, not by cranking amplitude:

- **Independent continuous control fields** (`_control`, smooth seeded value-noise,
  wavelengths 260–450 km, incommensurate with 64 km): `age`/erosional-maturity, `relief`
  (amplitude), `substrate`, `volcanic`/`plateau` tendency, `asym` (deformation).
- **Normalized structural-style weights** (`controls_at` → softmax): `belt / plateau /
  volcanic / cratonic`, blends not a named enum (block-fault character is the `asym`
  control within the belt family). Low temperature → one family dominates locally with
  smooth transition bands (continuity by construction).
- **Erosional-maturity operator** (`_belt`) affecting RELATIONSHIPS not just smoothing:
  young → high local relief + steep flanks + strong prominence + ridged facets; old →
  lower relief + broad rounded shoulders + shallow dissection grooves.
- **Asymmetric fault-block** (`_belt`, `asym`): steep scarp face / gentle dip-slope.
- **Volcanic cones** (`_massif`, `w_volcanic`): radial conical profile + summit crater.
- **Plateau clamp + escarpment** (`regional_field`, `w_plateau`): positive relief reshaped
  into a flat-topped tableland with a steep-but-finite `tanh`/smoothstep escarpment rim +
  shallow incision (real drainage-aligned canyons deferred to MV3.B).
- **Cratonic damping**: plains stay legitimately low without deleting the skeleton
  elsewhere.

Architecture kept the **additive structural skeleton** (base + belts + basins + massifs),
each feature *shaped* by the local controls, plus the plateau operator on the sum — this
avoids the relief-diluting family-blend that flattened early iterations.

## Certification — all green (`--cert-macro-authority`)

`Docs/provenance_mv2a_macro_authority_cert.txt`, 25-page ±2 ring:
- **morphology_diversity** PASS — families_present 3/4 (belt/plateau/cratonic; volcanic
  rare by design), age_spread 0.50, relief_spread 0.71.
- **seed_diversity_unbounded_world** PASS (the Minecraft-semantics cert) — same seed+coord
  deterministic; a different seed gives materially different controls (mean Δ 0.76);
  long-distance macro_z 0–2000 km relief variance 1535 m (no periodic repetition, no 64 km
  cadence).
- All MV2.A/A2 invariants preserved: determinism, non-repetition, **central anchoring**
  (center page ≡ frozen envelope, exact 0.0; four-boundary continuity 0.046 m/slope 1e-4),
  ring-2 96 km seams, cardinal ±2 → 160 km, feature-scale >64 km (880 km belt), cross-page
  continuity, **no square signature** (boundary step 4.3 m ≪ interior 226 m), cheap (25
  pages / 22 s, no `ReconstructedZ`).

Renderer green with the new pages (client unchanged): MV2.B 128 km raster + 32 km seam
PASS; MV1 coverage rotation-invariant, below-terrain holes = 0; Test B 24 m/s gameplay
hard gate PASS.

Visual: `Docs/provenance_mv3a_world_morphology.png` (±600 km hillshade — belts, massifs,
basins, plateau/cratonic regions varying by locale).

## Honest limit / next

The morphology **machinery** is in and objectively diverse (certs), but the **visual
family distinctness** at a glance is still moderate — the massif field (uniform lattice
blobs) and feature placement are not yet gated by structural style, so "belt vs plateau vs
cratonic" reads more in relief/roundedness than in dramatically different landform
skylines. Making families visually unmistakable (feature *presence*/placement driven by
style; stronger tableland/cone signatures) is the natural continuation — **do it as a
focused tuning pass, not by cranking global relief** (a low rounded range stays
legitimately low). This does **not** block MV3.B.

## MV3.B (deferred)

Drainage-aligned canyon systems (cheap cached macro drainage graph — downhill
connectivity real, forward-aligned with MW4), butte/remnant operator, richer volcanic
constructs, specialized scarps.

## NOT changed

MW1–MW8 authority, the frozen central region (anchored, unchanged), the client renderer,
MV2.A/.A2/.B/.C certs (still green), MV1/PX1–PX3. No `WrapIntoRegion`, no `ReconstructedZ`,
no global relief crank, no erosion simulation.
