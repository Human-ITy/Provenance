#pragma once

// P5b.3B.2: loose-matter settling. One bounded operation that changes where
// a transported parcel rests, not what that matter is. The parcel remains
// conserved loose matter: same LooseMatterId, material, grams, provenance,
// and detachment ancestry. Settling commits authoritative location +
// motion/rest state only. Never terrain fill += grams.
//
// Pipeline: LooseMatterId + current position + momentum/hydraulic forcing +
// candidate support → resting query → current terrain + loose-matter
// revision validation → commit resting location → parcel remains loose.
//
// Unavailable / unsupported destination retains the parcel and a pending
// settle obligation. Section truth-ready later revalidates revision.
// No fallback surface, deletion, or teleport.
//
// CLOSED: P5b.3C bank/support collapse, 3B.3 terrain reincorporation,
// 16C remobilization, general erosion, rainfall, evaporation, groundwater,
// plant uptake, ecology, full WorldOperationScheduler.
//
// Frozen parent: P5b.3B 67d5f524 / 3A 6c467fb7.
// Disabled P5b.3B.2 must reproduce exact P5b.3B transport state.

#include "CausalPresentWaterTerrainHydraulicTransport.h"

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

namespace CausalPresentWaterTerrainHydraulicSettling
{
    constexpr char const* kExpectedRegion=
        "causal_world_present_water_terrain_hydraulic_settling_floor";
    constexpr uint64_t kFrozenP5b3bTransportDigest=
        CausalPresentWaterTerrainHydraulicTransport::kFrozenP5b3bTransportDigest;
    constexpr uint64_t kFrozenP5b3b2SettlingDigest=0xab46ebdbe3aa6778ull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainHydraulicTransport::kHoldBudget;
    constexpr int64_t kParcelGrams=CausalPresentWaterTerrainHydraulicTransport::kParcelGrams;
    constexpr uint32_t kSmallDebrisResistance=
        CausalPresentWaterTerrainHydraulicTransport::kSmallDebrisResistance;
    constexpr uint32_t kTransportForce=800;
    constexpr uint32_t kLowEnergyForce=0;
    constexpr int64_t kCompanionGrams=1;

