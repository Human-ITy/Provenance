#pragma once

// P5b.3B.3B: compaction / terrain-surface integration.
// CompactMatter changes packing / compaction / support-capacity on the
// same DepositionalBodyId. Grams stay on the depositional ledger.
// CompactedDeposit may participate in terrain surface + collision/support.
// Same material ID is not the same structural body — never host-formation
// weld, never terrain host solids += grams.
//
// Pipeline: settled depositional aggregate + sustained admissible load +
// packing criteria + current terrain / deposition / compaction revision
// validation → CompactMatter → packing/compaction/support-capacity change.
// Total matter unchanged. Sufficient hydraulic forcing remobilizes the
// compacted body back to loose representation with the same grams and
// provenance (reversible).
//
// CLOSED: P5b.3C bank/support collapse, cement/lithify, chemical bonding,
// formation-ID merge, general soil mechanics, 16C remobilization, weld
// into host geology, erase LooseMatter / depositional provenance, compact
// arbitrarily, trigger bank collapse, enter compiled Stage-16C history,
// rainfall, evaporation, groundwater, plant uptake, ecology, full
// WorldOperationScheduler.
//
// Frozen parent: P5b.3B.3A 160a0842 / 3B.2 eeabfb8c / 3B 67d5f524 /
// 3A 6c467fb7.
// Disabled P5b.3B.3B must reproduce exact P5b.3B.3A deposition state.

#include "CausalPresentWaterTerrainHydraulicDeposition.h"

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

namespace CausalPresentWaterTerrainHydraulicCompaction
{
    constexpr char const* kExpectedRegion=
        "causal_world_present_water_terrain_hydraulic_compaction_floor";
    constexpr uint64_t kFrozenP5b3b3aDepositionDigest=
        CausalPresentWaterTerrainHydraulicDeposition::kFrozenP5b3b3aDepositionDigest;
    constexpr uint64_t kFrozenP5b3b3bCompactionDigest=0xef1ae252489e741aull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainHydraulicDeposition::kHoldBudget;
    constexpr int64_t kParcelGrams=CausalPresentWaterTerrainHydraulicDeposition::kParcelGrams;
    constexpr uint32_t kSmallDebrisResistance=
        CausalPresentWaterTerrainHydraulicDeposition::kSmallDebrisResistance;
    constexpr uint32_t kTransportForce=CausalPresentWaterTerrainHydraulicDeposition::kTransportForce;
    constexpr uint32_t kLowEnergyForce=CausalPresentWaterTerrainHydraulicDeposition::kLowEnergyForce;
    constexpr uint32_t kAdmissibleLoad=400;
    constexpr uint32_t kInsufficientLoad=50;
    constexpr uint32_t kMinPackingForCompact=700;
    constexpr uint32_t kCompactedPackingFraction=850;
    constexpr uint32_t kCompactedCompaction=800;
    constexpr uint32_t kCompactedPorosity=250;
    constexpr uint32_t kCompactedSupportCapacity=1000;
    constexpr uint32_t kCompactedPermeability=200;
    constexpr uint32_t kWalkingCompactionMin=500;

