# MW9 — Realized flora (fauna CLOSED)

```
MW9 FLORA REALIZATION CERTIFIED —
ECOLOGY IS PERSISTENT WORLD HISTORY, NOT BIOME PAINT
```

MW8 stays certified potential. MW9 records what actually establishes, persists,
spreads, competes, reproduces, declines, and dies. Suitability is not presence.
BiomeId is not a spawn mask. Flora is not a renderer scatter pass. Fauna stays
CLOSED.

```
live canonical geography
  → hydroclimate → regolith/soil → drainage → exposure
  → MW8 regime potential
  → source availability
  → establishment → growth / competition / mortality / spread
  → persistent MW9 ecological state
```

## Freeze

MW8 frozen @ `e8155fa3` (live Stage0 orographic consume). MW9-off leaves that
field exact. AdoptPage / QueryContext / ClassifyCell / CompileMw8 are not
rewritten. FableScript opcodes unchanged — realized state is client-side,
versioned, persisted with the world.

## Identity

| | |
|---|---|
| worldgen_id | `provenance_realized_flora_v1` |
| MW9 version | `1` |
| law | `orographic.phase17` only |
| fauna | CLOSED |
| biomass matter | CLOSED (debt recorded; harvest does not move voxel matter) |

Historical v11 library worlds do not receive canonical MW9.

## Proving guild (data-driven contracts)

1. `alpine_cushion_herb` — alpine low veg
2. `windward_moss_sedge` — moisture-loving
3. `leeward_dry_shrub`
4. `riparian_willow_herb`
5. `basin_wetland_sedge`
6. `generalist_bunchgrass`

No one-off special cases in establishment/growth. Species differ only by
contract fields (temperature, wetness, exposure, substrate, dispersal).

## State model

Separate from BiomeId: `SpeciesId`, `PatchId`, `PopulationId`, `OrganismId`,
`LifeStage`, biomass, density, health/stress, age, seed bank / inbound
propagule, competition pressure, disturbance.

Patch (64 m) is far truth. Near the player, individuals are instantiated from
the patch; the patch remains authority. Chopped `OrganismId` stays dead across
reload.

Init is keyed from world seed + genesis + orographic identity + MW8 digest +
SpeciesId + spatial identity + MW9 version. There is no one global RNG.
Page load does not reroll.

Dispersal: wind (exposure / moisture azimuth), water (downstream flow), gravity
(downslope). Page edges are not ecological boundaries.

Disturbance: harvest/cut → bare → pioneer → intermediate → mature.

## Cert

```
CERT_MW9_ECOLOGY.cmd
  live emit (orographic_production_page) then
  ProvenanceClient.exe --cert-mw9-ecology
```

Receipt: `Docs/provenance_mw9_ecology_cert.txt`. Banked pages are oracle only;
live emit is authority.

## Play

Default Stage0 path unchanged. After orographic page admit, the client compiles
MW9 flora state. No vegetation scatter renderer is opened.

## CLOSED

- fauna (no grazer guild)
- voxel biomass-matter ledger
- MW8 rewrite / GradeToZ / second heightfield
- FableScript producer/consumer/adopt_page
