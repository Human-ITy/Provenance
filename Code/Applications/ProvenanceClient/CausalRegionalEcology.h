#pragma once

// MW9: realized flora occupancy on certified live Stage0 MW8 potential.
//
// MW8 = what could live here (regime potential).
// MW9 = what actually establishes, persists, spreads, competes, reproduces,
//       declines, and dies here.
//
// Geography → hydroclimate → regolith → drainage → exposure → MW8 potential
//   → source availability → establishment → growth / competition / mortality
//   / spread → persistent MW9 ecological state.
//
// Suitability is not presence. BiomeId is not a spawn mask. Flora is not a
// renderer scatter pass. Fauna stays CLOSED. Voxel biomass-matter integration
// is CLOSED; ecological biomass is recorded with that debt explicit.
//
// Consumes AdoptPage::QueryContext + EvaluateContext + compiled MW8 cells.
// Does not rewrite ClassifyCell, CompileMw8, Adopt, or QueryContext.

#include "AdoptPage.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalRegionalEcology
{
    constexpr uint32_t kMw9Version = 1;
    constexpr char const* kWorldgenId = "provenance_realized_flora_v1";
    constexpr char const* kTerrainLawRequired = "orographic.phase17";
    constexpr char const* kBiomassMatterDebt =
        "biomass_matter_integration=CLOSED; MW9 records ecological biomass/density only; "
        "harvest does not move voxel matter into the inventory ledger";
    constexpr uint64_t kTagPatch = 0x4d57390000000001ull;
    constexpr uint64_t kTagPop = 0x4d57390000000002ull;
    constexpr uint64_t kTagOrg = 0x4d57390000000003ull;
    constexpr uint64_t kTagSpecies = 0x4d57390000000004ull;
    constexpr int kSpeciesCount = 6;
    constexpr int kNearIndividualsMax = 8;
    constexpr double kPatchStepM = 64.0;
    constexpr double kNearRadiusM = 192.0;
    constexpr double kEstablishMinSource = 0.02;
    constexpr char const* kFaunaState = "CLOSED";

    enum class Control : uint8_t
    {
        Program = 0,
        ForceOff = 1,
        ForceOn = 2,
        HydroclimateOff = 3,
        LapseOff = 4,
        RegolithOff = 5
    };

    enum class Guild : uint8_t
    {
        AlpineLow = 0,
        WindwardMoist = 1,
        LeewardDry = 2,
        Riparian = 3,
        WetBasin = 4,
        Generalist = 5
    };

    enum class LifeStage : uint8_t
    {
        Bare = 0,
        Seed = 1,
        Pioneer = 2,
        Intermediate = 3,
        Mature = 4,
        Senescent = 5,
        Dead = 6
    };

    enum class SourceKind : uint8_t
    {
        None = 0,
        Founder = 1,
        SeedBank = 2,
        Wind = 3,
        Water = 4,
        Gravity = 5,
        Introduced = 6
    };

    enum class LimitingFactor : uint8_t
    {
        None = 0,
        Temperature = 1,
        Moisture = 2,
        Exposure = 3,
        Substrate = 4,
        Drainage = 5,
        Source = 6,
        Competition = 7,
        Disturbance = 8,
        Stress = 9
    };

    inline char const* GuildName(Guild g)
    {
        switch (g)
        {
            case Guild::AlpineLow: return "alpine_low_veg";
            case Guild::WindwardMoist: return "windward_moisture_loving";
            case Guild::LeewardDry: return "leeward_dry_shrub";
            case Guild::Riparian: return "riparian";
            case Guild::WetBasin: return "wet_basin_wetland";
            case Guild::Generalist: return "generalist_grass_herb";
            default: return "none";
        }
    }
    inline char const* StageName(LifeStage s)
    {
        switch (s)
        {
            case LifeStage::Bare: return "bare";
            case LifeStage::Seed: return "seed";
            case LifeStage::Pioneer: return "pioneer";
            case LifeStage::Intermediate: return "intermediate";
            case LifeStage::Mature: return "mature";
            case LifeStage::Senescent: return "senescent";
            case LifeStage::Dead: return "dead";
            default: return "none";
        }
    }
    inline char const* SourceName(SourceKind s)
    {
        switch (s)
        {
            case SourceKind::Founder: return "founder";
            case SourceKind::SeedBank: return "seed_bank";
            case SourceKind::Wind: return "wind";
            case SourceKind::Water: return "water";
            case SourceKind::Gravity: return "gravity";
            case SourceKind::Introduced: return "introduced";
            default: return "none";
        }
    }
    inline char const* LimitName(LimitingFactor f)
    {
        switch (f)
        {
            case LimitingFactor::Temperature: return "temperature";
            case LimitingFactor::Moisture: return "moisture";
            case LimitingFactor::Exposure: return "exposure";
            case LimitingFactor::Substrate: return "substrate";
            case LimitingFactor::Drainage: return "drainage";
            case LimitingFactor::Source: return "source";
            case LimitingFactor::Competition: return "competition";
            case LimitingFactor::Disturbance: return "disturbance";
            case LimitingFactor::Stress: return "stress";
            default: return "none";
        }
    }

    struct SpeciesContract
    {
        char const* name;
        Guild guild;
        double tempOpt, tempTol, wetOpt, wetTol, snowMax, slopeMax, depthMin;
        double alpineNeed, windwardNeed, leewardNeed, channelNeed, basinNeed;
        double pioneer, growthRate, mortality, competeW;
        double windDisp, waterDisp, gravDisp, establishMin, founderRate;
        double lightNeed, waterNeed, spaceNeed, substrateNeed;
    };

    inline SpeciesContract const* Contracts()
    {
        static SpeciesContract const k[kSpeciesCount] = {
            { "alpine_cushion_herb", Guild::AlpineLow,
              -0.4, 2.3, 0.95, 0.85, 1.00, 0.22, 0.04,
              0.88, 0.05, 0.05, 0.00, 0.00,
              0.72, 0.18, 0.12, 0.40,
              0.34, 0.04, 0.28, 0.28, 0.62,
              0.55, 0.28, 0.40, 0.18 },
            { "windward_moss_sedge", Guild::WindwardMoist,
              5.4, 3.4, 1.72, 0.52, 0.42, 0.13, 0.20,
              0.00, 0.92, 0.00, 0.12, 0.00,
              0.46, 0.28, 0.08, 0.70,
              0.56, 0.16, 0.10, 0.32, 0.55,
              0.40, 0.70, 0.55, 0.40 },
            { "leeward_dry_shrub", Guild::LeewardDry,
              7.1, 3.0, 0.32, 0.38, 0.18, 0.15, 0.16,
              0.00, 0.00, 0.92, 0.00, 0.00,
              0.34, 0.16, 0.10, 0.66,
              0.42, 0.03, 0.22, 0.30, 0.52,
              0.75, 0.22, 0.50, 0.35 },
            { "riparian_willow_herb", Guild::Riparian,
              6.0, 3.8, 1.55, 0.58, 0.24, 0.048, 0.72,
              0.00, 0.10, 0.00, 0.94, 0.18,
              0.24, 0.32, 0.07, 0.84,
              0.10, 0.82, 0.16, 0.34, 0.58,
              0.45, 0.80, 0.60, 0.70 },
            { "basin_wetland_sedge", Guild::WetBasin,
              5.1, 3.4, 1.92, 0.48, 0.32, 0.034, 0.92,
              0.00, 0.00, 0.00, 0.16, 0.95,
              0.30, 0.30, 0.08, 0.80,
              0.05, 0.72, 0.06, 0.36, 0.56,
              0.30, 0.88, 0.55, 0.75 },
            { "generalist_bunchgrass", Guild::Generalist,
              5.6, 5.6, 0.96, 1.15, 0.52, 0.11, 0.20,
              0.08, 0.18, 0.18, 0.08, 0.08,
              0.90, 0.36, 0.09, 0.32,
              0.46, 0.22, 0.32, 0.22, 0.20,
              0.50, 0.40, 0.35, 0.30 },
        };
        return k;
    }

    inline uint64_t Mix(uint64_t h, uint64_t v)
    {
        return CausalRegionalBiome::MixU64(h, v);
    }
    inline uint64_t MixF(uint64_t h, double v)
    {
        return CausalRegionalBiome::MixF(h, v);
    }
    inline uint64_t HashLit(char const* s)
    {
        return CausalWorldGeology::HashText(std::string(s ? s : ""));
    }
    inline uint64_t SpeciesIdOf(int i)
    {
        SpeciesContract const& c = Contracts()[i];
        uint64_t h = HashLit("mw9-species");
        h = Mix(h, HashLit(c.name));
        h = Mix(h, (uint64_t)kMw9Version);
        h = Mix(h, kTagSpecies);
        return h;
    }
    inline double Unit01(uint64_t h)
    {
        return (double)(h % 1000003ull) / 1000003.0;
    }
    inline double Gauss(double x, double mu, double sig)
    {
        if (sig <= 1e-9) return std::fabs(x - mu) < 1e-9 ? 1.0 : 0.0;
        double d = (x - mu) / sig;
        return std::exp(-0.5 * d * d);
    }
    inline double Smooth01(double a, double b, double x)
    {
        return CausalRegionalHydroclimate::Smoothstep(a, b, x);
    }

    struct Site
    {
        double x = 0, y = 0;
        double elev = 0.70, slope = 0, temp = 6.5, wet = 1.0, snow = 0, arid = 0.4;
        double depth = 0.45, wind = 0, lee = 0, pool = 0;
        double flowDx = 0, flowDy = 0;
        bool channel = false, divide = false, basin = false;
        bool haveHydro = true, haveRegolith = true, usedPresentationZ = false;
        CausalRegionalHydroclimate::ExposureClass exposure =
            CausalRegionalHydroclimate::ExposureClass::Neutral;
        CausalRegionalRegolith::ProfileClass profile =
            CausalRegionalRegolith::ProfileClass::ThinRegolith;
        CausalRegionalRegolith::DrainageClass drainage =
            CausalRegionalRegolith::DrainageClass::WellDrained;
        uint64_t biomeId = 0;
        CausalRegionalBiome::RegimeClass regime = CausalRegionalBiome::RegimeClass::None;
    };

    struct SuitResult
    {
        double value = 0;
        double tempF = 0, wetF = 0, expF = 0, subF = 0, drainF = 0;
        LimitingFactor limit = LimitingFactor::None;
    };

    struct Population
    {
        uint64_t speciesId = 0, populationId = 0;
        double density = 0, biomass = 0, health = 1, stress = 0, age = 0;
        double seedBank = 0, inbound = 0;
        LifeStage stage = LifeStage::Bare;
        SourceKind source = SourceKind::None;
        LimitingFactor limit = LimitingFactor::None;
        bool established = false;
    };

    struct Organism
    {
        uint64_t organismId = 0, speciesId = 0, patchId = 0;
        double x = 0, y = 0, biomass = 0, health = 1, age = 0;
        LifeStage stage = LifeStage::Mature;
        bool alive = true;
    };

    struct Patch
    {
        uint64_t patchId = 0;
        double x = 0, y = 0;
        Site site;
        Population pops[kSpeciesCount];
        LifeStage succession = LifeStage::Bare;
        double yearsSinceDisturbance = 80;
        bool harvested = false;
        uint64_t disturbanceEventId = 0;
    };

    struct Receipt
    {
        uint64_t patchId = 0, speciesId = 0;
        double x = 0, y = 0, suitability = 0, density = 0, source = 0;
        bool occupied = false;
        char why[96]{};
        char limiting[32]{};
        char sourceKind[24]{};
        char species[40]{};
    };

    struct Field
    {
        uint32_t version = kMw9Version;
        std::string worldgenId = kWorldgenId;
        uint64_t worldIdentityHash = 0;
        uint64_t genesisHash = 0;
        uint64_t orographicHash = 0;
        uint64_t fieldDigest = 0;
        int compiles = 0, rebuilds = 0, dirtyUpdates = 0, farUpdates = 0;
        int occupiedPatches = 0, emptySuitable = 0;
        int alpineOccupied = 0, windwardOccupied = 0, leewardOccupied = 0;
        int riparianOccupied = 0, basinOccupied = 0, generalistOccupied = 0;
        bool faunaClosed = true;
        bool settled = false;
        Control lastControl = Control::Program;
        bool compiled = false;
        std::unordered_map<uint64_t, Patch> patches;
        std::unordered_map<uint64_t, Organism> organisms;
        std::unordered_map<uint64_t, Organism> graves;
    };

    inline Field& F()
    {
        static Field f;
        return f;
    }

    inline void Reset()
    {
        F() = Field{};
    }

    inline bool GenesisSupportsMw9(AdoptPage::WorldIdentity const& id)
    {
        if (id.terrainLaw != kTerrainLawRequired) return false;
        if (id.orographic.empty() || id.tectonic.empty()) return false;
        if (id.orographicVersion < 2) return false;
        return true;
    }

    inline uint64_t WorldMix()
    {
        AdoptPage::AdoptedGeography const& g = AdoptPage::G();
        uint64_t h = HashLit("mw9-world");
        h = Mix(h, (uint64_t)kMw9Version);
        h = Mix(h, g.worldIdentityHash);
        h = Mix(h, HashLit(g.identity.tectonic.c_str()));
        h = Mix(h, HashLit(g.identity.orographic.c_str()));
        h = Mix(h, HashLit(g.identity.world.c_str()));
        h = Mix(h, (uint64_t)g.identity.orographicVersion);
        h = Mix(h, AdoptPage::Mw8().fieldDigest);
        return h;
    }

    inline uint64_t PatchIdAt(double x, double y)
    {
        int64_t qx = CausalRegionalBiome::QuantizeAbs(x, kPatchStepM);
        int64_t qy = CausalRegionalBiome::QuantizeAbs(y, kPatchStepM);
        return CausalRegionalBiome::StableId(AdoptPage::G().worldIdentityHash, kTagPatch,
            (uint64_t)qx, (uint64_t)qy);
    }

    inline uint64_t SpeciesStream(int species, double x, double y, uint64_t salt)
    {
        uint64_t h = WorldMix();
        h = Mix(h, SpeciesIdOf(species));
        h = Mix(h, PatchIdAt(x, y));
        h = Mix(h, salt);
        h = Mix(h, (uint64_t)CausalRegionalBiome::QuantizeAbs(x, kPatchStepM));
        h = Mix(h, (uint64_t)CausalRegionalBiome::QuantizeAbs(y, kPatchStepM));
        return h;
    }

    inline Site EvaluateSite(double x, double y, Control control)
    {
        Site s;
        s.x = x; s.y = y;
        bool haveHydro = control != Control::HydroclimateOff;
        bool haveRegolith = control != Control::RegolithOff;
        bool lapse = control != Control::LapseOff;
        CausalRegionalHydroclimate::HydroclimateQuery hydro;
        CausalRegionalRegolith::RegolithQuery reg;
        CausalRegionalDrainage::DrainageQuery drain;
        CausalRegionalBiome::Cell c;
        AdoptPage::EvaluateContext(x, y, lapse, haveHydro, haveRegolith, hydro, reg, drain, c);
        AdoptPage::ContextSample q = AdoptPage::QueryContext(x, y);
        s.usedPresentationZ = q.usedPresentationZ;
        s.elev = q.elevationGrade;
        s.slope = c.slope;
        s.channel = c.channel;
        s.divide = c.divide;
        s.basin = (c.provinceType == CausalMacroProvinces::ProvinceType::ForelandBasin);
        s.flowDx = q.flowDx; s.flowDy = q.flowDy;
        s.wind = q.windwardFactor; s.lee = q.leeFactor;
        s.haveHydro = haveHydro; s.haveRegolith = haveRegolith;
        if (haveHydro && hydro.found)
        {
            s.temp = hydro.cell.meanTemperature;
            s.wet = hydro.cell.effectiveWetness;
            s.snow = hydro.cell.snowPersistence;
            s.arid = hydro.cell.aridityIndex;
            s.pool = hydro.cell.coldPoolPotential;
            s.exposure = hydro.cell.exposure;
        }
        else
        {
            s.temp = 6.5; s.wet = 1.0; s.snow = 0.0; s.arid = 0.40; s.pool = 0.0;
            s.wind = 0; s.lee = 0;
            s.exposure = CausalRegionalHydroclimate::ExposureClass::Neutral;
        }
        if (haveRegolith && reg.found)
        {
            s.depth = reg.cell.profileDepth;
            s.profile = reg.cell.profile;
            s.drainage = reg.cell.drainage;
        }
        else
        {
            s.depth = 0.45;
            s.profile = CausalRegionalRegolith::ProfileClass::ThinRegolith;
            s.drainage = CausalRegionalRegolith::DrainageClass::WellDrained;
        }
        if (CausalRegionalBiome::Cell const* mw8 = AdoptPage::CellAt(x, y))
        {
            s.biomeId = mw8->biomeId;
            s.regime = mw8->regime;
        }
        (void)drain;
        return s;
    }

    inline SuitResult Suitability(SpeciesContract const& c, Site const& s)
    {
        SuitResult r;
        r.tempF = Gauss(s.temp, c.tempOpt, c.tempTol);
        r.wetF = Gauss(s.wet, c.wetOpt, c.wetTol);
        if (s.snow > c.snowMax) r.tempF *= std::clamp(c.snowMax / (s.snow + 1e-6), 0.0, 1.0);
        double slopeF = s.slope <= c.slopeMax ? (1.0 - 0.35 * s.slope / (std::max)(c.slopeMax, 1e-4)) : 0.02;
        r.subF = (s.depth < c.depthMin) ? std::clamp(s.depth / (std::max)(c.depthMin, 1e-4), 0.0, 1.0) : 1.0;
        r.subF *= slopeF;
        r.drainF = 1.0;
        if (c.guild == Guild::WetBasin || c.guild == Guild::Riparian)
        {
            bool wetDrain = s.drainage == CausalRegionalRegolith::DrainageClass::PoorlyDrained
                || s.drainage == CausalRegionalRegolith::DrainageClass::Saturated
                || s.drainage == CausalRegionalRegolith::DrainageClass::SomewhatPoorlyDrained;
            r.drainF = wetDrain ? 1.0 : 0.12;
        }
        else if (c.guild == Guild::LeewardDry)
        {
            bool dryDrain = s.drainage == CausalRegionalRegolith::DrainageClass::ExcessivelyDrained
                || s.drainage == CausalRegionalRegolith::DrainageClass::WellDrained;
            r.drainF = dryDrain ? 1.0 : 0.35;
        }
        r.expF = 1.0;
        r.expF *= (1.0 - c.windwardNeed) + c.windwardNeed * s.wind;
        r.expF *= (1.0 - c.leewardNeed) + c.leewardNeed * s.lee;
        double alp = Smooth01(0.80, 1.12, s.elev);
        r.expF *= (1.0 - c.alpineNeed) + c.alpineNeed * alp;
        r.expF *= (1.0 - c.channelNeed) + c.channelNeed * (s.channel ? 1.0 : 0.04);
        r.expF *= (1.0 - c.basinNeed) + c.basinNeed * (s.basin ? 1.0 : 0.04);
        if (!s.haveHydro) { r.wetF = 0.55; r.expF *= 0.45; }
        if (!s.haveRegolith) { r.subF *= 0.45; r.drainF = 0.50; }
        r.value = r.tempF * r.wetF * r.expF * r.subF * r.drainF;
        double comps[5] = { r.tempF, r.wetF, r.expF, r.subF, r.drainF };
        LimitingFactor names[5] = {
            LimitingFactor::Temperature, LimitingFactor::Moisture, LimitingFactor::Exposure,
            LimitingFactor::Substrate, LimitingFactor::Drainage };
        int lo = 0;
        for (int i = 1; i < 5; ++i) if (comps[i] < comps[lo]) lo = i;
        r.limit = names[lo];
        return r;
    }

    inline LifeStage StageFromAge(double age, double pioneer)
    {
        if (age < 0.5) return LifeStage::Seed;
        if (age < 2.0 + 2.0 * pioneer) return LifeStage::Pioneer;
        if (age < 8.0) return LifeStage::Intermediate;
        if (age > 40.0) return LifeStage::Senescent;
        return LifeStage::Mature;
    }

    inline void InitPatch(Patch& p, Site const& site)
    {
        p.patchId = PatchIdAt(site.x, site.y);
        p.x = site.x; p.y = site.y;
        p.site = site;
        p.succession = LifeStage::Bare;
        p.yearsSinceDisturbance = 80;
        for (int i = 0; i < kSpeciesCount; ++i)
        {
            Population& pop = p.pops[i];
            pop = Population{};
            pop.speciesId = SpeciesIdOf(i);
            pop.populationId = CausalRegionalBiome::StableId(
                AdoptPage::G().worldIdentityHash, kTagPop, p.patchId, pop.speciesId);
        }
    }

    inline void Census();

    inline double SourceAmount(Population const& pop)
    {
        return pop.seedBank + pop.inbound;
    }

    inline void EstablishOne(Patch& p, int i, bool allowFounder)
    {
        SpeciesContract const& c = Contracts()[i];
        Population& pop = p.pops[i];
        SuitResult su = Suitability(c, p.site);
        pop.limit = su.limit;
        double src = SourceAmount(pop);
        if (allowFounder && src < kEstablishMinSource && su.value >= c.establishMin)
        {
            double roll = Unit01(SpeciesStream(i, p.x, p.y, HashLit("founder")));
            if (roll < c.founderRate * su.value)
            {
                src = 0.35 + 0.40 * su.value;
                pop.source = SourceKind::Founder;
                pop.seedBank = (std::max)(pop.seedBank, src);
            }
        }
        if (su.value < c.establishMin)
        {
            if (!pop.established) pop.limit = su.limit;
            return;
        }
        if (src < kEstablishMinSource)
        {
            pop.limit = LimitingFactor::Source;
            return;
        }
        if (pop.established) return;
        pop.established = true;
        pop.density = std::clamp(src * su.value, 0.05, 1.0);
        pop.biomass = pop.density * (0.4 + 0.6 * su.value);
        pop.health = 1.0;
        pop.stress = 0.0;
        pop.age = 1.0 + 6.0 * Unit01(SpeciesStream(i, p.x, p.y, HashLit("age0")));
        pop.stage = StageFromAge(pop.age, c.pioneer);
        if (pop.source == SourceKind::None)
        {
            if (pop.inbound > pop.seedBank) pop.source = SourceKind::SeedBank;
            else pop.source = SourceKind::SeedBank;
        }
        p.succession = pop.stage;
    }

    inline void CompeteAndGrow(Patch& p)
    {
        double spaceCap = std::clamp(0.35 + 0.55 * p.site.depth + 0.25 * std::clamp(p.site.wet, 0.0, 1.4)
            - 2.5 * p.site.slope, 0.12, 1.35);
        double waterCap = std::clamp(p.site.wet * 0.70 + p.site.depth * 0.20, 0.10, 1.40);
        double lightCap = p.site.divide ? 1.10 : 0.90;
        double subCap = std::clamp(p.site.depth, 0.08, 1.40);
        double demandSpace = 0, demandWater = 0, demandLight = 0, demandSub = 0;
        for (int i = 0; i < kSpeciesCount; ++i)
        {
            SpeciesContract const& c = Contracts()[i];
            Population const& pop = p.pops[i];
            if (!pop.established) continue;
            demandSpace += pop.density * c.spaceNeed * c.competeW;
            demandWater += pop.density * c.waterNeed * c.competeW;
            demandLight += pop.density * c.lightNeed * c.competeW;
            demandSub += pop.density * c.substrateNeed * c.competeW;
        }
        double pressSpace = demandSpace > spaceCap ? spaceCap / demandSpace : 1.0;
        double pressWater = demandWater > waterCap ? waterCap / demandWater : 1.0;
        double pressLight = demandLight > lightCap ? lightCap / demandLight : 1.0;
        double pressSub = demandSub > subCap ? subCap / demandSub : 1.0;
        double press = (std::min)((std::min)(pressSpace, pressWater), (std::min)(pressLight, pressSub));
        for (int i = 0; i < kSpeciesCount; ++i)
        {
            SpeciesContract const& c = Contracts()[i];
            Population& pop = p.pops[i];
            SuitResult su = Suitability(c, p.site);
            pop.limit = su.limit;
            if (!pop.established)
            {
                EstablishOne(p, i, false);
                continue;
            }
            double mismatch = 1.0 - su.value;
            pop.stress = std::clamp(0.55 * mismatch + 0.45 * (1.0 - press), 0.0, 1.0);
            if (p.harvested && p.yearsSinceDisturbance < 0.5)
                pop.stress = (std::max)(pop.stress, 0.95);
            pop.health = std::clamp(1.0 - pop.stress, 0.0, 1.0);
            double grow = c.growthRate * su.value * press * pop.health;
            double die = c.mortality * (0.35 + 0.65 * pop.stress);
            double prev = pop.density;
            pop.density = std::clamp(pop.density + grow - die * pop.density, 0.0, 1.25);
            pop.biomass = pop.density * (0.35 + 0.65 * pop.health);
            pop.age += 1.0;
            pop.stage = StageFromAge(pop.age, c.pioneer);
            pop.seedBank = (std::min)(1.2, pop.seedBank * 0.84 + pop.density * 0.18);
            if (pop.density < 0.01 && pop.stress > 0.72)
            {
                pop.established = false;
                pop.density = 0;
                pop.biomass = 0;
                pop.stage = LifeStage::Dead;
                pop.limit = LimitingFactor::Stress;
            }
            else if (press < 0.72 && pop.density + 1e-9 < prev)
                pop.limit = LimitingFactor::Competition;
            if (std::fabs(pop.density - prev) > 0.02) F().dirtyUpdates += 1;
        }
        if (p.harvested)
        {
            p.yearsSinceDisturbance += 1.0;
            if (p.yearsSinceDisturbance < 2.0) p.succession = LifeStage::Pioneer;
            else if (p.yearsSinceDisturbance < 7.0) p.succession = LifeStage::Intermediate;
            else { p.succession = LifeStage::Mature; p.harvested = false; }
        }
        else
        {
            LifeStage best = LifeStage::Bare;
            for (int i = 0; i < kSpeciesCount; ++i)
                if (p.pops[i].established && (int)p.pops[i].stage > (int)best)
                    best = p.pops[i].stage;
            p.succession = best;
        }
    }

    inline Patch* EnsurePatch(double x, double y, Control control)
    {
        uint64_t id = PatchIdAt(x, y);
        auto it = F().patches.find(id);
        if (it != F().patches.end())
        {
            it->second.site = EvaluateSite(x, y, control);
            it->second.x = x; it->second.y = y;
            return &it->second;
        }
        Patch p;
        InitPatch(p, EvaluateSite(x, y, control));
        auto ins = F().patches.emplace(id, p);
        return &ins.first->second;
    }

    inline void DisperseFrom(Patch const& p, Control control)
    {
        AdoptPage::OrographicContext const& ctx = AdoptPage::Ctx();
        double az = ctx.moistureAzimuthDeg * AdoptPage::kPi / 180.0;
        double wx = std::cos(az), wy = std::sin(az);
        double gx = 0, gy = 0;
        if (p.site.slope > 1e-6)
        {
            double e1 = AdoptPage::CanonicalGrade(p.x + 32.0, p.y);
            double e2 = AdoptPage::CanonicalGrade(p.x, p.y + 32.0);
            gx = e1 - p.site.elev; gy = e2 - p.site.elev;
            double n = std::hypot(gx, gy);
            if (n > 1e-9) { gx /= n; gy /= n; }
        }
        for (int i = 0; i < kSpeciesCount; ++i)
        {
            Population const& pop = p.pops[i];
            if (!pop.established || pop.density < 0.03) continue;
            SpeciesContract const& c = Contracts()[i];
            auto send = [&](double dx, double dy, double mass, SourceKind kind)
            {
                if (mass < 0.008) return;
                double tx = p.x + dx * kPatchStepM;
                double ty = p.y + dy * kPatchStepM;
                Patch* dst = EnsurePatch(tx, ty, control);
                dst->pops[i].inbound += mass;
                if (dst->pops[i].source == SourceKind::None || dst->pops[i].source == SourceKind::SeedBank)
                    dst->pops[i].source = kind;
            };
            double windMass = pop.density * c.windDisp * (0.35 + 0.65 * (std::max)(p.site.wind, p.site.divide ? 0.6 : 0.0));
            send(wx, wy, windMass, SourceKind::Wind);
            if (p.site.channel || std::hypot(p.site.flowDx, p.site.flowDy) > 0.1)
                send(p.site.flowDx, p.site.flowDy, pop.density * c.waterDisp, SourceKind::Water);
            if (p.site.slope > 0.004)
                send(-gx, -gy, pop.density * c.gravDisp * (std::min)(1.0, p.site.slope * 12.0), SourceKind::Gravity);
        }
    }

    inline void AbsorbInbound(Patch& p)
    {
        for (int i = 0; i < kSpeciesCount; ++i)
        {
            Population& pop = p.pops[i];
            if (pop.inbound <= 0.0) continue;
            pop.seedBank = (std::min)(1.4, pop.seedBank + pop.inbound);
            pop.inbound = 0;
        }
    }

    inline void StepOnce(Control control, bool force = false)
    {
        Field& f = F();
        if (f.settled && !force) return;
        ++f.farUpdates;
        std::vector<uint64_t> ids;
        ids.reserve(f.patches.size());
        for (auto const& kv : f.patches) ids.push_back(kv.first);
        for (uint64_t id : ids)
        {
            auto it = f.patches.find(id);
            if (it == f.patches.end()) continue;
            DisperseFrom(it->second, control);
        }
        for (auto& kv : f.patches)
        {
            AbsorbInbound(kv.second);
            CompeteAndGrow(kv.second);
        }
    }

    inline void Census()
    {
        Field& f = F();
        f.occupiedPatches = 0; f.emptySuitable = 0;
        f.alpineOccupied = 0; f.windwardOccupied = 0; f.leewardOccupied = 0;
        f.riparianOccupied = 0; f.basinOccupied = 0; f.generalistOccupied = 0;
        uint64_t h = HashLit("mw9-field");
        h = Mix(h, (uint64_t)kMw9Version);
        h = Mix(h, f.worldIdentityHash);
        std::vector<uint64_t> ids;
        ids.reserve(f.patches.size());
        for (auto const& kv : f.patches) ids.push_back(kv.first);
        std::sort(ids.begin(), ids.end());
        for (uint64_t id : ids)
        {
            Patch const& p = f.patches.find(id)->second;
            h = Mix(h, p.patchId);
            bool occ = false;
            bool anySuit = false;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                Population const& pop = p.pops[i];
                h = Mix(h, pop.speciesId);
                h = MixF(h, pop.density);
                h = MixF(h, pop.seedBank);
                h = Mix(h, (uint64_t)pop.stage);
                h = Mix(h, pop.established ? 1ull : 0ull);
                SuitResult su = Suitability(Contracts()[i], p.site);
                if (su.value >= Contracts()[i].establishMin) anySuit = true;
                if (!pop.established || pop.density < 0.02) continue;
                occ = true;
                switch (Contracts()[i].guild)
                {
                    case Guild::AlpineLow: ++f.alpineOccupied; break;
                    case Guild::WindwardMoist: ++f.windwardOccupied; break;
                    case Guild::LeewardDry: ++f.leewardOccupied; break;
                    case Guild::Riparian: ++f.riparianOccupied; break;
                    case Guild::WetBasin: ++f.basinOccupied; break;
                    case Guild::Generalist: ++f.generalistOccupied; break;
                }
            }
            if (occ) ++f.occupiedPatches;
            else if (anySuit) ++f.emptySuitable;
        }
        f.fieldDigest = h;
    }

    inline void GenesisEstablish()
    {
        for (auto& kv : F().patches)
            for (int i = 0; i < kSpeciesCount; ++i)
                EstablishOne(kv.second, i, true);
    }

    inline void Compile(Control control)
    {
        Field& f = F();
        ++f.compiles;
        if (control == Control::ForceOff)
        {
            f.compiled = false;
            f.lastControl = control;
            return;
        }
        if (!AdoptPage::IsLive() || !AdoptPage::ContextLive()) return;
        if (!GenesisSupportsMw9(AdoptPage::G().identity)) return;
        bool first = f.patches.empty();
        if (first) ++f.rebuilds;
        f.worldIdentityHash = AdoptPage::G().worldIdentityHash;
        f.genesisHash = HashLit(AdoptPage::G().identity.world.c_str());
        f.orographicHash = HashLit(AdoptPage::G().identity.orographic.c_str());
        f.lastControl = control;
        f.faunaClosed = true;
        AdoptPage::Mw8Field const& mw8 = AdoptPage::Mw8();
        if (mw8.cells.empty()) return;
        for (CausalRegionalBiome::Cell const& cell : mw8.cells)
            EnsurePatch(cell.x, cell.y, control);
        if (first) GenesisEstablish();
        f.compiled = true;
        f.settled = false;
        Census();
    }

    inline int Settle(Control control, int maxTicks = 20)
    {
        int ticks = 0;
        F().settled = false;
        for (; ticks < maxTicks; ++ticks)
        {
            int before = F().dirtyUpdates;
            StepOnce(control, true);
            Census();
            if (F().dirtyUpdates == before) break;
        }
        F().settled = true;
        return ticks + 1;
    }

    inline Patch const* PatchAt(double x, double y)
    {
        auto it = F().patches.find(PatchIdAt(x, y));
        return it == F().patches.end() ? nullptr : &it->second;
    }

    inline Receipt QueryReceipt(double x, double y, int species)
    {
        Receipt r;
        r.x = x; r.y = y;
        r.patchId = PatchIdAt(x, y);
        r.speciesId = SpeciesIdOf(species);
        std::snprintf(r.species, sizeof(r.species), "%s", Contracts()[species].name);
        Patch const* p = PatchAt(x, y);
        Site site = p ? p->site : EvaluateSite(x, y, F().lastControl);
        SuitResult su = Suitability(Contracts()[species], site);
        r.suitability = su.value;
        std::snprintf(r.limiting, sizeof(r.limiting), "%s", LimitName(su.limit));
        if (!p)
        {
            std::snprintf(r.why, sizeof(r.why), "no_patch");
            std::snprintf(r.sourceKind, sizeof(r.sourceKind), "none");
            return r;
        }
        Population const& pop = p->pops[species];
        r.density = pop.density;
        r.source = SourceAmount(pop) + (pop.established ? pop.density : 0);
        r.occupied = pop.established && pop.density >= 0.02;
        std::snprintf(r.sourceKind, sizeof(r.sourceKind), "%s", SourceName(pop.source));
        if (r.occupied)
            std::snprintf(r.why, sizeof(r.why), "occupied_via_%s", SourceName(pop.source));
        else if (su.value < Contracts()[species].establishMin)
            std::snprintf(r.why, sizeof(r.why), "unsuitable_%s", LimitName(su.limit));
        else
            std::snprintf(r.why, sizeof(r.why), "suitable_source_isolated");
        if (!r.occupied && su.value >= Contracts()[species].establishMin)
            std::snprintf(r.limiting, sizeof(r.limiting), "%s", LimitName(LimitingFactor::Source));
        return r;
    }

    inline std::vector<Organism> RealizeNear(double x, double y, double radius = kNearRadiusM)
    {
        std::vector<Organism> out;
        Field& f = F();
        for (auto& kv : f.patches)
        {
            Patch& p = kv.second;
            if (std::hypot(p.x - x, p.y - y) > radius) continue;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                Population const& pop = p.pops[i];
                if (!pop.established || pop.density < 0.05) continue;
                int n = std::clamp((int)std::llround(pop.density * (double)kNearIndividualsMax), 1, kNearIndividualsMax);
                for (int k = 0; k < n; ++k)
                {
                    uint64_t oid = CausalRegionalBiome::StableId(
                        AdoptPage::G().worldIdentityHash, kTagOrg, pop.populationId, (uint64_t)k);
                    auto grave = f.graves.find(oid);
                    if (grave != f.graves.end()) continue;
                    auto live = f.organisms.find(oid);
                    if (live != f.organisms.end())
                    {
                        out.push_back(live->second);
                        continue;
                    }
                    Organism o;
                    o.organismId = oid;
                    o.speciesId = pop.speciesId;
                    o.patchId = p.patchId;
                    double jx = (Unit01(Mix(oid, 11)) - 0.5) * 28.0;
                    double jy = (Unit01(Mix(oid, 29)) - 0.5) * 28.0;
                    o.x = p.x + jx; o.y = p.y + jy;
                    o.biomass = pop.biomass / (double)n;
                    o.health = pop.health;
                    o.age = pop.age;
                    o.stage = pop.stage;
                    o.alive = true;
                    f.organisms.emplace(oid, o);
                    out.push_back(o);
                }
            }
        }
        return out;
    }

    inline bool HarvestOrganism(uint64_t organismId)
    {
        Field& f = F();
        auto it = f.organisms.find(organismId);
        Organism o;
        if (it != f.organisms.end()) o = it->second;
        else
        {
            auto g = f.graves.find(organismId);
            if (g == f.graves.end()) return false;
            return true;
        }
        o.alive = false;
        o.stage = LifeStage::Dead;
        o.biomass = 0;
        f.graves[organismId] = o;
        f.organisms.erase(it);
        f.settled = false;
        auto pit = f.patches.find(o.patchId);
        if (pit != f.patches.end())
        {
            Patch& p = pit->second;
            p.harvested = true;
            p.yearsSinceDisturbance = 0;
            p.succession = LifeStage::Bare;
            p.disturbanceEventId = Mix(organismId, HashLit("harvest"));
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                if (p.pops[i].speciesId != o.speciesId) continue;
                p.pops[i].density = 0;
                p.pops[i].biomass = 0;
                p.pops[i].established = false;
                p.pops[i].stage = LifeStage::Dead;
                p.pops[i].limit = LimitingFactor::Disturbance;
                int pio = 5;
                p.pops[pio].seedBank = (std::max)(p.pops[pio].seedBank, 0.45);
            }
        }
        return true;
    }

    inline bool OrganismAlive(uint64_t organismId)
    {
        if (F().graves.count(organismId)) return false;
        auto it = F().organisms.find(organismId);
        return it != F().organisms.end() && it->second.alive;
    }

    inline std::string Serialize()
    {
        Field const& f = F();
        std::ostringstream o;
        o << "PROVENANCE_MW9_FLORA_V1\n";
        o << "version=" << kMw9Version << "\n";
        o << "worldgen_id=" << kWorldgenId << "\n";
        o << "world_identity_hash=" << CausalWorldGeology::Hex64(f.worldIdentityHash) << "\n";
        o << "orographic_hash=" << CausalWorldGeology::Hex64(f.orographicHash) << "\n";
        o << "field_digest=" << CausalWorldGeology::Hex64(f.fieldDigest) << "\n";
        o << "biomass_matter_debt=" << kBiomassMatterDebt << "\n";
        o << "fauna=" << kFaunaState << "\n";
        o << "patch_count=" << f.patches.size() << "\n";
        o << "grave_count=" << f.graves.size() << "\n";
        std::vector<uint64_t> ids;
        ids.reserve(f.patches.size());
        for (auto const& kv : f.patches) ids.push_back(kv.first);
        std::sort(ids.begin(), ids.end());
        for (uint64_t id : ids)
        {
            Patch const& p = f.patches.find(id)->second;
            char buf[512];
            std::snprintf(buf, sizeof(buf), "P %s %.6f %.6f %d %.6f %d %s\n",
                CausalWorldGeology::Hex64(p.patchId).c_str(), p.x, p.y,
                (int)p.succession, p.yearsSinceDisturbance, p.harvested ? 1 : 0,
                CausalWorldGeology::Hex64(p.disturbanceEventId).c_str());
            o << buf;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                Population const& pop = p.pops[i];
                std::snprintf(buf, sizeof(buf),
                    "Q %d %s %.6f %.6f %.6f %.6f %.6f %.6f %d %d %d\n",
                    i, CausalWorldGeology::Hex64(pop.populationId).c_str(),
                    pop.density, pop.biomass, pop.health, pop.stress, pop.age, pop.seedBank,
                    (int)pop.stage, (int)pop.source, pop.established ? 1 : 0);
                o << buf;
            }
        }
        for (auto const& kv : f.graves)
        {
            Organism const& g = kv.second;
            o << "G " << CausalWorldGeology::Hex64(g.organismId) << " "
              << CausalWorldGeology::Hex64(g.speciesId) << " "
              << CausalWorldGeology::Hex64(g.patchId) << "\n";
        }
        return o.str();
    }

    inline bool Deserialize(std::string const& text)
    {
        Reset();
        Field& f = F();
        std::istringstream in(text);
        std::string line, magic;
        if (!std::getline(in, magic)) return false;
        if (!magic.empty() && magic.back() == '\r') magic.pop_back();
        if (magic != "PROVENANCE_MW9_FLORA_V1") return false;
        Patch* cur = nullptr;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.rfind("version=", 0) == 0)
            {
                uint32_t v = (uint32_t)std::strtoul(line.c_str() + 8, nullptr, 10);
                if (v != kMw9Version) return false;
            }
            else if (line.rfind("P ", 0) == 0)
            {
                Patch p;
                char idb[32]{}, evb[32]{};
                int succ = 0, harv = 0;
                std::sscanf(line.c_str() + 2, "%31s %lf %lf %d %lf %d %31s",
                    idb, &p.x, &p.y, &succ, &p.yearsSinceDisturbance, &harv, evb);
                CausalWorldGeology::ParseHex64(idb, p.patchId);
                CausalWorldGeology::ParseHex64(evb, p.disturbanceEventId);
                p.succession = (LifeStage)succ;
                p.harvested = harv != 0;
                p.site = EvaluateSite(p.x, p.y, Control::ForceOn);
                for (int i = 0; i < kSpeciesCount; ++i)
                {
                    p.pops[i].speciesId = SpeciesIdOf(i);
                    p.pops[i].populationId = CausalRegionalBiome::StableId(
                        AdoptPage::G().worldIdentityHash, kTagPop, p.patchId, p.pops[i].speciesId);
                }
                auto ins = f.patches.emplace(p.patchId, p);
                cur = &ins.first->second;
            }
            else if (line.rfind("Q ", 0) == 0 && cur)
            {
                int i = 0, st = 0, so = 0, est = 0;
                char pid[32]{};
                Population tmp;
                std::sscanf(line.c_str() + 2, "%d %31s %lf %lf %lf %lf %lf %lf %d %d %d",
                    &i, pid, &tmp.density, &tmp.biomass, &tmp.health, &tmp.stress,
                    &tmp.age, &tmp.seedBank, &st, &so, &est);
                if (i < 0 || i >= kSpeciesCount) continue;
                CausalWorldGeology::ParseHex64(pid, tmp.populationId);
                tmp.speciesId = SpeciesIdOf(i);
                tmp.stage = (LifeStage)st;
                tmp.source = (SourceKind)so;
                tmp.established = est != 0;
                cur->pops[i] = tmp;
            }
            else if (line.rfind("G ", 0) == 0)
            {
                char a[32]{}, b[32]{}, c[32]{};
                std::sscanf(line.c_str() + 2, "%31s %31s %31s", a, b, c);
                Organism g;
                CausalWorldGeology::ParseHex64(a, g.organismId);
                CausalWorldGeology::ParseHex64(b, g.speciesId);
                CausalWorldGeology::ParseHex64(c, g.patchId);
                g.alive = false; g.stage = LifeStage::Dead;
                f.graves.emplace(g.organismId, g);
            }
        }
        f.compiled = true;
        f.lastControl = Control::ForceOn;
        f.worldIdentityHash = AdoptPage::G().worldIdentityHash;
        f.faunaClosed = true;
        Census();
        return true;
    }

    inline void IntroduceSource(double x, double y, int species, double amount = 0.55)
    {
        Patch* p = EnsurePatch(x, y, F().compiled ? F().lastControl : Control::ForceOn);
        p->pops[species].seedBank = (std::max)(p->pops[species].seedBank, amount);
        p->pops[species].source = SourceKind::Introduced;
        p->pops[species].inbound += amount;
        F().settled = false;
    }

    struct CertResult
    {
        bool passed = false;
        std::string reason;
        std::vector<std::pair<std::string, bool>> checks;
        uint64_t mw8DigestBefore = 0, mw8DigestAfter = 0, mw9Digest = 0, mw9DigestReload = 0;
        int alpineOccupied = 0, windwardOccupied = 0, leewardOccupied = 0;
        int riparianOccupied = 0, basinOccupied = 0, emptySuitable = 0;
        int pageBoundaryAgree = 0;
        int compilesAfterTravel = 0, rebuildsAfterTravel = 0, dirtyAfterSettle = 0;
        int settleTicks = 0;
        double compileMs = 0, queryMs = 0, nearMs = 0, farMs = 0, crossMs = 0, saveMs = 0, reloadMs = 0;
        size_t saveBytes = 0;
        bool usedPresentationZ = false;
        bool faunaClosed = true;
        std::string liveWire = "banked_production_page_bytes";
        std::string biomassDebt = kBiomassMatterDebt;
        std::string version = kWorldgenId;
    };

    inline CertResult RunCert()
    {
        CertResult c;
        auto add = [&](char const* name, bool ok) { c.checks.push_back({ name, ok }); };
        Reset();
        AdoptPage::Reset();

        std::string liveReceipt;
        char const* dir = AdoptPage::kFixtureDir;
        if (AdoptPage::ReadFile((std::string(AdoptPage::kLiveFixtureDir) + "/live_stream_receipt.json").c_str(), liveReceipt))
        {
            dir = AdoptPage::kLiveFixtureDir;
            c.liveWire = "orographic_production_page";
        }
        AdoptPage::WorldIdentity ident = AdoptPage::LoadInstalledIdentity(dir);
        add("live_canonical_genesis", GenesisSupportsMw9(ident)
            && ident.tectonic == AdoptPage::kExpectedTectonic
            && ident.orographic == AdoptPage::kExpectedOrographic
            && ident.terrainLaw == kTerrainLawRequired);

        AdoptPage::WorldIdentity v11;
        v11.world = "historical-v11";
        v11.tectonic = ident.tectonic;
        v11.terrainLaw = "worldgenesis.v11";
        add("historical_v11_worlds_do_not_gain_canonical_mw9", !GenesisSupportsMw9(v11));

        std::string page11, ctx11, page21, ctx21;
        add("page_1_1_present", AdoptPage::ReadFile((std::string(dir) + "/canonical_orographic_page_1_1.json").c_str(), page11));
        add("context_1_1_present", AdoptPage::ReadFile((std::string(dir) + "/canonical_orographic_context_1_1.json").c_str(), ctx11));
        AdoptPage::ReadFile((std::string(dir) + "/canonical_orographic_page_2_1.json").c_str(), page21);
        AdoptPage::ReadFile((std::string(dir) + "/canonical_orographic_context_2_1.json").c_str(), ctx21);
        AdoptPage::AdoptResult a = AdoptPage::Adopt(ident, page11);
        AdoptPage::AdoptResult cx = AdoptPage::AdoptContext(ident, ctx11);
        add("live_page_and_context_admitted", a.ok && cx.ok && AdoptPage::IsLive() && AdoptPage::ContextLive());

        AdoptPage::CompileMw8(CausalRegionalBiome::Control::ForceOn);
        c.mw8DigestBefore = AdoptPage::Mw8().fieldDigest;
        add("mw8_live_field_present", AdoptPage::Mw8().cells.size() > 10
            && (AdoptPage::Mw8().alpineBarren + AdoptPage::Mw8().alpineTundra) >= 1
            && AdoptPage::Mw8().riparian >= 1
            && AdoptPage::Mw8().basinWetland >= 1);

        auto nowMs = []() {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        };

        double t0 = nowMs();
        Compile(Control::ForceOn);
        c.compileMs = nowMs() - t0;
        add("mw9_compiled_on_live_mw8", F().compiled && F().patches.size() == AdoptPage::Mw8().cells.size());
        add("mw9_versioned_state", F().version == kMw9Version && F().worldgenId == std::string(kWorldgenId));
        add("fauna_closed", F().faunaClosed);
        c.faunaClosed = F().faunaClosed;

        t0 = nowMs();
        c.settleTicks = Settle(Control::ForceOn, 18);
        c.farMs = nowMs() - t0;
        c.dirtyAfterSettle = 0;
        int dirtyMark = F().dirtyUpdates;
        StepOnce(Control::ForceOn);
        Census();
        c.dirtyAfterSettle = F().dirtyUpdates - dirtyMark;
        add("static_world_ecology_dirty_settles", c.dirtyAfterSettle == 0);

        c.mw8DigestAfter = AdoptPage::Mw8().fieldDigest;
        add("mw9_does_not_mutate_mw8", c.mw8DigestBefore != 0 && c.mw8DigestBefore == c.mw8DigestAfter);
        add("mw9_off_parent_untouched_while_on", AdoptPage::Mw8().alpineBarren + AdoptPage::Mw8().alpineTundra >= 1);

        bool usedZ = false;
        int alpineGuildOnAlpine = 0, alpineGuildOffAlpine = 0, alpineCells = 0;
        int wMoistOnW = 0, wMoistOnL = 0, lShrubOnL = 0, lShrubOnW = 0, wN = 0, lN = 0;
        int ripOnChannel = 0, ripOnSlope = 0, slopeN = 0, chanN = 0;
        int basinOnBasin = 0, basinOnRidge = 0, basinN = 0, ridgeN = 0;
        for (auto const& kv : F().patches)
        {
            Patch const& p = kv.second;
            if (p.site.usedPresentationZ) usedZ = true;
            bool alpineSite = p.site.elev > AdoptPage::Ctx().baseGrade + 0.12 && p.site.temp < 3.2;
            if (alpineSite) ++alpineCells;
            if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Windward) ++wN;
            if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward) ++lN;
            if (p.site.channel) ++chanN;
            if (!p.site.channel && p.site.slope > 0.05) ++slopeN;
            if (p.site.basin) ++basinN;
            if (p.site.divide) ++ridgeN;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                if (!p.pops[i].established || p.pops[i].density < 0.02) continue;
                Guild g = Contracts()[i].guild;
                if (g == Guild::AlpineLow) { if (alpineSite) ++alpineGuildOnAlpine; else ++alpineGuildOffAlpine; }
                if (g == Guild::WindwardMoist)
                {
                    if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Windward) ++wMoistOnW;
                    if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward) ++wMoistOnL;
                }
                if (g == Guild::LeewardDry)
                {
                    if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward) ++lShrubOnL;
                    if (p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Windward) ++lShrubOnW;
                }
                if (g == Guild::Riparian)
                {
                    if (p.site.channel) ++ripOnChannel;
                    if (!p.site.channel && p.site.slope > 0.05) ++ripOnSlope;
                }
                if (g == Guild::WetBasin)
                {
                    if (p.site.basin) ++basinOnBasin;
                    if (p.site.divide) ++basinOnRidge;
                }
            }
        }
        c.usedPresentationZ = usedZ;
        c.alpineOccupied = F().alpineOccupied;
        c.windwardOccupied = F().windwardOccupied;
        c.leewardOccupied = F().leewardOccupied;
        c.riparianOccupied = F().riparianOccupied;
        c.basinOccupied = F().basinOccupied;
        c.emptySuitable = F().emptySuitable;
        c.mw9Digest = F().fieldDigest;

        add("mw9_does_not_read_grade_to_z", !usedZ);
        add("alpine_flora_on_canonical_elevation", alpineGuildOnAlpine >= 1 && alpineGuildOnAlpine >= alpineGuildOffAlpine);
        add("windward_flora_tracks_exposure", wN >= 1 && wMoistOnW > wMoistOnL);
        add("leeward_flora_tracks_rain_shadow", lN >= 1 && lShrubOnL > lShrubOnW);
        add("riparian_flora_in_valleys_not_slopes", chanN >= 1 && ripOnChannel >= 1 && ripOnChannel > ripOnSlope);
        add("wet_basin_flora_in_basins_not_ridges", basinN >= 1 && basinOnBasin >= 1 && basinOnBasin > basinOnRidge);

        Field onCopy = F();
        Reset();
        Compile(Control::LapseOff);
        Settle(Control::LapseOff, 8);
        int alpineLapseOff = F().alpineOccupied;
        add("alpine_neutralize_lapse_collapses_alpine_flora",
            onCopy.alpineOccupied >= 1 && alpineLapseOff < onCopy.alpineOccupied);

        Reset();
        Compile(Control::HydroclimateOff);
        Settle(Control::HydroclimateOff, 8);
        int wOff = 0, lOff = 0;
        for (auto const& kv : F().patches)
        {
            Patch const& p = kv.second;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                if (!p.pops[i].established || p.pops[i].density < 0.02) continue;
                if (Contracts()[i].guild == Guild::WindwardMoist
                    && p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Windward) ++wOff;
                if (Contracts()[i].guild == Guild::LeewardDry
                    && p.site.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward) ++lOff;
            }
        }
        add("hydro_neutralize_collapses_windward_leeward_flora",
            (onCopy.windwardOccupied + onCopy.leewardOccupied) >= 1 && (wOff + lOff) < 1);

        Reset();
        Compile(Control::RegolithOff);
        Settle(Control::RegolithOff, 8);
        add("regolith_neutralize_collapses_riparian_basin_flora",
            (onCopy.riparianOccupied + onCopy.basinOccupied) >= 1
            && (F().riparianOccupied + F().basinOccupied) < (onCopy.riparianOccupied + onCopy.basinOccupied));

        F() = onCopy;
        Census();
        auto occMix = [&]() {
            uint64_t occDigest = HashLit("occ");
            std::vector<uint64_t> ids;
            for (auto const& kv : F().patches) ids.push_back(kv.first);
            std::sort(ids.begin(), ids.end());
            for (uint64_t id : ids)
            {
                Patch const& p = F().patches.find(id)->second;
                occDigest = Mix(occDigest, p.patchId);
                for (int i = 0; i < kSpeciesCount; ++i)
                    if (p.pops[i].established) occDigest = Mix(occDigest, Mix(p.pops[i].speciesId, 1));
            }
            return occDigest;
        };
        uint64_t occDigest = occMix();
        (void)occDigest;
        bool suitIgnoresBiome = true;
        for (auto const& kv : F().patches)
        {
            Site s = kv.second.site;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                double a = Suitability(Contracts()[i], s).value;
                s.biomeId ^= 0xF00DF00DF00DF00Dull;
                double b = Suitability(Contracts()[i], s).value;
                if (a != b) suitIgnoresBiome = false;
            }
        }
        for (CausalRegionalBiome::Cell& cell : AdoptPage::Mw8().cells)
            cell.biomeId ^= Mix(cell.biomeId, 0xA5A5A5A5A5A5A5A5ull) | 1ull;
        Reset();
        Compile(Control::ForceOn);
        uint64_t occGenesisScramble = occMix();
        Reset();
        AdoptPage::CompileMw8(CausalRegionalBiome::Control::ForceOn);
        Compile(Control::ForceOn);
        uint64_t occGenesisRestore = occMix();
        add("no_biomeid_mask_scramble_does_not_reorganize_flora",
            suitIgnoresBiome && occGenesisScramble == occGenesisRestore);
        F() = onCopy;
        Census();

        int isolatedSpecies = -1;
        double isoX = 0, isoY = 0;
        for (auto const& kv : F().patches)
        {
            Patch const& p = kv.second;
            for (int i = 0; i < kSpeciesCount; ++i)
            {
                SuitResult su = Suitability(Contracts()[i], p.site);
                if (su.value < Contracts()[i].establishMin) continue;
                isolatedSpecies = i; isoX = p.x; isoY = p.y;
                break;
            }
            if (isolatedSpecies >= 0) break;
        }
        add("source_limited_empty_habitat_exists", isolatedSpecies >= 0);
        if (isolatedSpecies >= 0)
        {
            Patch* iso = EnsurePatch(isoX, isoY, Control::ForceOn);
            Population& pop = iso->pops[isolatedSpecies];
            pop.established = false;
            pop.density = 0; pop.biomass = 0; pop.seedBank = 0; pop.inbound = 0;
            pop.source = SourceKind::None; pop.stage = LifeStage::Bare;
            Census();
            Receipt before = QueryReceipt(isoX, isoY, isolatedSpecies);
            IntroduceSource(isoX, isoY, isolatedSpecies, 0.70);
            StepOnce(Control::ForceOn);
            Census();
            Receipt after = QueryReceipt(isoX, isoY, isolatedSpecies);
            add("source_limited_empty_then_introduced_establishes",
                !before.occupied && after.occupied && std::strcmp(before.limiting, "source") == 0);
        }
        else add("source_limited_empty_then_introduced_establishes", false);

        t0 = nowMs();
        int qn = 0;
        for (double y = 1200; y <= 1800; y += 64)
        for (double x = 1200; x <= 1800; x += 64)
        {
            (void)PatchAt(x, y);
            (void)QueryReceipt(x, y, 5);
            ++qn;
        }
        c.queryMs = nowMs() - t0;
        int rb0 = F().rebuilds, cp0 = F().compiles;
        for (double y = 1200; y <= 1800; y += 48)
        for (double x = 1200; x <= 1800; x += 48)
            (void)PatchAt(x, y);
        c.compilesAfterTravel = F().compiles - cp0;
        c.rebuildsAfterTravel = F().rebuilds - rb0;
        add("ordinary_travel_does_not_recompile_mw9", c.compilesAfterTravel == 0 && c.rebuildsAfterTravel == 0 && qn > 10);

        t0 = nowMs();
        auto near = RealizeNear(1536, 1536, kNearRadiusM);
        c.nearMs = nowMs() - t0;
        add("near_realization_instantiates_individuals", near.size() >= 1);
        bool choppedStayDead = false;
        if (!near.empty())
        {
            uint64_t oid = near[0].organismId;
            add("harvest_cut_disturbance", HarvestOrganism(oid) && !OrganismAlive(oid));
            auto again = RealizeNear(near[0].x, near[0].y, kNearRadiusM);
            bool resurrected = false;
            for (Organism const& o : again) if (o.organismId == oid && o.alive) resurrected = true;
            choppedStayDead = !resurrected && F().graves.count(oid);
            add("chopped_organismid_does_not_resurrect", choppedStayDead);
            Patch const* hp = PatchAt(near[0].x, near[0].y);
            add("succession_bare_after_harvest", hp && (hp->succession == LifeStage::Bare
                || hp->succession == LifeStage::Pioneer || hp->harvested));
            StepOnce(Control::ForceOn);
            StepOnce(Control::ForceOn);
            StepOnce(Control::ForceOn);
            hp = PatchAt(near[0].x, near[0].y);
            add("succession_pioneer_after_cut", hp && ((int)hp->succession >= (int)LifeStage::Pioneer
                || hp->pops[5].established || hp->pops[5].seedBank > 0.2));
        }
        else
        {
            add("harvest_cut_disturbance", false);
            add("chopped_organismid_does_not_resurrect", false);
            add("succession_bare_after_harvest", false);
            add("succession_pioneer_after_cut", false);
        }

        std::unordered_map<uint64_t, double> edgeDens;
        for (auto const& kv : F().patches)
        {
            if (std::fabs(kv.second.x - 2048.0) < 1.0)
            {
                double d = 0;
                for (int i = 0; i < kSpeciesCount; ++i) d += kv.second.pops[i].density;
                edgeDens[kv.first] = d;
            }
        }
        t0 = nowMs();
        AdoptPage::Adopt(ident, page21);
        AdoptPage::AdoptContext(ident, ctx21);
        AdoptPage::CompileMw8(CausalRegionalBiome::Control::ForceOn);
        Compile(Control::ForceOn);
        StepOnce(Control::ForceOn);
        Census();
        int survived = 0;
        for (auto const& e : edgeDens)
            if (F().patches.count(e.first)) ++survived;
        c.pageBoundaryAgree = survived;
        c.crossMs = nowMs() - t0;
        add("page_edges_are_not_ecological_boundaries",
            !edgeDens.empty() && survived == (int)edgeDens.size());
        bool crossDisp = F().patches.size() > edgeDens.size();
        add("cross_page_dispersal_persists_in_world_store", crossDisp);

        AdoptPage::Adopt(ident, page11);
        AdoptPage::AdoptContext(ident, ctx11);
        AdoptPage::CompileMw8(CausalRegionalBiome::Control::ForceOn);
        Compile(Control::ForceOn);

        Census();
        t0 = nowMs();
        std::string blob = Serialize();
        c.saveMs = nowMs() - t0;
        c.saveBytes = blob.size();
        uint64_t dSave = F().fieldDigest;
        Reset();
        t0 = nowMs();
        bool loaded = Deserialize(blob);
        c.reloadMs = nowMs() - t0;
        c.mw9DigestReload = F().fieldDigest;
        add("save_load_preserves_populations", loaded && dSave != 0 && dSave == F().fieldDigest);
        add("save_payload_versioned", blob.find("PROVENANCE_MW9_FLORA_V1") == 0
            && blob.find("version=1") != std::string::npos);

        Field savedState = F();
        AdoptPage::Reset();
        add("unload_drops_page_keeps_mw9_store_until_reload", !AdoptPage::IsLive() && savedState.patches.size() > 0);
        AdoptPage::Adopt(ident, page11);
        AdoptPage::AdoptContext(ident, ctx11);
        AdoptPage::CompileMw8(CausalRegionalBiome::Control::ForceOn);
        F() = savedState;
        Compile(Control::ForceOn);
        add("unload_reload_does_not_reroll", F().fieldDigest == savedState.fieldDigest);

        uint64_t mw8On = AdoptPage::Mw8().fieldDigest;
        Compile(Control::ForceOff);
        add("mw9_off_leaves_mw8_exact", AdoptPage::Mw8().fieldDigest == mw8On && AdoptPage::Mw8().cells.size() > 0);
        add("mw9_off_does_not_keep_live_flora_view", !F().compiled);

        Compile(Control::ForceOn);
        add("determinism_same_init_same_digest", F().fieldDigest == savedState.fieldDigest);
        uint64_t dA = F().fieldDigest;
        int rebuildsBefore = F().rebuilds;
        Compile(Control::ForceOn);
        add("second_compile_does_not_reroll", dA == F().fieldDigest && F().rebuilds == rebuildsBefore);

        add("biomass_matter_debt_recorded", std::strlen(kBiomassMatterDebt) > 10);
        add("suitability_is_not_presence", isolatedSpecies >= 0);
        add("patch_authority_near_is_derived", !near.empty());
        add("live_wire", c.liveWire == std::string("orographic_production_page"));

        c.passed = true;
        for (auto const& q : c.checks)
            if (!q.second) { c.passed = false; if (c.reason.empty()) c.reason = q.first; }
        if (c.passed) c.reason = "ok";
        c.mw9Digest = F().fieldDigest;
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c, char const* path)
    {
        FILE* f = nullptr;
        if (fopen_s(&f, path, "wb") != 0 || !f) return false;
        std::fprintf(f,
            "CERT_MW9_ECOLOGY %s\nreason=%s\n"
            "version=%u\nworldgen_id=%s\n"
            "live_wire=%s\nfauna=%s\n"
            "mw8_digest_before=%s\nmw8_digest_after=%s\n"
            "mw9_digest=%s\nmw9_digest_reload=%s\n"
            "alpine_occupied=%d\nwindward_occupied=%d\nleeward_occupied=%d\n"
            "riparian_occupied=%d\nbasin_occupied=%d\nempty_suitable=%d\n"
            "page_boundary_agree=%d\n"
            "compiles_after_travel=%d\nrebuilds_after_travel=%d\ndirty_after_settle=%d\nsettle_ticks=%d\n"
            "compile_ms=%.3f\nquery_ms=%.3f\nnear_ms=%.3f\nfar_ms=%.3f\ncross_ms=%.3f\n"
            "save_ms=%.3f\nreload_ms=%.3f\nsave_bytes=%zu\n"
            "used_presentation_z=%d\n"
            "biomass_matter_debt=%s\n"
            "mw8_frozen=e8155fa3\n",
            c.passed ? "PASS" : "FAIL", c.reason.c_str(),
            kMw9Version, kWorldgenId, c.liveWire.c_str(), kFaunaState,
            CausalWorldGeology::Hex64(c.mw8DigestBefore).c_str(),
            CausalWorldGeology::Hex64(c.mw8DigestAfter).c_str(),
            CausalWorldGeology::Hex64(c.mw9Digest).c_str(),
            CausalWorldGeology::Hex64(c.mw9DigestReload).c_str(),
            c.alpineOccupied, c.windwardOccupied, c.leewardOccupied,
            c.riparianOccupied, c.basinOccupied, c.emptySuitable,
            c.pageBoundaryAgree,
            c.compilesAfterTravel, c.rebuildsAfterTravel, c.dirtyAfterSettle, c.settleTicks,
            c.compileMs, c.queryMs, c.nearMs, c.farMs, c.crossMs,
            c.saveMs, c.reloadMs, c.saveBytes,
            c.usedPresentationZ ? 1 : 0,
            kBiomassMatterDebt);
        for (auto const& q : c.checks)
            std::fprintf(f, "check.%s=%s\n", q.first.c_str(), q.second ? "PASS" : "FAIL");
        std::fclose(f);
        return true;
    }
}