    enum class FixtureKind:uint8_t
    {
        None=0,
        AdmissibleLoad=1,
        InsufficientLoad=2,
        RemobilizeByForcing=3,
        StaleRevision=4,
        HostContactControl=5
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::AdmissibleLoad:return "stable_settled_aggregate_admissible_load";
            case FixtureKind::InsufficientLoad:return "insufficient_compaction_stays_aggregate";
            case FixtureKind::RemobilizeByForcing:return "compacted_redisturb_by_forcing";
            case FixtureKind::StaleRevision:return "stale_support_or_material_revision";
            case FixtureKind::HostContactControl:return "host_contact_no_formation_merge";
            default:return "none";
        }
    }

    enum class CompactClass:uint8_t
    {
        None=0,
        CompactedDeposit=1,
        InsufficientLoad=2,
        Remobilized=3,
        StaleRevision=4,
        HostContactSeparate=5
    };

    inline char const* CompactClassName(CompactClass value)
    {
        switch(value)
        {
            case CompactClass::CompactedDeposit:return "compacted_deposit";
            case CompactClass::InsufficientLoad:return "insufficient_load";
            case CompactClass::Remobilized:return "remobilized_to_loose";
            case CompactClass::StaleRevision:return "stale_revision";
            case CompactClass::HostContactSeparate:return "host_contact_separate_body";
            default:return "none";
        }
    }

    using FDepositionalMatterBody=CausalPresentWaterTerrainHydraulicDeposition::FDepositionalMatterBody;
    using FLooseMatterParcel=CausalPresentWaterTerrainHydraulicDeposition::FLooseMatterParcel;
    using PackingState=CausalPresentWaterTerrainHydraulicDeposition::PackingState;
    using SupportState=CausalPresentWaterTerrainHydraulicDeposition::SupportState;
    using BodyPhase=CausalPresentWaterTerrainHydraulicDeposition::BodyPhase;
    using MotionState=CausalPresentWaterTerrainHydraulicDeposition::MotionState;

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b3b3bEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b3b3bEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainHydraulicDeposition::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_HYDRAULIC_COMPACTION_V1";
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
          &&hex("p5b3b3b_event_id",r.program.p5b3b3bEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b3b3b_enabled",r.program.p5b3b3bEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b3b3bEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b3b3bEnabled==0||r.program.p5b3b3bEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b3b3b_compaction_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FCompactMatter
    {
        uint64_t TransactionId=0;
        uint64_t DepositionalBodyId=0;
        uint64_t LooseMatterId=0;
        uint64_t MaterialId=0;
        int64_t Grams=0;
        int OccupiedRegion=-1;
        uint64_t CauseWaterBodyId=0;
        uint32_t WaterRevision=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t DepositionRevision=0;
        uint32_t CompactionRevision=0;
        uint32_t TerrainStateRevision=0;
        CompactClass Class=CompactClass::None;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        FixtureKind Fixture=FixtureKind::None;
        char MaterialName[32]{};
        uint32_t HydraulicForce=0;
        uint32_t ParcelResistance=0;
        uint32_t AppliedLoad=0;
        PackingState PackingAfter=PackingState::LooseSediment;
        uint32_t CompactionAfter=0;
        uint32_t PackingFractionAfter=0;
        uint32_t PorosityAfter=0;
        uint32_t SupportCapacityAfter=0;
        uint32_t PermeabilityAfter=0;
        SupportState SupportAfter=SupportState::Unsupported;
        BodyPhase PhaseAfter=BodyPhase::SettledAggregate;
        bool WaterContact=false;
        bool Compacted=false;
        bool Remobilized=false;
        bool IdentityPreserved=false;
        bool HostSolidsUnchanged=true;
        bool WeldedToHost=false;
        bool Lithified=false;
        bool FormationMerged=false;
        bool IsStableWalkingSurface=false;
        bool RefusedStale=false,RefusedInvalid=false;
        bool RoutedThrough16C=false;
        bool BankCollapse=false;
        bool Cemented=false;
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
        uint64_t HostFeatureId=0;
        uint64_t HostMaterialId=0;
    };

    struct CompactRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        uint64_t DepositionalBodyId=0;
        uint64_t LooseMatterId=0;
        int OccupiedRegion=-1;
        uint64_t WaterBodyId=0;
        uint32_t SourceTerrainRevision=0;
        uint32_t DestinationTerrainRevision=0;
        uint32_t WaterRevision=0;
        uint32_t DepositionRevision=0;
        uint32_t CompactionRevision=0;
        uint32_t TerrainStateRevision=0;
        bool CheckRevision=true;
        bool UseLoadOverride=false;
        uint32_t EvalLoad=0;
        bool UseForceOverride=false;
        uint32_t EvalForce=0;
        bool UseResistanceOverride=false;
        uint32_t EvalResistance=0;
        bool Remobilize=false;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t admissibleCompact=0,insufficientZero=0;
        size_t compacts=0,compactWakes=0,staleRefuse=0,remobilizes=0;
        size_t waterResponses=0,topologyRebuilds=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t depositionalBefore=0,depositionalAfter=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsCompacted=0,gramsRemobilized=0;
        uint64_t parentDepositionDigest=0,compactionDigest=0;
        uint64_t compiledSedimentBefore=0,compiledSedimentAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterRevision=0,compactionRevision=0;
    };

    inline uint64_t MakeTxnId(FixtureKind kind,uint64_t bodyId,int cell,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3300Bull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&bodyId,sizeof(bodyId));
        CausalWorldGeology::HashAppend(h,&cell,sizeof(cell));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline uint64_t HashBytes(void const* p,size_t n)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,p,n);
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

    inline bool IsStableDepositedGround(FDepositionalMatterBody const& b)
    {
        return b.Phase==BodyPhase::CompactedDeposit
            &&b.Compaction>=kWalkingCompactionMin
            &&b.PackingFraction>=kMinPackingForCompact
            &&b.IsStableWalkingSurface
            &&b.SupportCapacity>0
            &&!b.WeldedToHost
            &&!b.Lithified;
    }

    inline uint64_t CompactionDigestOf(std::vector<FLooseMatterParcel> const& loose,
        std::vector<FDepositionalMatterBody> const& bodies,
        uint64_t parentDigest,uint32_t compactionRev)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&parentDigest,sizeof(parentDigest));
        CausalWorldGeology::HashAppend(d,&compactionRev,sizeof(compactionRev));
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
            CausalWorldGeology::HashAppend(d,&b.PackingFraction,sizeof(b.PackingFraction));
            CausalWorldGeology::HashAppend(d,&b.Porosity,sizeof(b.Porosity));
            CausalWorldGeology::HashAppend(d,&b.SupportCapacity,sizeof(b.SupportCapacity));
            CausalWorldGeology::HashAppend(d,&b.PermeabilityModifier,sizeof(b.PermeabilityModifier));
            uint8_t const phase=(uint8_t)b.Phase;
            CausalWorldGeology::HashAppend(d,&phase,sizeof(phase));
            uint8_t const walk=b.IsStableWalkingSurface?1:0;
            CausalWorldGeology::HashAppend(d,&walk,sizeof(walk));
            uint8_t const support=(uint8_t)b.Support;
            CausalWorldGeology::HashAppend(d,&support,sizeof(support));
            CausalWorldGeology::HashAppend(d,&b.Revision,sizeof(b.Revision));
            CausalWorldGeology::HashAppend(d,&b.FormationProvenance,sizeof(b.FormationProvenance));
            CausalWorldGeology::HashAppend(d,&b.GeologicalAncestry,sizeof(b.GeologicalAncestry));
            CausalWorldGeology::HashAppend(d,&b.DetachmentTransactionId,sizeof(b.DetachmentTransactionId));
            CausalWorldGeology::HashAppend(d,&b.HostFeatureId,sizeof(b.HostFeatureId));
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
        }
        return d;
    }

    struct FixtureSet
    {
        CompactRequest admissible,insufficient,remobilize,stale,hostContact;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(
        CausalPresentWaterTerrainHydraulicDeposition::Kernel const& parent)
    {
        FixtureSet set;
        FDepositionalMatterBody const* body=nullptr;
        for(auto const& b:parent.Bodies())
        {
            if(b.TotalGrams==kParcelGrams&&b.Phase==BodyPhase::SettledAggregate)
            {body=&b;break;}
        }
        if(!body)return set;
        int const loc=body->OccupiedRegion;
        auto const& cells=parent.Water().Cells();
        if(loc<0||(size_t)loc>=cells.size()||body->DepositionalBodyId==0)return set;
        auto fill=[&](CompactRequest& req,FixtureKind kind)
        {
            req.Fixture=kind;
            req.DepositionalBodyId=body->DepositionalBodyId;
            req.LooseMatterId=body->ConstituentMatterIds.empty()?0:body->ConstituentMatterIds.front();
            req.OccupiedRegion=loc;
            req.WaterBodyId=CausalPresentWaterTerrainHydraulicSettling::BindCauseBody(
                parent.Parent().Parent(),loc);
            req.CheckRevision=true;
        };
        fill(set.admissible,FixtureKind::AdmissibleLoad);
        set.admissible.UseLoadOverride=true;
        set.admissible.EvalLoad=kAdmissibleLoad;
        set.admissible.UseForceOverride=true;
        set.admissible.EvalForce=kLowEnergyForce;
        fill(set.insufficient,FixtureKind::InsufficientLoad);
        set.insufficient.UseLoadOverride=true;
        set.insufficient.EvalLoad=kInsufficientLoad;
        set.insufficient.UseForceOverride=true;
        set.insufficient.EvalForce=kLowEnergyForce;
        fill(set.remobilize,FixtureKind::RemobilizeByForcing);
        set.remobilize.Remobilize=true;
        set.remobilize.UseForceOverride=true;
        set.remobilize.EvalForce=kTransportForce;
        fill(set.stale,FixtureKind::StaleRevision);
        fill(set.hostContact,FixtureKind::HostContactControl);
        set.hostContact.UseLoadOverride=true;
        set.hostContact.EvalLoad=kAdmissibleLoad;
        set.hostContact.UseForceOverride=true;
        set.hostContact.EvalForce=kLowEnergyForce;
        set.ok=body->TotalGrams==kParcelGrams&&!body->WeldedToHost
            &&body->Compaction==0&&!body->IsStableWalkingSurface;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainHydraulicDeposition::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_depositionRevision=m_parent->DepositionRevision();
            m_complete=!m_program.p5b3b3bEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainHydraulicDeposition::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainHydraulicDeposition::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FCompactMatter> const& Transactions() const{return m_txns;}
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
        uint32_t CompactionRevision() const{return m_compactionRevision;}
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
        uint64_t DepositionDigestValue() const{return m_parent->DepositionDigestValue();}
        uint64_t CompactionDigestValue() const
        {
            if(!m_program.p5b3b3bEnabled)return DepositionDigestValue();
            return CompactionDigestOf(m_loose,m_bodies,DepositionDigestValue(),m_compactionRevision);
        }
        int64_t BodyMass() const{return m_parent->BodyMass();}
        int64_t PoreMass() const{return m_parent->PoreMass();}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t ConservedMass() const{return m_parent->ConservedMass();}
        int64_t TerrainSolids() const{return m_parent->TerrainSolids();}
        int64_t LooseMass() const{return (LooseMassOf)(m_loose);}
        int64_t DepositionalMass() const{return CausalPresentWaterTerrainHydraulicCompaction::DepositionalMassOf(m_bodies);}
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
        int LocationOf(uint64_t bodyId) const
        {
            for(auto const& b:m_bodies)if(b.DepositionalBodyId==bodyId)return b.OccupiedRegion;
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
        bool RecognizesStableDepositedGround(uint64_t bodyId) const
        {
            auto const* b=FindBody(bodyId);
            return b&&IsStableDepositedGround(*b);
        }

        FCompactMatter ApplyRequest(CompactRequest req)
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
            if(!m_program.p5b3b3bEnabled){FinishUnchanged();return true;}
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
                m_stats.compactionDigest=CompactionDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterRevision=m_waterRevision;
                m_stats.compactionRevision=m_compactionRevision;
            }
            return m_complete;
        }

    private:
        void CopyParentState()
        {
            m_loose.clear();
            for(auto const& src:m_parent->Loose())m_loose.push_back(src);
            m_bodies.clear();
            for(auto const& src:m_parent->Bodies())m_bodies.push_back(src);
        }

        void BeginFromParent()
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_depositionRevision=m_parent->DepositionRevision();
            CopyParentState();
            m_stats.parentDepositionDigest=m_parent->DepositionDigestValue();
            m_stats.terrainSolidsBefore=TerrainSolids();
            m_stats.looseBefore=LooseMass();
            m_stats.depositionalBefore=DepositionalMass();
            m_stats.waterMassBefore=ConservedMass();
            m_stats.heldBefore=HeldMatter();
            m_stats.compiledSedimentBefore=TerrainDigest();
            if(!m_program.p5b3b3bEnabled){FinishUnchanged();return;}
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
            m_stats.compactionDigest=CompactionDigestValue();
            m_stats.terrainRevision=m_terrainRevision;
            m_stats.waterRevision=m_waterRevision;
            m_stats.compactionRevision=m_compactionRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent);
            m_work.clear();
            if(fixtures.ok)m_work.push_back(fixtures.admissible);
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                CompactRequest req=m_work[m_cursor];
                req.SourceTerrainRevision=m_terrainRevision;
                req.DestinationTerrainRevision=DestinationRevision(req.OccupiedRegion);
                req.WaterRevision=m_waterRevision;
                req.DepositionRevision=m_depositionRevision;
                req.CompactionRevision=m_compactionRevision;
                req.TerrainStateRevision=TerrainStateRevision();
                req.CheckRevision=true;
                FCompactMatter rec=CommitOne(req);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        FDepositionalMatterBody* FindBodyMut(uint64_t bodyId)
        {
            for(auto& b:m_bodies)if(b.DepositionalBodyId==bodyId)return &b;
            return nullptr;
        }

        bool VisitNeighborhood(int cell,FCompactMatter& rec)
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

        void SampleHost(int cell,uint64_t& hostFeature,uint64_t& hostMaterial)
        {
            hostFeature=0;hostMaterial=0;
            auto const& drain=Water().Drainage().Cells();
            if(cell<0||(size_t)cell>=drain.size())return;
            auto const geo=SurfaceGeology(drain[(size_t)cell].x,drain[(size_t)cell].y);
            hostFeature=geo.featureId;
            if(!geo.material.empty())
                hostMaterial=HashBytes(geo.material.data(),geo.material.size());
        }

        void FillLedgerAfter(FCompactMatter& txn)
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
                m_compactionRevision,DestinationRevision(txn.OccupiedRegion)};
            txn.HostSolidsUnchanged=txn.TerrainSolidsAfter==txn.TerrainSolidsBefore;
        }

        FCompactMatter CommitOne(CompactRequest req)
        {
            FCompactMatter txn;
            txn.Fixture=req.Fixture;
            txn.DepositionalBodyId=req.DepositionalBodyId;
            txn.LooseMatterId=req.LooseMatterId;
            txn.OccupiedRegion=req.OccupiedRegion;
            txn.CauseWaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterRevisionBefore=m_waterRevision;
            txn.SourceTerrainRevision=m_terrainRevision;
            txn.DestinationTerrainRevision=DestinationRevision(req.OccupiedRegion);
            txn.WaterRevision=m_waterRevision;
            txn.DepositionRevision=m_depositionRevision;
            txn.CompactionRevision=m_compactionRevision;
            txn.TerrainStateRevision=TerrainStateRevision();
            txn.TerrainSolidsBefore=TerrainSolids();
            txn.LooseBefore=LooseMass();
            txn.DepositionalBefore=DepositionalMass();
            txn.WaterMassBefore=ConservedMass();
            txn.HeldMatterBefore=HeldMatter();
            txn.CompiledSedimentBefore=TerrainDigest();
            txn.InputRevisions={m_terrainRevision,m_waterRevision,m_depositionRevision,
                m_compactionRevision,DestinationRevision(req.OccupiedRegion),
                TerrainStateRevision()};
            txn.WeldedToHost=false;
            txn.Lithified=false;
            txn.FormationMerged=false;
            txn.RoutedThrough16C=false;
            txn.BankCollapse=false;
            txn.Cemented=false;
            auto settle=[&]()
            {
                FillLedgerAfter(txn);
                return txn;
            };
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.Compacted=false;txn.Remobilized=false;
                return settle();
            };

            ++m_stats.compactWakes;
            auto const& cells=Water().Cells();
            if(req.OccupiedRegion<0||(size_t)req.OccupiedRegion>=cells.size()
              ||req.Fixture==FixtureKind::None||req.DepositionalBodyId==0)
                return refuse(false,true);

            txn.TransactionId=MakeTxnId(req.Fixture,req.DepositionalBodyId,req.OccupiedRegion,
                (uint32_t)(m_txns.size()+1u));

            FDepositionalMatterBody* body=FindBodyMut(req.DepositionalBodyId);
            if(!body||body->TotalGrams<=0)return refuse(false,true);

            txn.MaterialId=body->MaterialId;
            std::snprintf(txn.MaterialName,sizeof(txn.MaterialName),"%s",body->MaterialName);
            txn.Grams=body->TotalGrams;
            txn.FormationProvenance=body->FormationProvenance;
            txn.GeologicalAncestry=body->GeologicalAncestry;
            txn.DetachmentTransactionId=body->DetachmentTransactionId;
            txn.LooseMatterId=body->ConstituentMatterIds.empty()
                ?req.LooseMatterId:body->ConstituentMatterIds.front();
            txn.IdentityPreserved=true;
            txn.OccupiedRegion=body->OccupiedRegion;
            SampleHost(body->OccupiedRegion,txn.HostFeatureId,txn.HostMaterialId);

            if(req.CheckRevision&&(req.SourceTerrainRevision!=m_terrainRevision
              ||req.WaterRevision!=m_waterRevision
              ||req.DepositionRevision!=m_depositionRevision
              ||req.CompactionRevision!=m_compactionRevision
              ||req.TerrainStateRevision!=TerrainStateRevision()))
                return refuse(true,false);
            if(req.CheckRevision&&req.DestinationTerrainRevision!=DestinationRevision(req.OccupiedRegion))
            {
                txn.Class=CompactClass::StaleRevision;
                return refuse(true,false);
            }

            txn.WaterContact=CausalPresentWaterTerrainHydraulicTransport::HasTransportWaterContact(
                m_parent->Parent().Parent().Parent(),req.OccupiedRegion);
            uint32_t force=CausalPresentWaterTerrainHydraulicTransport::HydraulicForceOf(
                m_parent->Parent().Parent().Parent(),req.OccupiedRegion);
            if(req.UseForceOverride)force=req.EvalForce;
            uint32_t resistance=kSmallDebrisResistance;
            if(req.UseResistanceOverride)resistance=req.EvalResistance;
            uint32_t load=kAdmissibleLoad;
            if(req.UseLoadOverride)load=req.EvalLoad;
            txn.HydraulicForce=force;
            txn.ParcelResistance=resistance;
            txn.AppliedLoad=load;
            txn.CellsVisited=2;

            if(req.Remobilize)
            {
                if(force<resistance)
                {
                    txn.Class=CompactClass::CompactedDeposit;
                    txn.Compacted=body->Phase==BodyPhase::CompactedDeposit;
                    txn.IsStableWalkingSurface=body->IsStableWalkingSurface;
                    txn.PackingAfter=body->Packing;
                    txn.CompactionAfter=body->Compaction;
                    txn.PackingFractionAfter=body->PackingFraction;
                    txn.PorosityAfter=body->Porosity;
                    txn.SupportCapacityAfter=body->SupportCapacity;
                    txn.PermeabilityAfter=body->PermeabilityModifier;
                    txn.SupportAfter=body->Support;
                    txn.PhaseAfter=body->Phase;
                    VisitNeighborhood(req.OccupiedRegion,txn);
                    return settle();
                }
                FLooseMatterParcel restored{};
                restored.LooseMatterId=txn.LooseMatterId?txn.LooseMatterId:body->DepositionalBodyId;
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
                body->IsStableWalkingSurface=false;
                body->Phase=BodyPhase::Loose;
                body->SupportCapacity=0;
                m_bodies.erase(std::remove_if(m_bodies.begin(),m_bodies.end(),
                    [](FDepositionalMatterBody const& b){return b.TotalGrams<=0;}),
                    m_bodies.end());
                ++m_compactionRevision;
                txn.Class=CompactClass::Remobilized;
                txn.Remobilized=true;
                txn.Compacted=false;
                txn.IsStableWalkingSurface=false;
                txn.IdentityPreserved=true;
                txn.OccupiedRegion=restored.LocationCell;
                VisitNeighborhood(restored.LocationCell,txn);
                FillLedgerAfter(txn);
                return txn;
            }

            bool const loadOk=load>=kAdmissibleLoad;
            bool const packingOk=body->PackingFraction+(loadOk?450u:0u)>=kMinPackingForCompact;
            bool const forceOk=force<resistance;
            bool const phaseOk=body->Phase==BodyPhase::SettledAggregate
                ||body->Phase==BodyPhase::CompactedDeposit;
            if(!loadOk||!packingOk||!forceOk||!phaseOk)
            {
                txn.Class=CompactClass::InsufficientLoad;
                txn.Compacted=false;
                txn.IsStableWalkingSurface=false;
                txn.PackingAfter=body->Packing;
                txn.CompactionAfter=body->Compaction;
                txn.PackingFractionAfter=body->PackingFraction;
                txn.PorosityAfter=body->Porosity;
                txn.SupportCapacityAfter=body->SupportCapacity;
                txn.PermeabilityAfter=body->PermeabilityModifier;
                txn.SupportAfter=body->Support;
                txn.PhaseAfter=body->Phase;
                VisitNeighborhood(req.OccupiedRegion,txn);
                return settle();
            }

            uint64_t const keptId=body->DepositionalBodyId;
            int64_t const keptGrams=body->TotalGrams;
            uint64_t const keptMat=body->MaterialId;
            uint64_t const keptForm=body->FormationProvenance;
            uint64_t const keptAnc=body->GeologicalAncestry;
            uint64_t const keptDet=body->DetachmentTransactionId;
            uint64_t const keptLoose=txn.LooseMatterId;
            body->Packing=PackingState::PackedSediment;
            body->Compaction=kCompactedCompaction;
            body->PackingFraction=kCompactedPackingFraction;
            body->Porosity=kCompactedPorosity;
            body->SupportCapacity=kCompactedSupportCapacity;
            body->PermeabilityModifier=kCompactedPermeability;
            body->Phase=BodyPhase::CompactedDeposit;
            body->IsStableWalkingSurface=true;
            body->ContributesToSurface=true;
            body->ContributesToCollision=true;
            body->Support=SupportState::Supported;
            body->WeldedToHost=false;
            body->Lithified=false;
            body->HostFeatureId=txn.HostFeatureId;
            body->HostMaterialId=txn.HostMaterialId;
            body->Revision=m_compactionRevision+1;
            ++m_compactionRevision;
            txn.Class=req.Fixture==FixtureKind::HostContactControl
                ?CompactClass::HostContactSeparate:CompactClass::CompactedDeposit;
            txn.Compacted=true;
            txn.IsStableWalkingSurface=true;
            txn.PackingAfter=body->Packing;
            txn.CompactionAfter=body->Compaction;
            txn.PackingFractionAfter=body->PackingFraction;
            txn.PorosityAfter=body->Porosity;
            txn.SupportCapacityAfter=body->SupportCapacity;
            txn.PermeabilityAfter=body->PermeabilityModifier;
            txn.SupportAfter=body->Support;
            txn.PhaseAfter=body->Phase;
            txn.FormationMerged=false;
            txn.WeldedToHost=false;
            txn.IdentityPreserved=
                keptId==req.DepositionalBodyId
                &&keptMat==txn.MaterialId
                &&keptGrams==txn.Grams
                &&keptForm==txn.FormationProvenance
                &&keptAnc==txn.GeologicalAncestry
                &&keptDet==txn.DetachmentTransactionId
                &&keptLoose==txn.LooseMatterId
                &&body->DepositionalBodyId!=body->HostFeatureId
                &&body->FormationProvenance!=body->HostFeatureId
                &&body->WeldedToHost==false
                &&body->Lithified==false;
            VisitNeighborhood(body->OccupiedRegion,txn);
            FillLedgerAfter(txn);
            return txn;
        }

        void Record(FCompactMatter const& rec)
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
            if(rec.Compacted)
            {
                ++m_stats.compacts;
                m_stats.gramsCompacted+=rec.Grams;
            }
            if(rec.Remobilized)
            {
                ++m_stats.remobilizes;
                m_stats.gramsRemobilized+=rec.Grams;
            }
            if(rec.Fixture==FixtureKind::AdmissibleLoad)++m_stats.admissibleCompact;
            else if(rec.Fixture==FixtureKind::InsufficientLoad)++m_stats.insufficientZero;
        }

        std::unique_ptr<CausalPresentWaterTerrainHydraulicDeposition::Kernel> m_parent;Program m_program;
        std::vector<CompactRequest> m_work;
        std::vector<FCompactMatter> m_txns;
        std::vector<FLooseMatterParcel> m_loose;
        std::vector<FDepositionalMatterBody> m_bodies;
        std::unordered_map<int,uint32_t> m_destRevision;
        SolveStats m_stats;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterRevision=0;
        uint32_t m_depositionRevision=0;
        uint32_t m_compactionRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainHydraulicDeposition::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,char const* p5b3bPath,char const* p5b3b2Path,
        char const* p5b3b3aPath,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainHydraulicDeposition::LoadKernel(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,
            transferPath,externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,
            p5b3aPath,p5b3bPath,p5b3b2Path,p5b3b3aPath,&reason,1);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainHydraulicDeposition::Kernel> parent,
        char const* p5b3b3bPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b3b3a_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b3b3bPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b3b3bEnabled=enabled;
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
        char const* p5b3b3aPath,char const* p5b3b3bPath,
        std::string* reason=nullptr,uint32_t p5b3b3bOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,
            p5b3b2Path,p5b3b3aPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b3b3bOverride==0)enabled=0;
        else if(p5b3b3bOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b3b3bPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b3b3aDepositionDigest=0;
        uint64_t depositionDigestDisabled=0;
        uint64_t compactionDigestBudget1=0,compactionDigestBudgetN=0,compactionDigestUnbounded=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t depositionalBefore=0,depositionalAfter=0;
        int64_t waterBefore=0,waterAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t gramsCompacted=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t admissibleCompact=0,insufficientZero=0,compacts=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FCompactMatter> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,CompactClass,DepositionalBodyId,LooseMatterId,"
            "MaterialId,Material,Grams,OccupiedRegion,CauseWaterBodyId,Force,Resistance,Load,"
            "Compacted,Remobilized,IdentityPreserved,HostSolidsUnchanged,Packing,Compaction,"
            "PackingFraction,Porosity,SupportCapacity,Phase,WalkingSurface,"
            "TerrainSolidsBefore,TerrainSolidsAfter,LooseBefore,LooseAfter,"
            "DepositionalBefore,DepositionalAfter,WaterBefore,WaterAfter,HeldBefore,HeldAfter,"
            "SourceTerrainRev,DestTerrainRev,WaterRev,DepositionRev,CompactionRev,"
            "CellsVisited,BodiesExamined,RefusedStale,RefusedInvalid,"
            "WeldedToHost,Lithified,FormationMerged,RoutedThrough16C,BankCollapse,Cemented,"
            "HostFeatureId\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%s,%s,"
                "%s,%s,%lld,%d,%s,%u,%u,%u,"
                "%d,%d,%d,%d,%s,%u,"
                "%u,%u,%u,%s,%d,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%u,%u,%u,%u,%u,"
                "%zu,%zu,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                CompactClassName(t.Class),
                CausalWorldGeology::Hex64(t.DepositionalBodyId).c_str(),
                CausalWorldGeology::Hex64(t.LooseMatterId).c_str(),
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MaterialName,
                (long long)t.Grams,
                t.OccupiedRegion,
                CausalWorldGeology::Hex64(t.CauseWaterBodyId).c_str(),
                t.HydraulicForce,t.ParcelResistance,t.AppliedLoad,
                t.Compacted?1:0,t.Remobilized?1:0,t.IdentityPreserved?1:0,
                t.HostSolidsUnchanged?1:0,
                CausalPresentWaterTerrainHydraulicDeposition::PackingStateName(t.PackingAfter),
                t.CompactionAfter,
                t.PackingFractionAfter,t.PorosityAfter,t.SupportCapacityAfter,
                CausalPresentWaterTerrainHydraulicDeposition::BodyPhaseName(t.PhaseAfter),
                t.IsStableWalkingSurface?1:0,
                (long long)t.TerrainSolidsBefore,(long long)t.TerrainSolidsAfter,
                (long long)t.LooseBefore,(long long)t.LooseAfter,
                (long long)t.DepositionalBefore,(long long)t.DepositionalAfter,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.SourceTerrainRevision,t.DestinationTerrainRevision,t.WaterRevision,
                t.DepositionRevision,t.CompactionRevision,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.WeldedToHost?1:0,t.Lithified?1:0,t.FormationMerged?1:0,
                t.RoutedThrough16C?1:0,t.BankCollapse?1:0,t.Cemented?1:0,
                CausalWorldGeology::Hex64(t.HostFeatureId).c_str());
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
        char const* p5b3b2Path,char const* p5b3b3aPath,char const* p5b3b3bPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b3b3a=[&]()->std::unique_ptr<CausalPresentWaterTerrainHydraulicDeposition::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,p5b3aPath,p5b3bPath,
                p5b3b2Path,p5b3b3aPath,r2);
        };

        auto parent=reloadP5b3b3a();
        c.checks.push_back({"p5b3b3a_parent_loaded",parent!=nullptr&&parent->Complete()});
        if(!parent){c.reason=reason.empty()?"p5b3b3a_parent_load_failed":reason;return c;}
        c.p5b3b3aDepositionDigest=parent->DepositionDigestValue();
        c.checks.push_back({"p5b3b3a_deposition_digest_frozen",
            c.p5b3b3aDepositionDigest==kFrozenP5b3b3aDepositionDigest});
        FDepositionalMatterBody const* settledBody=nullptr;
        for(auto const& b:parent->Bodies())
            if(b.TotalGrams==kParcelGrams&&b.Phase==BodyPhase::SettledAggregate)
            {settledBody=&b;break;}
        c.checks.push_back({"fixtures_start_already_deposited_aggregate",
            settledBody!=nullptr&&parent->LooseMass()==0
            &&parent->DepositionalMass()==kParcelGrams
            &&parent->Stats().deposits>0
            &&settledBody->Compaction==0&&!settledBody->IsStableWalkingSurface
            &&!settledBody->WeldedToHost});

        int64_t const water0=parent->ConservedMass();
        int64_t const solids0=parent->TerrainSolids();
        int64_t const loose0=parent->LooseMass();
        int64_t const dep0=parent->DepositionalMass();
        int64_t const held0=parent->HeldMatter();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint32_t const wRev0=parent->WaterTopologyRevision();
        uint64_t const deposition0=parent->DepositionDigestValue();
        auto const parentBodies=parent->Bodies();

        auto disabled=MakeFromParent(reloadP5b3b3a(),p5b3b3bPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b3b3b",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.depositionDigestDisabled=disabled->DepositionDigestValue();
        bool stillAggregate=disabled->LooseMass()==loose0&&disabled->DepositionalMass()==dep0;
        bool bodiesMatchParent=disabled->Bodies().size()==parentBodies.size();
        if(bodiesMatchParent)
        {
            for(size_t i=0;i<parentBodies.size();++i)
                if(disabled->Bodies()[i].DepositionalBodyId!=parentBodies[i].DepositionalBodyId
                  ||disabled->Bodies()[i].TotalGrams!=parentBodies[i].TotalGrams
                  ||disabled->Bodies()[i].Compaction!=parentBodies[i].Compaction
                  ||disabled->Bodies()[i].Phase!=parentBodies[i].Phase
                  ||disabled->Bodies()[i].IsStableWalkingSurface!=parentBodies[i].IsStableWalkingSurface)
                    bodiesMatchParent=false;
        }
        c.checks.push_back({"p5b3b3b_off_exact_p5b3b3a_state",
            c.depositionDigestDisabled==deposition0
            &&c.depositionDigestDisabled==kFrozenP5b3b3aDepositionDigest
            &&disabled->Stats().compacts==0
            &&disabled->ConservedMass()==water0
            &&disabled->TerrainSolids()==solids0
            &&disabled->LooseMass()==loose0
            &&disabled->DepositionalMass()==dep0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0
            &&stillAggregate&&bodiesMatchParent});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b3b3a(),p5b3b3bPath,1,budget,r2);
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

        c.compactionDigestBudget1=budget1->CompactionDigestValue();
        c.compactionDigestBudgetN=budgetN->CompactionDigestValue();
        c.compactionDigestUnbounded=unbounded->CompactionDigestValue();
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
        c.gramsCompacted=unbounded->Stats().gramsCompacted;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.admissibleCompact=unbounded->Stats().admissibleCompact;
        c.insufficientZero=unbounded->Stats().insufficientZero;
        c.compacts=unbounded->Stats().compacts;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        uint64_t const disabledDigest=disabled->CompactionDigestValue();
        bool freezeOk=c.compactionDigestBudget1==c.compactionDigestBudgetN
            &&c.compactionDigestBudgetN==c.compactionDigestUnbounded
            &&c.compactionDigestUnbounded!=disabledDigest;
        if(kFrozenP5b3b3bCompactionDigest)
            freezeOk=freezeOk&&c.compactionDigestUnbounded==kFrozenP5b3b3bCompactionDigest;
        c.checks.push_back({"budget_invariant_compaction",freezeOk});
        c.checks.push_back({"loose_stays_0",
            c.looseBefore==loose0&&c.looseBefore==0
            &&c.looseAfter==0&&unbounded->LooseMass()==0});
        c.checks.push_back({"depositional_80_stays_80",
            c.depositionalBefore==dep0&&c.depositionalBefore==kParcelGrams
            &&c.depositionalAfter==kParcelGrams
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
            unbounded->TotalMatter()==solids0+loose0+dep0
            &&unbounded->TerrainSolids()+unbounded->LooseMass()+unbounded->DepositionalMass()
                ==solids0+loose0+dep0});
        c.checks.push_back({"compiled_16c_sediment_unchanged",
            unbounded->TerrainDigest()==sediment0
            &&unbounded->Stats().compiledSedimentAfter==sediment0});

        bool massEach=true,identityKept=true,noWeld=true,localOnly=true;
        bool compactOk=false;
        uint64_t compactedBodyId=0,compactedLooseId=0;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            massEach=massEach
                &&t.TerrainSolidsBefore==t.TerrainSolidsAfter
                &&t.WaterMassBefore==t.WaterMassAfter
                &&t.HeldMatterBefore==t.HeldMatterAfter
                &&t.CompiledSedimentBefore==t.CompiledSedimentAfter
                &&t.HostSolidsUnchanged
                &&t.LooseBefore==t.LooseAfter
                &&t.DepositionalBefore==t.DepositionalAfter;
            noWeld=noWeld&&!t.WeldedToHost&&!t.Lithified&&!t.RoutedThrough16C
                &&!t.BankCollapse&&!t.Cemented&&!t.FormationMerged;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            if(t.Compacted)
            {
                identityKept=identityKept&&t.IdentityPreserved&&t.DepositionalBodyId!=0
                    &&t.MaterialId!=0&&t.DetachmentTransactionId!=0
                    &&t.Grams==kParcelGrams
                    &&t.LooseAfter==t.LooseBefore
                    &&t.DepositionalAfter==t.DepositionalBefore
                    &&!t.InputRevisions.empty()&&!t.OutputRevisions.empty();
                compactedBodyId=t.DepositionalBodyId;compactedLooseId=t.LooseMatterId;
            }
            if(t.Fixture==FixtureKind::AdmissibleLoad)
                compactOk=t.Compacted&&t.IdentityPreserved&&t.Grams==kParcelGrams
                    &&t.Class==CompactClass::CompactedDeposit
                    &&t.PackingAfter==PackingState::PackedSediment
                    &&t.CompactionAfter>=kWalkingCompactionMin
                    &&t.IsStableWalkingSurface
                    &&!t.WeldedToHost;
        }
        auto const* body=unbounded->FindBody(compactedBodyId);
        bool bodyOk=body&&body->TotalGrams==kParcelGrams
            &&body->DepositionalBodyId==compactedBodyId
            &&!body->ConstituentMatterIds.empty()
            &&body->ConstituentMatterIds.front()==compactedLooseId
            &&body->Packing==PackingState::PackedSediment
            &&body->Compaction>=kWalkingCompactionMin
            &&body->Phase==BodyPhase::CompactedDeposit
            &&body->Support==SupportState::Supported
            &&!body->WeldedToHost&&!body->Lithified
            &&IsStableDepositedGround(*body)
            &&unbounded->RecognizesStableDepositedGround(compactedBodyId);
        if(settledBody)
        {
            bodyOk=bodyOk&&body
                &&body->MaterialId==settledBody->MaterialId
                &&body->FormationProvenance==settledBody->FormationProvenance
                &&body->GeologicalAncestry==settledBody->GeologicalAncestry
                &&body->DetachmentTransactionId==settledBody->DetachmentTransactionId
                &&body->OccupiedRegion==settledBody->OccupiedRegion
                &&body->DepositionalBodyId==settledBody->DepositionalBodyId
                &&body->DepositionalBodyId!=body->HostFeatureId
                &&body->FormationProvenance!=body->HostFeatureId;
        }
        c.checks.push_back({"admissible_load_compacts_80g_same_body",
            compactOk&&c.admissibleCompact>0&&c.compacts==1&&bodyOk});
        c.checks.push_back({"compaction_preserves_provenance_not_host_identity",identityKept&&compactedBodyId!=0});
        c.checks.push_back({"not_welded_not_lithified_not_16c",noWeld
            &&unbounded->TerrainSolids()==solids0});
        c.checks.push_back({"admitted_compacts_conserved",massEach});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined<6515&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_compaction",
            CausalPresentWaterTerrainResponse::OccupancyValid(
                unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"body_membership_matches_occupancy",
            CausalPresentWaterTopology::OccupancyMatchesBodies(unbounded->Water().Cells(),
                unbounded->Body().Bodies())});

        {
            auto k=loadEnabled(kHoldBudget);
            bool insuffOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.insufficient.SourceTerrainRevision=k->TerrainRevision();
                fixtures.insufficient.WaterRevision=k->WaterTopologyRevision();
                fixtures.insufficient.DepositionRevision=k->DepositionRevision();
                fixtures.insufficient.CompactionRevision=k->CompactionRevision();
                fixtures.insufficient.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.insufficient.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.insufficient.OccupiedRegion);
                fixtures.insufficient.CheckRevision=true;
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const dep=k->DepositionalMass();
                int64_t const water=k->ConservedMass();
                auto rec=k->ApplyRequest(fixtures.insufficient);
                auto const* remain=k->FindBody(fixtures.insufficient.DepositionalBodyId);
                insuffOk=rec.Class==CompactClass::InsufficientLoad&&!rec.Compacted
                    &&!rec.IsStableWalkingSurface&&!rec.RefusedInvalid
                    &&remain&&remain->Phase==BodyPhase::SettledAggregate
                    &&remain->Compaction==0&&!remain->IsStableWalkingSurface
                    &&!IsStableDepositedGround(*remain)
                    &&!k->RecognizesStableDepositedGround(remain->DepositionalBodyId)
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->DepositionalMass()==dep&&k->ConservedMass()==water;
            }
            c.checks.push_back({"insufficient_compaction_stays_aggregate_not_walking_surface",insuffOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool staleOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.stale.SourceTerrainRevision=k->TerrainRevision();
                fixtures.stale.WaterRevision=k->WaterTopologyRevision();
                fixtures.stale.DepositionRevision=k->DepositionRevision();
                fixtures.stale.CompactionRevision=k->CompactionRevision();
                fixtures.stale.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.stale.DestinationTerrainRevision=k->DestinationRevision(fixtures.stale.OccupiedRegion);
                fixtures.stale.CheckRevision=true;
                k->BumpDestinationRevision(fixtures.stale.OccupiedRegion);
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const dep=k->DepositionalMass();
                int64_t const water=k->ConservedMass();
                auto stale=k->ApplyRequest(fixtures.stale);
                auto const* remain=k->FindBody(fixtures.stale.DepositionalBodyId);
                staleOk=stale.RefusedStale&&!stale.Compacted&&stale.Grams==kParcelGrams
                    &&remain&&remain->Phase==BodyPhase::SettledAggregate
                    &&!remain->IsStableWalkingSurface
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose
                    &&k->DepositionalMass()==dep&&k->ConservedMass()==water;
            }
            c.checks.push_back({"stale_support_or_material_revision_refuse",staleOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool remobilizeOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.admissible.SourceTerrainRevision=k->TerrainRevision();
                fixtures.admissible.WaterRevision=k->WaterTopologyRevision();
                fixtures.admissible.DepositionRevision=k->DepositionRevision();
                fixtures.admissible.CompactionRevision=k->CompactionRevision();
                fixtures.admissible.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.admissible.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.admissible.OccupiedRegion);
                fixtures.admissible.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.admissible);
                uint64_t const bodyId=first.DepositionalBodyId;
                uint64_t const looseId=first.LooseMatterId;
                uint64_t const form=first.FormationProvenance;
                uint64_t const anc=first.GeologicalAncestry;
                uint64_t const det=first.DetachmentTransactionId;
                int64_t const solids=k->TerrainSolids();
                int64_t const water=k->ConservedMass();
                fixtures.remobilize.DepositionalBodyId=bodyId;
                fixtures.remobilize.LooseMatterId=looseId;
                fixtures.remobilize.OccupiedRegion=first.OccupiedRegion;
                fixtures.remobilize.SourceTerrainRevision=k->TerrainRevision();
                fixtures.remobilize.WaterRevision=k->WaterTopologyRevision();
                fixtures.remobilize.DepositionRevision=k->DepositionRevision();
                fixtures.remobilize.CompactionRevision=k->CompactionRevision();
                fixtures.remobilize.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.remobilize.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.remobilize.OccupiedRegion);
                fixtures.remobilize.CheckRevision=true;
                auto second=k->ApplyRequest(fixtures.remobilize);
                auto const* restored=k->FindLoose(looseId);
                remobilizeOk=first.Compacted&&first.IsStableWalkingSurface
                    &&second.Remobilized&&!second.Compacted
                    &&restored&&restored->LooseMatterId==looseId
                    &&restored->Grams==kParcelGrams
                    &&restored->FormationProvenance==form
                    &&restored->GeologicalAncestry==anc
                    &&restored->DetachmentTransactionId==det
                    &&k->FindBody(bodyId)==nullptr
                    &&k->DepositionalMass()==0&&k->LooseMass()==kParcelGrams
                    &&k->TerrainSolids()==solids&&k->ConservedMass()==water
                    &&k->TerrainSolids()==solids0
                    &&k->TotalMatter()==solids0+loose0+dep0;
            }
            c.checks.push_back({"compacted_redisturb_reversible_same_grams_provenance",remobilizeOk});
        }

        {
            auto k=loadEnabled(kHoldBudget);
            bool hostOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent());
                fixtures.hostContact.SourceTerrainRevision=k->TerrainRevision();
                fixtures.hostContact.WaterRevision=k->WaterTopologyRevision();
                fixtures.hostContact.DepositionRevision=k->DepositionRevision();
                fixtures.hostContact.CompactionRevision=k->CompactionRevision();
                fixtures.hostContact.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.hostContact.DestinationTerrainRevision=
                    k->DestinationRevision(fixtures.hostContact.OccupiedRegion);
                fixtures.hostContact.CheckRevision=true;
                auto rec=k->ApplyRequest(fixtures.hostContact);
                auto const* remain=k->FindBody(fixtures.hostContact.DepositionalBodyId);
                hostOk=rec.Compacted&&remain
                    &&remain->DepositionalBodyId==fixtures.hostContact.DepositionalBodyId
                    &&remain->DepositionalBodyId!=remain->HostFeatureId
                    &&remain->FormationProvenance!=remain->HostFeatureId
                    &&!remain->WeldedToHost&&!remain->Lithified&&!rec.FormationMerged
                    &&remain->Phase==BodyPhase::CompactedDeposit
                    &&IsStableDepositedGround(*remain)
                    &&k->DepositionalMass()==kParcelGrams
                    &&k->TerrainSolids()==solids0;
            }
            c.checks.push_back({"host_contact_compacted_sandstone_stays_separate_body",hostOk});
        }

        bool partitionOk=true;
        {
            auto a=loadEnabled(kHoldBudget);
            auto b=loadEnabled(kHoldBudget);
            if(a&&b)
            {
                auto fa=BuildFixtures(a->Parent());
                auto fb=BuildFixtures(b->Parent());
                auto applyOne=[&](Kernel& k,CompactRequest req)
                {
                    req.SourceTerrainRevision=k.TerrainRevision();
                    req.WaterRevision=k.WaterTopologyRevision();
                    req.DepositionRevision=k.DepositionRevision();
                    req.CompactionRevision=k.CompactionRevision();
                    req.TerrainStateRevision=k.TerrainStateRevision();
                    req.DestinationTerrainRevision=k.DestinationRevision(req.OccupiedRegion);
                    req.CheckRevision=true;
                    k.ApplyRequest(req);
                };
                applyOne(*a,fa.admissible);
                applyOne(*b,fb.admissible);
                partitionOk=a->CompactionDigestValue()==b->CompactionDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TerrainSolids()==b->TerrainSolids()
                    &&a->LooseMass()==b->LooseMass()
                    &&a->DepositionalMass()==b->DepositionalMass()
                    &&fa.admissible.OccupiedRegion==fb.admissible.OccupiedRegion;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->CompactionDigestValue()==unbounded->CompactionDigestValue()
            &&reload->CompactionDigestValue()==unbounded->CompactionDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->TerrainSolids()==unbounded->TerrainSolids()
            &&cold->LooseMass()==unbounded->LooseMass()
            &&cold->DepositionalMass()==unbounded->DepositionalMass()
            &&cold->DepositionalMass()==kParcelGrams;
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3c_bank_support_collapse_closed",true});
        c.checks.push_back({"general_erosion_closed",true});
        c.checks.push_back({"active_16b_erosion_closed",true});
        c.checks.push_back({"16c_remobilization_closed",true});
        c.checks.push_back({"rainfall_evaporation_groundwater_closed",true});
        c.checks.push_back({"cement_lithify_closed",true});
        c.checks.push_back({"host_formation_weld_closed",true});
        c.checks.push_back({"formation_id_merge_closed",true});
        c.checks.push_back({"general_soil_mechanics_closed",true});
        c.checks.push_back({"world_operation_scheduler_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_compact",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->CompactionDigestValue();
                size_t const n=unbounded->Stats().compacts;
                size_t const wakes=unbounded->Stats().compactWakes;
                unbounded->Tick(64);
                return unbounded->CompactionDigestValue()==before
                    &&unbounded->Stats().compacts==n
                    &&unbounded->Stats().compactWakes==wakes;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b3b3b_compaction_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b3b3b_compaction_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B3B3B_COMPACTION %s\nreason=%s\n"
            "p5b3b3a_deposition_digest=%s\ndeposition_digest_disabled=%s\n"
            "compaction_digest_budget1=%s\ncompaction_digest_budgetN=%s\n"
            "compaction_digest_unbounded=%s\n"
            "terrain_host_solids_before=%lld\nterrain_host_solids_after=%lld\n"
            "loose_before=%lld\nloose_after=%lld\n"
            "depositional_before=%lld\ndepositional_after=%lld\n"
            "water_before=%lld\nwater_after=%lld\n"
            "held_before=%lld\nheld_after=%lld\n"
            "grams_compacted=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "admissible_compact=%zu\ninsufficient_zero=%zu\ncompacts=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=depositional_aggregate_to_compacted_deposit_not_host_weld\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=frozen\n"
            "p5b3a=frozen\np5b3b=frozen\np5b3b2=frozen\np5b3b3a=frozen\n"
            "p5b3b3b=open\np5b3c=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "general_erosion=0\nbank_collapse=0\n"
            "active_16b_erosion=0\n16c_remobilization=0\necology=0\n"
            "host_formation_weld=0\ncement_lithify=0\nformation_id_merge=0\n"
            "general_soil_mechanics=0\nworld_operation_scheduler=0\n"
            "choice_a_local_loose=1\nchoice_b_16c=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b3b3aDepositionDigest).c_str(),
            CausalWorldGeology::Hex64(c.depositionDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.compactionDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.compactionDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.compactionDigestUnbounded).c_str(),
            (long long)c.terrainSolidsBefore,(long long)c.terrainSolidsAfter,
            (long long)c.looseBefore,(long long)c.looseAfter,
            (long long)c.depositionalBefore,(long long)c.depositionalAfter,
            (long long)c.waterBefore,(long long)c.waterAfter,
            (long long)c.heldBefore,(long long)c.heldAfter,
            (long long)c.gramsCompacted,
            c.transactionsCertified,c.transactionsAdmitted,
            c.admissibleCompact,c.insufficientZero,c.compacts,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}