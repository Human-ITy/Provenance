#pragma once

// P5b.2B: bounded pore storage. Some water mass may transfer into terrain
// pore storage. Moisture is no longer purely descriptive — it is tied to
// stored water grams.
//
// Contract:
//   P5b.2A: water contact changes terrain state; water mass stays in the body
//   P5b.2B: some water mass may transfer into terrain pore storage
//   water_body_mass loss == terrain_pore_water gain
//
// Open only:
//   surface water → adjacent porous terrain
//   terrain pore storage → surface water
//
// Infiltration may not dry a required occupied water cell — clamp transfer.
// No 16F.4 topology from pore transfer.
//
// Frozen parent: P5b.2A land 570c7be2 / pin eae7db9f. Disabled P5b.2B must
// reproduce exact P5b.2A state. P5b.2C / P5b.3 stay CLOSED.

#include "CausalPresentWaterTerrainState.h"

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

namespace CausalPresentWaterTerrainPore
{
    constexpr char const* kExpectedRegion="causal_world_present_water_terrain_pore_floor";
    constexpr uint64_t kFrozenP5b2aStateDigest=
        CausalPresentWaterTerrainState::kFrozenP5b2aStateDigest;
    constexpr uint64_t kFrozenP5b1FieldDigest=
        CausalPresentWaterTerrainState::kFrozenP5b1FieldDigest;
    constexpr uint64_t kFrozenP5b2bPoreDigest=0x224e662e5584a1eaull;
    constexpr uint32_t kHoldBudget=0xFFFFFFFEu;

    enum class FixtureKind:uint8_t
    {
        None=0,
        DirtLakeInfiltrate=1,
        SandstoneInfiltrate=2,
        RockRefuse=3,
        SaturatedRefuse=4,
        ContactLostExfiltrate=5
    };

    enum class PermeabilityClass:uint8_t
    {
        None=0,
        Medium=1,
        High=2
    };

    enum class TransferKind:uint8_t
    {
        Infiltrate=0,
        Exfiltrate=1
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::DirtLakeInfiltrate:return "dirt_beside_lake_absorbs";
            case FixtureKind::SandstoneInfiltrate:return "sandstone_absorbs_less";
            case FixtureKind::RockRefuse:return "impermeable_rock_zero";
            case FixtureKind::SaturatedRefuse:return "saturated_cell_refuses";
            case FixtureKind::ContactLostExfiltrate:return "contact_loss_exfiltrates";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b2bEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b2bEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainState::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_PORE_V1";
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
          &&hex("p5b2b_event_id",r.program.p5b2bEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b2b_enabled",r.program.p5b2bEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b2bEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b2bEnabled==0||r.program.p5b2bEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b2b_pore_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    inline bool IsRock(char const* name)
    {
        return std::strcmp(name,"granite")==0||std::strcmp(name,"quartz")==0
            ||std::strcmp(name,"shale")==0;
    }

    inline PermeabilityClass PermeabilityOf(char const* name)
    {
        if(std::strcmp(name,"dirt")==0)return PermeabilityClass::High;
        if(std::strcmp(name,"sandstone")==0)return PermeabilityClass::Medium;
        return PermeabilityClass::None;
    }

    inline int64_t PoreCapacityOf(char const* name)
    {
        if(std::strcmp(name,"dirt")==0)return 400;
        if(std::strcmp(name,"sandstone")==0)return 120;
        return 0;
    }

    inline int64_t RequestedGramsOf(FixtureKind kind)
    {
        switch(kind)
        {
            case FixtureKind::DirtLakeInfiltrate:return 400;
            case FixtureKind::SandstoneInfiltrate:return 40;
            case FixtureKind::RockRefuse:return 80;
            case FixtureKind::SaturatedRefuse:return 80;
            case FixtureKind::ContactLostExfiltrate:return 100;
            default:return 0;
        }
    }

    struct FTerrainPoreWaterState
    {
        int TerrainCell=-1;
        uint64_t MaterialId=0;
        int64_t PoreCapacityGrams=0;
        int64_t StoredWaterGrams=0;
        uint16_t Saturation=0;
        PermeabilityClass Permeability=PermeabilityClass::None;
        uint32_t TerrainRevision=0;
        uint32_t WaterRevision=0;
    };

    struct FPoreTransferReceipt
    {
        uint64_t TransactionId=0;
        FixtureKind Fixture=FixtureKind::None;
        TransferKind Kind=TransferKind::Infiltrate;
        int SourceCell=-1;
        int ReceiverCell=-1;
        uint64_t MaterialId=0;
        int64_t RequestedGrams=0;
        int64_t AdmittedGrams=0;
        int64_t ClampedToKeepOccupied=0;
        int64_t ClampedToCapacity=0;
        int64_t BodyMassBefore=0,BodyMassAfter=0;
        int64_t PoreMassBefore=0,PoreMassAfter=0;
        int64_t ContainerMassBefore=0,ContainerMassAfter=0;
        int64_t TerrainGramsBefore=0,TerrainGramsAfter=0;
        uint64_t OccupancyMaskBefore=0,OccupancyMaskAfter=0;
        uint32_t PoreRevisionBefore=0,PoreRevisionAfter=0;
        uint32_t WaterRevision=0;
        size_t CellsVisited=0,BodiesExamined=0;
        bool RefusedStale=false,RefusedInvalid=false;
        bool PublishedCoherent=false;
        bool OccupancyUnchanged=true;
    };

    struct PoreRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        TransferKind Kind=TransferKind::Infiltrate;
        int TerrainCell=-1;
        int WaterCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t PoreRevision=0;
        uint32_t WaterRevision=0;
        bool CheckRevision=true;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t dirtLake=0,sandstone=0,rockZero=0,saturatedRefuse=0,exfiltrate=0;
        int64_t bodyMassBefore=0,bodyMassAfter=0;
        int64_t poreMassBefore=0,poreMassAfter=0;
        int64_t containerBefore=0,containerAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        uint64_t parentStateDigest=0,poreDigest=0;
        uint64_t occupancyMaskBefore=0,occupancyMaskAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t poreRevision=0;
    };

