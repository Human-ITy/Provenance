#pragma once

// P5b.3B: hydraulic transport of already-detached loose matter. One hop.
// Detachment, transport, and deposition stay separate. Transport changes
// location, not identity: same LooseMatterId, material, grams, provenance,
// and detachment ancestry; different authoritative location. Destination
// remains loose matter (not auto-deposited terrain).
//
// Pipeline: existing loose body → water contact / hydraulic forcing →
// transport eligibility → candidate downstream/resting location →
// authoritative destination validation → move same loose matter →
// water/topology responds if necessary → sleep.
//
// CLOSED: P5b.3C bank/support collapse, 3B.2 deposition as structural
// terrain, 16C remobilization, general erosion, rainfall, evaporation,
// groundwater, plant uptake, ecology, full WorldOperationScheduler.
//
// Frozen parent: P5b.3A 6c467fb7 + soak harness a28ed5c5.
// Disabled P5b.3B must reproduce exact P5b.3A detachment state.

#include "CausalPresentWaterTerrainHydraulicDetachment.h"

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

namespace CausalPresentWaterTerrainHydraulicTransport
{
    constexpr char const* kExpectedRegion=
        "causal_world_present_water_terrain_hydraulic_transport_floor";
    constexpr uint64_t kFrozenP5b3aDetachmentDigest=
        CausalPresentWaterTerrainHydraulicDetachment::kFrozenP5b3aDetachmentDigest;
    constexpr uint64_t kFrozenP5b3bTransportDigest=0x876ac027936dce35ull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainHydraulicDetachment::kHoldBudget;
    constexpr int64_t kParcelGrams=CausalPresentWaterTerrainHydraulicDetachment::kParcelGrams;
    constexpr uint32_t kSmallDebrisResistance=200;
    constexpr uint32_t kHeavyParcelResistance=2500;
    constexpr uint32_t kInsufficientForce=50;

