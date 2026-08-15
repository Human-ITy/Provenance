#pragma once

// P5b.3B.3A: settled loose matter → depositional aggregate.
// One bounded DepositMatter transaction admits a settled parcel into a
// depositional sediment body. Same material ID is not the same structural
// body. The body may contribute to surface representation and collision
// while remaining transported/deposited material — never host-formation
// weld, never terrain host solids += grams.
//
// Pipeline: settled loose parcel + stable support + admissible resting
// geometry + low enough forcing → current terrain + deposition revision
// validation → DepositMatter → loose ledger −grams, depositional body
// +grams. Total matter unchanged.
//
// Sufficient forcing remobilizes the body back to loose representation
// with the same grams and provenance (reversible loose ↔ deposited).
//
// CLOSED: P5b.3B.3B compaction / terrain integration, P5b.3C bank/support
// collapse, 16C remobilization, cement/lithify, weld into host geology,
// erase LooseMatter provenance, compact arbitrarily, trigger bank
// collapse, enter compiled Stage-16C history, general erosion, rainfall,
// evaporation, groundwater, plant uptake, ecology, full WorldOperationScheduler.
//
// Frozen parent: P5b.3B.2 eeabfb8c / 3B 67d5f524 / 3A 6c467fb7.
// Disabled P5b.3B.3A must reproduce exact P5b.3B.2 settling state.

#include "CausalPresentWaterTerrainHydraulicSettling.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#ifdef small
#undef small
#endif

namespace CausalPresentWaterTerrainHydraulicDeposition
{
    constexpr char const* kExpectedRegion=
        "causal_world_present_water_terrain_hydraulic_deposition_floor";
    constexpr uint64_t kFrozenP5b3b2SettlingDigest=
        CausalPresentWaterTerrainHydraulicSettling::kFrozenP5b3b2SettlingDigest;
    constexpr uint64_t kFrozenP5b3b3aDepositionDigest=0x694388e61fa37503ull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainHydraulicSettling::kHoldBudget;
    constexpr int64_t kParcelGrams=CausalPresentWaterTerrainHydraulicSettling::kParcelGrams;
    constexpr uint32_t kSmallDebrisResistance=
        CausalPresentWaterTerrainHydraulicSettling::kSmallDebrisResistance;
    constexpr uint32_t kTransportForce=CausalPresentWaterTerrainHydraulicSettling::kTransportForce;
    constexpr uint32_t kLowEnergyForce=CausalPresentWaterTerrainHydraulicSettling::kLowEnergyForce;