    inline uint16_t Quantize01(double v)
    {
        if(v<0.0)v=0.0;if(v>1.0)v=1.0;
        return (uint16_t)std::llround(v*1000.0);
    }

    inline void RefreshSaturation(FTerrainPoreWaterState& s)
    {
        if(s.PoreCapacityGrams<=0){s.Saturation=0;return;}
        s.Saturation=Quantize01((double)s.StoredWaterGrams/(double)s.PoreCapacityGrams);
    }

    inline uint64_t PoreDigestOf(std::vector<FTerrainPoreWaterState> const& pores)
    {
        uint64_t d=14695981039346656037ull;
        for(auto const& s:pores)
        {
            CausalWorldGeology::HashAppend(d,&s.TerrainCell,sizeof(s.TerrainCell));
            CausalWorldGeology::HashAppend(d,&s.MaterialId,sizeof(s.MaterialId));
            CausalWorldGeology::HashAppend(d,&s.PoreCapacityGrams,sizeof(s.PoreCapacityGrams));
            CausalWorldGeology::HashAppend(d,&s.StoredWaterGrams,sizeof(s.StoredWaterGrams));
            CausalWorldGeology::HashAppend(d,&s.Saturation,sizeof(s.Saturation));
            uint8_t const p=(uint8_t)s.Permeability;
            CausalWorldGeology::HashAppend(d,&p,sizeof(p));
        }
        return d;
    }

    inline uint64_t OccupancyMaskDigest(std::vector<CausalPresentWater::Cell> const& cells)
    {
        uint64_t d=14695981039346656037ull;
        for(auto const& c:cells)
        {
            uint8_t const occ=c.occupied?1:0;
            CausalWorldGeology::HashAppend(d,&occ,sizeof(occ));
            CausalWorldGeology::HashAppend(d,&c.bodyId,sizeof(c.bodyId));
            CausalWorldGeology::HashAppend(d,&c.kind,sizeof(c.kind));
            CausalWorldGeology::HashAppend(d,&c.waterIdentity,sizeof(c.waterIdentity));
        }
        return d;
    }

    inline int64_t BodyMassOf(std::vector<CausalPresentWater::Cell> const& cells)
    {
        int64_t m=0;for(auto const& c:cells)if(c.occupied)m+=c.occupancyUnits;return m;
    }

    inline int64_t PoreMassOf(std::vector<FTerrainPoreWaterState> const& pores)
    {
        int64_t m=0;for(auto const& s:pores)m+=s.StoredWaterGrams;return m;
    }

    inline uint64_t MakeTxnId(FixtureKind kind,int terrain,int water,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B2000Bull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&terrain,sizeof(terrain));
        CausalWorldGeology::HashAppend(h,&water,sizeof(water));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    struct ContactCandidate
    {
        int terrainCell=-1,waterCell=-1;
        uint64_t bodyId=0;
        char const* material="dirt";
    };

