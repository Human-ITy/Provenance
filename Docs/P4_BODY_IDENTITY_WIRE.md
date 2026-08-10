# P4.2 / P4.4 — Authoritative body / aggregate identity (wire contract)

**Law:** Fablescript issues identity. Clients (Esoterica, Unreal) do **not** invent
`lastAuthBodyId` / aggregate ids from local H2H prediction. Representation may sleep or
aggregate; the receipt identity survives.

## Receipt fields (carve / place success)

Top-level keys on the method result (newline JSON, protocol v1):

| Field | Type | Meaning |
|---|---|---|
| `body_id` | uint64 | Authoritative detached / scoop body id (0 if none) |
| `aggregate_id` | uint64 | Authoritative place / pile aggregate id (0 if none) |
| `form_seed` | uint64 | Authoritative form seed (optional; 0 if none) |
| `fracture_seed` | uint64 | Authoritative fracture seed (optional; 0 if none) |
| `parent_id` | uint64 | Parent / source identity (0 if none) |
| `provenance` | string | Compact provenance tag (source/action/tick) |
| `removed` / `placed_by` | object | Material → grams (existing) |
| `rev` | int | Terrain revision (existing) |

Both clients must present the same `body_id` / `aggregate_id` from the same receipt
(e.g. HUD / digest `body 18472`).

## Refuse

REFUSE / `nothing_to_dig` / empty place: no new identity. Client rolls back prediction (P4.1).

## Allocator

Fablescript owns a monotonic matter-identity allocator (floor may start at 18472 for
readability). Clients never allocate authoritative ids.

## Upstream land (P4.4)

Landed in production authority:

- Repo: `Human-ITy/plaintxt-decoded` branch `client-terrain-residency`
- File: `fablescript/engine/terrain_mutate.py` (`_issue_matter_identity` on carve/place success)
- SHA: `5a69afb3bc65690041f7c2fab69182df8db3bbda`

Live `_engine_truth_lane3` certs prove against that committed path. The historical local
bridge patch (`Docs/patches/p42_patch_terrain_mutate_body_identity.py`) is **deprecated**
and must not be required for `--cert-p4` / headless probes.

Save/reconnect matter-id fetch is not on the wire yet (`_next_matter_id` is process-local);
headless records that as SKIP until a persistence path exists.
