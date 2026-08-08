# Provenance Material × Tool Interaction Index v1.2 (Esoterica pin)

**Status:** normative companion to Horizon-to-Hand — implemented in `HorizonToHand.h`  
**Date:** 8 August 2026

## Governing rules

1. Tool supplies action volume/force; material supplies cut/fracture/settling language.  
2. Contact envelope ≠ transfer volume ≠ body mass.  
3. Canonical voxel = accounting; occupied geometry × density = mass (integer floor, never mint).  
4. Hands/shovel do not mine intact hard rock — pick frees it; hands collect loose only.

## LIVE values encoded

| Tool | Contact r (m) | Dig tier | Transfer budget |
|------|---------------|----------|-----------------|
| Hand | 0.16 | 0 | 244.14 mL (1/8 voxel) |
| Shovel | 0.28 | 0 | ~2× handful |
| Pick | 0.18 | 3 | ~1.5× handful |
| Axe | — | wood chop | not terrain dig |

| Material | g/voxel | Hardness |
|----------|---------|----------|
| dirt | 2500 | 1 |
| sand | 3125 | 1 |
| gravel | 3100 | 1 |
| clay | 3900 | 1 |
| sandstone | 4492 | 3 |
| shale | 4688 | 3 |
| limestone | 5078 | 3 |
| granite | 5273 | 3 |
| stone | 5300 | 3 |
| mica_schist | **5469** | 3 |
| basalt | 5859 | 3 |
| water | 1953 | 0 |

## Play checks

- Shovel on mica_schist → refuse (“collect loose only”)  
- Pick on mica_schist → H2H fracture path  
- Shovel on dirt → scoop with ~dirt density grams, not stone crater  
- Aim shows LIVE contact sphere; soft cup scar stays transfer-scale  

## TARGET not yet fully expressed

Per-material cavity language (sand repose vs clay shear vs granite joints), bucket solid admission, wood axe hinge, cave support graph, mutation receipts on wire.