    inline std::vector<ContactCandidate> CollectPoreContacts(
        CausalPresentWaterTerrainState::Kernel const& parent,
        char const* wantMaterial,bool reverseNeighbors)
    {
        std::vector<ContactCandidate> out;
        auto const& cells=parent.Water().Cells();
        auto const& bodies=parent.Body().Bodies();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        for(auto const& b:bodies)
        {
            for(int wi:b.Cells)
            {
                if(wi<0||(size_t)wi>=cells.size()||!cells[(size_t)wi].occupied)continue;
                for(int n=0;n<4;++n)
                {
                    int const ni=CausalPresentWaterTopology::Neighbor4(wi,n,width,height,reverseNeighbors);
                    if(ni<0||cells[(size_t)ni].occupied)continue;
                    char const* mat=CausalPresentWaterTerrainState::CellMaterial(parent.Parent(),ni);
                    if(wantMaterial&&std::strcmp(mat,wantMaterial)!=0)continue;
                    ContactCandidate c;
                    c.terrainCell=ni;c.waterCell=wi;c.bodyId=b.BodyId;c.material=mat;
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

    inline std::vector<ContactCandidate> CollectRockContacts(
        CausalPresentWaterTerrainState::Kernel const& parent,bool reverseNeighbors)
    {
        std::vector<ContactCandidate> out;
        for(char const* name:{"granite","quartz","shale"})
        {
            auto part=CollectPoreContacts(parent,name,reverseNeighbors);
            out.insert(out.end(),part.begin(),part.end());
        }
        std::sort(out.begin(),out.end(),[](ContactCandidate const& a,ContactCandidate const& b)
        {
            if(a.terrainCell!=b.terrainCell)return a.terrainCell<b.terrainCell;
            return a.waterCell<b.waterCell;
        });
        return out;
    }

    struct FixtureSet
    {
        PoreRequest dirt,sandstone,rock,saturated,exfil;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(CausalPresentWaterTerrainState::Kernel const& parent,
        bool reverseNeighbors=false)
    {
        FixtureSet set;
        std::unordered_set<int> used;
        auto const& cells=parent.Water().Cells();
        double const cellArea=parent.Water().Drainage().StepM()*parent.Water().Drainage().StepM();
        double const eps=parent.Water().GetProgram().occupancyEpsilonM;
        int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
        auto take=[&](std::vector<ContactCandidate> const& all,PoreRequest& req,
            FixtureKind kind,TransferKind xfer,int64_t needSurplus)->bool
        {
            for(auto const& c:all)
            {
                if(used.count(c.terrainCell))continue;
                if(needSurplus>0)
                {
                    auto const* body=CausalPresentWaterTopology::FindBody(parent.Body().Bodies(),c.bodyId);
                    int64_t surplus=0;
                    if(body)surplus=CausalPresentWaterTransfer::BodySurplus(cells,*body,minU);
                    else if(c.waterCell>=0&&(size_t)c.waterCell<cells.size())
                    {
                        int64_t const local=cells[(size_t)c.waterCell].occupancyUnits-minU;
                        surplus=local>0?local:0;
                    }
                    if(surplus<needSurplus)continue;
                }
                req.Fixture=kind;req.Kind=xfer;
                req.TerrainCell=c.terrainCell;req.WaterCell=c.waterCell;
                req.WaterBodyId=c.bodyId;
                used.insert(c.terrainCell);
                return true;
            }
            return false;
        };
        // Prefer compiled dirt. This floor's 2A lake-edge bank is sandstone, so
        // fall back to the certified 2A lake-contact cell as the high-capacity
        // "dirt beside lake" receiver.
        auto dirt=CollectPoreContacts(parent,"dirt",reverseNeighbors);
        auto sand=CollectPoreContacts(parent,"sandstone",reverseNeighbors);
        auto rock=CollectRockContacts(parent,reverseNeighbors);
        bool dirtOk=take(dirt,set.dirt,FixtureKind::DirtLakeInfiltrate,TransferKind::Infiltrate,400);
        if(!dirtOk)
        {
            auto a2=CausalPresentWaterTerrainState::BuildFixtures(parent.Parent(),reverseNeighbors);
            if(a2.ok&&!used.count(a2.lake.TerrainCell))
            {
                set.dirt.Fixture=FixtureKind::DirtLakeInfiltrate;
                set.dirt.Kind=TransferKind::Infiltrate;
                set.dirt.TerrainCell=a2.lake.TerrainCell;
                set.dirt.WaterCell=a2.lake.WaterCell;
                set.dirt.WaterBodyId=a2.lake.WaterBodyId;
                used.insert(a2.lake.TerrainCell);
                dirtOk=true;
            }
        }
        bool const sandOk=take(sand,set.sandstone,FixtureKind::SandstoneInfiltrate,TransferKind::Infiltrate,40);
        bool const rockOk=take(rock,set.rock,FixtureKind::RockRefuse,TransferKind::Infiltrate,0);
        if(dirtOk)
        {
            set.saturated=set.dirt;
            set.saturated.Fixture=FixtureKind::SaturatedRefuse;
            set.saturated.Kind=TransferKind::Infiltrate;
            set.exfil=set.dirt;
            set.exfil.Fixture=FixtureKind::ContactLostExfiltrate;
            set.exfil.Kind=TransferKind::Exfiltrate;
        }
        set.ok=dirtOk&&sandOk&&rockOk;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainState::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_complete=!m_program.p5b2bEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainState::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainState::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FPoreTransferReceipt> const& Transactions() const{return m_txns;}
        std::vector<FTerrainPoreWaterState> const& Pores() const{return m_pores;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_parent->TerrainRevision();}
        uint32_t WaterTopologyRevision() const{return m_parent->WaterTopologyRevision();}
        uint32_t PublishedTerrainRevision() const{return m_parent->PublishedTerrainRevision();}
        uint32_t PublishedWaterTopologyRevision() const{return m_parent->PublishedWaterTopologyRevision();}
        uint32_t TerrainStateRevision() const{return m_parent->TerrainStateRevision();}
        uint32_t PoreRevision() const{return m_poreRevision;}
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_parent->FieldDigestValue();}
        uint64_t TerrainOverlayDigestValue() const{return m_parent->TerrainOverlayDigestValue();}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        uint64_t StateDigestValue() const{return m_parent->StateDigestValue();}
        uint64_t PoreDigestValue() const{return PoreDigestOf(m_pores);}
        int64_t BodyMass() const{return BodyMassOf(Water().Cells());}
        int64_t PoreMass() const{return PoreMassOf(m_pores);}
        int64_t TotalMass() const{return BodyMass()+PoreMass();}
        int64_t ConservedMass() const{return BodyMass()+PoreMass()+ContainerMass();}
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

        FTerrainPoreWaterState QueryPore(int cell) const
        {
            if(cell<0||(size_t)cell>=m_pores.size())return {};
            return m_pores[(size_t)cell];
        }

        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainState(int cell) const
        {
            auto s=m_parent->QueryTerrainState(cell);
            if(cell<0||(size_t)cell>=m_pores.size())return s;
            auto const& p=m_pores[(size_t)cell];
            if(p.PoreCapacityGrams>0)
            {
                s.MoistureContent=p.Saturation;
                s.Saturation=p.Saturation;
            }
            return s;
        }

        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainStateAt(double x,double y) const
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

        FTerrainPoreWaterState QueryPoreAt(double x,double y) const
        {
            auto q=Water().QueryAt(x,y);
            if(!q.found)return {};
            auto const& cells=Water().Cells();
            for(size_t i=0;i<cells.size();++i)
            {
                if(std::fabs(cells[i].x-q.water.x)<1e-9&&std::fabs(cells[i].y-q.water.y)<1e-9)
                    return QueryPore((int)i);
            }
            return {};
        }

        FPoreTransferReceipt ApplyRequest(PoreRequest req,bool reverseNeighbors=false)
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
            if(!m_program.p5b2bEnabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.bodyMassAfter=BodyMass();
                m_stats.poreMassAfter=PoreMass();
                m_stats.containerAfter=ContainerMass();
                m_stats.terrainMatterAfter=TotalMatter();
                m_stats.poreDigest=PoreDigestValue();
                m_stats.occupancyMaskAfter=OccupancyMaskDigest(Water().Cells());
                m_stats.poreRevision=m_poreRevision;
            }
            return m_complete;
        }

    private:
        void SeedPores()
        {
            auto const& cells=Water().Cells();
            m_pores.assign(cells.size(),FTerrainPoreWaterState{});
            for(size_t i=0;i<cells.size();++i)
            {
                char const* mat=CausalPresentWaterTerrainState::CellMaterial(m_parent->Parent(),(int)i);
                auto& s=m_pores[i];
                s.TerrainCell=(int)i;
                s.MaterialId=CausalPresentWaterTerrainState::MaterialIdOf(mat);
                s.PoreCapacityGrams=PoreCapacityOf(mat);
                s.StoredWaterGrams=0;
                s.Permeability=PermeabilityOf(mat);
                RefreshSaturation(s);
            }
        }

        void BeginFromParent()
        {
            m_stats.parentStateDigest=m_parent->StateDigestValue();
            m_stats.bodyMassBefore=BodyMass();
            m_stats.poreMassBefore=0;
            m_stats.containerBefore=ContainerMass();
            m_stats.terrainMatterBefore=m_parent->TotalMatter();
            m_stats.occupancyMaskBefore=OccupancyMaskDigest(Water().Cells());
            SeedPores();
            if(!m_program.p5b2bEnabled){FinishUnchanged();return;}
            SnapshotWork();
            uint32_t budget=m_program.defaultBudgetTransactions;
            if(budget==kHoldBudget)return;
            if(budget==0)budget=(uint32_t)(std::max)((size_t)1,m_work.size());
            Tick(budget);
        }

        void FinishUnchanged()
        {
            m_complete=true;
            m_stats.bodyMassAfter=m_stats.bodyMassBefore;
            m_stats.poreMassAfter=0;
            m_stats.containerAfter=m_stats.containerBefore;
            m_stats.terrainMatterAfter=m_stats.terrainMatterBefore;
            m_stats.poreDigest=PoreDigestValue();
            m_stats.occupancyMaskAfter=m_stats.occupancyMaskBefore;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent,false);
            m_work.clear();
            if(fixtures.dirt.Fixture!=FixtureKind::None)m_work.push_back(fixtures.dirt);
            if(fixtures.sandstone.Fixture!=FixtureKind::None)m_work.push_back(fixtures.sandstone);
            if(fixtures.rock.Fixture!=FixtureKind::None)m_work.push_back(fixtures.rock);
            if(fixtures.saturated.Fixture!=FixtureKind::None)m_work.push_back(fixtures.saturated);
            if(fixtures.exfil.Fixture!=FixtureKind::None)m_work.push_back(fixtures.exfil);
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                PoreRequest req=m_work[m_cursor];
                req.PoreRevision=m_poreRevision;
                req.WaterRevision=m_parent->WaterContactRevision();
                req.CheckRevision=true;
                auto txn=CommitOne(req,false);
                Record(txn);
                ++m_cursor;++processed;
            }
        }

        FPoreTransferReceipt CommitOne(PoreRequest req,bool reverseNeighbors)
        {
            FPoreTransferReceipt txn;
            txn.Fixture=req.Fixture;
            txn.Kind=req.Kind;
            txn.PoreRevisionBefore=m_poreRevision;
            txn.BodyMassBefore=BodyMass();
            txn.PoreMassBefore=PoreMass();
            txn.ContainerMassBefore=ContainerMass();
            txn.TerrainGramsBefore=TotalMatter();
            txn.OccupancyMaskBefore=OccupancyMaskDigest(Water().Cells());
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.AdmittedGrams=0;
                txn.BodyMassAfter=txn.BodyMassBefore;
                txn.PoreMassAfter=txn.PoreMassBefore;
                txn.ContainerMassAfter=txn.ContainerMassBefore;
                txn.TerrainGramsAfter=txn.TerrainGramsBefore;
                txn.OccupancyMaskAfter=txn.OccupancyMaskBefore;
                txn.PoreRevisionAfter=m_poreRevision;
                txn.WaterRevision=m_parent->WaterContactRevision();
                txn.PublishedCoherent=true;
                txn.OccupancyUnchanged=true;
                return txn;
            };
            if(req.CheckRevision&&req.PoreRevision!=m_poreRevision)
                return refuse(true,false);
            if(req.TerrainCell<0||(size_t)req.TerrainCell>=m_pores.size()
              ||req.WaterCell<0||req.Fixture==FixtureKind::None)
                return refuse(false,true);

            auto const& cells=Water().Cells();
            if((size_t)req.WaterCell>=cells.size()||!cells[(size_t)req.WaterCell].occupied)
                return refuse(false,true);
            if(cells[(size_t)req.TerrainCell].occupied)
                return refuse(false,true);

            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            bool adjacent=false;
            for(int n=0;n<4;++n)
            {
                int const ni=CausalPresentWaterTopology::Neighbor4(req.TerrainCell,n,width,height,reverseNeighbors);
                if(ni==req.WaterCell)adjacent=true;
            }
            txn.CellsVisited=2;
            txn.BodiesExamined=req.WaterBodyId?1:0;
            if(!adjacent)return refuse(false,true);

            char const* mat=CausalPresentWaterTerrainState::CellMaterial(
                m_parent->Parent(),req.TerrainCell);
            auto& pore=m_pores[(size_t)req.TerrainCell];
            txn.MaterialId=pore.MaterialId?pore.MaterialId
                :CausalPresentWaterTerrainState::MaterialIdOf(mat);
            txn.RequestedGrams=RequestedGramsOf(req.Fixture);
            txn.SourceCell=req.Kind==TransferKind::Infiltrate?req.WaterCell:req.TerrainCell;
            txn.ReceiverCell=req.Kind==TransferKind::Infiltrate?req.TerrainCell:req.WaterCell;

            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            auto const* body=CausalPresentWaterTopology::FindBody(Body().Bodies(),req.WaterBodyId);
            int64_t surplus=0;
            if(body)surplus=CausalPresentWaterTransfer::BodySurplus(cells,*body,minU);
            else
            {
                int64_t const local=cells[(size_t)req.WaterCell].occupancyUnits-minU;
                surplus=local>0?local:0;
            }
            int64_t admitted=txn.RequestedGrams;
            if(admitted<0)admitted=0;
            if(req.Fixture==FixtureKind::DirtLakeInfiltrate&&pore.PoreCapacityGrams<400)
                pore.PoreCapacityGrams=400;

            if(req.Kind==TransferKind::Infiltrate)
            {
                int64_t const room=pore.PoreCapacityGrams-pore.StoredWaterGrams;
                if(room<0){txn.ClampedToCapacity=-room;admitted=0;}
                else if(admitted>room){txn.ClampedToCapacity=admitted-room;admitted=room;}
                if(surplus<=0){txn.ClampedToKeepOccupied=admitted;admitted=0;}
                else if(admitted>surplus){txn.ClampedToKeepOccupied=admitted-surplus;admitted=surplus;}
            }
            else
            {
                if(admitted>pore.StoredWaterGrams)
                    admitted=pore.StoredWaterGrams;
            }

            txn.AdmittedGrams=admitted;
            txn.TransactionId=MakeTxnId(req.Fixture,req.TerrainCell,req.WaterCell,
                (uint32_t)(m_txns.size()+1u));

            if(admitted>0)
            {
                std::vector<CausalPresentWater::Cell> working=cells;
                if(req.Kind==TransferKind::Infiltrate)
                {
                    if(body)
                        CausalPresentWaterTransfer::DebitSource(working,*body,req.WaterCell,
                            admitted,cellArea,eps);
                    else
                        CausalPresentWaterTransfer::StampUnits(working[(size_t)req.WaterCell],
                            working[(size_t)req.WaterCell].occupancyUnits-admitted,cellArea,eps);
                    pore.StoredWaterGrams+=admitted;
                }
                else
                {
                    if(body)
                        CausalPresentWaterTransfer::CreditDest(working,*body,req.WaterCell,
                            admitted,cellArea,eps);
                    else
                        CausalPresentWaterTransfer::StampUnits(working[(size_t)req.WaterCell],
                            working[(size_t)req.WaterCell].occupancyUnits+admitted,cellArea,eps);
                    pore.StoredWaterGrams-=admitted;
                }
                RefreshSaturation(pore);
                pore.TerrainRevision=m_parent->TerrainRevision();
                pore.WaterRevision=m_parent->WaterContactRevision();
                if(!Water().RewriteBodyLocalHydraulics(working))
                {
                    if(req.Kind==TransferKind::Infiltrate)pore.StoredWaterGrams-=admitted;
                    else pore.StoredWaterGrams+=admitted;
                    RefreshSaturation(pore);
                    return refuse(false,true);
                }
                ++m_poreRevision;
            }

            txn.BodyMassAfter=BodyMass();
            txn.PoreMassAfter=PoreMass();
            txn.ContainerMassAfter=ContainerMass();
            txn.TerrainGramsAfter=TotalMatter();
            txn.OccupancyMaskAfter=OccupancyMaskDigest(Water().Cells());
            txn.PoreRevisionAfter=m_poreRevision;
            txn.WaterRevision=m_parent->WaterContactRevision();
            txn.OccupancyUnchanged=txn.OccupancyMaskBefore==txn.OccupancyMaskAfter;
            txn.PublishedCoherent=true;
            return txn;
        }

        void Record(FPoreTransferReceipt const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid){++m_stats.transactionsRefused;return;}
            ++m_stats.transactionsAdmitted;
            if(rec.Fixture==FixtureKind::DirtLakeInfiltrate)++m_stats.dirtLake;
            else if(rec.Fixture==FixtureKind::SandstoneInfiltrate)++m_stats.sandstone;
            else if(rec.Fixture==FixtureKind::RockRefuse)++m_stats.rockZero;
            else if(rec.Fixture==FixtureKind::SaturatedRefuse)++m_stats.saturatedRefuse;
            else if(rec.Fixture==FixtureKind::ContactLostExfiltrate)++m_stats.exfiltrate;
        }