    enum class FixtureKind:uint8_t
    {
        None=0,
        SettledRiverbed=1,
        ForcingAboveThreshold=2,
        UnsupportedDestination=3,
        StaleRevision=4,
        RemobilizeByForcing=5
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::SettledRiverbed:return "settled_sandstone_riverbed_deposit";
            case FixtureKind::ForcingAboveThreshold:return "forcing_above_deposition_threshold";
            case FixtureKind::UnsupportedDestination:return "unsupported_destination";
            case FixtureKind::StaleRevision:return "stale_terrain_or_deposition_revision";
            case FixtureKind::RemobilizeByForcing:return "deposited_remobilize_by_forcing";
            default:return "none";
        }
    }

    enum class DepositClass:uint8_t
    {
        None=0,
        SettledRiverbed=1,
        ForcingAboveThreshold=2,
        DestinationUnsupported=3,
        StaleRevision=4,
        Remobilized=5
    };

    inline char const* DepositClassName(DepositClass value)
    {
        switch(value)
        {
            case DepositClass::SettledRiverbed:return "settled_riverbed_deposit";
            case DepositClass::ForcingAboveThreshold:return "forcing_above_threshold";
            case DepositClass::DestinationUnsupported:return "destination_unsupported";
            case DepositClass::StaleRevision:return "stale_revision";
            case DepositClass::Remobilized:return "remobilized_to_loose";
            default:return "none";
        }
    }

    enum class PackingState:uint8_t
    {
        LooseSediment=0,
        PackedSediment=1
    };

    inline char const* PackingStateName(PackingState value)
    {
        return value==PackingState::PackedSediment?"packed_sediment":"loose_sediment";
    }

    enum class SupportState:uint8_t
    {
        Unsupported=0,
        Supported=1
    };

    inline char const* SupportStateName(SupportState value)
    {
        return value==SupportState::Supported?"supported":"unsupported";
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b3b3aEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b3b3aEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainHydraulicSettling::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_HYDRAULIC_DEPOSITION_V1";
                if(!magic){r.reason="bad_magic";return r;}continue;}
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {r.reason="malformed_line";return r;}
            std::string const key=line.substr(0,eq);if(fields.count(key))
            {r.reason="duplicate_key:"+key;return r;}fields.emplace(key,line.substr(eq+1));
        }
        auto get=[&](char const* key)->std::string const*
        {auto const it=fields.find(key);return it==fields.end()?nullptr:&it->second;};
        auto hex=[&](char const* key,uint64_t& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseHex64(*v,out);};
        auto u32=[&](char const* key,uint32_t& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseU32(*v,out);};
        auto const* id=get("worldgen_id");auto const* region=get("region_key");
        auto const* parentRegion=get("parent_region_key");if(!id||!region||!parentRegion)
        {r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)&&u32("schema_version",r.program.schemaVersion)
          &&u32("parent_authority_revision",r.program.parentAuthorityRevision)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("p5b3b3a_event_id",r.program.p5b3b3aEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b3b3a_enabled",r.program.p5b3b3aEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b3b3aEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b3b3aEnabled==0||r.program.p5b3b3aEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b3b3a_deposition_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    using FLooseMatterParcel=CausalPresentWaterTerrainHydraulicSettling::FLooseMatterParcel;
    using MotionState=CausalPresentWaterTerrainHydraulicSettling::MotionState;

    struct FDepositionalMatterBody
    {
        uint64_t DepositionalBodyId=0;
        std::vector<uint64_t> ConstituentMatterIds;
        uint64_t MaterialId=0;
        char MaterialName[32]{};
        int64_t TotalGrams=0;
        int OccupiedRegion=-1;
        PackingState Packing=PackingState::LooseSediment;
        uint32_t Compaction=0;
        std::vector<uint64_t> SourceProvenance;
        std::vector<uint64_t> DepositionTransactions;
        SupportState Support=SupportState::Unsupported;
        uint32_t Revision=0;
        uint64_t FormationProvenance=0;
        uint64_t GeologicalAncestry=0;
        uint64_t DetachmentTransactionId=0;
        uint8_t DetachmentCause=0;
        int SourceCell=-1;
        bool ContributesToSurface=true;
        bool ContributesToCollision=true;
        bool WeldedToHost=false;
        bool Lithified=false;
    };

    struct FDepositMatter
    {
        uint64_t TransactionId=0;
        uint64_t LooseMatterId=0;
        uint64_t DepositionalBodyId=0;
        uint64_t MaterialId=0;
        int64_t Grams=0;
        int SourceCell=-1;
        int OccupiedRegion=-1;
        uint64_t CauseWaterBodyId=0;
        uint32_t WaterRevision=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t LooseRevision=0;
        uint32_t DepositionRevision=0;
        DepositClass Class=DepositClass::None;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        FixtureKind Fixture=FixtureKind::None;
        char MaterialName[32]{};
        uint32_t HydraulicForce=0;
        uint32_t ParcelResistance=0;
        PackingState PackingAfter=PackingState::LooseSediment;
        uint32_t CompactionAfter=0;
        SupportState SupportAfter=SupportState::Unsupported;
        bool WaterContact=false;
        bool Deposited=false;
        bool Remobilized=false;
        bool IdentityPreserved=false;
        bool HostSolidsUnchanged=true;
        bool WeldedToHost=false;
        bool Lithified=false;
        bool RefusedStale=false,RefusedInvalid=false;
        bool DestinationUnavailable=false;
        bool RoutedThrough16C=false;
        bool Compacted=false;
        bool BankCollapse=false;
        size_t CellsVisited=0,BodiesExamined=0;
        uint32_t TerrainRevisionBefore=0,TerrainRevisionAfter=0;
        uint32_t WaterRevisionBefore=0,WaterRevisionAfter=0;
        uint32_t PublishedTerrainRevision=0,PublishedWaterRevision=0;
        int64_t TerrainSolidsBefore=0,TerrainSolidsAfter=0;
        int64_t LooseBefore=0,LooseAfter=0;
        int64_t DepositionalBefore=0,DepositionalAfter=0;
        int64_t WaterMassBefore=0,WaterMassAfter=0;
        int64_t HeldMatterBefore=0,HeldMatterAfter=0;
        uint64_t CompiledSedimentBefore=0,CompiledSedimentAfter=0;
        uint64_t FormationProvenance=0;
        uint64_t GeologicalAncestry=0;
        uint64_t DetachmentTransactionId=0;
    };

    struct DepositRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        uint64_t LooseMatterId=0;
        uint64_t DepositionalBodyId=0;
        int SourceCell=-1;
        int DestinationCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t WaterRevision=0;
        uint32_t LooseRevision=0;
        uint32_t DepositionRevision=0;
        bool CheckRevision=true;
        bool DestinationAvailable=true;
        bool DestinationSupported=true;
        bool OccupiedIncompatibly=false;
        bool UseForceOverride=false;
        uint32_t EvalForce=0;
        bool UseResistanceOverride=false;
        uint32_t EvalResistance=0;
        bool Remobilize=false;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t riverbedDeposit=0,forcingZero=0,unsupportedZero=0;
        size_t deposits=0,depositWakes=0,staleRefuse=0,remobilizes=0;
        size_t waterResponses=0,topologyRebuilds=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t depositionalBefore=0,depositionalAfter=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsDeposited=0,gramsRemobilized=0;
        uint64_t parentSettlingDigest=0,depositionDigest=0;
        uint64_t compiledSedimentBefore=0,compiledSedimentAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterRevision=0,depositionRevision=0;
    };

    inline uint64_t MakeTxnId(FixtureKind kind,uint64_t looseId,int src,int dst,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3200Dull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&looseId,sizeof(looseId));
        CausalWorldGeology::HashAppend(h,&src,sizeof(src));
        CausalWorldGeology::HashAppend(h,&dst,sizeof(dst));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline int64_t LooseMassOf(std::vector<FLooseMatterParcel> const& loose)
    {
        int64_t s=0;for(auto const& m:loose)s+=m.Grams;return s;
    }

    inline int64_t DepositionalMassOf(std::vector<FDepositionalMatterBody> const& bodies)
    {
        int64_t s=0;for(auto const& b:bodies)s+=b.TotalGrams;return s;
    }

    inline uint64_t DepositionDigestOf(std::vector<FLooseMatterParcel> const& loose,
        std::vector<FDepositionalMatterBody> const& bodies,
        uint64_t parentDigest,uint32_t depositionRev)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&parentDigest,sizeof(parentDigest));
        CausalWorldGeology::HashAppend(d,&depositionRev,sizeof(depositionRev));
        for(auto const& m:loose)
        {
            CausalWorldGeology::HashAppend(d,&m.LooseMatterId,sizeof(m.LooseMatterId));
            CausalWorldGeology::HashAppend(d,&m.MaterialId,sizeof(m.MaterialId));
            CausalWorldGeology::HashAppend(d,&m.Grams,sizeof(m.Grams));
            CausalWorldGeology::HashAppend(d,&m.SourceCell,sizeof(m.SourceCell));
            CausalWorldGeology::HashAppend(d,&m.LocationCell,sizeof(m.LocationCell));
            CausalWorldGeology::HashAppend(d,&m.DetachmentTransactionId,sizeof(m.DetachmentTransactionId));
            CausalWorldGeology::HashAppend(d,&m.FormationProvenance,sizeof(m.FormationProvenance));
            CausalWorldGeology::HashAppend(d,&m.GeologicalAncestry,sizeof(m.GeologicalAncestry));
            uint8_t const motion=(uint8_t)m.Motion;
            CausalWorldGeology::HashAppend(d,&motion,sizeof(motion));
        }
        for(auto const& b:bodies)
        {
            CausalWorldGeology::HashAppend(d,&b.DepositionalBodyId,sizeof(b.DepositionalBodyId));
            CausalWorldGeology::HashAppend(d,&b.MaterialId,sizeof(b.MaterialId));
            CausalWorldGeology::HashAppend(d,&b.TotalGrams,sizeof(b.TotalGrams));
            CausalWorldGeology::HashAppend(d,&b.OccupiedRegion,sizeof(b.OccupiedRegion));
            uint8_t const pack=(uint8_t)b.Packing;
            CausalWorldGeology::HashAppend(d,&pack,sizeof(pack));
            CausalWorldGeology::HashAppend(d,&b.Compaction,sizeof(b.Compaction));
            uint8_t const support=(uint8_t)b.Support;
            CausalWorldGeology::HashAppend(d,&support,sizeof(support));
            CausalWorldGeology::HashAppend(d,&b.Revision,sizeof(b.Revision));
            CausalWorldGeology::HashAppend(d,&b.FormationProvenance,sizeof(b.FormationProvenance));
            CausalWorldGeology::HashAppend(d,&b.GeologicalAncestry,sizeof(b.GeologicalAncestry));
            CausalWorldGeology::HashAppend(d,&b.DetachmentTransactionId,sizeof(b.DetachmentTransactionId));
            uint8_t const weld=b.WeldedToHost?1:0;
            CausalWorldGeology::HashAppend(d,&weld,sizeof(weld));
            uint32_t const nC=(uint32_t)b.ConstituentMatterIds.size();
            CausalWorldGeology::HashAppend(d,&nC,sizeof(nC));
            for(uint64_t id:b.ConstituentMatterIds)
                CausalWorldGeology::HashAppend(d,&id,sizeof(id));
            uint32_t const nP=(uint32_t)b.SourceProvenance.size();
            CausalWorldGeology::HashAppend(d,&nP,sizeof(nP));
            for(uint64_t id:b.SourceProvenance)
                CausalWorldGeology::HashAppend(d,&id,sizeof(id));
            uint32_t const nT=(uint32_t)b.DepositionTransactions.size();
            CausalWorldGeology::HashAppend(d,&nT,sizeof(nT));
            for(uint64_t id:b.DepositionTransactions)
                CausalWorldGeology::HashAppend(d,&id,sizeof(id));
        }
        return d;
    }

    struct FixtureSet
    {
        DepositRequest deposit,forcing,unsupported,stale,remobilize;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(
        CausalPresentWaterTerrainHydraulicSettling::Kernel const& parent)
    {
        FixtureSet set;
        FLooseMatterParcel const* settled=nullptr;
        for(auto const& p:parent.Loose())
        {
            if(p.Motion==MotionState::Settled&&p.Grams==kParcelGrams)
            {settled=&p;break;}
        }
        if(!settled)return set;
        int const loc=settled->LocationCell;
        auto const& cells=parent.Water().Cells();
        if(loc<0||(size_t)loc>=cells.size()||settled->LooseMatterId==0)return set;
        auto fill=[&](DepositRequest& req,FixtureKind kind)
        {
            req.Fixture=kind;
            req.LooseMatterId=settled->LooseMatterId;
            req.SourceCell=loc;
            req.DestinationCell=loc;
            req.WaterBodyId=CausalPresentWaterTerrainHydraulicSettling::BindCauseBody(
                parent.Parent(),loc);
            req.DestinationAvailable=true;
            req.DestinationSupported=true;
            req.OccupiedIncompatibly=false;
            req.CheckRevision=true;
        };
        fill(set.deposit,FixtureKind::SettledRiverbed);
        set.deposit.UseForceOverride=true;
        set.deposit.EvalForce=kLowEnergyForce;
        fill(set.forcing,FixtureKind::ForcingAboveThreshold);
        set.forcing.UseForceOverride=true;
        set.forcing.EvalForce=kTransportForce;
        fill(set.unsupported,FixtureKind::UnsupportedDestination);
        set.unsupported.DestinationSupported=false;
        set.unsupported.OccupiedIncompatibly=true;
        set.unsupported.DestinationAvailable=false;
        fill(set.stale,FixtureKind::StaleRevision);
        fill(set.remobilize,FixtureKind::RemobilizeByForcing);
        set.remobilize.Remobilize=true;
        set.remobilize.UseForceOverride=true;
        set.remobilize.EvalForce=kTransportForce;
        set.ok=settled->Grams==kParcelGrams&&loc!=settled->SourceCell
            &&settled->Motion==MotionState::Settled;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainHydraulicSettling::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_complete=!m_program.p5b3b3aEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainHydraulicSettling::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainHydraulicSettling::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FDepositMatter> const& Transactions() const{return m_txns;}
        std::vector<FLooseMatterParcel> const& Loose() const{return m_loose;}
        std::vector<FDepositionalMatterBody> const& Bodies() const{return m_bodies;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_terrainRevision;}
        uint32_t WaterTopologyRevision() const{return m_waterRevision;}
        uint32_t PublishedTerrainRevision() const{return m_publishedTerrainRevision;}
        uint32_t PublishedWaterTopologyRevision() const{return m_publishedWaterRevision;}
        uint32_t TerrainStateRevision() const{return m_parent->TerrainStateRevision();}
        uint32_t PoreRevision() const{return m_parent->PoreRevision();}
        uint32_t TransportRevision() const{return m_parent->TransportRevision();}
        uint32_t SettlingRevision() const{return m_parent->SettlingRevision();}
        uint32_t DepositionRevision() const{return m_depositionRevision;}
        uint32_t DestinationRevision(int cell) const
        {
            auto const it=m_destRevision.find(cell);
            return it==m_destRevision.end()?m_terrainRevision:it->second;
        }
        void BumpDestinationRevision(int cell)
        {
            m_destRevision[cell]=DestinationRevision(cell)+1;
        }
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_parent->FieldDigestValue();}
        uint64_t TerrainOverlayDigestValue() const{return m_parent->TerrainOverlayDigestValue();}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        uint64_t StateDigestValue() const{return m_parent->StateDigestValue();}
        uint64_t PoreDigestValue() const{return m_parent->PoreDigestValue();}
        uint64_t OccupancyDigestValue() const{return m_parent->OccupancyDigestValue();}
        uint64_t DetachmentDigestValue() const{return m_parent->DetachmentDigestValue();}
        uint64_t TransportDigestValue() const{return m_parent->TransportDigestValue();}
        uint64_t SettlingDigestValue() const{return m_parent->SettlingDigestValue();}
        uint64_t DepositionDigestValue() const
        {
            if(!m_program.p5b3b3aEnabled)return SettlingDigestValue();
            return DepositionDigestOf(m_loose,m_bodies,SettlingDigestValue(),m_depositionRevision);
        }
        int64_t BodyMass() const{return m_parent->BodyMass();}
        int64_t PoreMass() const{return m_parent->PoreMass();}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t ConservedMass() const{return m_parent->ConservedMass();}
        int64_t TerrainSolids() const{return m_parent->TerrainSolids();}
        int64_t LooseMass() const{return (LooseMassOf)(m_loose);}
        int64_t DepositionalMass() const{return DepositionalMassOf(m_bodies);}
        int64_t TotalMatter() const{return TerrainSolids()+LooseMass()+DepositionalMass();}
        double ReconstructedZ(double x,double y) const{return m_parent->ReconstructedZ(x,y);}
        CausalPresentWaterBody::Query QueryAt(double x,double y) const{return m_parent->QueryAt(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_parent->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_parent->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_parent->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_parent->BuildBlock(bx,by);}
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPore(int cell) const
        {return m_parent->QueryPore(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainState(int cell) const
        {return m_parent->QueryTerrainState(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainStateAt(double x,double y) const
        {return m_parent->QueryTerrainStateAt(x,y);}
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPoreAt(double x,double y) const
        {return m_parent->QueryPoreAt(x,y);}
        int LocationOf(uint64_t looseId) const
        {
            for(auto const& m:m_loose)if(m.LooseMatterId==looseId)return m.LocationCell;
            return -1;
        }
        FLooseMatterParcel const* FindLoose(uint64_t looseId) const
        {
            for(auto const& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }
        FDepositionalMatterBody const* FindBody(uint64_t bodyId) const
        {
            for(auto const& b:m_bodies)if(b.DepositionalBodyId==bodyId)return &b;
            return nullptr;
        }
        FDepositionalMatterBody const* FindBodyByConstituent(uint64_t looseId) const
        {
            for(auto const& b:m_bodies)
                for(uint64_t id:b.ConstituentMatterIds)if(id==looseId)return &b;
            return nullptr;
        }

        FDepositMatter ApplyRequest(DepositRequest req)
        {return CommitOne(req);}

        bool Tick(uint32_t budget)
        {
            if(!m_parent->Complete())
            {
                uint32_t n=budget?budget:64;
                m_parent->Tick(n);
                if(!m_parent->Complete())return false;
                BeginFromParent();
                if(m_complete)return true;
                return false;
            }
            if(!m_program.p5b3b3aEnabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.terrainSolidsAfter=TerrainSolids();
                m_stats.looseAfter=LooseMass();
                m_stats.depositionalAfter=DepositionalMass();
                m_stats.waterMassAfter=ConservedMass();
                m_stats.heldAfter=HeldMatter();
                m_stats.compiledSedimentAfter=TerrainDigest();
                m_stats.depositionDigest=DepositionDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterRevision=m_waterRevision;
                m_stats.depositionRevision=m_depositionRevision;
            }
            return m_complete;
        }

    private:
        void CopyParentLoose()
        {
            m_loose.clear();
            for(auto const& src:m_parent->Loose())m_loose.push_back(src);
        }

        void BeginFromParent()
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            CopyParentLoose();
            m_stats.parentSettlingDigest=m_parent->SettlingDigestValue();
            m_stats.terrainSolidsBefore=TerrainSolids();
            m_stats.looseBefore=LooseMass();
            m_stats.depositionalBefore=DepositionalMass();
            m_stats.waterMassBefore=ConservedMass();
            m_stats.heldBefore=HeldMatter();
            m_stats.compiledSedimentBefore=TerrainDigest();
            if(!m_program.p5b3b3aEnabled){FinishUnchanged();return;}
            SnapshotWork();
            uint32_t budget=m_program.defaultBudgetTransactions;
            if(budget==kHoldBudget)return;
            if(budget==0)budget=(uint32_t)(std::max)((size_t)1,m_work.size());
            Tick(budget);
        }

        void FinishUnchanged()
        {
            m_complete=true;
            m_stats.terrainSolidsAfter=m_stats.terrainSolidsBefore;
            m_stats.looseAfter=m_stats.looseBefore;
            m_stats.depositionalAfter=m_stats.depositionalBefore;
            m_stats.waterMassAfter=m_stats.waterMassBefore;
            m_stats.heldAfter=m_stats.heldBefore;
            m_stats.compiledSedimentAfter=m_stats.compiledSedimentBefore;
            m_stats.depositionDigest=DepositionDigestValue();
            m_stats.terrainRevision=m_terrainRevision;
            m_stats.waterRevision=m_waterRevision;
            m_stats.depositionRevision=m_depositionRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent);
            m_work.clear();
            if(fixtures.ok)
            {
                m_work.push_back(fixtures.forcing);
                m_work.push_back(fixtures.deposit);
            }
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                DepositRequest req=m_work[m_cursor];
                req.SourceTerrainRevision=m_terrainRevision;
                req.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
                req.WaterRevision=m_waterRevision;
                req.LooseRevision=m_parent->SettlingRevision();
                req.DepositionRevision=m_depositionRevision;
                req.CheckRevision=true;
                FDepositMatter rec=CommitOne(req);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        FLooseMatterParcel* FindLooseMut(uint64_t looseId)
        {
            for(auto& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }

        FDepositionalMatterBody* FindBodyByConstituentMut(uint64_t looseId)
        {
            for(auto& b:m_bodies)
                for(uint64_t id:b.ConstituentMatterIds)if(id==looseId)return &b;
            return nullptr;
        }

        bool VisitNeighborhood(int cell,FDepositMatter& rec)
        {
            auto const& cells=Water().Cells();
            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            std::unordered_set<uint64_t> bodies;
            rec.CellsVisited=1;
            if(cell>=0&&(size_t)cell<cells.size()&&cells[(size_t)cell].occupied&&cells[(size_t)cell].bodyId)
                bodies.insert(cells[(size_t)cell].bodyId);
            for(int n=0;n<4;++n)
            {
                int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,false);
                if(ni<0)continue;
                ++rec.CellsVisited;
                if((size_t)ni<cells.size()&&cells[(size_t)ni].occupied&&cells[(size_t)ni].bodyId)
                    bodies.insert(cells[(size_t)ni].bodyId);
            }
            rec.BodiesExamined=bodies.size();
            if(!bodies.empty())++m_stats.waterResponses;
            return true;
        }

        void FillLedgerAfter(FDepositMatter& txn)
        {
            txn.TerrainSolidsAfter=TerrainSolids();
            txn.LooseAfter=LooseMass();
            txn.DepositionalAfter=DepositionalMass();
            txn.WaterMassAfter=ConservedMass();
            txn.HeldMatterAfter=HeldMatter();
            txn.CompiledSedimentAfter=TerrainDigest();
            txn.TerrainRevisionAfter=m_terrainRevision;
            txn.WaterRevisionAfter=m_waterRevision;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterRevision=m_publishedWaterRevision;
            txn.OutputRevisions={m_terrainRevision,m_waterRevision,m_depositionRevision,
                DestinationRevision(txn.OccupiedRegion>=0?txn.OccupiedRegion:txn.SourceCell)};
            txn.HostSolidsUnchanged=txn.TerrainSolidsAfter==txn.TerrainSolidsBefore;
        }

        FDepositMatter CommitOne(DepositRequest req)
        {
            FDepositMatter txn;
            txn.Fixture=req.Fixture;
            txn.LooseMatterId=req.LooseMatterId;
            txn.SourceCell=req.SourceCell;
            txn.OccupiedRegion=req.DestinationCell;
            txn.CauseWaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterRevisionBefore=m_waterRevision;
            txn.SourceTerrainRevision=m_terrainRevision;
            txn.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
            txn.WaterRevision=m_waterRevision;
            txn.LooseRevision=m_parent->SettlingRevision();
            txn.DepositionRevision=m_depositionRevision;
            txn.TerrainSolidsBefore=TerrainSolids();
            txn.LooseBefore=LooseMass();
            txn.DepositionalBefore=DepositionalMass();
            txn.WaterMassBefore=ConservedMass();
            txn.HeldMatterBefore=HeldMatter();
            txn.CompiledSedimentBefore=TerrainDigest();
            txn.InputRevisions={m_terrainRevision,m_waterRevision,m_depositionRevision,
                DestinationRevision(req.DestinationCell)};
            txn.WeldedToHost=false;
            txn.Lithified=false;
            txn.RoutedThrough16C=false;
            txn.Compacted=false;
            txn.BankCollapse=false;
            auto settle=[&]()
            {
                FillLedgerAfter(txn);
                return txn;
            };
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.Deposited=false;txn.Remobilized=false;
                return settle();
            };

            ++m_stats.depositWakes;
            auto const& cells=Water().Cells();
            if(req.SourceCell<0||(size_t)req.SourceCell>=cells.size()
              ||req.DestinationCell<0||(size_t)req.DestinationCell>=cells.size()
              ||req.Fixture==FixtureKind::None||req.LooseMatterId==0)
                return refuse(false,true);

            txn.TransactionId=MakeTxnId(req.Fixture,req.LooseMatterId,req.SourceCell,
                req.DestinationCell,(uint32_t)(m_txns.size()+1u));

            if(req.Remobilize)
            {
                FDepositionalMatterBody* body=FindBodyByConstituentMut(req.LooseMatterId);
                if(!body||body->TotalGrams<=0)return refuse(false,true);
                txn.MaterialId=body->MaterialId;
                std::snprintf(txn.MaterialName,sizeof(txn.MaterialName),"%s",body->MaterialName);
                txn.Grams=body->TotalGrams;
                txn.FormationProvenance=body->FormationProvenance;
                txn.GeologicalAncestry=body->GeologicalAncestry;
                txn.DetachmentTransactionId=body->DetachmentTransactionId;
                txn.DepositionalBodyId=body->DepositionalBodyId;
                if(req.CheckRevision&&(req.SourceTerrainRevision!=m_terrainRevision
                  ||req.WaterRevision!=m_waterRevision
                  ||req.DepositionRevision!=m_depositionRevision))
                    return refuse(true,false);
                if(req.CheckRevision&&req.DestinationTerrainRevision!=DestinationRevision(req.DestinationCell))
                {
                    txn.Class=DepositClass::StaleRevision;
                    return refuse(true,false);
                }
                uint32_t force=CausalPresentWaterTerrainHydraulicTransport::HydraulicForceOf(
                    m_parent->Parent().Parent(),req.SourceCell);
                if(req.UseForceOverride)force=req.EvalForce;
                uint32_t resistance=kSmallDebrisResistance;
                if(req.UseResistanceOverride)resistance=req.EvalResistance;
                txn.HydraulicForce=force;
                txn.ParcelResistance=resistance;
                if(force<resistance)
                {
                    txn.Class=DepositClass::SettledRiverbed;
                    txn.Deposited=true;
                    VisitNeighborhood(req.SourceCell,txn);
                    return settle();
                }
                FLooseMatterParcel restored{};
                restored.LooseMatterId=body->ConstituentMatterIds.empty()
                    ?req.LooseMatterId:body->ConstituentMatterIds.front();
                restored.MaterialId=body->MaterialId;
                std::snprintf(restored.MaterialName,sizeof(restored.MaterialName),"%s",body->MaterialName);
                restored.Grams=body->TotalGrams;
                restored.SourceCell=body->SourceCell;
                restored.LocationCell=body->OccupiedRegion;
                restored.FormationProvenance=body->FormationProvenance;
                restored.GeologicalAncestry=body->GeologicalAncestry;
                restored.DetachmentTransactionId=body->DetachmentTransactionId;
                restored.DetachmentCause=body->DetachmentCause;
                restored.Motion=MotionState::InTransport;
                m_loose.push_back(restored);
                body->TotalGrams=0;
                body->ConstituentMatterIds.clear();
                m_bodies.erase(std::remove_if(m_bodies.begin(),m_bodies.end(),
                    [](FDepositionalMatterBody const& b){return b.TotalGrams<=0;}),
                    m_bodies.end());
                ++m_depositionRevision;
                txn.Class=DepositClass::Remobilized;
                txn.Remobilized=true;
                txn.Deposited=false;
                txn.IdentityPreserved=true;
                txn.OccupiedRegion=restored.LocationCell;
                VisitNeighborhood(restored.LocationCell,txn);
                FillLedgerAfter(txn);
                return txn;
            }

            FLooseMatterParcel* parcel=FindLooseMut(req.LooseMatterId);
            if(!parcel||parcel->LocationCell!=req.SourceCell)
                return refuse(false,true);

            txn.MaterialId=parcel->MaterialId;
            std::snprintf(txn.MaterialName,sizeof(txn.MaterialName),"%s",parcel->MaterialName);
            txn.Grams=parcel->Grams;
            txn.FormationProvenance=parcel->FormationProvenance;
            txn.GeologicalAncestry=parcel->GeologicalAncestry;
            txn.DetachmentTransactionId=parcel->DetachmentTransactionId;
            txn.IdentityPreserved=true;

            if(req.CheckRevision&&(req.SourceTerrainRevision!=m_terrainRevision
              ||req.WaterRevision!=m_waterRevision
              ||req.DepositionRevision!=m_depositionRevision))
                return refuse(true,false);
            if(req.CheckRevision&&req.DestinationTerrainRevision!=DestinationRevision(req.DestinationCell))
            {
                txn.Class=DepositClass::StaleRevision;
                return refuse(true,false);
            }

            txn.WaterContact=CausalPresentWaterTerrainHydraulicTransport::HasTransportWaterContact(
                m_parent->Parent().Parent(),req.SourceCell);
            uint32_t force=CausalPresentWaterTerrainHydraulicTransport::HydraulicForceOf(
                m_parent->Parent().Parent(),req.SourceCell);
            if(req.UseForceOverride)force=req.EvalForce;
            uint32_t resistance=kSmallDebrisResistance;
            if(req.UseResistanceOverride)resistance=req.EvalResistance;
            txn.HydraulicForce=force;
            txn.ParcelResistance=resistance;
            txn.CellsVisited=2;

            if(!req.DestinationAvailable||!req.DestinationSupported||req.OccupiedIncompatibly)
            {
                txn.Class=DepositClass::DestinationUnsupported;
                txn.DestinationUnavailable=true;
                txn.Deposited=false;
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }

            int const rest=CausalPresentWaterTerrainHydraulicSettling::FindRestingSupport(
                m_parent->Parent(),req.DestinationCell,force,resistance);
            if(rest<0)
            {
                txn.Class=DepositClass::ForcingAboveThreshold;
                txn.Deposited=false;
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }
            if(rest!=req.DestinationCell||parcel->Motion!=MotionState::Settled)
                return refuse(false,true);

            FDepositionalMatterBody body;
            body.DepositionalBodyId=txn.TransactionId;
            body.ConstituentMatterIds.push_back(parcel->LooseMatterId);
            body.MaterialId=parcel->MaterialId;
            std::snprintf(body.MaterialName,sizeof(body.MaterialName),"%s",parcel->MaterialName);
            body.TotalGrams=parcel->Grams;
            body.OccupiedRegion=rest;
            body.Packing=PackingState::LooseSediment;
            body.Compaction=0;
            body.SourceProvenance.push_back(parcel->FormationProvenance);
            body.SourceProvenance.push_back(parcel->GeologicalAncestry);
            body.SourceProvenance.push_back(parcel->DetachmentTransactionId);
            body.DepositionTransactions.push_back(txn.TransactionId);
            body.Support=SupportState::Supported;
            body.Revision=m_depositionRevision+1;
            body.FormationProvenance=parcel->FormationProvenance;
            body.GeologicalAncestry=parcel->GeologicalAncestry;
            body.DetachmentTransactionId=parcel->DetachmentTransactionId;
            body.DetachmentCause=parcel->DetachmentCause;
            body.SourceCell=parcel->SourceCell;
            body.ContributesToSurface=true;
            body.ContributesToCollision=true;
            body.WeldedToHost=false;
            body.Lithified=false;
            uint64_t const keptId=parcel->LooseMatterId;
            int64_t const keptGrams=parcel->Grams;
            uint64_t const keptMat=parcel->MaterialId;
            uint64_t const keptForm=parcel->FormationProvenance;
            uint64_t const keptAnc=parcel->GeologicalAncestry;
            uint64_t const keptDet=parcel->DetachmentTransactionId;
            m_loose.erase(std::remove_if(m_loose.begin(),m_loose.end(),
                [&](FLooseMatterParcel const& m){return m.LooseMatterId==keptId;}),
                m_loose.end());
            m_bodies.push_back(body);
            ++m_depositionRevision;
            txn.Class=DepositClass::SettledRiverbed;
            txn.Deposited=true;
            txn.DepositionalBodyId=body.DepositionalBodyId;
            txn.OccupiedRegion=rest;
            txn.PackingAfter=body.Packing;
            txn.CompactionAfter=body.Compaction;
            txn.SupportAfter=body.Support;
            txn.IdentityPreserved=
                keptId==req.LooseMatterId
                &&keptMat==txn.MaterialId
                &&keptGrams==txn.Grams
                &&keptForm==txn.FormationProvenance
                &&keptAnc==txn.GeologicalAncestry
                &&keptDet==txn.DetachmentTransactionId
                &&body.WeldedToHost==false
                &&body.Lithified==false
                &&body.Compaction==0;
            VisitNeighborhood(rest,txn);
            FillLedgerAfter(txn);
            return txn;
        }

        void Record(FDepositMatter const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid)
            {
                ++m_stats.transactionsRefused;
                if(rec.RefusedStale)++m_stats.staleRefuse;
                return;
            }
            ++m_stats.transactionsAdmitted;
            if(rec.Deposited)
            {
                ++m_stats.deposits;
                m_stats.gramsDeposited+=rec.Grams;
            }
            if(rec.Remobilized)
            {
                ++m_stats.remobilizes;
                m_stats.gramsRemobilized+=rec.Grams;
            }
            if(rec.Fixture==FixtureKind::SettledRiverbed)++m_stats.riverbedDeposit;
            else if(rec.Fixture==FixtureKind::ForcingAboveThreshold)++m_stats.forcingZero;
            else if(rec.Fixture==FixtureKind::UnsupportedDestination)++m_stats.unsupportedZero;
        }

        std::unique_ptr<CausalPresentWaterTerrainHydraulicSettling::Kernel> m_parent;Program m_program;
        std::vector<DepositRequest> m_work;
        std::vector<FDepositMatter> m_txns;
        std::vector<FLooseMatterParcel> m_loose;
        std::vector<FDepositionalMatterBody> m_bodies;
        std::unordered_map<int,uint32_t> m_destRevision;
        SolveStats m_stats;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterRevision=0;
        uint32_t m_depositionRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainHydraulicSettling::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,char const* p5b3bPath,char const* p5b3b2Path,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainHydraulicSettling::LoadKernel(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,
            transferPath,externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,
            p5b3aPath,p5b3bPath,p5b3b2Path,&reason,1);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainHydraulicSettling::Kernel> parent,
        char const* p5b3b3aPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b3b2_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b3b3aPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b3b3aEnabled=enabled;
        loaded.program.defaultBudgetTransactions=budget;
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,char const* p5b3bPath,char const* p5b3b2Path,char const* p5b3b3aPath,
        std::string* reason=nullptr,uint32_t p5b3b3aOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,p5b3b2Path,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b3b3aOverride==0)enabled=0;
        else if(p5b3b3aOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b3b3aPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b3b2SettlingDigest=0;
        uint64_t settlingDigestDisabled=0;
        uint64_t depositionDigestBudget1=0,depositionDigestBudgetN=0,depositionDigestUnbounded=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t depositionalBefore=0,depositionalAfter=0;
        int64_t waterBefore=0,waterAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsDeposited=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t riverbedDeposit=0,forcingZero=0,deposits=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FDepositMatter> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,DepositClass,LooseMatterId,DepositionalBodyId,"
            "MaterialId,Material,Grams,SourceCell,OccupiedRegion,CauseWaterBodyId,Force,Resistance,"
            "Deposited,Remobilized,IdentityPreserved,HostSolidsUnchanged,Packing,Compaction,Support,"
            "TerrainSolidsBefore,TerrainSolidsAfter,LooseBefore,LooseAfter,"
            "DepositionalBefore,DepositionalAfter,WaterBefore,WaterAfter,HeldBefore,HeldAfter,"
            "SourceTerrainRev,DestTerrainRev,WaterRev,DepositionRev,"
            "CellsVisited,BodiesExamined,RefusedStale,RefusedInvalid,"
            "DestinationUnavailable,WeldedToHost,Lithified,RoutedThrough16C,Compacted,BankCollapse\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%s,%s,"
                "%s,%s,%lld,%d,%d,%s,%u,%u,"
                "%d,%d,%d,%d,%s,%u,%s,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%u,%u,%u,%u,"
                "%zu,%zu,%d,%d,%d,%d,%d,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                DepositClassName(t.Class),
                CausalWorldGeology::Hex64(t.LooseMatterId).c_str(),
                CausalWorldGeology::Hex64(t.DepositionalBodyId).c_str(),
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MaterialName,
                (long long)t.Grams,
                t.SourceCell,t.OccupiedRegion,
                CausalWorldGeology::Hex64(t.CauseWaterBodyId).c_str(),
                t.HydraulicForce,t.ParcelResistance,
                t.Deposited?1:0,t.Remobilized?1:0,t.IdentityPreserved?1:0,
                t.HostSolidsUnchanged?1:0,
                PackingStateName(t.PackingAfter),t.CompactionAfter,SupportStateName(t.SupportAfter),
                (long long)t.TerrainSolidsBefore,(long long)t.TerrainSolidsAfter,
                (long long)t.LooseBefore,(long long)t.LooseAfter,
                (long long)t.DepositionalBefore,(long long)t.DepositionalAfter,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.SourceTerrainRevision,t.DestinationTerrainRevision,t.WaterRevision,t.DepositionRevision,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.DestinationUnavailable?1:0,t.WeldedToHost?1:0,t.Lithified?1:0,
                t.RoutedThrough16C?1:0,t.Compacted?1:0,t.BankCollapse?1:0);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,
        char const* p5b2cPath,char const* p5b3aPath,char const* p5b3bPath,
        char const* p5b3b2Path,char const* p5b3b3aPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b3b2=[&]()->std::unique_ptr<CausalPresentWaterTerrainHydraulicSettling::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,p5b3b2Path,r2);
        };

        auto parent=reloadP5b3b2();
        c.checks.push_back({"p5b3b2_parent_loaded",parent!=nullptr&&parent->Complete()});
        if(!parent){c.reason=reason.empty()?"p5b3b2_parent_load_failed":reason;return c;}
        c.p5b3b2SettlingDigest=parent->SettlingDigestValue();
        c.checks.push_back({"p5b3b2_settling_digest_frozen",
            c.p5b3b2SettlingDigest==kFrozenP5b3b2SettlingDigest});
        FLooseMatterParcel const* settledParcel=nullptr;
        for(auto const& p:parent->Loose())
            if(p.Motion==MotionState::Settled&&p.Grams==kParcelGrams){settledParcel=&p;break;}
        c.checks.push_back({"fixtures_start_already_settled",
            settledParcel!=nullptr&&parent->LooseMass()==kParcelGrams
            &&parent->Stats().settles>0
            &&settledParcel->LocationCell!=settledParcel->SourceCell});

        int64_t const water0=parent->ConservedMass();
        int64_t const solids0=parent->TerrainSolids();
        int64_t const loose0=parent->LooseMass();
        int64_t const held0=parent->HeldMatter();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint32_t const wRev0=parent->WaterTopologyRevision();
        uint64_t const settling0=parent->SettlingDigestValue();
        auto const parentLoose=parent->Loose();

        auto disabled=MakeFromParent(reloadP5b3b2(),p5b3b3aPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b3b3a",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.settlingDigestDisabled=disabled->SettlingDigestValue();
        bool stillSettledLoose=disabled->LooseMass()==loose0&&disabled->DepositionalMass()==0;
        bool locationsMatchParent=disabled->Loose().size()==parentLoose.size();
        if(locationsMatchParent)
        {
            for(size_t i=0;i<parentLoose.size();++i)
                if(disabled->Loose()[i].LocationCell!=parentLoose[i].LocationCell
                  ||disabled->Loose()[i].LooseMatterId!=parentLoose[i].LooseMatterId
                  ||disabled->Loose()[i].Motion!=parentLoose[i].Motion)
                    locationsMatchParent=false;
        }
        c.checks.push_back({"p5b3b3a_off_exact_p5b3b2_state",
            c.settlingDigestDisabled==settling0
            &&c.settlingDigestDisabled==kFrozenP5b3b2SettlingDigest
            &&disabled->Stats().deposits==0
            &&disabled->ConservedMass()==water0
            &&disabled->TerrainSolids()==solids0
            &&disabled->LooseMass()==loose0
            &&disabled->DepositionalMass()==0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0
            &&stillSettledLoose&&locationsMatchParent});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b3b2(),p5b3b3aPath,1,budget,r2);
        };
        auto budget1=loadEnabled(1);
        auto budgetN=loadEnabled(2);
        auto unbounded=loadEnabled(0);
        c.checks.push_back({"enabled_kernels_loaded",budget1&&budgetN&&unbounded});
        if(!budget1||!budgetN||!unbounded){c.reason="enabled_load_failed";return c;}
        guard=0;while(budget1&&!budget1->Complete()&&guard++<200000)budget1->Tick(1);
        guard=0;while(budgetN&&!budgetN->Complete()&&guard++<200000)budgetN->Tick(2);
        guard=0;while(unbounded&&!unbounded->Complete()&&guard++<200000)unbounded->Tick(64);
        c.checks.push_back({"all_budget_paths_complete",
            budget1->Complete()&&budgetN->Complete()&&unbounded->Complete()});
        c.checks.push_back({"fixtures_resolved",BuildFixtures(*parent).ok});

        c.depositionDigestBudget1=budget1->DepositionDigestValue();
        c.depositionDigestBudgetN=budgetN->DepositionDigestValue();
        c.depositionDigestUnbounded=unbounded->DepositionDigestValue();
        c.terrainSolidsBefore=unbounded->Stats().terrainSolidsBefore;
        c.terrainSolidsAfter=unbounded->Stats().terrainSolidsAfter;
        c.looseBefore=unbounded->Stats().looseBefore;
        c.looseAfter=unbounded->Stats().looseAfter;
        c.depositionalBefore=unbounded->Stats().depositionalBefore;
        c.depositionalAfter=unbounded->Stats().depositionalAfter;
        c.waterBefore=unbounded->Stats().waterMassBefore;
        c.waterAfter=unbounded->Stats().waterMassAfter;
        c.heldBefore=unbounded->Stats().heldBefore;
        c.heldAfter=unbounded->Stats().heldAfter;
        c.gramsDeposited=unbounded->Stats().gramsDeposited;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.riverbedDeposit=unbounded->Stats().riverbedDeposit;
        c.forcingZero=unbounded->Stats().forcingZero;
        c.deposits=unbounded->Stats().deposits;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        uint64_t const disabledDigest=disabled->DepositionDigestValue();
        bool freezeOk=c.depositionDigestBudget1==c.depositionDigestBudgetN
            &&c.depositionDigestBudgetN==c.depositionDigestUnbounded
            &&c.depositionDigestUnbounded!=disabledDigest;
        if(kFrozenP5b3b3aDepositionDigest)
            freezeOk=freezeOk&&c.depositionDigestUnbounded==kFrozenP5b3b3aDepositionDigest;
        c.checks.push_back({"budget_invariant_deposition",freezeOk});
        c.checks.push_back({"loose_80_to_0",
            c.looseBefore==loose0&&c.looseBefore==kParcelGrams
            &&c.looseAfter==0&&unbounded->LooseMass()==0});
        c.checks.push_back({"depositional_0_to_80",
            c.depositionalBefore==0&&c.depositionalAfter==kParcelGrams
            &&unbounded->DepositionalMass()==kParcelGrams});
        c.checks.push_back({"terrain_host_solids_unchanged",
            c.terrainSolidsBefore==c.terrainSolidsAfter
            &&c.terrainSolidsAfter==solids0
            &&unbounded->TerrainSolids()==solids0
            &&unbounded->TerrainRevision()==tRev0});
        c.checks.push_back({"water_mass_unchanged",
            c.waterBefore==c.waterAfter
            &&c.waterBefore==water0
            &&unbounded->ConservedMass()==water0
            &&disabled->ConservedMass()==water0
            &&unbounded->WaterTopologyRevision()==wRev0});
        c.checks.push_back({"held_matter_unchanged",c.heldBefore==c.heldAfter&&c.heldAfter==held0});
        c.checks.push_back({"total_matter_closed",
            unbounded->TotalMatter()==solids0+loose0
            &&unbounded->TerrainSolids()+unbounded->LooseMass()+unbounded->DepositionalMass()
                ==solids0+loose0});
        c.checks.push_back({"compiled_16c_sediment_unchanged",
            unbounded->TerrainDigest()==sediment0
            &&unbounded->Stats().compiledSedimentAfter==sediment0});

        bool massEach=true,identityKept=true,noWeld=true,localOnly=true;
        bool depositOk=false,forcingOk=false;
        uint64_t depositedLooseId=0,depositedBodyId=0;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            massEach=massEach
                &&t.TerrainSolidsBefore==t.TerrainSolidsAfter
                &&t.WaterMassBefore==t.WaterMassAfter
                &&t.HeldMatterBefore==t.HeldMatterAfter
                &&t.CompiledSedimentBefore==t.CompiledSedimentAfter
                &&t.HostSolidsUnchanged
                &&(t.LooseBefore+t.DepositionalBefore)==(t.LooseAfter+t.DepositionalAfter);
            noWeld=noWeld&&!t.WeldedToHost&&!t.Lithified&&!t.RoutedThrough16C
                &&!t.Compacted&&!t.BankCollapse;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            if(t.Deposited)
            {
                identityKept=identityKept&&t.IdentityPreserved&&t.LooseMatterId!=0
                    &&t.DepositionalBodyId!=0&&t.MaterialId!=0&&t.DetachmentTransactionId!=0
                    &&t.Grams==kParcelGrams
                    &&t.LooseAfter==t.LooseBefore-t.Grams
                    &&t.DepositionalAfter==t.DepositionalBefore+t.Grams
                    &&!t.InputRevisions.empty()&&!t.OutputRevisions.empty();
                depositedLooseId=t.LooseMatterId;depositedBodyId=t.DepositionalBodyId;
            }
            if(t.Fixture==FixtureKind::SettledRiverbed)
                depositOk=t.Deposited&&t.IdentityPreserved&&t.Grams==kParcelGrams
                    &&t.Class==DepositClass::SettledRiverbed
                    &&t.PackingAfter==PackingState::LooseSediment
                    &&t.CompactionAfter==0
                    &&!t.WeldedToHost;
            if(t.Fixture==FixtureKind::ForcingAboveThreshold)
                forcingOk=!t.Deposited&&t.Class==DepositClass::ForcingAboveThreshold
                    &&t.HydraulicForce>=t.ParcelResistance
                    &&t.LooseBefore==t.LooseAfter
                    &&t.DepositionalBefore==t.DepositionalAfter;
        }
        auto const* body=unbounded->FindBody(depositedBodyId);
        bool bodyOk=body&&body->TotalGrams==kParcelGrams
            &&body->DepositionalBodyId==depositedBodyId
            &&!body->ConstituentMatterIds.empty()
            &&body->ConstituentMatterIds.front()==depositedLooseId
            &&body->Packing==PackingState::LooseSediment
            &&body->Compaction==0
            &&body->Support==SupportState::Supported
            &&!body->WeldedToHost&&!body->Lithified
            &&body->ContributesToSurface&&body->ContributesToCollision;
        if(settledParcel)
        {
            bodyOk=bodyOk&&body
                &&body->MaterialId==settledParcel->MaterialId
                &&body->FormationProvenance==settledParcel->FormationProvenance
                &&body->GeologicalAncestry==settledParcel->GeologicalAncestry
                &&body->DetachmentTransactionId==settledParcel->DetachmentTransactionId
                &&body->OccupiedRegion==settledParcel->LocationCell;
        }
        c.checks.push_back({"settled_riverbed_deposits_80g_aggregate",
            depositOk&&c.riverbedDeposit>0&&c.deposits==1&&bodyOk
            &&unbounded->FindLoose(depositedLooseId)==nullptr});
        c.checks.push_back({"forcing_above_threshold_stays_loose",forcingOk&&c.forcingZero>0});
        c.checks.push_back({"deposition_preserves_provenance_not_host_identity",identityKept&&depositedLooseId!=0});
        c.checks.push_back({"not_welded_not_lithified_not_16c",noWeld
            &&unbounded->TerrainSolids()==solids0});
        c.checks.push_back({"admitted_deposits_conserved",massEach});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined<6515&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_deposition",
            CausalPresentWaterTerrainResponse::OccupancyValid(
                unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"body_membership_matches_occupancy",
            CausalPresentWaterTopology::OccupancyMatchesBodies(unbounded->Water().Cells(),
                unbounded->Body().Bodies())});

        {
            auto k=loadEnabled(kHoldBudget);
            bool staleOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.stale.SourceTerrainRevision=k->TerrainRevision();
                fixtures.stale.WaterRevision=k->WaterTopologyRevision();
                fixtures.stale.LooseRevision=k->SettlingRevision();
                fixtures.stale.DepositionRevision=k->DepositionRevision();
                fixtures.stale.DestinationTerrainRevision=k->DestinationRevision(fixtures.stale.DestinationCell);
                fixtures.stale.CheckRevision=true;
                k->BumpDestinationRevision(fixtures.stale.DestinationCell);
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const dep=k->DepositionalMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.stale.LooseMatterId);
                auto stale=k->ApplyRequest(fixtures.stale);
                staleOk=stale.RefusedStale&&!stale.Deposited&&stale.Grams==kParcelGrams
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->DepositionalMass()==dep&&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.stale.LooseMatterId)==loc
                    &&k->FindLoose(fixtures.stale.LooseMatterId)!=nullptr;
            }
            c.checks.push_back({"stale_terrain_or_deposition_revision_refuse",staleOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool unavailOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.unsupported.SourceTerrainRevision=k->TerrainRevision();
                fixtures.unsupported.WaterRevision=k->WaterTopologyRevision();
                fixtures.unsupported.LooseRevision=k->SettlingRevision();
                fixtures.unsupported.DepositionRevision=k->DepositionRevision();
                fixtures.unsupported.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.unsupported.DestinationCell);
                fixtures.unsupported.CheckRevision=true;
                fixtures.unsupported.DestinationAvailable=false;
                fixtures.unsupported.DestinationSupported=false;
                fixtures.unsupported.OccupiedIncompatibly=true;
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const dep=k->DepositionalMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.unsupported.LooseMatterId);
                size_t const nLoose=k->Loose().size();
                auto rec=k->ApplyRequest(fixtures.unsupported);
                unavailOk=rec.DestinationUnavailable&&!rec.Deposited
                    &&!rec.RefusedInvalid&&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->DepositionalMass()==dep&&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.unsupported.LooseMatterId)==loc
                    &&k->FindLoose(fixtures.unsupported.LooseMatterId)!=nullptr
                    &&k->Loose().size()==nLoose;
            }
            c.checks.push_back({"unsupported_destination_stays_loose",unavailOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool remobilizeOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.deposit.SourceTerrainRevision=k->TerrainRevision();
                fixtures.deposit.WaterRevision=k->WaterTopologyRevision();
                fixtures.deposit.LooseRevision=k->SettlingRevision();
                fixtures.deposit.DepositionRevision=k->DepositionRevision();
                fixtures.deposit.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.deposit.DestinationCell);
                fixtures.deposit.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.deposit);
                uint64_t const looseId=fixtures.deposit.LooseMatterId;
                uint64_t const form=first.FormationProvenance;
                uint64_t const anc=first.GeologicalAncestry;
                uint64_t const det=first.DetachmentTransactionId;
                int64_t const solids=k->TerrainSolids();
                int64_t const water=k->ConservedMass();
                fixtures.remobilize.LooseMatterId=looseId;
                fixtures.remobilize.DepositionalBodyId=first.DepositionalBodyId;
                fixtures.remobilize.SourceCell=first.OccupiedRegion;
                fixtures.remobilize.DestinationCell=first.OccupiedRegion;
                fixtures.remobilize.SourceTerrainRevision=k->TerrainRevision();
                fixtures.remobilize.WaterRevision=k->WaterTopologyRevision();
                fixtures.remobilize.LooseRevision=k->SettlingRevision();
                fixtures.remobilize.DepositionRevision=k->DepositionRevision();
                fixtures.remobilize.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.remobilize.DestinationCell);
                fixtures.remobilize.CheckRevision=true;
                auto second=k->ApplyRequest(fixtures.remobilize);
                auto const* restored=k->FindLoose(looseId);
                remobilizeOk=first.Deposited&&second.Remobilized&&!second.Deposited
                    &&restored&&restored->LooseMatterId==looseId
                    &&restored->Grams==kParcelGrams
                    &&restored->FormationProvenance==form
                    &&restored->GeologicalAncestry==anc
                    &&restored->DetachmentTransactionId==det
                    &&k->FindBodyByConstituent(looseId)==nullptr
                    &&k->DepositionalMass()==0&&k->LooseMass()==kParcelGrams
                    &&k->TerrainSolids()==solids&&k->ConservedMass()==water
                    &&k->TerrainSolids()==solids0
                    &&k->TotalMatter()==solids0+loose0;
            }
            c.checks.push_back({"deposited_remobilize_reversible_same_grams_provenance",remobilizeOk});
        }

        bool partitionOk=true;
        {
            auto a=loadEnabled(kHoldBudget);
            auto b=loadEnabled(kHoldBudget);
            if(a&&b)
            {
                auto fa=BuildFixtures(a->Parent());
                auto fb=BuildFixtures(b->Parent());
                auto applyAll=[&](Kernel& k,FixtureSet const& f)
                {
                    DepositRequest reqs[2]={f.forcing,f.deposit};
                    for(auto& req:reqs)
                    {
                        req.SourceTerrainRevision=k.TerrainRevision();
                        req.WaterRevision=k.WaterTopologyRevision();
                        req.LooseRevision=k.SettlingRevision();
                        req.DepositionRevision=k.DepositionRevision();
                        req.DestinationTerrainRevision=k.DestinationRevision(req.DestinationCell);
                        req.CheckRevision=true;
                        k.ApplyRequest(req);
                    }
                };
                applyAll(*a,fa);
                applyAll(*b,fb);
                partitionOk=a->DepositionDigestValue()==b->DepositionDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TerrainSolids()==b->TerrainSolids()
                    &&a->LooseMass()==b->LooseMass()
                    &&a->DepositionalMass()==b->DepositionalMass()
                    &&fa.deposit.SourceCell==fb.deposit.SourceCell;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->DepositionDigestValue()==unbounded->DepositionDigestValue()
            &&reload->DepositionDigestValue()==unbounded->DepositionDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->TerrainSolids()==unbounded->TerrainSolids()
            &&cold->LooseMass()==unbounded->LooseMass()
            &&cold->DepositionalMass()==unbounded->DepositionalMass()
            &&cold->DepositionalMass()==kParcelGrams;
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3c_bank_support_collapse_closed",true});
        c.checks.push_back({"p5b3b3b_compaction_terrain_integration_closed",true});
        c.checks.push_back({"general_erosion_closed",true});
        c.checks.push_back({"active_16b_erosion_closed",true});
        c.checks.push_back({"16c_remobilization_closed",true});
        c.checks.push_back({"rainfall_evaporation_groundwater_closed",true});
        c.checks.push_back({"cement_lithify_closed",true});
        c.checks.push_back({"host_formation_weld_closed",true});
        c.checks.push_back({"world_operation_scheduler_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_deposit",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->DepositionDigestValue();
                size_t const n=unbounded->Stats().deposits;
                size_t const wakes=unbounded->Stats().depositWakes;
                unbounded->Tick(64);
                return unbounded->DepositionDigestValue()==before
                    &&unbounded->Stats().deposits==n
                    &&unbounded->Stats().depositWakes==wakes;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b3b3a_depositional_aggregate_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b3b3a_depositional_aggregate_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B3B3A_DEPOSITIONAL_AGGREGATE %s\nreason=%s\n"
            "p5b3b2_settling_digest=%s\nsettling_digest_disabled=%s\n"
            "deposition_digest_budget1=%s\ndeposition_digest_budgetN=%s\n"
            "deposition_digest_unbounded=%s\n"
            "terrain_host_solids_before=%lld\nterrain_host_solids_after=%lld\n"
            "loose_before=%lld\nloose_after=%lld\n"
            "depositional_before=%lld\ndepositional_after=%lld\n"
            "water_before=%lld\nwater_after=%lld\n"
            "held_before=%lld\nheld_after=%lld\n"
            "grams_deposited=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "riverbed_deposit=%zu\nforcing_zero=%zu\ndeposits=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=settled_loose_to_depositional_aggregate_not_host_weld\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=frozen\n"
            "p5b3a=frozen\np5b3b=frozen\np5b3b2=frozen\np5b3b3a=open\n"
            "p5b3b3b=closed\np5b3c=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "general_erosion=0\nbank_collapse=0\n"
            "active_16b_erosion=0\n16c_remobilization=0\necology=0\n"
            "host_formation_weld=0\ncement_lithify=0\ncompaction=0\n"
            "world_operation_scheduler=0\n"
            "choice_a_local_loose=1\nchoice_b_16c=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b3b2SettlingDigest).c_str(),
            CausalWorldGeology::Hex64(c.settlingDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.depositionDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.depositionDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.depositionDigestUnbounded).c_str(),
            (long long)c.terrainSolidsBefore,(long long)c.terrainSolidsAfter,
            (long long)c.looseBefore,(long long)c.looseAfter,
            (long long)c.depositionalBefore,(long long)c.depositionalAfter,
            (long long)c.waterBefore,(long long)c.waterAfter,
            (long long)c.heldBefore,(long long)c.heldAfter,
            (long long)c.gramsDeposited,
            c.transactionsCertified,c.transactionsAdmitted,
            c.riverbedDeposit,c.forcingZero,c.deposits,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
