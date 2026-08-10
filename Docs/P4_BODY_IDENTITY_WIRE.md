# P4.2 — Authoritative body / aggregate identity (wire contract)

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

## Local cert note

Live cert against `_engine_truth_lane3` may carry a local bridge patch that emits these
fields. That patch is **not** committed to Mygame git; this contract is the Esoterica
source of truth for Unreal + Esoterica parity.