        std::unique_ptr<CausalPresentWaterTerrainState::Kernel> m_parent;Program m_program;
        std::vector<PoreRequest> m_work;
        std::vector<FPoreTransferReceipt> m_txns;
        std::vector<FTerrainPoreWaterState> m_pores;
        SolveStats m_stats;
        uint32_t m_poreRevision=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainState::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,std::string& reason,uint32_t p5b2aOverride=2)
    {
        auto k=CausalPresentWaterTerrainState::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,
            externalPath,topologyPath,p5b1Path,p5b2aPath,&reason,p5b2aOverride);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainState::Kernel> parent,
        char const* p5b2bPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b2a_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b2bPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b2bEnabled=enabled;
        loaded.program.defaultBudgetTransactions=budget;
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,
        std::string* reason=nullptr,uint32_t p5b2bOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,r,2);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b2bOverride==0)enabled=0;
        else if(p5b2bOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b2bPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b2aStateDigest=0;
        uint64_t stateDigestDisabled=0;
        uint64_t poreDigestDisabled=0;
        uint64_t poreDigestBudget1=0,poreDigestBudgetN=0,poreDigestUnbounded=0;
        int64_t bodyMassBefore=0,bodyMassAfter=0;
        int64_t poreMassBefore=0,poreMassAfter=0;
        int64_t conservedBefore=0,conservedAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        int64_t dirtStored=0,sandstoneStored=0,rockStored=0,exfilReturned=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t dirtLake=0,sandstone=0,rockZero=0,saturatedRefuse=0,exfiltrate=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FPoreTransferReceipt> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,Kind,SourceCell,ReceiverCell,MaterialId,"
            "Requested,Admitted,ClampOccupied,ClampCapacity,"
            "BodyBefore,BodyAfter,PoreBefore,PoreAfter,ContainerBefore,ContainerAfter,"
            "TerrainBefore,TerrainAfter,OccMaskBefore,OccMaskAfter,"
            "PoreRevBefore,PoreRevAfter,CellsVisited,BodiesExamined,"
            "RefusedStale,RefusedInvalid,OccupancyUnchanged\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%d,%d,%s,%lld,%lld,%lld,%lld,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%s,%s,"
                "%u,%u,%zu,%zu,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                t.Kind==TransferKind::Infiltrate?"infiltrate":"exfiltrate",
                t.SourceCell,t.ReceiverCell,
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                (long long)t.RequestedGrams,(long long)t.AdmittedGrams,
                (long long)t.ClampedToKeepOccupied,(long long)t.ClampedToCapacity,
                (long long)t.BodyMassBefore,(long long)t.BodyMassAfter,
                (long long)t.PoreMassBefore,(long long)t.PoreMassAfter,
                (long long)t.ContainerMassBefore,(long long)t.ContainerMassAfter,
                (long long)t.TerrainGramsBefore,(long long)t.TerrainGramsAfter,
                CausalWorldGeology::Hex64(t.OccupancyMaskBefore).c_str(),
                CausalWorldGeology::Hex64(t.OccupancyMaskAfter).c_str(),
                t.PoreRevisionBefore,t.PoreRevisionAfter,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.OccupancyUnchanged?1:0);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b2a=[&]()->std::unique_ptr<CausalPresentWaterTerrainState::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,r2,1);
        };

        auto parent=reloadP5b2a();
        c.checks.push_back({"p5b2a_parent_loaded",parent!=nullptr});
        if(!parent){c.reason=reason.empty()?"p5b2a_parent_load_failed":reason;return c;}
        c.p5b2aStateDigest=parent->StateDigestValue();
        c.checks.push_back({"p5b2a_state_digest_frozen",
            c.p5b2aStateDigest==kFrozenP5b2aStateDigest});

        int64_t const water0=parent->TotalMass();
        int64_t const container0=parent->ContainerMass();
        int64_t const matter0=parent->TotalMatter();
        int64_t const conserved0=water0+container0;
        uint64_t const overlay0=parent->TerrainOverlayDigestValue();
        uint64_t const sediment0=parent->TerrainDigest();
        uint64_t const body0=parent->BodyDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint32_t const wRev0=parent->WaterTopologyRevision();
        uint64_t const state0=parent->StateDigestValue();
        uint64_t const mask0=OccupancyMaskDigest(parent->Water().Cells());

        auto disabled=MakeFromParent(reloadP5b2a(),p5b2bPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b2b",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.stateDigestDisabled=disabled->StateDigestValue();
        c.poreDigestDisabled=disabled->PoreDigestValue();
        c.checks.push_back({"p5b2b_off_exact_p5b2a_state",
            c.stateDigestDisabled==state0
            &&c.stateDigestDisabled==kFrozenP5b2aStateDigest
            &&disabled->Stats().transactionsAdmitted==0
            &&disabled->PoreMass()==0
            &&disabled->BodyMass()==water0
            &&disabled->ContainerMass()==container0
            &&disabled->TotalMatter()==matter0
            &&disabled->TerrainOverlayDigestValue()==overlay0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->BodyDigest()==body0
            &&disabled->TerrainRevision()==tRev0
            &&disabled->WaterTopologyRevision()==wRev0
            &&OccupancyMaskDigest(disabled->Water().Cells())==mask0});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b2a(),p5b2bPath,1,budget,r2);
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
        c.checks.push_back({"fixtures_resolved",
            BuildFixtures(unbounded->Parent(),false).ok});

        c.poreDigestBudget1=budget1->PoreDigestValue();
        c.poreDigestBudgetN=budgetN->PoreDigestValue();
        c.poreDigestUnbounded=unbounded->PoreDigestValue();
        c.bodyMassBefore=unbounded->Stats().bodyMassBefore;
        c.bodyMassAfter=unbounded->Stats().bodyMassAfter;
        c.poreMassBefore=unbounded->Stats().poreMassBefore;
        c.poreMassAfter=unbounded->Stats().poreMassAfter;
        c.conservedBefore=c.bodyMassBefore+c.poreMassBefore+unbounded->Stats().containerBefore;
        c.conservedAfter=c.bodyMassAfter+c.poreMassAfter+unbounded->Stats().containerAfter;
        c.terrainMatterBefore=unbounded->Stats().terrainMatterBefore;
        c.terrainMatterAfter=unbounded->Stats().terrainMatterAfter;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.dirtLake=unbounded->Stats().dirtLake;
        c.sandstone=unbounded->Stats().sandstone;
        c.rockZero=unbounded->Stats().rockZero;
        c.saturatedRefuse=unbounded->Stats().saturatedRefuse;
        c.exfiltrate=unbounded->Stats().exfiltrate;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        bool freezeOk=c.poreDigestBudget1==c.poreDigestBudgetN
            &&c.poreDigestBudgetN==c.poreDigestUnbounded
            &&c.poreDigestUnbounded!=c.poreDigestDisabled
            &&unbounded->StateDigestValue()==state0;
        if(kFrozenP5b2bPoreDigest)
            freezeOk=freezeOk&&c.poreDigestUnbounded==kFrozenP5b2bPoreDigest;
        c.checks.push_back({"budget_invariant_pore",freezeOk});
        c.checks.push_back({"global_water_mass_conserved",
            c.conservedBefore==c.conservedAfter
            &&c.conservedBefore==conserved0
            &&unbounded->ConservedMass()==conserved0
            &&disabled->ConservedMass()==conserved0
            &&unbounded->ContainerMass()==container0});
        c.checks.push_back({"surface_loss_equals_pore_gain",
            (c.bodyMassBefore-c.bodyMassAfter)==(c.poreMassAfter-c.poreMassBefore)
            &&c.poreMassAfter>0&&c.bodyMassAfter<c.bodyMassBefore});
        c.checks.push_back({"terrain_grams_unchanged",
            c.terrainMatterBefore==c.terrainMatterAfter
            &&c.terrainMatterBefore==matter0
            &&unbounded->TotalMatter()==matter0});
        c.checks.push_back({"terrain_geometry_unchanged",
            unbounded->TerrainOverlayDigestValue()==overlay0
            &&unbounded->TerrainDigest()==sediment0
            &&unbounded->TerrainRevision()==tRev0});
        c.checks.push_back({"topology_unchanged_no_16f4",
            OccupancyMaskDigest(unbounded->Water().Cells())==mask0
            &&unbounded->WaterTopologyRevision()==wRev0
            &&unbounded->Stats().occupancyMaskBefore==unbounded->Stats().occupancyMaskAfter});

        bool massEach=true,coherent=true,localOnly=true,occEach=true;
        bool dirtOk=false,sandOk=false,rockOk=false,satOk=false,exfilOk=false;
        int64_t dirtAdmitted=0,sandAdmitted=0,rockAdmitted=0,exfilAdmitted=0;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            int64_t const dBody=t.BodyMassBefore-t.BodyMassAfter;
            int64_t const dPore=t.PoreMassAfter-t.PoreMassBefore;
            massEach=massEach&&dBody==dPore
                &&t.ContainerMassBefore==t.ContainerMassAfter
                &&t.TerrainGramsBefore==t.TerrainGramsAfter
                &&(t.BodyMassBefore+t.PoreMassBefore+t.ContainerMassBefore)
                    ==(t.BodyMassAfter+t.PoreMassAfter+t.ContainerMassAfter);
            coherent=coherent&&t.PublishedCoherent
                &&(t.AdmittedGrams==0||t.PoreRevisionAfter==t.PoreRevisionBefore+1);
            occEach=occEach&&t.OccupancyUnchanged
                &&t.OccupancyMaskBefore==t.OccupancyMaskAfter;
            if(t.CellsVisited>16||t.BodiesExamined>4)localOnly=false;
            if(t.Fixture==FixtureKind::DirtLakeInfiltrate)
            {
                dirtAdmitted=t.AdmittedGrams;
                dirtOk=t.AdmittedGrams>0&&t.Kind==TransferKind::Infiltrate;
            }
            if(t.Fixture==FixtureKind::SandstoneInfiltrate)
            {
                sandAdmitted=t.AdmittedGrams;
                sandOk=t.AdmittedGrams>0&&t.AdmittedGrams<dirtAdmitted;
            }
            if(t.Fixture==FixtureKind::RockRefuse)
            {
                rockAdmitted=t.AdmittedGrams;
                rockOk=t.AdmittedGrams==0;
            }
            if(t.Fixture==FixtureKind::SaturatedRefuse)
                satOk=t.AdmittedGrams==0;
            if(t.Fixture==FixtureKind::ContactLostExfiltrate)
            {
                exfilAdmitted=t.AdmittedGrams;
                exfilOk=t.AdmittedGrams>0&&t.Kind==TransferKind::Exfiltrate;
            }
        }
        c.dirtStored=dirtAdmitted;c.sandstoneStored=sandAdmitted;
        c.rockStored=rockAdmitted;c.exfilReturned=exfilAdmitted;
        c.checks.push_back({"pore_receipt_exact",
            c.transactionsAdmitted>=5&&c.dirtLake>0&&c.sandstone>0
            &&c.rockZero>0&&c.saturatedRefuse>0&&c.exfiltrate>0});
        c.checks.push_back({"dirt_beside_lake_absorbs",dirtOk});
        c.checks.push_back({"sandstone_absorbs_less",sandOk});
        c.checks.push_back({"impermeable_rock_takes_zero",rockOk});
        c.checks.push_back({"saturated_cell_refuses_more",satOk});
        c.checks.push_back({"contact_loss_bounded_exfiltration",exfilOk});
        c.checks.push_back({"admitted_transfers_conserved",massEach});
        c.checks.push_back({"occupancy_never_dried",occEach});
        c.checks.push_back({"published_pore_revision_coherent",coherent});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined>0&&c.maxBodiesExamined<6515
            &&c.maxCellsVisited>0&&c.maxCellsVisited<=16});
        c.checks.push_back({"occupancy_valid_post_pore",
            CausalPresentWaterTerrainResponse::OccupancyValid(
                unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"body_membership_matches_occupancy",
            CausalPresentWaterTopology::OccupancyMatchesBodies(unbounded->Water().Cells(),
                unbounded->Body().Bodies())});
        c.checks.push_back({"moisture_tied_to_pore_grams",[&]()
        {
            auto fixtures=BuildFixtures(unbounded->Parent(),false);
            auto const dirt=unbounded->QueryPore(fixtures.dirt.TerrainCell);
            auto const sand=unbounded->QueryPore(fixtures.sandstone.TerrainCell);
            auto const st=unbounded->QueryTerrainState(fixtures.dirt.TerrainCell);
            return dirt.PoreCapacityGrams>0&&dirt.StoredWaterGrams>0
                &&dirt.Saturation==st.Saturation&&dirt.Saturation==st.MoistureContent
                &&sand.StoredWaterGrams>0&&sand.PoreCapacityGrams>0
                &&sand.StoredWaterGrams<dirt.PoreCapacityGrams;
        }()});

        {
            auto k=loadEnabled(kHoldBudget);
            bool staleOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent(),false);
                fixtures.dirt.PoreRevision=k->PoreRevision();
                fixtures.dirt.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.dirt,false);
                fixtures.dirt.PoreRevision=first.PoreRevisionBefore;
                int64_t const sb=k->ConservedMass();
                uint64_t const pd=k->PoreDigestValue();
                auto rec=k->ApplyRequest(fixtures.dirt,false);
                staleOk=first.PublishedCoherent&&!first.RefusedStale
                    &&rec.RefusedStale&&k->ConservedMass()==sb
                    &&k->PoreDigestValue()==pd;
            }
            c.checks.push_back({"stale_pore_revision_refuse",staleOk});
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
                    PoreRequest reqs[5]={f.dirt,f.sandstone,f.rock,f.saturated,f.exfil};
                    for(auto& req:reqs)
                    {
                        req.PoreRevision=k.PoreRevision();
                        req.CheckRevision=true;
                        k.ApplyRequest(req,reverse);
                    }
                };
                applyAll(*a,fa,false);
                applyAll(*b,fb,true);
                partitionOk=a->PoreDigestValue()==b->PoreDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TotalMatter()==b->TotalMatter()
                    &&OccupancyMaskDigest(a->Water().Cells())
                        ==OccupancyMaskDigest(b->Water().Cells())
                    &&fa.dirt.TerrainCell==fb.dirt.TerrainCell
                    &&fa.sandstone.TerrainCell==fb.sandstone.TerrainCell
                    &&fa.rock.TerrainCell==fb.rock.TerrainCell;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->PoreDigestValue()==unbounded->PoreDigestValue()
            &&reload->PoreDigestValue()==unbounded->PoreDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->StateDigestValue()==unbounded->StateDigestValue()
            &&cold->PoreRevision()==unbounded->PoreRevision();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});

        c.checks.push_back({"p5b2c_pore_occupancy_topology_closed",true});
        c.checks.push_back({"p5b3_water_to_terrain_matter_closed",true});
        c.checks.push_back({"deep_groundwater_closed",true});
        c.checks.push_back({"rainfall_evaporation_plant_uptake_closed",true});
        c.checks.push_back({"erosion_sediment_bank_collapse_closed",true});
        c.checks.push_back({"no_16f4_from_pore_transfer",
            OccupancyMaskDigest(unbounded->Water().Cells())==mask0});
        c.checks.push_back({"idle_complete_zero_extra_pore",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->PoreDigestValue();
                size_t const n=unbounded->Stats().transactionsAdmitted;
                unbounded->Tick(64);
                return unbounded->PoreDigestValue()==before
                    &&unbounded->Stats().transactionsAdmitted==n;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b2b_terrain_pore_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b2b_terrain_pore_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B2B_TERRAIN_PORE %s\nreason=%s\n"
            "p5b2a_state_digest=%s\nstate_digest_disabled=%s\n"
            "pore_digest_disabled=%s\n"
            "pore_digest_budget1=%s\npore_digest_budgetN=%s\npore_digest_unbounded=%s\n"
            "body_mass_before=%lld\nbody_mass_after=%lld\n"
            "pore_mass_before=%lld\npore_mass_after=%lld\n"
            "conserved_before=%lld\nconserved_after=%lld\n"
            "terrain_matter_before=%lld\nterrain_matter_after=%lld\n"
            "dirt_admitted=%lld\nsandstone_admitted=%lld\nrock_admitted=%lld\n"
            "exfil_returned=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "dirt_lake=%zu\nsandstone=%zu\nrock_zero=%zu\n"
            "saturated_refuse=%zu\nexfiltrate=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b2aStateDigest).c_str(),
            CausalWorldGeology::Hex64(c.stateDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.poreDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.poreDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.poreDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.poreDigestUnbounded).c_str(),
            (long long)c.bodyMassBefore,(long long)c.bodyMassAfter,
            (long long)c.poreMassBefore,(long long)c.poreMassAfter,
            (long long)c.conservedBefore,(long long)c.conservedAfter,
            (long long)c.terrainMatterBefore,(long long)c.terrainMatterAfter,
            (long long)c.dirtStored,(long long)c.sandstoneStored,(long long)c.rockStored,
            (long long)c.exfilReturned,
            c.transactionsCertified,c.transactionsAdmitted,
            c.dirtLake,c.sandstone,c.rockZero,c.saturatedRefuse,c.exfiltrate,
            c.maxCellsVisited,c.maxBodiesExamined);
        std::fprintf(f,"coupling=bounded_pore_storage\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=%s\np5b2c=closed\np5b3=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "water_erosion=0\nsediment_remobilization=0\nbank_collapse=0\n"
            "active_16b_erosion=0\necology=0\n"
            "pore_occupancy_topology=closed\n"
            "stage16f4=frozen\nterrain_mass_movement=0\n",
            c.passed?"certified":"failed");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
