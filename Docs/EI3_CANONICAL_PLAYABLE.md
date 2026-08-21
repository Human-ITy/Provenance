# EI3 canonical playable package

Run `PLAY_EI3_CANONICAL.cmd` from the canonical ProvenanceClient root.

The launcher owns exactly one local FableScript process. It refuses occupied
ports, performs canonical control and bulk handshakes, admits all 25 Page V2
artifacts, validates one detailed projection, and only then opens
`ProvenanceClient.exe --ei3-authority`. Closing the client requests a clean
shutdown and targets only the authority PID created by that launcher.

Persistent playthrough identity:

`C:\Users\D-Day\ProvenanceWorkspace\State\canonical-playable\world-instance.json`

Replaceable drainage contexts live under workspace `Cache`, never beside the
committed authority pages. Per-launch logs and machine receipts live under
`Evidence\Playable\launcher-runs`.

The visible startup declaration must report:

```text
AUTHORITY SESSION: VALID
WORLD UUID: ...
MACRO GENESIS: ...
WORLD BASELINE: ...
MACRO PAGES: 25/25 VALID
DETAIL PROJECTION: ACTIVE
PRESENTATION PATH: EI3 AUTHORITATIVE
```

This deployment does not certify EI3.V player-view continuity or EI3.Q visual
richness. Human play smoke remains a separate interaction receipt.
