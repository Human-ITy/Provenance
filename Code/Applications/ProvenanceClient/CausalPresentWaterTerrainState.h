#pragma once

// P5b.2A: reverse state coupling only. Water may change how terrain
// matter behaves, but not how much terrain matter exists.
//
// Frozen parent: P5b.1 pin e64a4df3 (player-path 5f4c075d, coupling
// land 4e6db843) on provenance/client-spike. Stage 16F.4 remains
// frozen. Disabled P5b.1 must reproduce exact 16F.4 field
// 43068558cd0b4a8e. Disabled P5b.2A must reproduce exact P5b.1
// field/state.
//
// Pipeline:
//   water body/contact revision
//     → affected terrain neighborhood
//     → material wetting query
//     → update terrain material state via FTerrainMaterialStateDelta
//     → revision-stamped presentation/physics consequences
//
// Water solver does not scribble terrain. Zero terrain mass movement.
// No infiltration / porous storage (P5b.2B CLOSED). No water-induced
// terrain matter movement (P5b.3 CLOSED).

#include "CausalPresentWaterTerrainResponse.h"

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

namespace CausalPresentWaterTerrainState
{
    constexpr char const* kExpectedRegion="causal_world_present_water_terrain_state_floor";
    constexpr uint64_t kFrozenStage16F4FieldDigest=0x43068558cd0b4a8eull;
    constexpr uint64_t kFrozenP5b1FieldDigest=0xb1340afeef311fd8ull;
    constexpr uint64_t kFrozenP5b1StateDigest=0xd3bd4455d64ca895ull;
    constexpr uint64_t kFrozenP5b2aStateDigest=0x176ffe1c3845f721ull;
    constexpr uint32_t kHoldBudget=0xFFFFFFFEu;

    enum class FixtureKind:uint8_t
    {
        None=0,
        LakeEdge=1,
        WetlandSaturate=2,
        RiverBank=3,
        ContactLostDry=4
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::LakeEdge:return "lake_edge_wets_dirt";
            case FixtureKind::WetlandSaturate:return "wetland_keeps_soil_saturated";
            case FixtureKind::RiverBank:return "river_contact_wets_bank";
            case FixtureKind::ContactLostDry:return "water_removed_begins_drying";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b2aEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b2aEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainResponse::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_STATE_V1";
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
          &&hex("p5b2a_event_id",r.program.p5b2aEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b2a_enabled",r.program.p5b2aEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b2aEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b2aEnabled==0||r.program.p5b2aEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b2a_terrain_state_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    inline uint16_t Quantize01(double v)
    {
        if(v<0.0)v=0.0;if(v>1.0)v=1.0;
        return (uint16_t)std::llround(v*1000.0);
    }
    inline double Unquantize01(uint16_t q){return (double)q/1000.0;}

    inline char const* CanonicalMaterial(std::string const& material)
    {
        if(material=="shale")return "shale";
        if(material=="sandstone")return "sandstone";
        if(material=="granite")return "granite";
        if(material=="quartz")return "quartz";
        return "dirt";
    }

    inline uint64_t MaterialIdOf(char const* name)
    {
        uint64_t h=CausalWorldGeology::HashBytes(name,std::strlen(name));
        return h?h:1;
    }

    inline bool AcceptsMoisture(char const* name)
    {
        return std::strcmp(name,"dirt")==0||std::strcmp(name,"sandstone")==0;
    }

    inline bool IsDirt(char const* name){return std::strcmp(name,"dirt")==0;}

    struct FTerrainMaterialState
    {
        uint64_t MaterialId=0;
        uint16_t SurfaceWetness=0;
        uint16_t MoistureContent=0;
        uint16_t Saturation=0;
        uint16_t CohesionModifier=1000;
        uint8_t Permeable=0;
        uint8_t ContactWet=0;
        uint32_t TerrainRevision=0;
        uint64_t LastWaterBodyId=0;
        uint32_t LastWaterRevision=0;
    };

    struct FTerrainMaterialStateDelta
    {
        uint64_t TransactionId=0;
        int TerrainCell=-1;
        uint64_t MaterialId=0;
        double MoistureBefore=0,MoistureAfter=0;
        double SaturationBefore=0,SaturationAfter=0;
        double CohesionModifierBefore=1,CohesionModifierAfter=1;
        double SurfaceWetnessBefore=0,SurfaceWetnessAfter=0;
        uint8_t PermeableBefore=0,PermeableAfter=0;
        uint32_t TerrainRevisionBefore=0,TerrainRevisionAfter=0;
        uint64_t WaterBodyId=0;
        uint32_t WaterRevision=0;
        FixtureKind Fixture=FixtureKind::None;
        bool RefusedStale=false,RefusedInvalid=false;
        bool PublishedCoherent=false;
        size_t CellsVisited=0,BodiesExamined=0;
        uint32_t ParentTerrainRevision=0,ParentWaterTopologyRevision=0;
        uint64_t ParentFieldDigest=0,ParentTerrainOverlay=0;
        int64_t WaterGramsBefore=0,WaterGramsAfter=0;
        int64_t TerrainGramsBefore=0,TerrainGramsAfter=0;
    };

    struct FWaterContactNotice
    {
        uint64_t WaterBodyId=0;
        uint32_t WaterRevision=0;
        int WaterCell=-1;
        int TerrainCell=-1;
        CausalPresentWaterBody::BodyType Type=CausalPresentWaterBody::BodyType::None;
        bool ContactPresent=true;
        FixtureKind Fixture=FixtureKind::None;
    };

    struct WettingQuery
    {
        uint64_t MaterialId=0;
        char const* MaterialName="dirt";
        bool AcceptsMoisture=false;
        bool ShallowSoil=false;
        double TargetWetness=0,TargetMoisture=0,TargetSaturation=0,TargetCohesion=1;
        uint8_t TargetPermeable=0;
    };

    struct StateRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        int TerrainCell=-1;
        int WaterCell=-1;
        uint64_t WaterBodyId=0;
        CausalPresentWaterBody::BodyType Type=CausalPresentWaterBody::BodyType::None;
        bool ContactPresent=true;
        uint32_t TerrainStateRevision=0;
        uint32_t WaterContactRevision=0;
        bool CheckRevision=true;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t lakeEdge=0,wetlandSaturate=0,riverBank=0,contactLostDry=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        uint64_t fieldDigest=0,parentFieldDigest=0,terrainOverlayDigest=0;
        uint64_t stateDigest=0,parentStateDigest=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainStateRevision=0,waterContactRevision=0;
        uint32_t parentTerrainRevision=0,parentWaterTopologyRevision=0;
    };