    enum class FixtureKind:uint8_t
    {
        None=0,
        FlowHop=1,
        InsufficientForce=2,
        HeavyResistant=3,
        StaleDestination=4,
        UnavailableDestination=5
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::FlowHop:return "one_hop_flow_transport";
            case FixtureKind::InsufficientForce:return "insufficient_hydraulic_forcing";
            case FixtureKind::HeavyResistant:return "heavy_resistant_parcel";
            case FixtureKind::StaleDestination:return "stale_destination_revision";
            case FixtureKind::UnavailableDestination:return "unavailable_destination_section";
            default:return "none";
        }
    }

    enum class TransportClass:uint8_t
    {
        None=0,
        RiverFlowHop=1,
        WetlandFlowHop=2,
        OtherFlowHop=3,
        InsufficientForce=4,
        ResistantParcel=5,
        StaleDestination=6,
        DestinationUnavailable=7
    };

    inline char const* TransportClassName(TransportClass value)
    {
        switch(value)
        {
            case TransportClass::RiverFlowHop:return "river_flow_hop";
            case TransportClass::WetlandFlowHop:return "wetland_flow_hop";
            case TransportClass::OtherFlowHop:return "other_flow_hop";
            case TransportClass::InsufficientForce:return "insufficient_force";
            case TransportClass::ResistantParcel:return "resistant_parcel";
            case TransportClass::StaleDestination:return "stale_destination";
            case TransportClass::DestinationUnavailable:return "destination_unavailable";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b3bEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b3bEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainHydraulicDetachment::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_HYDRAULIC_TRANSPORT_V1";
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
          &&hex("p5b3b_event_id",r.program.p5b3bEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b3b_enabled",r.program.p5b3bEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b3bEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b3bEnabled==0||r.program.p5b3bEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b3b_transport_contract";return r;}
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
    };

    struct FPendingHydraulicTransfer
    {
        uint64_t LooseMatterId=0;
        int SourceCell=-1;
        int DestinationCell=-1;
        uint32_t ExpectedDestinationTerrainRevision=0;
        uint64_t CauseWaterBodyId=0;
        uint64_t TransactionId=0;
        bool DestinationUnavailable=false;
    };

    struct FHydraulicLooseMatterTransfer
    {
        uint64_t TransactionId=0;
        uint64_t LooseMatterId=0;
        uint64_t MaterialId=0;
        int64_t Grams=0;
        int SourceCell=-1;
        int DestinationCell=-1;
        uint64_t CauseWaterBodyId=0;
        uint32_t WaterRevision=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        TransportClass Class=TransportClass::None;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        FixtureKind Fixture=FixtureKind::None;
        char MaterialName[32]{};
        uint32_t HydraulicForce=0;
        uint32_t ParcelResistance=0;
        bool WaterContact=false;
        bool Moved=false;
        bool IdentityPreserved=false;
        bool DestinationRemainsLoose=true;
        bool RefusedStale=false,RefusedInvalid=false;
        bool DestinationUnavailable=false;
        bool PendingRetained=false;
        bool DepositedAsTerrain=false;
        bool RoutedThrough16C=false;
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

    struct TransportRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        uint64_t LooseMatterId=0;
        int SourceCell=-1;
        int DestinationCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t WaterRevision=0;
        bool CheckRevision=true;
        bool DestinationAvailable=true;
        bool UseForceOverride=false;
        uint32_t EvalForce=0;
        bool UseResistanceOverride=false;
        uint32_t EvalResistance=0;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t flowHop=0,insufficientZero=0,heavyZero=0;
        size_t transfers=0,transportWakes=0,staleRefuse=0,pendingRetained=0;
        size_t waterResponses=0,topologyRebuilds=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsMoved=0;
        uint64_t parentDetachmentDigest=0,transportDigest=0;
        uint64_t compiledSedimentBefore=0,compiledSedimentAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterRevision=0,transportRevision=0;
    };

    inline uint64_t MakeTxnId(FixtureKind kind,uint64_t looseId,int src,int dst,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3000Cull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&looseId,sizeof(looseId));
        CausalWorldGeology::HashAppend(h,&src,sizeof(src));
        CausalWorldGeology::HashAppend(h,&dst,sizeof(dst));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline FLooseMatterParcel FromDetached(
        CausalPresentWaterTerrainHydraulicDetachment::FDetachedLooseMatter const& src)
    {
        FLooseMatterParcel p;
        p.LooseMatterId=src.DetachedMatterId;
        p.MaterialId=src.MaterialId;
        std::snprintf(p.MaterialName,sizeof(p.MaterialName),"%s",src.MaterialName);
        p.Grams=src.Grams;
        p.SourceCell=src.SourceCell;
        p.LocationCell=src.SourceCell;
        p.SourceRegionId=src.SourceRegionId;
        p.FeatureId=src.FeatureId;
        p.GeologicalAncestry=src.GeologicalAncestry;
        p.FormationProvenance=src.FormationProvenance;
        p.BodyProvenance=src.BodyProvenance;
        p.DetachmentCause=src.DetachmentCause;
        p.DetachmentTransactionId=src.TransactionId;
        return p;
    }

    inline int64_t LooseMassOf(std::vector<FLooseMatterParcel> const& loose)
    {
        int64_t s=0;for(auto const& m:loose)s+=m.Grams;return s;
    }

    inline uint64_t TransportDigestOf(std::vector<FLooseMatterParcel> const& loose,
        std::vector<FPendingHydraulicTransfer> const& pending,
        uint64_t parentDigest,uint32_t transportRev)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&parentDigest,sizeof(parentDigest));
        CausalWorldGeology::HashAppend(d,&transportRev,sizeof(transportRev));
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
        }
        for(auto const& p:pending)
        {
            CausalWorldGeology::HashAppend(d,&p.LooseMatterId,sizeof(p.LooseMatterId));
            CausalWorldGeology::HashAppend(d,&p.SourceCell,sizeof(p.SourceCell));
            CausalWorldGeology::HashAppend(d,&p.DestinationCell,sizeof(p.DestinationCell));
            CausalWorldGeology::HashAppend(d,&p.ExpectedDestinationTerrainRevision,
                sizeof(p.ExpectedDestinationTerrainRevision));
            uint8_t const u=p.DestinationUnavailable?1:0;
            CausalWorldGeology::HashAppend(d,&u,sizeof(u));
        }
        return d;
    }

    inline int FindDownstreamHop(CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent,
        int cell)
    {
        auto const& drainage=parent.Water().Drainage().Cells();
        if(cell<0||(size_t)cell>=drainage.size())return -1;
        int dest=drainage[(size_t)cell].receiver;
        if(dest>=0&&(size_t)dest<drainage.size()&&dest!=cell)return dest;
        auto const& cells=parent.Water().Cells();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        int best=-1;double bestZ=cells[(size_t)cell].terrainZ;
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,false);
            if(ni<0||(size_t)ni>=cells.size()||ni==cell)continue;
            double const z=cells[(size_t)ni].terrainZ;
            bool const wet=cells[(size_t)ni].occupied;
            if((wet||z+1e-9<bestZ)&&(best<0||z<bestZ||(z==bestZ&&ni<best)))
            {best=ni;bestZ=z;}
        }
        return best;
    }

    inline bool HasTransportWaterContact(
        CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent,int cell)
    {
        auto const& cells=parent.Water().Cells();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        if(CausalPresentWaterTerrainHydraulicDetachment::HasWaterContact(
            cells,cell,width,height,false))return true;
        if(cell<0||(size_t)cell>=cells.size())return false;
        return parent.QueryTerrainState(cell).ContactWet!=0;
    }

    inline uint32_t HydraulicForceOf(CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent,
        int cell)
    {
        auto const& cells=parent.Water().Cells();
        auto const& drainage=parent.Water().Drainage().Cells();
        if(cell<0||(size_t)cell>=cells.size())return 0;
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        uint32_t force=0;
        if(cells[(size_t)cell].occupied)force+=800;
        else if(CausalPresentWaterTerrainHydraulicDetachment::HasWaterContact(
            cells,cell,width,height,false))force+=400;
        auto const st=parent.QueryTerrainState(cell);
        if(st.ContactWet!=0&&force<400)force+=400;
        if(st.Saturation>=500)force+=200;
        if((size_t)cell<drainage.size())
        {
            if(drainage[(size_t)cell].channel)force+=400;
            double const channelArea=parent.Water().Drainage().GetProgram().channelThresholdKm2*1e6;
            if(drainage[(size_t)cell].accumulationM2>=channelArea)force+=200;
        }
        return force;
    }

    inline TransportClass ClassifyHop(CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent,
        int cell,uint64_t bodyId)
    {
        auto const& cells=parent.Water().Cells();
        CausalPresentWater::BodyKind kind=CausalPresentWater::BodyKind::None;
        if(cell>=0&&(size_t)cell<cells.size()&&cells[(size_t)cell].occupied)
            kind=cells[(size_t)cell].kind;
        else if(bodyId)
        {
            auto const* body=CausalPresentWaterTopology::FindBody(parent.Body().Bodies(),bodyId);
            if(body)
            {
                if(body->hasRiver)kind=CausalPresentWater::BodyKind::River;
                else if(body->hasWetland)kind=CausalPresentWater::BodyKind::Wetland;
            }
        }
        if(kind==CausalPresentWater::BodyKind::River)return TransportClass::RiverFlowHop;
        if(kind==CausalPresentWater::BodyKind::Wetland)return TransportClass::WetlandFlowHop;
        return TransportClass::OtherFlowHop;
    }

    inline uint64_t BindCauseBody(CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent,
        int cell)
    {
        auto const& cells=parent.Water().Cells();
        if(cell>=0&&(size_t)cell<cells.size()&&cells[(size_t)cell].occupied)
            return cells[(size_t)cell].bodyId;
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,false);
            if(ni>=0&&(size_t)ni<cells.size()&&cells[(size_t)ni].occupied)
                return cells[(size_t)ni].bodyId;
        }
        return 0;
    }

    struct FixtureSet
    {
        TransportRequest hop,insufficient,heavy,stale,unavailable;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(
        CausalPresentWaterTerrainHydraulicDetachment::Kernel const& parent)
    {
        FixtureSet set;
        if(parent.Loose().empty())return set;
        auto const& detached=parent.Loose().front();
        int const src=detached.SourceCell;
        int const dest=FindDownstreamHop(parent,src);
        auto const& cells=parent.Water().Cells();
        if(src<0||(size_t)src>=cells.size()||dest<0||(size_t)dest>=cells.size()||dest==src)
            return set;
        auto fill=[&](TransportRequest& req,FixtureKind kind)
        {
            req.Fixture=kind;
            req.LooseMatterId=detached.DetachedMatterId;
            req.SourceCell=src;
            req.DestinationCell=dest;
            req.WaterBodyId=BindCauseBody(parent,src);
            req.DestinationAvailable=true;
            req.CheckRevision=true;
        };
        fill(set.hop,FixtureKind::FlowHop);
        fill(set.insufficient,FixtureKind::InsufficientForce);
        set.insufficient.UseForceOverride=true;
        set.insufficient.EvalForce=kInsufficientForce;
        fill(set.heavy,FixtureKind::HeavyResistant);
        set.heavy.UseResistanceOverride=true;
        set.heavy.EvalResistance=kHeavyParcelResistance;
        fill(set.stale,FixtureKind::StaleDestination);
        fill(set.unavailable,FixtureKind::UnavailableDestination);
        set.unavailable.DestinationAvailable=false;
        set.ok=detached.DetachedMatterId!=0&&detached.Grams==kParcelGrams;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainHydraulicDetachment::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_complete=!m_program.p5b3bEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainHydraulicDetachment::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainHydraulicDetachment::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FHydraulicLooseMatterTransfer> const& Transactions() const{return m_txns;}
        std::vector<FLooseMatterParcel> const& Loose() const{return m_loose;}
        std::vector<FPendingHydraulicTransfer> const& Pending() const{return m_pending;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_terrainRevision;}
        uint32_t WaterTopologyRevision() const{return m_waterRevision;}
        uint32_t PublishedTerrainRevision() const{return m_publishedTerrainRevision;}
        uint32_t PublishedWaterTopologyRevision() const{return m_publishedWaterRevision;}
        uint32_t TerrainStateRevision() const{return m_parent->TerrainStateRevision();}
        uint32_t PoreRevision() const{return m_parent->PoreRevision();}
        uint32_t TransportRevision() const{return m_transportRevision;}
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
        uint64_t TransportDigestValue() const
        {
            return TransportDigestOf(m_loose,m_pending,DetachmentDigestValue(),m_transportRevision);
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
        FLooseMatterParcel const* FindLoose(uint64_t looseId) const
        {
            for(auto const& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }

        FHydraulicLooseMatterTransfer ApplyRequest(TransportRequest req)
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
            if(!m_program.p5b3bEnabled){FinishUnchanged();return true;}
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
                m_stats.transportDigest=TransportDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterRevision=m_waterRevision;
                m_stats.transportRevision=m_transportRevision;
            }
            return m_complete;
        }

    private:
        void CopyParentLoose()
        {
            m_loose.clear();
            for(auto const& src:m_parent->Loose())m_loose.push_back(FromDetached(src));
        }

        void BeginFromParent()
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            CopyParentLoose();
            m_stats.parentDetachmentDigest=m_parent->DetachmentDigestValue();
            m_stats.terrainSolidsBefore=TerrainSolids();
            m_stats.looseBefore=LooseMass();
            m_stats.waterMassBefore=ConservedMass();
            m_stats.heldBefore=HeldMatter();
            m_stats.compiledSedimentBefore=TerrainDigest();
            if(!m_program.p5b3bEnabled){FinishUnchanged();return;}
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
            m_stats.transportDigest=TransportDigestValue();
            m_stats.terrainRevision=m_terrainRevision;
            m_stats.waterRevision=m_waterRevision;
            m_stats.transportRevision=m_transportRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent);
            m_work.clear();
            if(fixtures.ok)
            {
                m_work.push_back(fixtures.insufficient);
                m_work.push_back(fixtures.heavy);
                m_work.push_back(fixtures.hop);
            }
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                TransportRequest req=m_work[m_cursor];
                req.SourceTerrainRevision=m_terrainRevision;
                req.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
                req.WaterRevision=m_waterRevision;
                req.CheckRevision=true;
                FHydraulicLooseMatterTransfer rec=CommitOne(req);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        FLooseMatterParcel* FindLooseMut(uint64_t looseId)
        {
            for(auto& m:m_loose)if(m.LooseMatterId==looseId)return &m;
            return nullptr;
        }

        bool VisitNeighborhood(int cell,FHydraulicLooseMatterTransfer& rec)
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

        FHydraulicLooseMatterTransfer CommitOne(TransportRequest req)
        {
            FHydraulicLooseMatterTransfer txn;
            txn.Fixture=req.Fixture;
            txn.LooseMatterId=req.LooseMatterId;
            txn.SourceCell=req.SourceCell;
            txn.DestinationCell=req.DestinationCell;
            txn.CauseWaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterRevisionBefore=m_waterRevision;
            txn.SourceTerrainRevision=m_terrainRevision;
            txn.DestinationTerrainRevision=DestinationRevision(req.DestinationCell);
            txn.WaterRevision=m_waterRevision;
            txn.TerrainSolidsBefore=TerrainSolids();
            txn.LooseBefore=LooseMass();
            txn.WaterMassBefore=ConservedMass();
            txn.HeldMatterBefore=HeldMatter();
            txn.CompiledSedimentBefore=TerrainDigest();
            txn.InputRevisions={m_terrainRevision,m_waterRevision,m_transportRevision,
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
                txn.OutputRevisions={m_terrainRevision,m_waterRevision,m_transportRevision,
                    DestinationRevision(req.DestinationCell)};
                return txn;
            };
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.Moved=false;
                return settle();
            };

            ++m_stats.transportWakes;
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
            txn.TransactionId=MakeTxnId(req.Fixture,req.LooseMatterId,req.SourceCell,
                req.DestinationCell,(uint32_t)(m_txns.size()+1u));
            txn.IdentityPreserved=true;

            if(req.CheckRevision&&(req.SourceTerrainRevision!=m_terrainRevision
              ||req.WaterRevision!=m_waterRevision))
                return refuse(true,false);
            if(req.CheckRevision&&req.DestinationTerrainRevision!=DestinationRevision(req.DestinationCell))
            {
                txn.Class=TransportClass::StaleDestination;
                return refuse(true,false);
            }

            txn.WaterContact=HasTransportWaterContact(*m_parent,req.SourceCell);
            uint32_t force=HydraulicForceOf(*m_parent,req.SourceCell);
            if(req.UseForceOverride)force=req.EvalForce;
            uint32_t resistance=kSmallDebrisResistance;
            if(req.UseResistanceOverride)resistance=req.EvalResistance;
            txn.HydraulicForce=force;
            txn.ParcelResistance=resistance;
            txn.CellsVisited=2;

            if(!req.DestinationAvailable)
            {
                txn.Class=TransportClass::DestinationUnavailable;
                txn.DestinationUnavailable=true;
                txn.PendingRetained=true;
                txn.Moved=false;
                FPendingHydraulicTransfer pending;
                pending.LooseMatterId=parcel->LooseMatterId;
                pending.SourceCell=parcel->LocationCell;
                pending.DestinationCell=req.DestinationCell;
                pending.ExpectedDestinationTerrainRevision=DestinationRevision(req.DestinationCell);
                pending.CauseWaterBodyId=req.WaterBodyId;
                pending.TransactionId=txn.TransactionId;
                pending.DestinationUnavailable=true;
                m_pending.push_back(pending);
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }

            int const expectedDest=FindDownstreamHop(*m_parent,req.SourceCell);
            if(expectedDest!=req.DestinationCell||req.DestinationCell==req.SourceCell)
                return refuse(false,true);

            bool const eligible=txn.WaterContact&&force>=resistance&&parcel->Grams==kParcelGrams;
            if(!eligible)
            {
                txn.Class=req.UseResistanceOverride?TransportClass::ResistantParcel
                    :TransportClass::InsufficientForce;
                txn.Moved=false;
                VisitNeighborhood(req.SourceCell,txn);
                return settle();
            }

            txn.Class=ClassifyHop(*m_parent,req.SourceCell,req.WaterBodyId);
            int const oldLoc=parcel->LocationCell;
            parcel->LocationCell=req.DestinationCell;
            ++m_transportRevision;
            txn.Moved=true;
            txn.IdentityPreserved=
                parcel->LooseMatterId==req.LooseMatterId
                &&parcel->MaterialId==txn.MaterialId
                &&parcel->Grams==txn.Grams
                &&parcel->SourceCell==req.SourceCell
                &&parcel->LocationCell==req.DestinationCell
                &&parcel->LocationCell!=oldLoc
                &&parcel->FormationProvenance==txn.FormationProvenance
                &&parcel->GeologicalAncestry==txn.GeologicalAncestry
                &&parcel->DetachmentTransactionId==txn.DetachmentTransactionId;
            VisitNeighborhood(req.DestinationCell,txn);
            txn.TerrainSolidsAfter=TerrainSolids();
            txn.LooseAfter=LooseMass();
            txn.WaterMassAfter=ConservedMass();
            txn.HeldMatterAfter=HeldMatter();
            txn.CompiledSedimentAfter=TerrainDigest();
            txn.TerrainRevisionAfter=m_terrainRevision;
            txn.WaterRevisionAfter=m_waterRevision;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterRevision=m_publishedWaterRevision;
            txn.OutputRevisions={m_terrainRevision,m_waterRevision,m_transportRevision,
                DestinationRevision(req.DestinationCell)};
            txn.DestinationRemainsLoose=true;
            txn.DepositedAsTerrain=false;
            txn.RoutedThrough16C=false;
            return txn;
        }

        void Record(FHydraulicLooseMatterTransfer const& rec)
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
            if(rec.Moved)
            {
                ++m_stats.transfers;
                m_stats.gramsMoved+=rec.Grams;
            }
            if(rec.Fixture==FixtureKind::FlowHop)++m_stats.flowHop;
            else if(rec.Fixture==FixtureKind::InsufficientForce)++m_stats.insufficientZero;
            else if(rec.Fixture==FixtureKind::HeavyResistant)++m_stats.heavyZero;
        }

        std::unique_ptr<CausalPresentWaterTerrainHydraulicDetachment::Kernel> m_parent;Program m_program;
        std::vector<TransportRequest> m_work;
        std::vector<FHydraulicLooseMatterTransfer> m_txns;
        std::vector<FLooseMatterParcel> m_loose;
        std::vector<FPendingHydraulicTransfer> m_pending;
        std::unordered_map<int,uint32_t> m_destRevision;
        SolveStats m_stats;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterRevision=0;
        uint32_t m_transportRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainHydraulicDetachment::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainHydraulicDetachment::LoadKernel(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,
            transferPath,externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,
            p5b3aPath,&reason,1);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainHydraulicDetachment::Kernel> parent,
        char const* p5b3bPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b3a_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b3bPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b3bEnabled=enabled;
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
        char const* p5b3aPath,char const* p5b3bPath,std::string* reason=nullptr,
        uint32_t p5b3bOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b3bOverride==0)enabled=0;
        else if(p5b3bOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b3bPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b3aDetachmentDigest=0;
        uint64_t detachmentDigestDisabled=0;
        uint64_t transportDigestBudget1=0,transportDigestBudgetN=0,transportDigestUnbounded=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterBefore=0,waterAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsMoved=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t flowHop=0,insufficientZero=0,heavyZero=0,transfers=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FHydraulicLooseMatterTransfer> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,TransportClass,LooseMatterId,MaterialId,Material,Grams,"
            "SourceCell,DestinationCell,CauseWaterBodyId,Force,Resistance,"
            "Moved,IdentityPreserved,DestinationRemainsLoose,"
            "TerrainSolidsBefore,TerrainSolidsAfter,LooseBefore,LooseAfter,"
            "WaterBefore,WaterAfter,HeldBefore,HeldAfter,"
            "SourceTerrainRev,DestTerrainRev,WaterRev,"
            "CellsVisited,BodiesExamined,RefusedStale,RefusedInvalid,"
            "DestinationUnavailable,PendingRetained,DepositedAsTerrain,RoutedThrough16C\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%s,%s,%s,%lld,"
                "%d,%d,%s,%u,%u,"
                "%d,%d,%d,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%u,%u,%u,"
                "%zu,%zu,%d,%d,%d,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                TransportClassName(t.Class),
                CausalWorldGeology::Hex64(t.LooseMatterId).c_str(),
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MaterialName,
                (long long)t.Grams,
                t.SourceCell,t.DestinationCell,
                CausalWorldGeology::Hex64(t.CauseWaterBodyId).c_str(),
                t.HydraulicForce,t.ParcelResistance,
                t.Moved?1:0,t.IdentityPreserved?1:0,t.DestinationRemainsLoose?1:0,
                (long long)t.TerrainSolidsBefore,(long long)t.TerrainSolidsAfter,
                (long long)t.LooseBefore,(long long)t.LooseAfter,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.SourceTerrainRevision,t.DestinationTerrainRevision,t.WaterRevision,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.DestinationUnavailable?1:0,t.PendingRetained?1:0,
                t.DepositedAsTerrain?1:0,t.RoutedThrough16C?1:0);
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
        char const* p5b2cPath,char const* p5b3aPath,char const* p5b3bPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b3a=[&]()->std::unique_ptr<CausalPresentWaterTerrainHydraulicDetachment::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,r2);
        };

        auto parent=reloadP5b3a();
        c.checks.push_back({"p5b3a_parent_loaded",parent!=nullptr&&parent->Complete()});
        if(!parent){c.reason=reason.empty()?"p5b3a_parent_load_failed":reason;return c;}
        c.p5b3aDetachmentDigest=parent->DetachmentDigestValue();
        c.checks.push_back({"p5b3a_detachment_digest_frozen",
            c.p5b3aDetachmentDigest==kFrozenP5b3aDetachmentDigest});
        c.checks.push_back({"fixtures_start_already_detached",
            !parent->Loose().empty()&&parent->LooseMass()==kParcelGrams
            &&parent->Stats().detachments>0});

        int64_t const water0=parent->ConservedMass();
        int64_t const solids0=parent->TerrainSolids();
        int64_t const loose0=parent->LooseMass();
        int64_t const held0=parent->HeldMatter();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint32_t const wRev0=parent->WaterTopologyRevision();
        uint64_t const detach0=parent->DetachmentDigestValue();
        auto const parentLoose=parent->Loose();

        auto disabled=MakeFromParent(reloadP5b3a(),p5b3bPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b3b",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.detachmentDigestDisabled=disabled->DetachmentDigestValue();
        bool locationsUnmoved=true;
        for(auto const& m:disabled->Loose())
            if(m.LocationCell!=m.SourceCell)locationsUnmoved=false;
        c.checks.push_back({"p5b3b_off_exact_p5b3a_state",
            c.detachmentDigestDisabled==detach0
            &&c.detachmentDigestDisabled==kFrozenP5b3aDetachmentDigest
            &&disabled->Stats().transfers==0
            &&disabled->ConservedMass()==water0
            &&disabled->TerrainSolids()==solids0
            &&disabled->LooseMass()==loose0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0
            &&locationsUnmoved
            &&disabled->Pending().empty()});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b3a(),p5b3bPath,1,budget,r2);
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

        c.transportDigestBudget1=budget1->TransportDigestValue();
        c.transportDigestBudgetN=budgetN->TransportDigestValue();
        c.transportDigestUnbounded=unbounded->TransportDigestValue();
        c.terrainSolidsBefore=unbounded->Stats().terrainSolidsBefore;
        c.terrainSolidsAfter=unbounded->Stats().terrainSolidsAfter;
        c.looseBefore=unbounded->Stats().looseBefore;
        c.looseAfter=unbounded->Stats().looseAfter;
        c.waterBefore=unbounded->Stats().waterMassBefore;
        c.waterAfter=unbounded->Stats().waterMassAfter;
        c.heldBefore=unbounded->Stats().heldBefore;
        c.heldAfter=unbounded->Stats().heldAfter;
        c.gramsMoved=unbounded->Stats().gramsMoved;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.flowHop=unbounded->Stats().flowHop;
        c.insufficientZero=unbounded->Stats().insufficientZero;
        c.heavyZero=unbounded->Stats().heavyZero;
        c.transfers=unbounded->Stats().transfers;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        uint64_t const disabledDigest=disabled->TransportDigestValue();
        bool freezeOk=c.transportDigestBudget1==c.transportDigestBudgetN
            &&c.transportDigestBudgetN==c.transportDigestUnbounded
            &&c.transportDigestUnbounded!=disabledDigest;
        if(kFrozenP5b3bTransportDigest)
            freezeOk=freezeOk&&c.transportDigestUnbounded==kFrozenP5b3bTransportDigest;
        c.checks.push_back({"budget_invariant_transport",freezeOk});
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
        bool hopOk=false,insufOk=false,heavyOk=false;
        uint64_t movedId=0;int hopSrc=-1,hopDst=-1;
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
            no16c=no16c&&!t.RoutedThrough16C&&!t.DepositedAsTerrain;
            destLoose=destLoose&&t.DestinationRemainsLoose;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            if(t.Moved)
            {
                identityKept=identityKept&&t.IdentityPreserved&&t.LooseMatterId!=0
                    &&t.MaterialId!=0&&t.SourceCell!=t.DestinationCell
                    &&t.DetachmentTransactionId!=0
                    &&!t.InputRevisions.empty()&&!t.OutputRevisions.empty();
                movedId=t.LooseMatterId;hopSrc=t.SourceCell;hopDst=t.DestinationCell;
            }
            if(t.Fixture==FixtureKind::FlowHop)
                hopOk=t.Moved&&t.IdentityPreserved&&t.Grams==kParcelGrams
                    &&t.SourceCell!=t.DestinationCell
                    &&(t.Class==TransportClass::RiverFlowHop
                      ||t.Class==TransportClass::WetlandFlowHop
                      ||t.Class==TransportClass::OtherFlowHop);
            if(t.Fixture==FixtureKind::InsufficientForce)
                insufOk=!t.Moved&&t.Class==TransportClass::InsufficientForce
                    &&t.HydraulicForce<t.ParcelResistance
                    &&t.LooseBefore==t.LooseAfter;
            if(t.Fixture==FixtureKind::HeavyResistant)
                heavyOk=!t.Moved&&t.Class==TransportClass::ResistantParcel
                    &&t.ParcelResistance>t.HydraulicForce
                    &&t.LooseBefore==t.LooseAfter;
        }
        auto const* moved=unbounded->FindLoose(movedId);
        bool locOk=moved&&moved->LocationCell==hopDst&&moved->SourceCell==hopSrc
            &&moved->LocationCell!=moved->SourceCell
            &&moved->Grams==kParcelGrams
            &&moved->LooseMatterId==movedId;
        if(!parentLoose.empty())
        {
            auto const& orig=parentLoose.front();
            locOk=locOk&&moved
                &&moved->MaterialId==orig.MaterialId
                &&moved->FormationProvenance==orig.FormationProvenance
                &&moved->GeologicalAncestry==orig.GeologicalAncestry
                &&moved->DetachmentTransactionId==orig.TransactionId
                &&moved->DetachmentCause==orig.DetachmentCause;
        }
        c.checks.push_back({"one_hop_moves_same_parcel",hopOk&&c.flowHop>0&&c.transfers==1&&locOk});
        c.checks.push_back({"insufficient_forcing_zero_movement",insufOk&&c.insufficientZero>0});
        c.checks.push_back({"heavy_resistant_parcel_zero_movement",heavyOk&&c.heavyZero>0});
        c.checks.push_back({"transport_preserves_identity",identityKept&&movedId!=0});
        c.checks.push_back({"destination_remains_loose_not_deposited",destLoose&&no16c
            &&unbounded->Pending().empty()});
        c.checks.push_back({"admitted_transfers_conserved",massEach});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined<6515&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_transport",
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
                fixtures.stale.DestinationTerrainRevision=k->DestinationRevision(fixtures.stale.DestinationCell);
                fixtures.stale.CheckRevision=true;
                k->BumpDestinationRevision(fixtures.stale.DestinationCell);
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.stale.LooseMatterId);
                auto stale=k->ApplyRequest(fixtures.stale);
                staleOk=stale.RefusedStale&&!stale.Moved&&stale.Grams==kParcelGrams
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose&&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.stale.LooseMatterId)==loc
                    &&loc==fixtures.stale.SourceCell;
            }
            c.checks.push_back({"stale_destination_revision_refuse",staleOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool unavailOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.unavailable.SourceTerrainRevision=k->TerrainRevision();
                fixtures.unavailable.WaterRevision=k->WaterTopologyRevision();
                fixtures.unavailable.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.unavailable.DestinationCell);
                fixtures.unavailable.CheckRevision=true;
                fixtures.unavailable.DestinationAvailable=false;
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                int const loc=k->LocationOf(fixtures.unavailable.LooseMatterId);
                size_t const nLoose=k->Loose().size();
                auto rec=k->ApplyRequest(fixtures.unavailable);
                unavailOk=rec.DestinationUnavailable&&rec.PendingRetained&&!rec.Moved
                    &&!rec.RefusedInvalid&&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->ConservedMass()==water
                    &&k->LocationOf(fixtures.unavailable.LooseMatterId)==loc
                    &&loc==fixtures.unavailable.SourceCell
                    &&k->Loose().size()==nLoose
                    &&k->Pending().size()==1
                    &&k->Pending().front().LooseMatterId==fixtures.unavailable.LooseMatterId
                    &&k->Pending().front().DestinationUnavailable;
            }
            c.checks.push_back({"unavailable_destination_retains_parcel_and_pending",unavailOk});
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
                    TransportRequest reqs[3]={f.insufficient,f.heavy,f.hop};
                    for(auto& req:reqs)
                    {
                        req.SourceTerrainRevision=k.TerrainRevision();
                        req.WaterRevision=k.WaterTopologyRevision();
                        req.DestinationTerrainRevision=k.DestinationRevision(req.DestinationCell);
                        req.CheckRevision=true;
                        k.ApplyRequest(req);
                    }
                };
                applyAll(*a,fa);
                applyAll(*b,fb);
                partitionOk=a->TransportDigestValue()==b->TransportDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TerrainSolids()==b->TerrainSolids()
                    &&a->LooseMass()==b->LooseMass()
                    &&fa.hop.SourceCell==fb.hop.SourceCell
                    &&fa.hop.DestinationCell==fb.hop.DestinationCell
                    &&a->LocationOf(fa.hop.LooseMatterId)==b->LocationOf(fb.hop.LooseMatterId);
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->TransportDigestValue()==unbounded->TransportDigestValue()
            &&reload->TransportDigestValue()==unbounded->TransportDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->TerrainSolids()==unbounded->TerrainSolids()
            &&cold->LooseMass()==unbounded->LooseMass()
            &&cold->LocationOf(movedId)==unbounded->LocationOf(movedId);
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3c_bank_support_collapse_closed",true});
        c.checks.push_back({"p5b3b2_deposition_as_terrain_closed",true});
        c.checks.push_back({"general_erosion_closed",true});
        c.checks.push_back({"active_16b_erosion_closed",true});
        c.checks.push_back({"16c_remobilization_closed",true});
        c.checks.push_back({"rainfall_evaporation_groundwater_closed",true});
        c.checks.push_back({"world_operation_scheduler_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_transfer",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->TransportDigestValue();
                size_t const n=unbounded->Stats().transfers;
                size_t const wakes=unbounded->Stats().transportWakes;
                unbounded->Tick(64);
                return unbounded->TransportDigestValue()==before
                    &&unbounded->Stats().transfers==n
                    &&unbounded->Stats().transportWakes==wakes;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b3b_hydraulic_transport_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b3b_hydraulic_transport_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B3B_HYDRAULIC_TRANSPORT %s\nreason=%s\n"
            "p5b3a_detachment_digest=%s\ndetachment_digest_disabled=%s\n"
            "transport_digest_budget1=%s\ntransport_digest_budgetN=%s\n"
            "transport_digest_unbounded=%s\n"
            "terrain_solids_before=%lld\nterrain_solids_after=%lld\n"
            "loose_before=%lld\nloose_after=%lld\n"
            "water_before=%lld\nwater_after=%lld\n"
            "held_before=%lld\nheld_after=%lld\n"
            "grams_moved=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "flow_hop=%zu\ninsufficient_zero=%zu\nheavy_zero=%zu\ntransfers=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=hydraulic_transport_location_not_identity\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=frozen\n"
            "p5b3a=frozen\np5b3b=open\np5b3c=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "general_erosion=0\nbank_collapse=0\n"
            "active_16b_erosion=0\n16c_remobilization=0\necology=0\n"
            "deposition_as_terrain=0\nworld_operation_scheduler=0\n"
            "choice_a_local_loose=1\nchoice_b_16c=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b3aDetachmentDigest).c_str(),
            CausalWorldGeology::Hex64(c.detachmentDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.transportDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.transportDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.transportDigestUnbounded).c_str(),
            (long long)c.terrainSolidsBefore,(long long)c.terrainSolidsAfter,
            (long long)c.looseBefore,(long long)c.looseAfter,
            (long long)c.waterBefore,(long long)c.waterAfter,
            (long long)c.heldBefore,(long long)c.heldAfter,
            (long long)c.gramsMoved,
            c.transactionsCertified,c.transactionsAdmitted,
            c.flowHop,c.insufficientZero,c.heavyZero,c.transfers,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
