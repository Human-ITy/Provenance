# Default Stage0 live stream = orographic.phase17

```
DEFAULT STAGE0 LIVE STREAM = OROGRAPHIC.PHASE17
```

MW9 flora realized on this same live consume path (fauna CLOSED). MW8 doctrine/gates untouched. WorldGenesis v11 library worlds
remain v11. Native Stage0 consume (`AdoptPage` / `QueryContext`) is unchanged
except it now prefers **live-emitted** pages when the live receipt is present.

## Architecture

```
PLAY_PROVENANCE_STAGE0_WORLD.cmd
        ↓
Worlds UI  (default new world seed 20260827)
        ↓
tools.serve_worldgen_projection   127.0.0.1:8765 / bulk 8766
        ↓
opcode orographic_production_page + orographic_ecological_context
        ↓
AdoptPage / QueryContext / MW8
```

Fail-closed: those opcodes refuse unless the persisted origin stamps
`terrain_law=orographic.phase17`. Existing v11 origins (`provenance-stage0-genesis-010`
and other library seeds) still stream WorldGenesis `macro_page` and cannot be
silently relabeled.

## Canonical openworld

- seed `20260827`
- `terrain_law=orographic.phase17`
- tectonic `b74f957a7fdb429a`
- orographic `3198784442eccd99`
- page (1,1) digest `03f579fed39e4685…`
- instance file `world-instance-stage0-orographic-phase17-<seedhash>.json`
  (distinct from `world-instance-stage0-genesis-v11-*`)

Handshake still carries WorldGenesis session binding so `--ei3-authority` attaches.
Additive hello fields name the live law: `terrain_law`, `genesis_identity=orographic.phase17`,
`tectonic_identity`, `orographic_identity`. Macro pages are not stamped as phase17.

## Proof

```
python -m tools.cert_stage0_live_orographic_stream
CERT_STAGE0_MW8_OROGRAPHIC.cmd
```

Live emit matches the banked golden (oracle). TCP control/bulk lanes carry
`orographic_production_page`. Native MW8 on those live bytes:

- alpine 74, windward 0.890 / leeward 0.203, riparian 379, basin 149
- H2H 289, MW8-off Δ 0, hydro-off 0, page-boundary 51
- no GradeToZ, no second heightfield, travel rebuilds 0
- `live_wire=orographic_production_page`
- MW9 flora realized on the same live pages (fauna CLOSED)

## Preserved

- native launcher / Worlds UI
- streaming / page scheduling / 8765+8766
- collision / grounding / render path
- AdoptPage / QueryContext
- `_grade_at = tectonic.grade_at`
- MW8 gates
- MW9 flora (CERT_MW9_ECOLOGY.cmd); fauna CLOSED