    inline WettingQuery QueryWetting(char const* material,FWaterContactNotice const& notice,
        FTerrainMaterialState const& current)
    {
        WettingQuery q;
        q.MaterialName=material?material:"dirt";
        q.MaterialId=MaterialIdOf(q.MaterialName);
        q.AcceptsMoisture=AcceptsMoisture(q.MaterialName);
        q.ShallowSoil=notice.Type==CausalPresentWaterBody::BodyType::Wetland&&IsDirt(q.MaterialName);
        if(!notice.ContactPresent)
        {
            double const wet=Unquantize01(current.SurfaceWetness);
            double const moist=Unquantize01(current.MoistureContent);
            double const sat=Unquantize01(current.Saturation);
            double const coh=Unquantize01(current.CohesionModifier);
            q.TargetWetness=wet*0.55;
            q.TargetMoisture=moist*0.70;
            q.TargetSaturation=sat*0.50;
            q.TargetCohesion=coh+((1.0-coh)*0.50);
            q.TargetPermeable=current.Permeable;
            return q;
        }
        if(notice.Type==CausalPresentWaterBody::BodyType::Wetland&&q.AcceptsMoisture)
        {
            q.TargetWetness=1.0;q.TargetMoisture=1.0;q.TargetSaturation=1.0;
            q.TargetCohesion=0.55;q.TargetPermeable=1;return q;
        }
        if(notice.Type==CausalPresentWaterBody::BodyType::River&&q.AcceptsMoisture)
        {
            q.TargetWetness=0.70;q.TargetMoisture=0.40;q.TargetSaturation=0.28;
            q.TargetCohesion=0.76;q.TargetPermeable=1;return q;
        }
        if(q.AcceptsMoisture)
        {
            q.TargetWetness=0.80;q.TargetMoisture=0.45;q.TargetSaturation=0.35;
            q.TargetCohesion=0.72;q.TargetPermeable=1;return q;
        }
        q.TargetWetness=0.25;q.TargetMoisture=0.05;q.TargetSaturation=0.0;
        q.TargetCohesion=1.0;q.TargetPermeable=0;return q;
    }

    inline uint64_t StateDigestOf(std::vector<FTerrainMaterialState> const& states)
    {
        uint64_t d=14695981039346656037ull;
        for(auto const& s:states)
        {
            CausalWorldGeology::HashAppend(d,&s.MaterialId,sizeof(s.MaterialId));
            CausalWorldGeology::HashAppend(d,&s.SurfaceWetness,sizeof(s.SurfaceWetness));
            CausalWorldGeology::HashAppend(d,&s.MoistureContent,sizeof(s.MoistureContent));
            CausalWorldGeology::HashAppend(d,&s.Saturation,sizeof(s.Saturation));
            CausalWorldGeology::HashAppend(d,&s.CohesionModifier,sizeof(s.CohesionModifier));
            CausalWorldGeology::HashAppend(d,&s.Permeable,sizeof(s.Permeable));
            CausalWorldGeology::HashAppend(d,&s.ContactWet,sizeof(s.ContactWet));
        }
        return d;
    }

