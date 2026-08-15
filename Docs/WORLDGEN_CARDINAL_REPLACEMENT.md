# Worldgen Cardinal Replacement Gate

This permanent certificate proves complete live-terrain replacement and exact
seed-space regeneration. It is a required integration gate before the
bare-earth macro geography proof.

The test runs Stage 0 and certified Stages 5-11. For every stage it performs:

```text
N  +192 m -> origin
E  +192 m -> origin
S  +192 m -> origin
W  +192 m -> origin
```

Every route is incremental and contains walking, sprinting, and free-flight
segments in both directions. It does not teleport to a prebuilt endpoint.

At each outer station the certificate requires:

- zero origin terrain-package keys remain live;
- no more than 400 terrain packages and 15,000 resident cells;
- render, collision, and authority queries remain available;
- resident material names match the active stage authority oracle;
- sampled geological FeatureIds exist where causal geology is active;
- zero lower-frame sky pixels and zero fallback-green pixels;
- an outer-station visual receipt is written.

After returning, the origin must reproduce:

- the same geometry digest;
- the same material/FeatureId digest;
- the same collision digest;
- the same package-key and triangle digest;
- zero changed fixed-camera pixels above a two-channel tolerance.

The terrain display-list path uses stable terrain lighting. Camera-dependent
specular lighting must never be baked into retained packages because that would
make regenerated terrain change appearance based on approach direction.

Stage 7 maintains a deterministic one-package prefetch collar. This prevents a
cold origin and returned origin from having different path-shaped package sets.

Run:

```text
CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd
```

Outputs:

```text
Docs/provenance_worldgen_cardinal_replacement_cert.txt
Docs/provenance_worldgen_cardinal_replacement_trace.csv
Docs/provenance_cardinal_<stage>_<bearing>_outer.ppm
Docs/provenance_cardinal_visual_audit_all.png
```

Passing this certificate closes world-replacement correctness only. It is
**Test A** of the standing traversal matrix. It does not prove indefinite
travel. Long-haul bounded residency is **Test B**:
`TRAVERSAL_STREAMING_SOAK_HANDOFF.md` / `--cert-streaming-soak`.

Synchronous residency/reconstruction spikes remain a separate performance
gate. Macro geography remains blocked until the Stage 6 and compiled-relief
hitch lane is closed.
