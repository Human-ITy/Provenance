# Stage 10 - Causal Contact Mineralization

Stable certificate ID: `GEO.CONTACT_MINERALIZATION`

Status: certified read-only causal worldgen authority.

## Scope

Stage 10 adds one mineralizing event whose admission is caused by the Stage-9
granite contact. It does not add generic ore noise or surface decoration.

```text
folded sandstone/shale host
  + younger granite intrusion
  + bounded exterior contact band
  + host structure and deterministic fracture permeability
  + mineralizing event
  -> persistent quartz contact body
```

The deposit descriptor records its own deposit-system identity, `FeatureId`,
parent intrusion feature/event, mineralizing event, and chronology. The active
surface is reconstructed exactly as Stage 9 and then re-queried against the
assembled geology; terrain height is never used to assign quartz.

## Certificate gates

- Stage 9 must pass unchanged.
- Disabling the mineralizing event yields zero deposit samples.
- Every admitted sample is outside granite, in sandstone or shale, inside the
  bounded contact band, and above the permeability threshold.
- Raising only the permeability threshold predictably removes deposit samples.
- Deposit identity is invariant across query order, partitioning, and reload.
- Parent geology remains identical when the mineralizing event is disabled.
- Stage-9 geometry digest remains unchanged.
- The 64 m surface fixture remains bounded.

The certified fixture produces 33 surface deposit samples under the default
control and one under the stricter permeability control.

## Explicit closures

Faulting, surface-breach continuity, exact occupancy, D2 materialization,
player fracture, active erosion, water, sediment transport, and P5b remain
closed.

## Run

```text
CERT_CAUSAL_WORLD_CONTACT_MINERALIZATION.cmd
```

The playable counterpart is available from `PLAY_WORLDGEN_STAGE0.cmd` as
`GEO.CONTACT_MINERALIZATION` (shortcut `8`).
