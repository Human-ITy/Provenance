# FableScript ↔ Esoterica Geology Authority Bridge

Status: Cut A integration gate. Stage 11 remains closed.

> FableScript determines what geological history exists. Esoterica may evaluate
> that history locally, but may not originate a competing geological answer.

`Tools/Worldgen/compile_geology_authority_bridge.py` runs on the authority side
of the boundary. It compiles the certified Stage 5–10 products into:

- `Data/Worldgen/fablescript_geology_authority_bridge_v1.cgab` — the canonical
  world, generator, feature, event, intrusion, deposit-system, and deposit-body
  envelope;
- `Data/Worldgen/fablescript_geology_authority_parity_v1.tsv` — 4,103 hostile
  three-dimensional reference queries evaluated independently of Esoterica.

`CERT_GEOLOGY_AUTHORITY_PARITY.cmd` regenerates those products and asks the
Esoterica reference client to compare material, `FeatureId`, formation/body,
deposit system/body, structural frame, inside/outside admission, contact
distance, permeability, chronology, and event ancestry.

Different evaluators are permitted. Different geological answers are not.

This gate does not claim FableScript issue #18, exact occupancy, D2 integration,
faulting, water, or P5b. Those remain Cut B/C and later work.
