# MV2 Sky-Gap Attribution — Silhouette-Stack Diagnostic (measurement only)

The red-marked sky band between mid terrain and the distant MV2 range was investigated
as an **unresolved visual-depth question**, not assumed solved. Measurement instrument
only — **the renderer was not changed.**

## Method

For each of the 10 viewpoints, per screen column, measure the **authority** terrain
elevation-angle envelope by distance band and compare to the **rendered** silhouette:

- bands: **0–32 km** (MV1 `ReconstructedZ`), **32–50 / 50–80 / 80–128 km** (macro pages)
- per column: authority max elevation angle in each band; rendered near-terrain-top and
  far-massif-bottom from the terrain-present mask (py → elevation angle)
- classify each column that has a distant range:
  - **LAYERED** — mid bands (32–80 km) rise above the near (0–32) silhouette **and** are drawn
  - **LOST (B)** — mid bands rise above the near silhouette in *authority* but the render shows sky
  - **BASIN (A)** — mid bands stay at/below the near silhouette (far massif above a genuinely low mid-ground)

Run: `ProvenanceClient.exe --cert-mv2-silhouette` →
`Docs/provenance_mv2_silhouette_analysis.txt` + per-view CSVs +
`Docs/provenance_mv2_silhouette_stacks.png`.

## Result — owner is A, not B

```
AGGREGATE (views with far terrain):  frac_basin=0.72  frac_lost=0.00  frac_layered=0.28
OWNER = A  authority genuine negative space
```

**`frac_lost = 0.00` in every single view.** Not one column, in any viewpoint, has
authority mid-band (32–80 km) terrain rising above the near silhouette that the render
fails to draw. **No intermediate terrain is being lost or under-represented by the
MV1→MV2 handoff.**

The band-angle envelopes (authority) tell the geometry:

| view | near(0-32)° | 32-50° | 50-80° | 80-128° | verdict |
|---|---|---|---|---|---|
| trunk_valley | **14.4** | 1.0 | 0.7 | 0.0 | BASIN — near hill towers, distant terrain flat at the horizon |
| ridge_shoulder | 0.1 | −1.5 | 0.2 | 0.1 | LAYERED — mid/far rise above near and **are** rendered |
| high_divide | −0.9 | −1.8 | −1.2 | −0.8 | elevated camera; all terrain below the horizon (looking out over low country) |
| foreland_basin | 7.6 | 0.8 | 0.9 | 0.4 | BASIN — foreground rim high, mid/far low |
| cardinal_N/E | 5.0 / 6.2 | <0 | <0 | <0 | BASIN — origin datum rim above low distance |

The distant range reads as a thin strip because it **is geometrically low and far**
(≈0–1° of apparent rise), separated from the near ground by **real basin negative
space** — exactly the mountain-vista phenomenon the earlier gap-classifier already
flagged as legitimate line-of-sight. The silhouette stack proves this is faithful to
the authority: the mid-ground genuinely sits below the near/elevated silhouette; the far
massif rises above a real basin. `provenance_mv2_silhouette_stacks.png` shows the near
line (white) towering while the mid/far bands hug the horizon (trunk_valley), and mid/far
bands stacking above near where they *are* drawn (ridge_shoulder).

## Verdict per case & smallest next cut

- **Ownership: Case A — genuine geographic negative space. Not Case B.** The MV1→MV2
  representation is faithful; nothing is over-smoothed away. **No representation fix is
  warranted** (there is no lost intermediate structure to restore).
- The "floating strip" impression is the combination of (1) real basin negative space
  and (2) the far range being genuinely low-relief in these directions.
- **Smallest next cut (only if the *perception* is to be improved — a deliberate
  choice, not a correctness fix):** a presentation-only **basin depth cue** — a graded
  distance value/haze gradient so the negative space reads as basin *depth* rather than a
  flat sky void. This is the lever the earlier ruling reserved; it does **not** change
  authority, relief, or the page step. It was **not** done here (this cut is measurement
  only, and presentation tuning was explicitly out of scope).
- Cranking macro relief or the page step is **not** justified by this data: the relief is
  faithfully represented; the ranges are low here because the world is genuinely low in
  these directions (a Smokies-style low, layered horizon is legitimate).

No implementation beyond the measurement instrument. Renderer, authority, palette, and
aerial perspective all unchanged.
