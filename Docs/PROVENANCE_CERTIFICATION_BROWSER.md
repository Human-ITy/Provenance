# Provenance Certification Browser

Status: Stage-12-integrated playtest shell.

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
GEO.FAULT_DISPLACEMENT
GEO.BREACH_CONTINUITY
```

Stage numbers remain useful historical labels, but stable identifiers are the
permanent keys. The browser groups entries by domain and shows actual runtime
state, requirements, and provided authority. It does not impose a numbered-stage
ceiling.

The browser groups stages by domain. Left/Right selects a domain, Up/Down or the
mouse wheel selects a stage, and Enter loads it. Number keys remain convenience
shortcuts for the entries that have them; later stages do not depend on adding
another global key.

Stage 12 is a proof-only extension of Stage 11. It does not mint a new deposit
or alter terrain. The selected present surface directly intersects the existing
faulted quartz body, and the geology flashlight follows that same FeatureId
below the surface.

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
