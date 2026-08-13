# Stage 9 - Granite Intrusion

Status: certified younger 3D geological body over the Stage-8 surface.

Run `CERT_CAUSAL_WORLD_GRANITE_INTRUSION.cmd`, or launch
`PLAY_WORLDGEN_STAGE0.cmd` and select `GEO.GRANITE_INTRUSION` (shortcut `7`).

Stage 9 preserves the folded sedimentary catalog and Stage-8 relief. It adds one
irregular subsurface pluton with a new stable `FeatureId`, intrusion chronology
40, cooling chronology 50, and a granite joint frame independent of host
bedding. The body wins overlap only inside its analytical 3D boundary because
it is younger than the folded host.

The intrusion-disabled control contains no granite. The enabled control replaces
host only where the body physically occupies space. Surface exposure is always
queried after Stage-8 reconstruction; granite is never selected by elevation or
painted onto terrain.

Certified gates include new identity, unchanged sedimentary IDs, host
truncation, deterministic chronology, non-blended overlap, independent joint
frame, surface re-query, vertical/lateral continuity, order invariance,
unchanged Stage-8 geometry, and bounded residency.

Still excluded: mineralization, faulting, active erosion, water, sediment
transport, digging, bodies, and P5b. Contact mineralization belongs to Stage 10.
