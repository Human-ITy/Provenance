# Provenance Certification Browser

Status: Stage-10-integrated playtest shell.

## Ruling

Certificates define the truth under test. Tools inspect that truth. Overlays
change diagnostic presentation. A tool or overlay does not become another
world merely so the player can inspect the active certificate.

The playable shell therefore has two independent surfaces:

```text
M  Certification Browser -> selects authoritative playable runtime
T  Global Playtest Toolkit -> inspects the current runtime
```

## Stable certificate identities

```text
PERF.CLEAN_WORLD
CAL.COMBINED
GEO.KERNEL
GEO.EXPOSURE
GEO.VISIBLE_EXPOSURE
GEOMORPH.DIFFERENTIAL_EROSION
GEO.GRANITE_INTRUSION
GEO.CONTACT_MINERALIZATION
```

Stage numbers remain useful historical labels, but stable identifiers are the
permanent keys. The browser groups entries by domain and shows actual runtime
state, requirements, and provided authority. It does not impose a numbered-stage
ceiling.

Navigation is Up/Down or mouse wheel plus Enter. Keys 1-8 are convenience
shortcuts only. Adding later certificates does not require a new control scheme.

## Tool availability

The current global tools are ruler, palette, geology identity, formation
contacts, bedding data, residency rings, performance HUD, mutation/revision HUD,
and provenance trace. `L`, `P`, and `R` remain direct shortcuts.

The drawer reports unavailable features honestly:

```text
Occupancy / fill       LOCKED: Stage 0 has no occupancy
D2 topology            LOCKED: Stage 0 keeps D2 dormant
Water authority        LOCKED: P5b closed
Collision              PLANNED
Package boundaries     PLANNED
Structural faces       PLANNED
```

## Deferred shell extensions

A/B comparison, camera bookmarks, deterministic paths, generic performance
capture, seed/world controls, the full aimed material inspector, and failure
jump remain planned extensions. Their eventual implementation belongs to this
shared playtest shell, not to bespoke certificate worlds.