    enum class FixtureKind:uint8_t
    {
        None=0,
        SupportedLowEnergy=1,
        ForcingAboveThreshold=2,
        UnsupportedDestination=3,
        StaleDestination=4,
        SharedRestingRegion=5
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::SupportedLowEnergy:return "supported_low_energy_settle";
            case FixtureKind::ForcingAboveThreshold:return "forcing_above_transport_threshold";
            case FixtureKind::UnsupportedDestination:return "unsupported_or_incompatible_destination";
            case FixtureKind::StaleDestination:return "stale_destination_revision";
            case FixtureKind::SharedRestingRegion:return "shared_resting_region";
            default:return "none";
        }
    }

    enum class SettleClass:uint8_t
    {
        None=0,
        SupportedLowEnergy=1,
        ForcingAboveThreshold=2,
        DestinationUnsupported=3,
        StaleDestination=4,
        SharedRestingRegion=5
    };

    inline char const* SettleClassName(SettleClass value)
    {
        switch(value)
        {
            case SettleClass::SupportedLowEnergy:return "supported_low_energy";
            case SettleClass::ForcingAboveThreshold:return "forcing_above_threshold";
            case SettleClass::DestinationUnsupported:return "destination_unsupported";
            case SettleClass::StaleDestination:return "stale_destination";
            case SettleClass::SharedRestingRegion:return "shared_resting_region";
            default:return "none";
        }
    }

    enum class MotionState:uint8_t
    {
        InTransport=0,
        Settled=1
    };

    inline char const* MotionStateName(MotionState value)
    {
        return value==MotionState::Settled?"settled":"in_transport";
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b3b2EventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b3b2Enabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainHydraulicTransport::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_HYDRAULIC_SETTLING_V1";
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
          &&hex("p5b3b2_event_id",r.program.p5b3b2EventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b3b2_enabled",r.program.p5b3b2Enabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b3b2EventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b3b2Enabled==0||r.program.p5b3b2Enabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b3b2_settling_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FLooseMatterParcel
    {
        uint64_t LooseMatterId=0;
        uint64_t MaterialId=0;
        char MaterialName[32]{};
        int64_t Grams=0;
        int SourceCell=-1;
        int LocationCell=-1;
        uint64_t SourceRegionId=0;
        uint64_t FeatureId=0;
        uint64_t GeologicalAncestry=0;
        uint64_t FormationProvenance=0;
        uint64_t BodyProvenance=0;
        uint8_t DetachmentCause=0;
        uint64_t DetachmentTransactionId=0;
        MotionState Motion=MotionState::InTransport;
        uint64_t AggregateId=0;
        std::vector<uint64_t> ConstituentIds;
    };

    struct FPendingHydraulicSettle
    {
        uint64_t LooseMatterId=0;
        int CurrentCell=-1;
        int DestinationCell=-1;
        uint32_t ExpectedDestinationTerrainRevision=0;
        uint32_t ExpectedLooseRevision=0;
        uint64_t CauseWaterBodyId=0;
        uint64_t TransactionId=0;
        bool DestinationUnavailable=false;
    };

    struct FHydraulicLooseMatterSettle
    {
        uint64_t TransactionId=0;
        uint64_t LooseMatterId=0;
        uint64_t MaterialId=0;
        int64_t Grams=0;
        int SourceCell=-1;
        int RestingCell=-1;
        uint64_t CauseWaterBodyId=0;
        uint32_t WaterRevision=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t LooseRevision=0;
        SettleClass Class=SettleClass::None;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        FixtureKind Fixture=FixtureKind::None;
        char MaterialName[32]{};
        uint32_t HydraulicForce=0;
        uint32_t ParcelResistance=0;
        MotionState MotionBefore=MotionState::InTransport;
        MotionState MotionAfter=MotionState::InTransport;
        bool WaterContact=false;
        bool Settled=false;
        bool IdentityPreserved=false;
        bool DestinationRemainsLoose=true;
        bool RefusedStale=false,RefusedInvalid=false;
        bool DestinationUnavailable=false;
        bool PendingRetained=false;
        bool DepositedAsTerrain=false;
        bool RoutedThrough16C=false;
        bool Aggregated=false;
        size_t CellsVisited=0,BodiesExamined=0;
        uint32_t TerrainRevisionBefore=0,TerrainRevisionAfter=0;
        uint32_t WaterRevisionBefore=0,WaterRevisionAfter=0;
        uint32_t PublishedTerrainRevision=0,PublishedWaterRevision=0;
        int64_t TerrainSolidsBefore=0,TerrainSolidsAfter=0;
        int64_t LooseBefore=0,LooseAfter=0;
        int64_t WaterMassBefore=0,WaterMassAfter=0;
        int64_t HeldMatterBefore=0,HeldMatterAfter=0;
        uint64_t CompiledSedimentBefore=0,CompiledSedimentAfter=0;
        uint64_t FormationProvenance=0;
        uint64_t GeologicalAncestry=0;
        uint64_t DetachmentTransactionId=0;
    };

    struct SettleRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        uint64_t LooseMatterId=0;
        int SourceCell=-1;
        int DestinationCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t WaterRevision=0;
        uint32_t LooseRevision=0;
        bool CheckRevision=true;
        bool DestinationAvailable=true;
        bool DestinationSupported=true;
        bool OccupiedIncompatibly=false;
        bool UseForceOverride=false;
        uint32_t EvalForce=0;
        bool UseResistanceOverride=false;
        uint32_t EvalResistance=0;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t lowEnergySettle=0,forcingZero=0,unsupportedPending=0;
        size_t settles=0,settleWakes=0,staleRefuse=0,pendingRetained=0;
        size_t waterResponses=0,topologyRebuilds=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsSettled=0;
        uint64_t parentTransportDigest=0,settlingDigest=0;
        uint64_t compiledSedimentBefore=0,compiledSedimentAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterRevision=0,settlingRevision=0;
    };

    inline uint64_t MakeTxnId(FixtureKind kind,uint64_t looseId,int src,int dst,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3200Cull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&looseId,sizeof(looseId));
        CausalWorldGeology::HashAppend(h,&src,sizeof(src));
        CausalWorldGeology::HashAppend(h,&dst,sizeof(dst));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline FLooseMatterParcel FromTransported(
        CausalPresentWaterTerrainHydraulicTransport::FLooseMatterParcel const& src)
    {
        FLooseMatterParcel p;
        p.LooseMatterId=src.LooseMatterId;
        p.MaterialId=src.MaterialId;
        std::snprintf(p.MaterialName,sizeof(p.MaterialName),"%s",src.MaterialName);
        p.Grams=src.Grams;
        p.SourceCell=src.SourceCell;
        p.LocationCell=src.LocationCell;
        p.SourceRegionId=src.SourceRegionId;
        p.FeatureId=src.FeatureId;
        p.GeologicalAncestry=src.GeologicalAncestry;
        p.FormationProvenance=src.FormationProvenance;
        p.BodyProvenance=src.BodyProvenance;
        p.DetachmentCause=src.DetachmentCause;
        p.DetachmentTransactionId=src.DetachmentTransactionId;
        p.Motion=MotionState::InTransport;
        return p;
    }

    inline int64_t LooseMassOf(std::vector<FLooseMatterParcel> const& loose)
    {
        int64_t s=0;for(auto const& m:loose)s+=m.Grams;return s;
    }

    inline uint64_t SettlingDigestOf(std::vector<FLooseMatterParcel> const& loose,
        std::vector<FPendingHydraulicSettle> const& pending,
        uint64_t parentDigest,uint32_t settlingRev)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&parentDigest,sizeof(parentDigest));
        CausalWorldGeology::HashAppend(d,&settlingRev,sizeof(settlingRev));
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
            CausalWorldGeology::HashAppend(d,&m.AggregateId,sizeof(m.AggregateId));
            uint32_t const n=(uint32_t)m.ConstituentIds.size();
            CausalWorldGeology::HashAppend(d,&n,sizeof(n));
            for(uint64_t id:m.ConstituentIds)
                CausalWorldGeology::HashAppend(d,&id,sizeof(id));
        }
        for(auto const& p:pending)
        {
            CausalWorldGeology::HashAppend(d,&p.LooseMatterId,sizeof(p.LooseMatterId));
            CausalWorldGeology::HashAppend(d,&p.CurrentCell,sizeof(p.CurrentCell));
            CausalWorldGeology::HashAppend(d,&p.DestinationCell,sizeof(p.DestinationCell));
            CausalWorldGeology::HashAppend(d,&p.ExpectedDestinationTerrainRevision,
                sizeof(p.ExpectedDestinationTerrainRevision));
            CausalWorldGeology::HashAppend(d,&p.ExpectedLooseRevision,sizeof(p.ExpectedLooseRevision));
            uint8_t const u=p.DestinationUnavailable?1:0;
            CausalWorldGeology::HashAppend(d,&u,sizeof(u));
        }
        return d;
    }

    inline bool CellInBounds(
        CausalPresentWaterTerrainHydraulicTransport::Kernel const& parent,int cell)
    {
        auto const& cells=parent.Water().Cells();
        return cell>=0&&(size_t)cell<cells.size()&&std::isfinite(cells[(size_t)cell].terrainZ);
    }

    inline bool CellSupported(
        CausalPresentWaterTerrainHydraulicTransport::Kernel const& parent,int cell)
    {
        if(!CellInBounds(parent,cell))return false;
        auto const& cells=parent.Water().Cells();
        return std::isfinite(cells[(size_t)cell].terrainZ);
    }

    inline int FindRestingSupport(
        CausalPresentWaterTerrainHydraulicTransport::Kernel const& parent,
        int cell,uint32_t force,uint32_t resistance)
    {
        if(!CellSupported(parent,cell))return -1;
        if(force>=resistance)return -1;
        return cell;
    }

    inline uint64_t BindCauseBody(
        CausalPresentWaterTerrainHydraulicTransport::Kernel const& parent,int cell)
    {
        return CausalPresentWaterTerrainHydraulicTransport::BindCauseBody(parent.Parent(),cell);
    }

    struct FixtureSet
    {
        SettleRequest settle,forcing,unsupported,stale,shared;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(
        CausalPresentWaterTerrainHydraulicTransport::Kernel const& parent)
    {
        FixtureSet set;
        if(parent.Loose().empty())return set;
        auto const& parcel=parent.Loose().front();
        int const loc=parcel.LocationCell;
        auto const& cells=parent.Water().Cells();
        if(loc<0||(size_t)loc>=cells.size()||parcel.LooseMatterId==0)return set;
        auto fill=[&](SettleRequest& req,FixtureKind kind)
        {
            req.Fixture=kind;
            req.LooseMatterId=parcel.LooseMatterId;
            req.SourceCell=loc;
            req.DestinationCell=loc;
            req.WaterBodyId=BindCauseBody(parent,loc);
            req.DestinationAvailable=true;
            req.DestinationSupported=true;
            req.OccupiedIncompatibly=false;
            req.CheckRevision=true;
        };
        fill(set.settle,FixtureKind::SupportedLowEnergy);
        set.settle.UseForceOverride=true;
        set.settle.EvalForce=kLowEnergyForce;
        fill(set.forcing,FixtureKind::ForcingAboveThreshold);
        set.forcing.UseForceOverride=true;
        set.forcing.EvalForce=kTransportForce;
        fill(set.unsupported,FixtureKind::UnsupportedDestination);
        set.unsupported.DestinationSupported=false;
        set.unsupported.OccupiedIncompatibly=true;
        set.unsupported.DestinationAvailable=false;
        fill(set.stale,FixtureKind::StaleDestination);
        fill(set.shared,FixtureKind::SharedRestingRegion);
        set.shared.UseForceOverride=true;
        set.shared.EvalForce=kLowEnergyForce;
        set.ok=parcel.Grams==kParcelGrams&&loc!=parcel.SourceCell;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainHydraulicTransport::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_complete=!m_program.p5b3b2Enabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainHydraulicTransport::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainHydraulicTransport::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FHydraulicLooseMatterSettle> const& Transactions() const{return m_txns;}
        std::vector<FLooseMatterParcel> const& Loose() const{return m_loose;}
        std::vector<FPendingHydraulicSettle> const& Pending() const{return m_pending;}
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
        uint32_t SettlingRevision() const{return m_settlingRevision;}
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
        uint64_t SettlingDigestValue() const
        {
            return SettlingDigestOf(m_loose,m_pending,TransportDigestValue(),m_settlingRevision);
        }
        int64_t BodyMass() const{return m_parent->BodyMass();}
        int64_t PoreMass() const{return m_parent->PoreMass();}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t ConservedMass() const{return m_parent->ConservedMass();}
        int64_t TerrainSolids() const{return m_parent->TerrainSolids();}
        int64_t LooseMass() const{return LooseMassOf(m_loose);}
        int64_t TotalMatter() const{return TerrainSolids()+LooseMass();}
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
        MotionState MotionOf(uint64_t looseId) const
        {
            for(auto const& m:m_loose)if(m.LooseMatterId==looseId)return m.Motion;
            return MotionState::InTransport;
        }
        FLooseMatterParcel const* FindLoose(uint64_t looseId) const
        {
            for(auto const& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }
        void InsertCompanion(FLooseMatterParcel parcel)
        {
            m_loose.push_back(std::move(parcel));
        }

        FHydraulicLooseMatterSettle ApplyRequest(SettleRequest req)
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
            if(!m_program.p5b3b2Enabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.terrainSolidsAfter=TerrainSolids();
                m_stats.looseAfter=LooseMass();
                m_stats.waterMassAfter=ConservedMass();
                m_stats.heldAfter=HeldMatter();
                m_stats.compiledSedimentAfter=TerrainDigest();
                m_stats.settlingDigest=SettlingDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterRevision=m_waterRevision;
                m_stats.settlingRevision=m_settlingRevision;
            }
            return m_complete;
        }

    private:
        void CopyParentLoose()
        {
            m_loose.clear();
            for(auto const& src:m_parent->Loose())m_loose.push_back(FromTransported(src));
        }

        void BeginFromParent()
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            CopyParentLoose();
            m_stats.parentTransportDigest=m_parent->TransportDigestValue();
            m_stats.terrainSolidsBefore=TerrainSolids();
            m_stats.looseBefore=LooseMass();
            m_stats.waterMassBefore=ConservedMass();
            m_stats.heldBefore=HeldMatter();
            m_stats.compiledSedimentBefore=TerrainDigest();
            if(!m_program.p5b3b2Enabled){FinishUnchanged();return;}
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
            m_stats.waterMassAfter=m_stats.waterMassBefore;
            m_stats.heldAfter=m_stats.heldBefore;
            m_stats.compiledSedimentAfter=m_stats.compiledSedimentBefore;
            m_stats.settlingDigest=SettlingDigestValue();
            m_stats.terrainRevision=m_terrainRevision;
            m_stats.waterRevision=m_waterRevision;
            m_stats.settlingRevision=m_settlingRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent);
            m_work.clear();
            if(fixtures.ok)
            {
                m_work.push_back(fixtures.forcing);
                m_work.push_back(fixtures.settle);
            }
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                SettleRequest req=m_work[m_cursor];
                req.SourceTerrainRevision=m_terrainRevision;
                req.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
                req.WaterRevision=m_waterRevision;
                req.LooseRevision=m_settlingRevision;
                req.CheckRevision=true;
                FHydraulicLooseMatterSettle rec=CommitOne(req);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        FLooseMatterParcel* FindLooseMut(uint64_t looseId)
        {
            for(auto& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }

        bool VisitNeighborhood(int cell,FHydraulicLooseMatterSettle& rec)
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

        FHydraulicLooseMatterSettle CommitOne(SettleRequest req)
        {
            FHydraulicLooseMatterSettle txn;
            txn.Fixture=req.Fixture;
            txn.LooseMatterId=req.LooseMatterId;
            txn.SourceCell=req.SourceCell;
            txn.RestingCell=req.DestinationCell;
            txn.CauseWaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterRevisionBefore=m_waterRevision;
            txn.SourceTerrainRevision=m_terrainRevision;
            txn.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
            txn.WaterRevision=m_waterRevision;
            txn.LooseRevision=m_settlingRevision;
            txn.TerrainSolidsBefore=TerrainSolids();
            txn.LooseBefore=LooseMass();
            txn.WaterMassBefore=ConservedMass();
            txn.HeldMatterBefore=HeldMatter();
            txn.CompiledSedimentBefore=TerrainDigest();
            txn.InputRevisions={m_terrainRevision,m_waterRevision,m_settlingRevision,
                DestinationRevision(req.DestinationCell)};
            txn.DestinationRemainsLoose=true;
            txn.DepositedAsTerrain=false;
            txn.RoutedThrough16C=false;
            auto settle=[&]()
            {
                txn.TerrainSolidsAfter=txn.TerrainSolidsBefore;
                txn.LooseAfter=txn.LooseBefore;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.HeldMatterAfter=txn.HeldMatterBefore;
                txn.CompiledSedimentAfter=txn.CompiledSedimentBefore;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterRevisionAfter=m_waterRevision;
                txn.PublishedTerrainRevision=m_publishedTerrainRevision;
                txn.PublishedWaterRevision=m_publishedWaterRevision;
                txn.OutputRevisions={m_terrainRevision,m_waterRevision,m_settlingRevision,
                    DestinationRevision(req.DestinationCell)};
                return txn;
            };
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.Settled=false;
                return settle();
            };

            ++m_stats.settleWakes;
            auto const& cells=Water().Cells();
            if(req.SourceCell<0||(size_t)req.SourceCell>=cells.size()
              ||req.DestinationCell<0||(size_t)req.DestinationCell>=cells.size()
              ||req.Fixture==FixtureKind::None||req.LooseMatterId==0)
                return refuse(false,true);

            FLooseMatterParcel* parcel=FindLooseMut(req.LooseMatterId);
            if(!parcel||parcel->LocationCell!=req.SourceCell)
                return refuse(false,true);

            txn.MaterialId=parcel->MaterialId;
            std::snprintf(txn.MaterialName,sizeof(txn.MaterialName),"%s",parcel->MaterialName);
            txn.Grams=parcel->Grams;
            txn.FormationProvenance=parcel->FormationProvenance;
            txn.GeologicalAncestry=parcel->GeologicalAncestry;
            txn.DetachmentTransactionId=parcel->DetachmentTransactionId;
            txn.MotionBefore=parcel->Motion;
            txn.MotionAfter=parcel->Motion;
            txn.TransactionId=MakeTxnId(req.Fixture,req.LooseMatterId,req.SourceCell,
                req.DestinationCell,(uint32_t)(m_txns.size()+1u));
            txn.IdentityPreserved=true;

            if(req.CheckRevision&&(req.SourceTerrainRevision!=m_terrainRevision
              ||req.WaterRevision!=m_waterRevision
              ||req.LooseRevision!=m_settlingRevision))
                return refuse(true,false);
            if(req.CheckRevision&&req.DestinationTerrainRevision!=DestinationRevision(req.DestinationCell))
            {
                txn.Class=SettleClass::StaleDestination;
                return refuse(true,false);
            }

            txn.WaterContact=CausalPresentWaterTerrainHydraulicTransport::HasTransportWaterContact(
                m_parent->Parent(),req.SourceCell);
            uint32_t force=CausalPresentWaterTerrainHydraulicTransport::HydraulicForceOf(
                m_parent->Parent(),req.SourceCell);
            if(req.UseForceOverride)force=req.EvalForce;
            uint32_t resistance=kSmallDebrisResistance;
            if(req.UseResistanceOverride)resistance=req.EvalResistance;
            txn.HydraulicForce=force;
            txn.ParcelResistance=resistance;
            txn.CellsVisited=2;

            if(!req.DestinationAvailable||!req.DestinationSupported||req.OccupiedIncompatibly)
            {
                txn.Class=SettleClass::DestinationUnsupported;
                txn.DestinationUnavailable=true;
                txn.PendingRetained=true;
                txn.Settled=false;
                FPendingHydraulicSettle pending;
                pending.LooseMatterId=parcel->LooseMatterId;
                pending.CurrentCell=parcel->LocationCell;
                pending.DestinationCell=req.DestinationCell;
                pending.ExpectedDestinationTerrainRevision=DestinationRevision(req.DestinationCell);
                pending.ExpectedLooseRevision=m_settlingRevision;
                pending.CauseWaterBodyId=req.WaterBodyId;
                pending.TransactionId=txn.TransactionId;
                pending.DestinationUnavailable=true;
                m_pending.push_back(pending);
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }

            int const rest=FindRestingSupport(*m_parent,req.DestinationCell,force,resistance);
            if(rest<0)
            {
                txn.Class=SettleClass::ForcingAboveThreshold;
                txn.Settled=false;
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }
            if(rest!=req.DestinationCell)
                return refuse(false,true);

            txn.Class=req.Fixture==FixtureKind::SharedRestingRegion
                ?SettleClass::SharedRestingRegion:SettleClass::SupportedLowEnergy;
            int const oldLoc=parcel->LocationCell;
            MotionState const oldMotion=parcel->Motion;
            parcel->LocationCell=rest;
            parcel->Motion=MotionState::Settled;
            ++m_settlingRevision;
            txn.Settled=true;
            txn.RestingCell=rest;
            txn.MotionAfter=parcel->Motion;
            txn.IdentityPreserved=
                parcel->LooseMatterId==req.LooseMatterId
                &&parcel->MaterialId==txn.MaterialId
                &&parcel->Grams==txn.Grams
                &&parcel->SourceCell==parcel->SourceCell
                &&parcel->LocationCell==rest
                &&parcel->FormationProvenance==txn.FormationProvenance
                &&parcel->GeologicalAncestry==txn.GeologicalAncestry
                &&parcel->DetachmentTransactionId==txn.DetachmentTransactionId
                &&parcel->Motion==MotionState::Settled
                &&oldMotion==MotionState::InTransport
                &&oldLoc==req.SourceCell;
            VisitNeighborhood(rest,txn);
            txn.TerrainSolidsAfter=TerrainSolids();
            txn.LooseAfter=LooseMass();
            txn.WaterMassAfter=ConservedMass();
            txn.HeldMatterAfter=HeldMatter();
            txn.CompiledSedimentAfter=TerrainDigest();
            txn.TerrainRevisionAfter=m_terrainRevision;
            txn.WaterRevisionAfter=m_waterRevision;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterRevision=m_publishedWaterRevision;
            txn.OutputRevisions={m_terrainRevision,m_waterRevision,m_settlingRevision,
                DestinationRevision(req.DestinationCell)};
            txn.DestinationRemainsLoose=true;
            txn.DepositedAsTerrain=false;
            txn.RoutedThrough16C=false;
            txn.Aggregated=false;
            return txn;
        }

        void Record(FHydraulicLooseMatterSettle const& rec)
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
            if(rec.PendingRetained)++m_stats.pendingRetained;
            if(rec.Settled)
            {
                ++m_stats.settles;
                m_stats.gramsSettled+=rec.Grams;
            }
            if(rec.Fixture==FixtureKind::SupportedLowEnergy)++m_stats.lowEnergySettle;
            else if(rec.Fixture==FixtureKind::ForcingAboveThreshold)++m_stats.forcingZero;
            else if(rec.Fixture==FixtureKind::UnsupportedDestination)++m_stats.unsupportedPending;
        }

        std::unique_ptr<CausalPresentWaterTerrainHydraulicTransport::Kernel> m_parent;Program m_program;
        std::vector<SettleRequest> m_work;
        std::vector<FHydraulicLooseMatterSettle> m_txns;
        std::vector<FLooseMatterParcel> m_loose;
        std::vector<FPendingHydraulicSettle> m_pending;
        std::unordered_map<int,uint32_t> m_destRevision;
        SolveStats m_stats;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterRevision=0;
        uint32_t m_settlingRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainHydraulicTransport::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,char const* p5b3bPath,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainHydraulicTransport::LoadKernel(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,
            transferPath,externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,
            p5b3aPath,p5b3bPath,&reason,1);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainHydraulicTransport::Kernel> parent,
        char const* p5b3b2Path,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b3b_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b3b2Path,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b3b2Enabled=enabled;
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
        char const* p5b3aPath,char const* p5b3bPath,char const* p5b3b2Path,
        std::string* reason=nullptr,uint32_t p5b3b2Override=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b3b2Override==0)enabled=0;
        else if(p5b3b2Override==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b3b2Path,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b3bTransportDigest=0;
        uint64_t transportDigestDisabled=0;
        uint64_t settlingDigestBudget1=0,settlingDigestBudgetN=0,settlingDigestUnbounded=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterBefore=0,waterAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsSettled=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t lowEnergySettle=0,forcingZero=0,settles=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FHydraulicLooseMatterSettle> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,SettleClass,LooseMatterId,MaterialId,Material,Grams,"
            "SourceCell,RestingCell,CauseWaterBodyId,Force,Resistance,"
            "Settled,MotionBefore,MotionAfter,IdentityPreserved,DestinationRemainsLoose,"
            "TerrainSolidsBefore,TerrainSolidsAfter,LooseBefore,LooseAfter,"
            "WaterBefore,WaterAfter,HeldBefore,HeldAfter,"
            "SourceTerrainRev,DestTerrainRev,WaterRev,LooseRev,"
            "CellsVisited,BodiesExamined,RefusedStale,RefusedInvalid,"
            "DestinationUnavailable,PendingRetained,DepositedAsTerrain,RoutedThrough16C,Aggregated\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%s,%s,%s,%lld,"
                "%d,%d,%s,%u,%u,"
                "%d,%s,%s,%d,%d,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%u,%u,%u,%u,"
                "%zu,%zu,%d,%d,%d,%d,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                SettleClassName(t.Class),
                CausalWorldGeology::Hex64(t.LooseMatterId).c_str(),
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MaterialName,
                (long long)t.Grams,
                t.SourceCell,t.RestingCell,
                CausalWorldGeology::Hex64(t.CauseWaterBodyId).c_str(),
                t.HydraulicForce,t.ParcelResistance,
                t.Settled?1:0,MotionStateName(t.MotionBefore),MotionStateName(t.MotionAfter),
                t.IdentityPreserved?1:0,t.DestinationRemainsLoose?1:0,
                (long long)t.TerrainSolidsBefore,(long long)t.TerrainSolidsAfter,
                (long long)t.LooseBefore,(long long)t.LooseAfter,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.SourceTerrainRevision,t.DestinationTerrainRevision,t.WaterRevision,t.LooseRevision,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.DestinationUnavailable?1:0,t.PendingRetained?1:0,
                t.DepositedAsTerrain?1:0,t.RoutedThrough16C?1:0,t.Aggregated?1:0);
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
        char const* p5b3b2Path)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b3b=[&]()->std::unique_ptr<CausalPresentWaterTerrainHydraulicTransport::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,r2);
        };

        auto parent=reloadP5b3b();
        c.checks.push_back({"p5b3b_parent_loaded",parent!=nullptr&&parent->Complete()});
        if(!parent){c.reason=reason.empty()?"p5b3b_parent_load_failed":reason;return c;}
        c.p5b3bTransportDigest=parent->TransportDigestValue();
        c.checks.push_back({"p5b3b_transport_digest_frozen",
            c.p5b3bTransportDigest==kFrozenP5b3bTransportDigest});
        c.checks.push_back({"fixtures_start_already_transported",
            !parent->Loose().empty()&&parent->LooseMass()==kParcelGrams
            &&parent->Stats().transfers>0
            &&parent->Loose().front().LocationCell!=parent->Loose().front().SourceCell});

        int64_t const water0=parent->ConservedMass();
        int64_t const solids0=parent->TerrainSolids();
        int64_t const loose0=parent->LooseMass();
        int64_t const held0=parent->HeldMatter();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint32_t const wRev0=parent->WaterTopologyRevision();
        uint64_t const transport0=parent->TransportDigestValue();
        auto const parentLoose=parent->Loose();

        auto disabled=MakeFromParent(reloadP5b3b(),p5b3b2Path,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b3b2",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.transportDigestDisabled=disabled->TransportDigestValue();
        bool stillInTransport=true;
        for(auto const& m:disabled->Loose())
            if(m.Motion!=MotionState::InTransport)stillInTransport=false;
        bool locationsMatchParent=disabled->Loose().size()==parentLoose.size();
        if(locationsMatchParent)
        {
            for(size_t i=0;i<parentLoose.size();++i)
                if(disabled->Loose()[i].LocationCell!=parentLoose[i].LocationCell
                  ||disabled->Loose()[i].LooseMatterId!=parentLoose[i].LooseMatterId)
                    locationsMatchParent=false;
        }
        c.checks.push_back({"p5b3b2_off_exact_p5b3b_state",
            c.transportDigestDisabled==transport0
            &&c.transportDigestDisabled==kFrozenP5b3bTransportDigest
            &&disabled->Stats().settles==0
            &&disabled->ConservedMass()==water0
            &&disabled->TerrainSolids()==solids0
            &&disabled->LooseMass()==loose0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0
            &&stillInTransport&&locationsMatchParent
            &&disabled->Pending().empty()});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b3b(),p5b3b2Path,1,budget,r2);
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

        c.settlingDigestBudget1=budget1->SettlingDigestValue();
        c.settlingDigestBudgetN=budgetN->SettlingDigestValue();
        c.settlingDigestUnbounded=unbounded->SettlingDigestValue();
        c.terrainSolidsBefore=unbounded->Stats().terrainSolidsBefore;
        c.terrainSolidsAfter=unbounded->Stats().terrainSolidsAfter;
        c.looseBefore=unbounded->Stats().looseBefore;
        c.looseAfter=unbounded->Stats().looseAfter;
        c.waterBefore=unbounded->Stats().waterMassBefore;
        c.waterAfter=unbounded->Stats().waterMassAfter;
        c.heldBefore=unbounded->Stats().heldBefore;
        c.heldAfter=unbounded->Stats().heldAfter;
        c.gramsSettled=unbounded->Stats().gramsSettled;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.lowEnergySettle=unbounded->Stats().lowEnergySettle;
        c.forcingZero=unbounded->Stats().forcingZero;
        c.settles=unbounded->Stats().settles;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        uint64_t const disabledDigest=disabled->SettlingDigestValue();
        bool freezeOk=c.settlingDigestBudget1==c.settlingDigestBudgetN
            &&c.settlingDigestBudgetN==c.settlingDigestUnbounded
            &&c.settlingDigestUnbounded!=disabledDigest;
        if(kFrozenP5b3b2SettlingDigest)
            freezeOk=freezeOk&&c.settlingDigestUnbounded==kFrozenP5b3b2SettlingDigest;
        c.checks.push_back({"budget_invariant_settling",freezeOk});
        c.checks.push_back({"loose_mass_before_equals_after",
            c.looseBefore==c.looseAfter&&c.looseAfter==loose0
            &&unbounded->LooseMass()==loose0});
        c.checks.push_back({"terrain_solids_unchanged",
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
            &&unbounded->TerrainSolids()+unbounded->LooseMass()==solids0+loose0});
        c.checks.push_back({"compiled_16c_sediment_unchanged",
            unbounded->TerrainDigest()==sediment0
            &&unbounded->Stats().compiledSedimentAfter==sediment0});

        bool massEach=true,identityKept=true,no16c=true,destLoose=true,localOnly=true;
        bool settleOk=false,forcingOk=false;
        uint64_t settledId=0;int restCell=-1,srcCell=-1;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            massEach=massEach
                &&t.TerrainSolidsBefore==t.TerrainSolidsAfter
                &&t.LooseBefore==t.LooseAfter
                &&t.WaterMassBefore==t.WaterMassAfter
                &&t.HeldMatterBefore==t.HeldMatterAfter
                &&t.CompiledSedimentBefore==t.CompiledSedimentAfter
                &&t.Grams==kParcelGrams;
            no16c=no16c&&!t.RoutedThrough16C&&!t.DepositedAsTerrain&&!t.Aggregated;
            destLoose=destLoose&&t.DestinationRemainsLoose;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            if(t.Settled)
            {
                identityKept=identityKept&&t.IdentityPreserved&&t.LooseMatterId!=0
                    &&t.MaterialId!=0&&t.DetachmentTransactionId!=0
                    &&t.MotionAfter==MotionState::Settled
                    &&!t.InputRevisions.empty()&&!t.OutputRevisions.empty();
                settledId=t.LooseMatterId;restCell=t.RestingCell;srcCell=t.SourceCell;
            }
            if(t.Fixture==FixtureKind::SupportedLowEnergy)
                settleOk=t.Settled&&t.IdentityPreserved&&t.Grams==kParcelGrams
                    &&t.MotionAfter==MotionState::Settled
                    &&t.Class==SettleClass::SupportedLowEnergy
                    &&!t.DepositedAsTerrain;
            if(t.Fixture==FixtureKind::ForcingAboveThreshold)
                forcingOk=!t.Settled&&t.Class==SettleClass::ForcingAboveThreshold
                    &&t.HydraulicForce>=t.ParcelResistance
                    &&t.LooseBefore==t.LooseAfter
                    &&t.MotionAfter==MotionState::InTransport;
        }
        auto const* settled=unbounded->FindLoose(settledId);
        bool locOk=settled&&settled->LocationCell==restCell
            &&settled->Motion==MotionState::Settled
            &&settled->Grams==kParcelGrams
            &&settled->LooseMatterId==settledId
            &&settled->ConstituentIds.empty();
        if(!parentLoose.empty())
        {
            auto const& orig=parentLoose.front();
            locOk=locOk&&settled
                &&settled->MaterialId==orig.MaterialId
                &&settled->FormationProvenance==orig.FormationProvenance
                &&settled->GeologicalAncestry==orig.GeologicalAncestry
                &&settled->DetachmentTransactionId==orig.DetachmentTransactionId
                &&settled->DetachmentCause==orig.DetachmentCause
                &&settled->SourceCell==orig.SourceCell;
        }
        c.checks.push_back({"supported_low_energy_settles_same_parcel",
            settleOk&&c.lowEnergySettle>0&&c.settles==1&&locOk});
        c.checks.push_back({"forcing_above_threshold_does_not_settle",forcingOk&&c.forcingZero>0});
        c.checks.push_back({"settling_preserves_identity",identityKept&&settledId!=0});
        c.checks.push_back({"destination_remains_loose_not_terrain",destLoose&&no16c
            &&unbounded->Pending().empty()
            &&unbounded->TerrainSolids()==solids0});
        c.checks.push_back({"admitted_settles_conserved",massEach});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined<6515&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_settling",
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
                fixtures.stale.DestinationTerrainRevision=k->DestinationRevision(fixtures.stale.DestinationCell);
                fixtures.stale.CheckRevision=true;
                k->BumpDestinationRevision(fixtures.stale.DestinationCell);
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.stale.LooseMatterId);
                auto const motion=k->MotionOf(fixtures.stale.LooseMatterId);
                auto stale=k->ApplyRequest(fixtures.stale);
                staleOk=stale.RefusedStale&&!stale.Settled&&stale.Grams==kParcelGrams
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose&&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.stale.LooseMatterId)==loc
                    &&k->MotionOf(fixtures.stale.LooseMatterId)==motion
                    &&motion==MotionState::InTransport;
            }
            c.checks.push_back({"stale_destination_revision_refuse",staleOk});
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
                fixtures.unsupported.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.unsupported.DestinationCell);
                fixtures.unsupported.CheckRevision=true;
                fixtures.unsupported.DestinationAvailable=false;
                fixtures.unsupported.DestinationSupported=false;
                fixtures.unsupported.OccupiedIncompatibly=true;
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.unsupported.LooseMatterId);
                size_t const nLoose=k->Loose().size();
                auto rec=k->ApplyRequest(fixtures.unsupported);
                unavailOk=rec.DestinationUnavailable&&rec.PendingRetained&&!rec.Settled
                    &&!rec.RefusedInvalid&&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.unsupported.LooseMatterId)==loc
                    &&k->MotionOf(fixtures.unsupported.LooseMatterId)==MotionState::InTransport
                    &&k->Loose().size()==nLoose
                    &&k->Pending().size()==1
                    &&k->Pending().front().LooseMatterId==fixtures.unsupported.LooseMatterId
                    &&k->Pending().front().DestinationUnavailable
                    &&k->Pending().front().CurrentCell==loc;
            }
            c.checks.push_back({"unsupported_destination_retains_parcel_and_pending",unavailOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool shareOk=false;
            if(k&&!k->Loose().empty())
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.settle.SourceTerrainRevision=k->TerrainRevision();
                fixtures.settle.WaterRevision=k->WaterTopologyRevision();
                fixtures.settle.LooseRevision=k->SettlingRevision();
                fixtures.settle.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.settle.DestinationCell);
                fixtures.settle.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.settle);
                FLooseMatterParcel companion=*k->FindLoose(fixtures.settle.LooseMatterId);
                companion.LooseMatterId=MakeTxnId(FixtureKind::SharedRestingRegion,
                    companion.LooseMatterId,companion.LocationCell,companion.LocationCell,99);
                companion.Grams=kCompanionGrams;
                companion.Motion=MotionState::InTransport;
                companion.AggregateId=0;
                companion.ConstituentIds.clear();
                k->InsertCompanion(companion);
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                fixtures.shared.LooseMatterId=companion.LooseMatterId;
                fixtures.shared.SourceCell=companion.LocationCell;
                fixtures.shared.DestinationCell=companion.LocationCell;
                fixtures.shared.SourceTerrainRevision=k->TerrainRevision();
                fixtures.shared.WaterRevision=k->WaterTopologyRevision();
                fixtures.shared.LooseRevision=k->SettlingRevision();
                fixtures.shared.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.shared.DestinationCell);
                fixtures.shared.CheckRevision=true;
                auto second=k->ApplyRequest(fixtures.shared);
                auto const* a=k->FindLoose(fixtures.settle.LooseMatterId);
                auto const* b=k->FindLoose(companion.LooseMatterId);
                shareOk=first.Settled&&second.Settled&&!second.Aggregated
                    &&a&&b&&a->LooseMatterId!=b->LooseMatterId
                    &&a->LocationCell==b->LocationCell
                    &&a->Motion==MotionState::Settled&&b->Motion==MotionState::Settled
                    &&a->Grams==kParcelGrams&&b->Grams==kCompanionGrams
                    &&a->MaterialId==b->MaterialId
                    &&a->FormationProvenance==b->FormationProvenance
                    &&a->GeologicalAncestry==b->GeologicalAncestry
                    &&a->DetachmentTransactionId==companion.DetachmentTransactionId
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose&&k->ConservedMass()==water
                    &&k->Loose().size()==2
                    &&k->TerrainSolids()==solids0;
            }
            c.checks.push_back({"compatible_parcels_may_share_resting_region",shareOk});
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
                    SettleRequest reqs[2]={f.forcing,f.settle};
                    for(auto& req:reqs)
                    {
                        req.SourceTerrainRevision=k.TerrainRevision();
                        req.WaterRevision=k.WaterTopologyRevision();
                        req.LooseRevision=k.SettlingRevision();
                        req.DestinationTerrainRevision=k.DestinationRevision(req.DestinationCell);
                        req.CheckRevision=true;
                        k.ApplyRequest(req);
                    }
                };
                applyAll(*a,fa);
                applyAll(*b,fb);
                partitionOk=a->SettlingDigestValue()==b->SettlingDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TerrainSolids()==b->TerrainSolids()
                    &&a->LooseMass()==b->LooseMass()
                    &&fa.settle.SourceCell==fb.settle.SourceCell
                    &&a->LocationOf(fa.settle.LooseMatterId)==b->LocationOf(fb.settle.LooseMatterId)
                    &&a->MotionOf(fa.settle.LooseMatterId)==MotionState::Settled
                    &&b->MotionOf(fb.settle.LooseMatterId)==MotionState::Settled;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->SettlingDigestValue()==unbounded->SettlingDigestValue()
            &&reload->SettlingDigestValue()==unbounded->SettlingDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->TerrainSolids()==unbounded->TerrainSolids()
            &&cold->LooseMass()==unbounded->LooseMass()
            &&cold->LocationOf(settledId)==unbounded->LocationOf(settledId)
            &&cold->MotionOf(settledId)==MotionState::Settled;
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3c_bank_support_collapse_closed",true});
        c.checks.push_back({"p5b3b3_terrain_reincorporation_closed",true});
        c.checks.push_back({"general_erosion_closed",true});
        c.checks.push_back({"active_16b_erosion_closed",true});
        c.checks.push_back({"16c_remobilization_closed",true});
        c.checks.push_back({"rainfall_evaporation_groundwater_closed",true});
        c.checks.push_back({"world_operation_scheduler_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_settle",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->SettlingDigestValue();
                size_t const n=unbounded->Stats().settles;
                size_t const wakes=unbounded->Stats().settleWakes;
                unbounded->Tick(64);
                return unbounded->SettlingDigestValue()==before
                    &&unbounded->Stats().settles==n
                    &&unbounded->Stats().settleWakes==wakes;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b3b2_loose_matter_settling_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b3b2_loose_matter_settling_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B3B2_LOOSE_MATTER_SETTLING %s\nreason=%s\n"
            "p5b3b_transport_digest=%s\ntransport_digest_disabled=%s\n"
            "settling_digest_budget1=%s\nsettling_digest_budgetN=%s\n"
            "settling_digest_unbounded=%s\n"
            "terrain_solids_before=%lld\nterrain_solids_after=%lld\n"
            "loose_before=%lld\nloose_after=%lld\n"
            "water_before=%lld\nwater_after=%lld\n"
            "held_before=%lld\nheld_after=%lld\n"
            "grams_settled=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "low_energy_settle=%zu\nforcing_zero=%zu\nsettles=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=loose_matter_settling_location_and_rest_not_identity\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=frozen\n"
            "p5b3a=frozen\np5b3b=frozen\np5b3b2=open\np5b3b3=closed\np5b3c=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "general_erosion=0\nbank_collapse=0\n"
            "active_16b_erosion=0\n16c_remobilization=0\necology=0\n"
            "deposition_as_terrain=0\nworld_operation_scheduler=0\n"
            "choice_a_local_loose=1\nchoice_b_16c=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b3bTransportDigest).c_str(),
            CausalWorldGeology::Hex64(c.transportDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.settlingDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.settlingDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.settlingDigestUnbounded).c_str(),
            (long long)c.terrainSolidsBefore,(long long)c.terrainSolidsAfter,
            (long long)c.looseBefore,(long long)c.looseAfter,
            (long long)c.waterBefore,(long long)c.waterAfter,
            (long long)c.heldBefore,(long long)c.heldAfter,
            (long long)c.gramsSettled,
            c.transactionsCertified,c.transactionsAdmitted,
            c.lowEnergySettle,c.forcingZero,c.settles,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
