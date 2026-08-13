# Playable Runtime Independence and Toolkit Attachment

Status: certified prerequisite for Stage 11.

## Runtime independence

Every playable certificate must establish its own declared terrain,
residency, collision, and grounding prerequisites. It may not depend on a
previously visited certificate having initialized a presentation runtime.

The windowed certificate covers direct clean-process Stage-10 selection and:

```text
5 -> 10   6 -> 10   7 -> 10   8 -> 10   9 -> 10
10 -> 5   10 -> 7   10 -> 9   10 -> 10
```

Each case requires successful selection, non-empty residency, settled queues,
visible terrain packages and triangles, valid collision, and a grounded camera.
Every path into Stage 10 must publish the same deterministic surface digest.

The closed defect was an outer Stage-8/9 draw guard that omitted the Stage-10
runtime pointer even though the inner block builder already understood Stage
10. Direct Stage-10 selection therefore returned before publishing terrain.

## Toolkit surface attachment

Ruler and palette truth remains the unmodified authoritative terrain anchor.
Rendered diagnostic geometry uses:

```text
rendered point = authoritative surface point + surface normal * 0.008 m
```

The normal comes from local 12.5 cm terrain probes. It is not a global `+Z`
offset. Palette material relief is also applied along that normal. Gravel meso
details use a tangent frame derived from the same surface normal.

Broad ruler panels, narrow ruler dashes, and palette swatch bases are clipped to
the active runtime's canonical 0.5 m terrain triangles. Clipped vertices inherit
the exact source triangle plane before the diagnostic normal offset is applied.
This prevents a nominally fitted four-corner quad from bridging intervening HF
or D2 facets and clipping through them.

The certificate checks 810 hostile surface samples across the runtime
transition suite, reports zero penetration, limits gross floating, verifies the
ruler's triangle-lattice fit error, and confirms that ruler and palette display
lists rebuild after stage changes.

## Run

```text
CERT_PLAYABLE_RUNTIME_INDEPENDENCE.cmd
```

Receipts:

```text
Docs/provenance_playable_runtime_independence_cert.txt
Docs/provenance_playtest_toolkit_surface_attachment_cert.txt
```