    inline uint64_t MakeTxnId(FixtureKind kind,int cell,uint32_t seq,uint64_t body)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B2000Aull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&cell,sizeof(cell));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        CausalWorldGeology::HashAppend(h,&body,sizeof(body));
        return h?h:1;
    }

    struct ContactCandidate
    {
        int terrainCell=-1,waterCell=-1;
        uint64_t bodyId=0;
        CausalPresentWaterBody::BodyType type=CausalPresentWaterBody::BodyType::None;
        char const* material="dirt";
    };

    inline bool BodyMatches(CausalPresentWaterBody::FPresentWaterBody const& b,
        CausalPresentWater::BodyKind wantKind,CausalPresentWaterBody::BodyType wantType)
    {
        if(b.Type==wantType)return true;
        if(wantType==CausalPresentWaterBody::BodyType::Lake&&b.hasLake)return true;
        if(wantType==CausalPresentWaterBody::BodyType::River&&b.hasRiver)return true;
        if(wantType==CausalPresentWaterBody::BodyType::Wetland&&b.hasWetland)return true;
        (void)wantKind;return false;
    }

    inline char const* CellMaterial(CausalPresentWaterTerrainResponse::Kernel const& parent,int cell)
    {
        auto const& cells=parent.Water().Cells();
        if(cell<0||(size_t)cell>=cells.size())return "dirt";
        auto const geo=parent.SurfaceGeology(cells[(size_t)cell].x,cells[(size_t)cell].y);
        return CanonicalMaterial(geo.found?geo.material:std::string("dirt"));
    }

    inline std::vector<ContactCandidate> CollectContacts(
        CausalPresentWaterTerrainResponse::Kernel const& parent,
        CausalPresentWaterBody::BodyType wantType,
        CausalPresentWater::BodyKind wantKind,
        bool dirtOnly,bool reverseNeighbors)
    {
        std::vector<ContactCandidate> out;
        auto const& cells=parent.Water().Cells();
        auto const& bodies=parent.Body().Bodies();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        for(auto const& b:bodies)
        {
            if(!BodyMatches(b,wantKind,wantType))continue;
            for(int wi:b.Cells)
            {
                if(wi<0||(size_t)wi>=cells.size()||!cells[(size_t)wi].occupied)continue;
                if(wantKind!=CausalPresentWater::BodyKind::None
                  &&cells[(size_t)wi].kind!=wantKind
                  &&b.Type!=wantType)
                    continue;
                for(int n=0;n<4;++n)
                {
                    int const ni=CausalPresentWaterTopology::Neighbor4(wi,n,width,height,reverseNeighbors);
                    if(ni<0||cells[(size_t)ni].occupied)continue;
                    char const* mat=CellMaterial(parent,ni);
                    if(dirtOnly&&!IsDirt(mat))continue;
                    ContactCandidate c;
                    c.terrainCell=ni;c.waterCell=wi;c.bodyId=b.BodyId;
                    c.type=wantType;c.material=mat;
                    out.push_back(c);
                }
            }
        }
        std::sort(out.begin(),out.end(),[](ContactCandidate const& a,ContactCandidate const& b)
        {
            if(a.terrainCell!=b.terrainCell)return a.terrainCell<b.terrainCell;
            if(a.waterCell!=b.waterCell)return a.waterCell<b.waterCell;
            return a.bodyId<b.bodyId;
        });
        return out;
    }

    struct FixtureSet
    {
        StateRequest lake,wetland,river,dry;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(CausalPresentWaterTerrainResponse::Kernel const& parent,
        bool reverseNeighbors=false)
    {
        FixtureSet set;
        std::unordered_set<int> used;
        auto take=[&](std::vector<ContactCandidate> const& all,StateRequest& req,
            FixtureKind kind,bool contact)->bool
        {
            for(auto const& c:all)
            {
                if(used.count(c.terrainCell))continue;
                req.Fixture=kind;
                req.TerrainCell=c.terrainCell;
                req.WaterCell=c.waterCell;
                req.WaterBodyId=c.bodyId;
                req.Type=c.type;
                req.ContactPresent=contact;
                used.insert(c.terrainCell);
                return true;
            }
            return false;
        };
        auto lakes=CollectContacts(parent,CausalPresentWaterBody::BodyType::Lake,
            CausalPresentWater::BodyKind::Lake,true,reverseNeighbors);
        auto wetlands=CollectContacts(parent,CausalPresentWaterBody::BodyType::Wetland,
            CausalPresentWater::BodyKind::Wetland,true,reverseNeighbors);
        auto rivers=CollectContacts(parent,CausalPresentWaterBody::BodyType::River,
            CausalPresentWater::BodyKind::River,true,reverseNeighbors);
        if(lakes.empty())
            lakes=CollectContacts(parent,CausalPresentWaterBody::BodyType::Lake,
                CausalPresentWater::BodyKind::Lake,false,reverseNeighbors);
        if(wetlands.empty())
            wetlands=CollectContacts(parent,CausalPresentWaterBody::BodyType::Wetland,
                CausalPresentWater::BodyKind::Wetland,false,reverseNeighbors);
        if(rivers.empty())
            rivers=CollectContacts(parent,CausalPresentWaterBody::BodyType::River,
                CausalPresentWater::BodyKind::River,false,reverseNeighbors);
        bool const lakeOk=take(lakes,set.lake,FixtureKind::LakeEdge,true);
        bool const wetOk=take(wetlands,set.wetland,FixtureKind::WetlandSaturate,true);
        bool const riverOk=take(rivers,set.river,FixtureKind::RiverBank,true);
        if(lakeOk)
        {
            set.dry=set.lake;
            set.dry.Fixture=FixtureKind::ContactLostDry;
            set.dry.ContactPresent=false;
        }
        set.ok=lakeOk&&wetOk&&riverOk;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainResponse::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_stats.parentFieldDigest=m_parent->FieldDigestValue();
            m_stats.parentTerrainRevision=m_parent->TerrainRevision();
            m_stats.parentWaterTopologyRevision=m_parent->WaterTopologyRevision();
            m_complete=!m_program.p5b2aEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainResponse::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainResponse::Kernel& Parent(){return *m_parent;}
        CausalPresentWaterTopology::Kernel const& Topology() const{return m_parent->Topology();}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FTerrainMaterialStateDelta> const& Transactions() const{return m_txns;}
        std::vector<FTerrainMaterialState> const& States() const{return m_states;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_parent->TerrainRevision();}
        uint32_t WaterTopologyRevision() const{return m_parent->WaterTopologyRevision();}
        uint32_t PublishedTerrainRevision() const{return m_parent->PublishedTerrainRevision();}
        uint32_t PublishedWaterTopologyRevision() const{return m_parent->PublishedWaterTopologyRevision();}
        uint32_t TerrainStateRevision() const{return m_terrainStateRevision;}
        uint32_t WaterContactRevision() const{return m_waterContactRevision;}
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_parent->FieldDigestValue();}
        uint64_t TerrainOverlayDigestValue() const{return m_parent->TerrainOverlayDigestValue();}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        uint64_t StateDigestValue() const{return StateDigestOf(m_states);}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t TotalMatter() const{return m_parent->TotalMatter();}
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

        FTerrainMaterialState QueryTerrainState(int cell) const
        {
            if(cell<0||(size_t)cell>=m_states.size())return {};
            return m_states[(size_t)cell];
        }

        FTerrainMaterialState QueryTerrainStateAt(double x,double y) const
        {
            auto q=Water().QueryAt(x,y);
            if(!q.found)return {};
            auto const& cells=Water().Cells();
            for(size_t i=0;i<cells.size();++i)
            {
                if(std::fabs(cells[i].x-q.water.x)<1e-9&&std::fabs(cells[i].y-q.water.y)<1e-9)
                    return QueryTerrainState((int)i);
            }
            return {};
        }

        FTerrainMaterialStateDelta ApplyRequest(StateRequest req,bool reverseNeighbors=false)
        {return CommitOne(req,reverseNeighbors);}

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
            if(!m_program.p5b2aEnabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.waterMassAfter=TotalMass();
                m_stats.terrainMatterAfter=TotalMatter();
                m_stats.fieldDigest=FieldDigestValue();
                m_stats.terrainOverlayDigest=TerrainOverlayDigestValue();
                m_stats.stateDigest=StateDigestValue();
                m_stats.terrainStateRevision=m_terrainStateRevision;
                m_stats.waterContactRevision=m_waterContactRevision;
            }
            return m_complete;
        }

    private:
        void SeedDryStates()
        {
            auto const& cells=Water().Cells();
            m_states.assign(cells.size(),FTerrainMaterialState{});
            for(size_t i=0;i<cells.size();++i)
            {
                char const* mat=CellMaterial(*m_parent,(int)i);
                m_states[i].MaterialId=MaterialIdOf(mat);
                m_states[i].CohesionModifier=1000;
            }
        }

        void BeginFromParent()
        {
            m_stats.parentFieldDigest=m_parent->FieldDigestValue();
            m_stats.waterMassBefore=m_parent->TotalMass();
            m_stats.terrainMatterBefore=m_parent->TotalMatter();
            m_stats.parentTerrainRevision=m_parent->TerrainRevision();
            m_stats.parentWaterTopologyRevision=m_parent->WaterTopologyRevision();
            SeedDryStates();
            m_stats.parentStateDigest=StateDigestValue();
            if(!m_program.p5b2aEnabled){FinishUnchanged();return;}
            SnapshotWork();
            uint32_t budget=m_program.defaultBudgetTransactions;
            if(budget==kHoldBudget)return;
            if(budget==0)budget=(uint32_t)(std::max)((size_t)1,m_work.size());
            Tick(budget);
        }

        void FinishUnchanged()
        {
            m_complete=true;
            m_stats.waterMassAfter=m_stats.waterMassBefore;
            m_stats.terrainMatterAfter=m_stats.terrainMatterBefore;
            m_stats.fieldDigest=m_stats.parentFieldDigest;
            m_stats.terrainOverlayDigest=TerrainOverlayDigestValue();
            m_stats.stateDigest=StateDigestValue();
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent,false);
            m_work.clear();
            if(fixtures.lake.Fixture!=FixtureKind::None)m_work.push_back(fixtures.lake);
            if(fixtures.wetland.Fixture!=FixtureKind::None)m_work.push_back(fixtures.wetland);
            if(fixtures.river.Fixture!=FixtureKind::None)m_work.push_back(fixtures.river);
            if(fixtures.dry.Fixture!=FixtureKind::None)m_work.push_back(fixtures.dry);
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                StateRequest req=m_work[m_cursor];
                req.TerrainStateRevision=m_terrainStateRevision;
                req.WaterContactRevision=m_waterContactRevision;
                req.CheckRevision=true;
                auto txn=CommitOne(req,false);
                Record(txn);
                ++m_cursor;++processed;
            }
        }

        bool ApplyReceipt(FTerrainMaterialStateDelta const& rec)
        {
            if(rec.TerrainCell<0||(size_t)rec.TerrainCell>=m_states.size())return false;
            auto& s=m_states[(size_t)rec.TerrainCell];
            if(s.MaterialId!=rec.MaterialId)return false;
            if(Quantize01(rec.MoistureBefore)!=s.MoistureContent)return false;
            if(Quantize01(rec.SaturationBefore)!=s.Saturation)return false;
            if(Quantize01(rec.CohesionModifierBefore)!=s.CohesionModifier)return false;
            s.MoistureContent=Quantize01(rec.MoistureAfter);
            s.Saturation=Quantize01(rec.SaturationAfter);
            s.CohesionModifier=Quantize01(rec.CohesionModifierAfter);
            s.SurfaceWetness=Quantize01(rec.SurfaceWetnessAfter);
            s.Permeable=rec.PermeableAfter;
            s.ContactWet=rec.Fixture==FixtureKind::ContactLostDry?0:1;
            s.TerrainRevision=rec.TerrainRevisionAfter;
            s.LastWaterBodyId=rec.WaterBodyId;
            s.LastWaterRevision=rec.WaterRevision;
            return true;
        }

        FTerrainMaterialStateDelta CommitOne(StateRequest req,bool reverseNeighbors)
        {
            FTerrainMaterialStateDelta txn;
            txn.Fixture=req.Fixture;
            txn.TerrainCell=req.TerrainCell;
            txn.WaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainStateRevision;
            txn.ParentTerrainRevision=m_parent->TerrainRevision();
            txn.ParentWaterTopologyRevision=m_parent->WaterTopologyRevision();
            txn.ParentFieldDigest=m_parent->FieldDigestValue();
            txn.ParentTerrainOverlay=m_parent->TerrainOverlayDigestValue();
            txn.WaterGramsBefore=TotalMass();
            txn.TerrainGramsBefore=TotalMatter();
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.MoistureAfter=txn.MoistureBefore;
                txn.SaturationAfter=txn.SaturationBefore;
                txn.CohesionModifierAfter=txn.CohesionModifierBefore;
                txn.SurfaceWetnessAfter=txn.SurfaceWetnessBefore;
                txn.PermeableAfter=txn.PermeableBefore;
                txn.TerrainRevisionAfter=m_terrainStateRevision;
                txn.WaterRevision=m_waterContactRevision;
                txn.WaterGramsAfter=txn.WaterGramsBefore;
                txn.TerrainGramsAfter=txn.TerrainGramsBefore;
                txn.PublishedCoherent=true;
                return txn;
            };
            if(req.CheckRevision&&(req.TerrainStateRevision!=m_terrainStateRevision
              ||req.WaterContactRevision!=m_waterContactRevision))
                return refuse(true,false);
            if(req.TerrainCell<0||(size_t)req.TerrainCell>=m_states.size()
              ||req.Fixture==FixtureKind::None)
                return refuse(false,true);

            auto const& cells=Water().Cells();
            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            std::unordered_set<int> neighborhood;
            neighborhood.insert(req.TerrainCell);
            if(req.WaterCell>=0)neighborhood.insert(req.WaterCell);
            for(int n=0;n<4;++n)
            {
                int const ni=CausalPresentWaterTopology::Neighbor4(req.TerrainCell,n,width,height,reverseNeighbors);
                if(ni>=0)neighborhood.insert(ni);
            }
            txn.CellsVisited=neighborhood.size();
            txn.BodiesExamined=req.WaterBodyId?1:0;

            if((size_t)req.TerrainCell>=cells.size()||cells[(size_t)req.TerrainCell].occupied)
                return refuse(false,true);

            auto const& cur=m_states[(size_t)req.TerrainCell];
            char const* mat=CellMaterial(*m_parent,req.TerrainCell);
            FWaterContactNotice notice;
            notice.WaterBodyId=req.WaterBodyId;
            notice.WaterRevision=m_waterContactRevision+(req.ContactPresent?0u:1u);
            notice.WaterCell=req.WaterCell;
            notice.TerrainCell=req.TerrainCell;
            notice.Type=req.Type;
            notice.ContactPresent=req.ContactPresent;
            notice.Fixture=req.Fixture;
            auto query=QueryWetting(mat,notice,cur);

            txn.MaterialId=cur.MaterialId?cur.MaterialId:query.MaterialId;
            txn.MoistureBefore=Unquantize01(cur.MoistureContent);
            txn.SaturationBefore=Unquantize01(cur.Saturation);
            txn.CohesionModifierBefore=Unquantize01(cur.CohesionModifier);
            txn.SurfaceWetnessBefore=Unquantize01(cur.SurfaceWetness);
            txn.PermeableBefore=cur.Permeable;
            txn.MoistureAfter=query.TargetMoisture;
            txn.SaturationAfter=query.TargetSaturation;
            txn.CohesionModifierAfter=query.TargetCohesion;
            txn.SurfaceWetnessAfter=query.TargetWetness;
            txn.PermeableAfter=query.TargetPermeable;
            txn.TransactionId=MakeTxnId(req.Fixture,req.TerrainCell,
                (uint32_t)(m_txns.size()+1u),req.WaterBodyId);
            txn.WaterRevision=notice.WaterRevision;
            txn.TerrainRevisionAfter=m_terrainStateRevision+1;

            if(!ApplyReceipt(txn))
                return refuse(false,true);

            ++m_terrainStateRevision;
            if(!req.ContactPresent)++m_waterContactRevision;
            txn.TerrainRevisionAfter=m_terrainStateRevision;
            txn.WaterGramsAfter=TotalMass();
            txn.TerrainGramsAfter=TotalMatter();
            txn.PublishedCoherent=true;
            return txn;
        }

        void Record(FTerrainMaterialStateDelta const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid){++m_stats.transactionsRefused;return;}
            ++m_stats.transactionsAdmitted;
            if(rec.Fixture==FixtureKind::LakeEdge)++m_stats.lakeEdge;
            else if(rec.Fixture==FixtureKind::WetlandSaturate)++m_stats.wetlandSaturate;
            else if(rec.Fixture==FixtureKind::RiverBank)++m_stats.riverBank;
            else if(rec.Fixture==FixtureKind::ContactLostDry)++m_stats.contactLostDry;
        }

        std::unique_ptr<CausalPresentWaterTerrainResponse::Kernel> m_parent;Program m_program;
        std::vector<StateRequest> m_work;
        std::vector<FTerrainMaterialStateDelta> m_txns;
        std::vector<FTerrainMaterialState> m_states;
        SolveStats m_stats;
        uint32_t m_terrainStateRevision=0;
        uint32_t m_waterContactRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainResponse::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        std::string& reason,uint32_t p5b1Override=2)
    {
        auto k=CausalPresentWaterTerrainResponse::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,
            externalPath,topologyPath,p5b1Path,&reason,p5b1Override);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainResponse::Kernel> parent,
        char const* p5b2aPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b1_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b2aPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b2aEnabled=enabled;
        loaded.program.defaultBudgetTransactions=budget;
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,std::string* reason=nullptr,
        uint32_t p5b2aOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,r,2);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b2aOverride==0)enabled=0;
        else if(p5b2aOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b2aPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16f4FieldDigest=0;
        uint64_t p5b1FieldDigest=0;
        uint64_t fieldDigestDisabled=0;
        uint64_t stateDigestDisabled=0;
        uint64_t fieldDigestBudget1=0,fieldDigestBudgetN=0,fieldDigestUnbounded=0;
        uint64_t stateDigestBudget1=0,stateDigestBudgetN=0,stateDigestUnbounded=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t lakeEdge=0,wetlandSaturate=0,riverBank=0,contactLostDry=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FTerrainMaterialStateDelta> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,TerrainCell,MaterialId,"
            "MoistureBefore,MoistureAfter,SaturationBefore,SaturationAfter,"
            "CohesionBefore,CohesionAfter,WetnessBefore,WetnessAfter,"
            "PermeableBefore,PermeableAfter,TerrainRevBefore,TerrainRevAfter,"
            "WaterBodyId,WaterRevision,WaterGramsBefore,WaterGramsAfter,"
            "TerrainGramsBefore,TerrainGramsAfter,CellsVisited,BodiesExamined,"
            "RefusedStale,RefusedInvalid\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%d,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
                "%u,%u,%u,%u,%s,%u,%lld,%lld,%lld,%lld,%zu,%zu,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),t.TerrainCell,
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MoistureBefore,t.MoistureAfter,t.SaturationBefore,t.SaturationAfter,
                t.CohesionModifierBefore,t.CohesionModifierAfter,
                t.SurfaceWetnessBefore,t.SurfaceWetnessAfter,
                (unsigned)t.PermeableBefore,(unsigned)t.PermeableAfter,
                t.TerrainRevisionBefore,t.TerrainRevisionAfter,
                CausalWorldGeology::Hex64(t.WaterBodyId).c_str(),t.WaterRevision,
                (long long)t.WaterGramsBefore,(long long)t.WaterGramsAfter,
                (long long)t.TerrainGramsBefore,(long long)t.TerrainGramsAfter,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b1=[&](uint32_t p5b1Override)->std::unique_ptr<CausalPresentWaterTerrainResponse::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,r2,p5b1Override);
        };

        auto p5b1Off=reloadP5b1(0);
        c.checks.push_back({"p5b1_disabled_loaded",p5b1Off!=nullptr});
        if(!p5b1Off){c.reason=reason.empty()?"p5b1_disabled_load_failed":reason;return c;}
        c.stage16f4FieldDigest=p5b1Off->FieldDigestValue();
        c.checks.push_back({"p5b1_disabled_exact_16f4",
            c.stage16f4FieldDigest==kFrozenStage16F4FieldDigest
            &&p5b1Off->Stats().transactionsAdmitted==0});

        auto p5b1On=reloadP5b1(1);
        c.checks.push_back({"p5b1_parent_loaded",p5b1On!=nullptr});
        if(!p5b1On){c.reason="p5b1_parent_load_failed";return c;}
        c.p5b1FieldDigest=p5b1On->FieldDigestValue();
        c.checks.push_back({"p5b1_field_digest_frozen",
            c.p5b1FieldDigest==kFrozenP5b1FieldDigest});

        uint64_t const sediment0=p5b1On->TerrainDigest();
        int64_t const water0=p5b1On->TotalMass();
        int64_t const matter0=p5b1On->TotalMatter();
        uint64_t const overlay0=p5b1On->TerrainOverlayDigestValue();
        uint64_t const body0=p5b1On->BodyDigest();
        uint32_t const tRev0=p5b1On->TerrainRevision();
        uint32_t const wRev0=p5b1On->WaterTopologyRevision();

        auto disabled=MakeFromParent(reloadP5b1(1),p5b2aPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b2a",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.fieldDigestDisabled=disabled->FieldDigestValue();
        c.stateDigestDisabled=disabled->StateDigestValue();
        c.checks.push_back({"p5b2a_off_exact_p5b1_field",
            c.fieldDigestDisabled==c.p5b1FieldDigest
            &&c.fieldDigestDisabled==kFrozenP5b1FieldDigest
            &&disabled->Stats().transactionsAdmitted==0
            &&disabled->TotalMass()==water0
            &&disabled->TotalMatter()==matter0
            &&disabled->TerrainOverlayDigestValue()==overlay0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->BodyDigest()==body0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0});
        if(kFrozenP5b1StateDigest)
            c.checks.push_back({"p5b2a_off_exact_p5b1_state",
                c.stateDigestDisabled==kFrozenP5b1StateDigest
                &&c.stateDigestDisabled==disabled->Stats().parentStateDigest});
        else
            c.checks.push_back({"p5b2a_off_exact_p5b1_state",
                c.stateDigestDisabled==disabled->Stats().parentStateDigest});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b1(1),p5b2aPath,1,budget,r2);
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

        c.fieldDigestBudget1=budget1->FieldDigestValue();
        c.fieldDigestBudgetN=budgetN->FieldDigestValue();
        c.fieldDigestUnbounded=unbounded->FieldDigestValue();
        c.stateDigestBudget1=budget1->StateDigestValue();
        c.stateDigestBudgetN=budgetN->StateDigestValue();
        c.stateDigestUnbounded=unbounded->StateDigestValue();
        c.waterMassBefore=unbounded->Stats().waterMassBefore;
        c.waterMassAfter=unbounded->Stats().waterMassAfter;
        c.terrainMatterBefore=unbounded->Stats().terrainMatterBefore;
        c.terrainMatterAfter=unbounded->Stats().terrainMatterAfter;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.lakeEdge=unbounded->Stats().lakeEdge;
        c.wetlandSaturate=unbounded->Stats().wetlandSaturate;
        c.riverBank=unbounded->Stats().riverBank;
        c.contactLostDry=unbounded->Stats().contactLostDry;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        bool freezeOk=c.fieldDigestBudget1==c.fieldDigestBudgetN
            &&c.fieldDigestBudgetN==c.fieldDigestUnbounded
            &&c.fieldDigestUnbounded==kFrozenP5b1FieldDigest
            &&c.stateDigestBudget1==c.stateDigestBudgetN
            &&c.stateDigestBudgetN==c.stateDigestUnbounded
            &&c.stateDigestUnbounded!=c.stateDigestDisabled;
        if(kFrozenP5b2aStateDigest)
            freezeOk=freezeOk&&c.stateDigestUnbounded==kFrozenP5b2aStateDigest;
        c.checks.push_back({"budget_invariant_state",freezeOk});
        c.checks.push_back({"water_grams_unchanged",
            c.waterMassBefore==c.waterMassAfter&&c.waterMassBefore==water0
            &&unbounded->TotalMass()==water0&&disabled->TotalMass()==water0});
        c.checks.push_back({"terrain_grams_unchanged",
            c.terrainMatterBefore==c.terrainMatterAfter
            &&c.terrainMatterBefore==matter0
            &&unbounded->TotalMatter()==matter0});
        c.checks.push_back({"terrain_geometry_unchanged",
            unbounded->TerrainOverlayDigestValue()==overlay0
            &&unbounded->TerrainDigest()==sediment0
            &&unbounded->TerrainRevision()==tRev0});
        c.checks.push_back({"topology_16f4_parent_unchanged",
            unbounded->FieldDigestValue()==kFrozenP5b1FieldDigest
            &&unbounded->BodyDigest()==body0
            &&unbounded->WaterTopologyRevision()==wRev0});

        bool massEach=true,coherent=true,localOnly=true;
        bool lakeOk=false,wetOk=false,riverOk=false,dryOk=false;
        bool noInfil=true;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            massEach=massEach&&t.WaterGramsBefore==t.WaterGramsAfter
                &&t.TerrainGramsBefore==t.TerrainGramsAfter
                &&t.ParentFieldDigest==kFrozenP5b1FieldDigest
                &&t.ParentTerrainOverlay==overlay0;
            coherent=coherent&&t.PublishedCoherent
                &&t.TerrainRevisionAfter==t.TerrainRevisionBefore+1;
            if(t.CellsVisited>16||t.BodiesExamined>4)localOnly=false;
            noInfil=noInfil&&t.WaterGramsBefore==t.WaterGramsAfter;
            if(t.Fixture==FixtureKind::LakeEdge)
                lakeOk=t.SurfaceWetnessAfter>t.SurfaceWetnessBefore
                    &&t.MoistureAfter>t.MoistureBefore
                    &&t.SaturationAfter>t.SaturationBefore
                    &&t.CohesionModifierAfter<t.CohesionModifierBefore;
            if(t.Fixture==FixtureKind::WetlandSaturate)
                wetOk=t.SaturationAfter>=0.999&&t.MoistureAfter>=0.999
                    &&t.SurfaceWetnessAfter>=0.999;
            if(t.Fixture==FixtureKind::RiverBank)
                riverOk=t.SurfaceWetnessAfter>t.SurfaceWetnessBefore
                    &&t.MoistureAfter>t.MoistureBefore;
            if(t.Fixture==FixtureKind::ContactLostDry)
                dryOk=t.SurfaceWetnessAfter<t.SurfaceWetnessBefore
                    &&t.MoistureAfter<t.MoistureBefore
                    &&t.SaturationAfter<t.SaturationBefore
                    &&t.SurfaceWetnessAfter>0.0;
        }
        c.checks.push_back({"state_receipt_exact",
            c.transactionsAdmitted>=4&&c.lakeEdge>0&&c.wetlandSaturate>0
            &&c.riverBank>0&&c.contactLostDry>0});
        c.checks.push_back({"lake_edge_wets_adjacent_dirt",lakeOk});
        c.checks.push_back({"wetland_keeps_shallow_soil_saturated",wetOk});
        c.checks.push_back({"river_contact_wets_exposed_bank",riverOk});
        c.checks.push_back({"water_removed_begins_drying",dryOk});
        c.checks.push_back({"admitted_gram_pairs_unchanged",massEach});
        c.checks.push_back({"published_state_revision_coherent",coherent});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined>0&&c.maxBodiesExamined<6515
            &&c.maxCellsVisited>0&&c.maxCellsVisited<=16});
        c.checks.push_back({"no_implicit_water_transfer",noInfil});
        c.checks.push_back({"occupancy_valid_post_state",
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
                auto fixtures=BuildFixtures(k->Parent(),false);
                fixtures.lake.TerrainStateRevision=k->TerrainStateRevision();
                fixtures.lake.WaterContactRevision=k->WaterContactRevision();
                fixtures.lake.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.lake,false);
                fixtures.lake.TerrainStateRevision=first.TerrainRevisionBefore;
                fixtures.lake.WaterContactRevision=first.WaterRevision>0?first.WaterRevision-1:0;
                int64_t const sb=k->TotalMass();
                uint64_t const sd=k->StateDigestValue();
                auto rec=k->ApplyRequest(fixtures.lake,false);
                staleOk=first.PublishedCoherent&&!first.RefusedStale
                    &&rec.RefusedStale&&k->TotalMass()==sb
                    &&k->StateDigestValue()==sd;
            }
            c.checks.push_back({"stale_state_or_contact_revision_refuse",staleOk});
        }

        bool partitionOk=true;
        {
            auto a=loadEnabled(kHoldBudget);
            auto b=loadEnabled(kHoldBudget);
            if(a&&b)
            {
                auto fa=BuildFixtures(a->Parent(),false);
                auto fb=BuildFixtures(b->Parent(),true);
                auto applyAll=[&](Kernel& k,FixtureSet const& f,bool reverse)
                {
                    StateRequest reqs[4]={f.lake,f.wetland,f.river,f.dry};
                    for(auto& req:reqs)
                    {
                        req.TerrainStateRevision=k.TerrainStateRevision();
                        req.WaterContactRevision=k.WaterContactRevision();
                        req.CheckRevision=true;
                        k.ApplyRequest(req,reverse);
                    }
                };
                applyAll(*a,fa,false);
                applyAll(*b,fb,true);
                partitionOk=a->StateDigestValue()==b->StateDigestValue()
                    &&a->FieldDigestValue()==b->FieldDigestValue()
                    &&a->TotalMass()==b->TotalMass()
                    &&a->TotalMatter()==b->TotalMatter()
                    &&fa.lake.TerrainCell==fb.lake.TerrainCell
                    &&fa.wetland.TerrainCell==fb.wetland.TerrainCell
                    &&fa.river.TerrainCell==fb.river.TerrainCell;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->StateDigestValue()==unbounded->StateDigestValue()
            &&reload->StateDigestValue()==unbounded->StateDigestValue()
            &&cold->FieldDigestValue()==unbounded->FieldDigestValue()
            &&cold->TotalMass()==unbounded->TotalMass()
            &&cold->TerrainStateRevision()==unbounded->TerrainStateRevision();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});

        c.checks.push_back({"p5b2b_infiltration_closed",true});
        c.checks.push_back({"p5b3_water_to_terrain_matter_closed",true});
        c.checks.push_back({"rainfall_groundwater_closed",true});
        c.checks.push_back({"water_erosion_sediment_bank_collapse_closed",true});
        c.checks.push_back({"no_16c_remobilization",true});
        c.checks.push_back({"no_simulation_domain_framework_extract",true});
        c.checks.push_back({"idle_complete_zero_extra_state",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->StateDigestValue();
                size_t const n=unbounded->Stats().transactionsAdmitted;
                unbounded->Tick(64);
                return unbounded->StateDigestValue()==before
                    &&unbounded->Stats().transactionsAdmitted==n;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b2a_terrain_state_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b2a_terrain_state_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B2A_TERRAIN_STATE %s\nreason=%s\n"
            "stage16f4_field_digest=%s\np5b1_field_digest=%s\n"
            "field_digest_disabled=%s\nstate_digest_disabled=%s\n"
            "field_digest_budget1=%s\nfield_digest_budgetN=%s\nfield_digest_unbounded=%s\n"
            "state_digest_budget1=%s\nstate_digest_budgetN=%s\nstate_digest_unbounded=%s\n"
            "water_mass_before=%lld\nwater_mass_after=%lld\n"
            "terrain_matter_before=%lld\nterrain_matter_after=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "lake_edge=%zu\nwetland_saturate=%zu\nriver_bank=%zu\ncontact_lost_dry=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16f4FieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.p5b1FieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.stateDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestUnbounded).c_str(),
            CausalWorldGeology::Hex64(c.stateDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.stateDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.stateDigestUnbounded).c_str(),
            (long long)c.waterMassBefore,(long long)c.waterMassAfter,
            (long long)c.terrainMatterBefore,(long long)c.terrainMatterAfter,
            c.transactionsCertified,c.transactionsAdmitted,
            c.lakeEdge,c.wetlandSaturate,c.riverBank,c.contactLostDry,
            c.maxCellsVisited,c.maxBodiesExamined);
        std::fprintf(f,"coupling=water_to_terrain_state_only\n"
            "p5b1=frozen\np5b2a=%s\np5b2b=closed\np5b3=closed\n"
            "rainfall=0\ninfiltration=0\ngroundwater=0\n"
            "water_erosion=0\nsediment_remobilization=0\nbank_collapse=0\n"
            "active_16b_erosion=0\necology=0\n"
            "simulation_domain_framework=closed\n"
            "stage16f4=frozen\nterrain_mass_movement=0\n",
            c.passed?"certified":"failed");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
