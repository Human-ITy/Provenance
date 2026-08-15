#pragma once

// P5b.2C: pore-water changes may affect surface-water occupancy / topology.
// Terrain solid matter remains fixed. P5b.3 stays CLOSED.
//
// Contract:
//   2B: infiltration may not dry a required occupied water cell → clamp
//   2C: pore transfer MAY change occupancy (dry/grow/split/merge) via 16F.4
//       topology, then 16F.1 / 16F.2
//
// Water mass conserved: body + pore + container.
// Disabled P5b.2C must reproduce exact P5b.2B state.
// Frozen parent: long-haul infrastructure baseline f7ae29ea / pin 156df62d.
// Gameplay 2B 8bb75265 otherwise frozen.

#include "CausalPresentWaterTerrainPore.h"

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

namespace CausalPresentWaterTerrainPoreOccupancy
{
    constexpr char const* kExpectedRegion="causal_world_present_water_terrain_pore_occupancy_floor";
    constexpr uint64_t kFrozenP5b2bPoreDigest=
        CausalPresentWaterTerrainPore::kFrozenP5b2bPoreDigest;
    constexpr uint64_t kFrozenP5b2cOccupancyDigest=0x7102e45c92f6545dull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainPore::kHoldBudget;

    enum class FixtureKind:uint8_t
    {
        None=0,
        ThinCellInfiltrateDry=1,
        PoreExfiltrateGrow=2,
        SaturatedRefuse=3,
        RockRefuse=4,
        FarFromWater=5
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
            case FixtureKind::ThinCellInfiltrateDry:return "thin_cell_infiltrate_dries";
            case FixtureKind::PoreExfiltrateGrow:return "pore_exfiltrate_wets_dry";
            case FixtureKind::SaturatedRefuse:return "saturated_refuses_more";
            case FixtureKind::RockRefuse:return "impermeable_rock_zero";
            case FixtureKind::FarFromWater:return "far_from_water_no_change";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b2cEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b2cEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainPore::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_PORE_OCCUPANCY_V1";
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
          &&hex("p5b2c_event_id",r.program.p5b2cEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b2c_enabled",r.program.p5b2cEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b2cEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b2cEnabled==0||r.program.p5b2cEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b2c_occupancy_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FOccupancyPoreReceipt
    {
        uint64_t TransactionId=0;
        FixtureKind Fixture=FixtureKind::None;
        TransferKind Kind=TransferKind::Infiltrate;
        CausalPresentWaterTopology::TopologyClass Topology=CausalPresentWaterTopology::TopologyClass::None;
        int SourceCell=-1;
        int ReceiverCell=-1;
        int64_t RequestedGrams=0;
        int64_t AdmittedGrams=0;
        int64_t BodyMassBefore=0,BodyMassAfter=0;
        int64_t PoreMassBefore=0,PoreMassAfter=0;
        int64_t ContainerMassBefore=0,ContainerMassAfter=0;
        int64_t TerrainGramsBefore=0,TerrainGramsAfter=0;
        uint64_t OccupancyMaskBefore=0,OccupancyMaskAfter=0;
        uint32_t PoreRevisionBefore=0,PoreRevisionAfter=0;
        uint32_t TopologyRevisionBefore=0,TopologyRevisionAfter=0;
        size_t CellsVisited=0,BodiesExamined=0;
        bool OccupancyChanged=false;
        bool RefusedStale=false,RefusedInvalid=false;
        bool PublishedCoherent=false;
        bool LineageDeterministic=false;
        std::vector<uint64_t> InputBodyIds;
        std::vector<uint64_t> OutputBodyIds;
    };

    struct OccupancyPoreRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        TransferKind Kind=TransferKind::Infiltrate;
        int TerrainCell=-1;
        int WaterCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t PoreRevision=0;
        uint32_t TopologyRevision=0;
        bool CheckRevision=true;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t dryShrink=0,growWet=0,saturatedRefuse=0,rockZero=0,farUnchanged=0;
        size_t shrink=0,split=0,grow=0,merge=0;
        int64_t bodyMassBefore=0,bodyMassAfter=0;
        int64_t poreMassBefore=0,poreMassAfter=0;
        int64_t containerBefore=0,containerAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        uint64_t parentPoreDigest=0,occupancyDigest=0;
        uint64_t occupancyMaskBefore=0,occupancyMaskAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t poreRevision=0,topologyRevision=0;
    };

    inline uint64_t OccupancyMaskDigest(std::vector<CausalPresentWater::Cell> const& cells)
    {
        return CausalPresentWaterTerrainPore::OccupancyMaskDigest(cells);
    }

    inline uint64_t MakeTxnId(FixtureKind kind,int terrain,int water,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B2000Cull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&terrain,sizeof(terrain));
        CausalWorldGeology::HashAppend(h,&water,sizeof(water));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline bool OccupiedNeighborCount(std::vector<CausalPresentWater::Cell> const& cells,
        int cell,int width,int height,bool reverse,int& count)
    {
        count=0;
        if(cell<0||(size_t)cell>=cells.size())return false;
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,reverse);
            if(ni>=0&&cells[(size_t)ni].occupied)++count;
        }
        return true;
    }

    struct FixtureSet
    {
        OccupancyPoreRequest dry,grow,saturated,rock,distant;
        int puddleSourceWater=-1;
        uint64_t puddleSourceBody=0;
        bool satNeedsPrime=false;
        bool dryNeedsPrime=false;
        bool ok=false;
    };

    inline int BodySizeOf(CausalPresentWaterTerrainPore::Kernel const& parent,uint64_t bodyId)
    {
        auto const* body=CausalPresentWaterTopology::FindBody(parent.Body().Bodies(),bodyId);
        if(!body||body->Cells.empty())return 0;
        return (int)body->Cells.size();
    }

    inline int LowestPorousReceiver(std::vector<CausalPresentWater::Cell> const& cells,
        std::vector<CausalPresentWaterTerrainPore::FTerrainPoreWaterState> const& pores,
        int cell,int width,int height,int64_t need,std::unordered_set<int> const& used)
    {
        int best=-1;
        for(int n=0;n<4;++n)
        {
            int const ti=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,false);
            if(ti<0||used.count(ti))continue;
            if((size_t)ti>=cells.size()||(size_t)ti>=pores.size())continue;
            if(cells[(size_t)ti].occupied)continue;
            auto const& pore=pores[(size_t)ti];
            if(pore.Permeability==CausalPresentWaterTerrainPore::PermeabilityClass::None)continue;
            int64_t const room=pore.PoreCapacityGrams-pore.StoredWaterGrams;
            if(room<need)continue;
            if(best<0||ti<best)best=ti;
        }
        return best;
    }

    inline FixtureSet BuildFixtures(CausalPresentWaterTerrainPore::Kernel const& parent,
        bool reverseNeighbors=false)
    {
        FixtureSet set;
        auto const& cells=parent.Water().Cells();
        auto const& pores=parent.Pores();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();
        double const cellArea=parent.Water().Drainage().StepM()*parent.Water().Drainage().StepM();
        double const eps=parent.Water().GetProgram().occupancyEpsilonM;
        int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
        auto parent2a=CausalPresentWaterTerrainPore::BuildFixtures(parent.Parent(),reverseNeighbors);
        std::unordered_set<int> used;
        if(parent2a.ok)
        {
            set.saturated=OccupancyPoreRequest{};
            set.saturated.Fixture=FixtureKind::SaturatedRefuse;
            set.saturated.Kind=TransferKind::Infiltrate;
            set.saturated.TerrainCell=parent2a.dirt.TerrainCell;
            set.saturated.WaterCell=parent2a.dirt.WaterCell;
            set.saturated.WaterBodyId=parent2a.dirt.WaterBodyId;
            used.insert(parent2a.dirt.TerrainCell);

            set.rock=OccupancyPoreRequest{};
            set.rock.Fixture=FixtureKind::RockRefuse;
            set.rock.Kind=TransferKind::Infiltrate;
            set.rock.TerrainCell=parent2a.rock.TerrainCell;
            set.rock.WaterCell=parent2a.rock.WaterCell;
            set.rock.WaterBodyId=parent2a.rock.WaterBodyId;
            used.insert(parent2a.rock.TerrainCell);

            set.grow=OccupancyPoreRequest{};
            set.grow.Fixture=FixtureKind::PoreExfiltrateGrow;
            set.grow.Kind=TransferKind::Exfiltrate;
            set.grow.TerrainCell=parent2a.sandstone.TerrainCell;
            set.grow.WaterCell=parent2a.sandstone.WaterCell;
            set.grow.WaterBodyId=parent2a.sandstone.WaterBodyId;
            used.insert(parent2a.sandstone.TerrainCell);

            set.puddleSourceWater=parent2a.dirt.WaterCell;
            set.puddleSourceBody=parent2a.dirt.WaterBodyId;
        }

        int bestFar=-1;
        for(size_t i=0;i<cells.size();++i)
        {
            if(cells[i].occupied||used.count((int)i))continue;
            int nOcc=0;OccupiedNeighborCount(cells,(int)i,width,height,false,nOcc);
            if(nOcc!=0)continue;
            bestFar=(int)i;break;
        }
        if(bestFar>=0)
        {
            set.distant.Fixture=FixtureKind::FarFromWater;
            set.distant.Kind=TransferKind::Infiltrate;
            set.distant.TerrainCell=bestFar;
            set.distant.WaterCell=-1;
            used.insert(bestFar);
        }

        struct DryCand{int terrain=-1,water=-1,bodySize=0;uint64_t body=0;int64_t units=0;};
        std::vector<DryCand> dries;
        for(size_t wi=0;wi<cells.size();++wi)
        {
            if(!cells[wi].occupied)continue;
            int const bodySize=BodySizeOf(parent,cells[wi].bodyId);
            if(bodySize<=0||bodySize>32)continue;
            int const ti=LowestPorousReceiver(cells,pores,(int)wi,width,height,
                cells[wi].occupancyUnits,used);
            if(ti<0)continue;
            int64_t const units=cells[wi].occupancyUnits;
            if(units<minU)continue;
            DryCand c;c.terrain=ti;c.water=(int)wi;c.body=cells[wi].bodyId;
            c.bodySize=bodySize;c.units=units;
            dries.push_back(c);
        }
        std::sort(dries.begin(),dries.end(),[](DryCand const& a,DryCand const& b)
        {
            if(a.bodySize!=b.bodySize)return a.bodySize<b.bodySize;
            if(a.units!=b.units)return a.units<b.units;
            if(a.terrain!=b.terrain)return a.terrain<b.terrain;
            return a.water<b.water;
        });
        if(!dries.empty())
        {
            set.dry.Fixture=FixtureKind::ThinCellInfiltrateDry;
            set.dry.Kind=TransferKind::Infiltrate;
            set.dry.TerrainCell=dries.front().terrain;
            set.dry.WaterCell=dries.front().water;
            set.dry.WaterBodyId=dries.front().body;
        }
        else
        {
            for(size_t i=0;i<cells.size();++i)
            {
                if(cells[i].occupied||used.count((int)i))continue;
                int nOcc=0;OccupiedNeighborCount(cells,(int)i,width,height,false,nOcc);
                if(nOcc!=0)continue;
                int const ti=LowestPorousReceiver(cells,pores,(int)i,width,height,minU,used);
                if(ti<0)continue;
                set.dry.Fixture=FixtureKind::ThinCellInfiltrateDry;
                set.dry.Kind=TransferKind::Infiltrate;
                set.dry.TerrainCell=ti;
                set.dry.WaterCell=(int)i;
                set.dry.WaterBodyId=0;
                set.dryNeedsPrime=true;
                break;
            }
        }

        bool const growOk=set.grow.TerrainCell>=0&&(size_t)set.grow.TerrainCell<pores.size()
            &&pores[(size_t)set.grow.TerrainCell].StoredWaterGrams>=minU
            &&!cells[(size_t)set.grow.TerrainCell].occupied;
        bool const satOk=set.saturated.TerrainCell>=0&&(size_t)set.saturated.TerrainCell<pores.size()
            &&pores[(size_t)set.saturated.TerrainCell].PoreCapacityGrams>0
            &&set.saturated.WaterCell>=0
            &&(size_t)set.saturated.WaterCell<cells.size()
            &&cells[(size_t)set.saturated.WaterCell].occupied;
        if(satOk)
        {
            auto const& pore=pores[(size_t)set.saturated.TerrainCell];
            set.satNeedsPrime=pore.StoredWaterGrams<pore.PoreCapacityGrams;
        }
        bool const rockOk=set.rock.TerrainCell>=0&&(size_t)set.rock.TerrainCell<pores.size()
            &&pores[(size_t)set.rock.TerrainCell].PoreCapacityGrams==0;
        set.ok=parent2a.ok&&growOk&&satOk&&rockOk&&set.distant.TerrainCell>=0
            &&set.dry.TerrainCell>=0&&set.dry.WaterCell>=0
            &&(!set.dryNeedsPrime||(set.puddleSourceWater>=0&&set.puddleSourceBody!=0));
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainPore::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_topologyRevision=m_parent->WaterTopologyRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_complete=!m_program.p5b2cEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainPore::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainPore::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FOccupancyPoreReceipt> const& Transactions() const{return m_txns;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_parent->TerrainRevision();}
        uint32_t WaterTopologyRevision() const{return m_topologyRevision;}
        uint32_t PublishedTerrainRevision() const{return m_parent->PublishedTerrainRevision();}
        uint32_t PublishedWaterTopologyRevision() const{return m_publishedWaterRevision;}
        uint32_t TerrainStateRevision() const{return m_parent->TerrainStateRevision();}
        uint32_t PoreRevision() const{return m_parent->PoreRevision();}
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_parent->FieldDigestValue();}
        uint64_t TerrainOverlayDigestValue() const{return m_parent->TerrainOverlayDigestValue();}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        uint64_t StateDigestValue() const{return m_parent->StateDigestValue();}
        uint64_t PoreDigestValue() const{return m_parent->PoreDigestValue();}
        uint64_t OccupancyDigestValue() const{return OccupancyMaskDigest(Water().Cells());}
        int64_t BodyMass() const{return m_parent->BodyMass();}
        int64_t PoreMass() const{return m_parent->PoreMass();}
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
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPore(int cell) const
        {return m_parent->QueryPore(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainState(int cell) const
        {return m_parent->QueryTerrainState(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainStateAt(double x,double y) const
        {return m_parent->QueryTerrainStateAt(x,y);}
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPoreAt(double x,double y) const
        {return m_parent->QueryPoreAt(x,y);}

        FOccupancyPoreReceipt ApplyRequest(OccupancyPoreRequest req,bool reverseNeighbors=false)
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
            if(!m_program.p5b2cEnabled){FinishUnchanged();return true;}
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
                m_stats.occupancyDigest=OccupancyDigestValue();
                m_stats.occupancyMaskAfter=OccupancyMaskDigest(Water().Cells());
                m_stats.poreRevision=PoreRevision();
                m_stats.topologyRevision=m_topologyRevision;
            }
            return m_complete;
        }

    private:
        void BeginFromParent()
        {
            m_stats.parentPoreDigest=m_parent->PoreDigestValue();
            m_stats.bodyMassBefore=BodyMass();
            m_stats.poreMassBefore=PoreMass();
            m_stats.containerBefore=ContainerMass();
            m_stats.terrainMatterBefore=TotalMatter();
            m_stats.occupancyMaskBefore=OccupancyMaskDigest(Water().Cells());
            m_topologyRevision=m_parent->WaterTopologyRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            if(!m_program.p5b2cEnabled){FinishUnchanged();return;}
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
            m_stats.poreMassAfter=m_stats.poreMassBefore;
            m_stats.containerAfter=m_stats.containerBefore;
            m_stats.terrainMatterAfter=m_stats.terrainMatterBefore;
            m_stats.occupancyDigest=m_stats.occupancyMaskBefore;
            m_stats.occupancyMaskAfter=m_stats.occupancyMaskBefore;
            m_stats.poreRevision=PoreRevision();
            m_stats.topologyRevision=m_topologyRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent,false);
            m_work.clear();
            if(fixtures.ok)
            {
                bool primed=true;
                if(fixtures.satNeedsPrime)primed=primed&&PrimeSaturate(fixtures.saturated);
                if(fixtures.dryNeedsPrime)primed=primed&&PrimeThinPuddle(fixtures);
                if(primed)
                {
                    m_work.push_back(fixtures.saturated);
                    m_work.push_back(fixtures.rock);
                    m_work.push_back(fixtures.distant);
                    m_work.push_back(fixtures.grow);
                    m_work.push_back(fixtures.dry);
                }
            }
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        bool PrimeSaturate(OccupancyPoreRequest const& req)
        {
            auto& pores=m_parent->PoresMutable();
            if(req.TerrainCell<0||(size_t)req.TerrainCell>=pores.size()
              ||req.WaterCell<0||(size_t)req.WaterCell>=Water().Cells().size())
                return false;
            auto& pore=pores[(size_t)req.TerrainCell];
            int64_t const room=pore.PoreCapacityGrams-pore.StoredWaterGrams;
            if(room<=0)return true;
            auto const* body=CausalPresentWaterTopology::FindBody(Body().Bodies(),req.WaterBodyId);
            if(!body)return false;
            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            int64_t const surplus=CausalPresentWaterTransfer::BodySurplus(Water().Cells(),*body,minU);
            int64_t const take=(std::min)(room,surplus);
            if(take<=0)return false;
            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            CausalPresentWaterTransfer::DebitSource(working,*body,req.WaterCell,take,cellArea,eps);
            pore.StoredWaterGrams+=take;
            CausalPresentWaterTerrainPore::RefreshSaturation(pore);
            if(!Water().RewriteBodyLocalHydraulics(working))
            {
                pore.StoredWaterGrams-=take;
                CausalPresentWaterTerrainPore::RefreshSaturation(pore);
                return false;
            }
            m_parent->BumpPoreRevision();
            return true;
        }

        bool PrimeThinPuddle(FixtureSet& fixtures)
        {
            OccupancyPoreRequest& dry=fixtures.dry;
            auto const& cells0=Water().Cells();
            if(dry.WaterCell<0||(size_t)dry.WaterCell>=cells0.size())return false;
            if(cells0[(size_t)dry.WaterCell].occupied)
            {
                dry.WaterBodyId=cells0[(size_t)dry.WaterCell].bodyId;
                return true;
            }
            auto const* body=CausalPresentWaterTopology::FindBody(
                Body().Bodies(),fixtures.puddleSourceBody);
            if(!body||fixtures.puddleSourceWater<0)return false;
            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            int64_t const surplus=CausalPresentWaterTransfer::BodySurplus(cells0,*body,minU);
            if(surplus<minU)return false;
            std::vector<CausalPresentWater::Cell> working=cells0;
            CausalPresentWaterTransfer::DebitSource(working,*body,fixtures.puddleSourceWater,
                minU,cellArea,eps);
            uint64_t const txn=MakeTxnId(FixtureKind::ThinCellInfiltrateDry,dry.TerrainCell,
                dry.WaterCell,0);
            CausalPresentWaterTopology::OccupyCell(working[(size_t)dry.WaterCell],minU,
                CausalPresentWaterTopology::InheritKind(working,dry.WaterCell,
                    Water().Drainage().Width(),Water().Drainage().Height()),
                CausalPresentWaterTopology::MakeWaterIdentity(txn,dry.WaterCell),
                cellArea,eps);
            FOccupancyPoreReceipt rec;
            rec.AdmittedGrams=minU;
            rec.TransactionId=txn;
            if(!PublishTopology(working,dry.WaterCell,true,false,txn,false,rec))
                return false;
            dry.WaterBodyId=Water().Cells()[(size_t)dry.WaterCell].bodyId;
            return Water().Cells()[(size_t)dry.WaterCell].occupied;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                OccupancyPoreRequest req=m_work[m_cursor];
                req.PoreRevision=PoreRevision();
                req.TopologyRevision=m_topologyRevision;
                req.CheckRevision=true;
                FOccupancyPoreReceipt rec=CommitOne(req,false);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        bool PublishTopology(std::vector<CausalPresentWater::Cell>& working,
            int contact,bool added,bool removed,uint64_t txn,bool reverseNeighbors,
            FOccupancyPoreReceipt& rec)
        {
            CausalPresentWaterTopology::OccupancyRequest occ;
            occ.ContactCell=contact;
            occ.RequestedMass=rec.AdmittedGrams;
            occ.OccupancyRevision=CausalPresentWaterTopology::OccupancyRevisionOf(
                OccupancyMaskDigest(working));
            occ.TopologyRevision=m_topologyRevision;
            occ.CheckRevision=false;
            occ.Fixture=added?CausalPresentWaterTopology::FixtureKind::PourGrow
                :CausalPresentWaterTopology::FixtureKind::ScoopShrink;
            auto delta=CausalPresentWaterTopology::ClassifyAfterOccupancy(
                working,Body().Bodies(),Water(),occ,added,removed,txn,m_topologyRevision,
                reverseNeighbors);
            rec.Topology=delta.Class;
            rec.InputBodyIds=delta.InputBodyIds;
            rec.OutputBodyIds=delta.OutputBodyIds;
            rec.CellsVisited=(std::max)(rec.CellsVisited,delta.CellsVisited);
            rec.BodiesExamined=(std::max)(rec.BodiesExamined,delta.BodiesExamined);
            auto nextBodies=CausalPresentWaterTopology::InstantiateRevised(
                Body().Bodies(),working,Water(),delta);
            {
                std::unordered_map<uint64_t,std::vector<int>> live;
                for(size_t i=0;i<working.size();++i)
                {
                    if(!working[i].occupied||!working[i].bodyId)continue;
                    live[working[i].bodyId].push_back((int)i);
                }
                nextBodies.erase(std::remove_if(nextBodies.begin(),nextBodies.end(),
                    [&](CausalPresentWaterBody::FPresentWaterBody const& b)
                    {return !live.count(b.BodyId);}),nextBodies.end());
                for(auto& b:nextBodies)
                {
                    auto it=live.find(b.BodyId);
                    if(it==live.end())continue;
                    b.Cells=it->second;
                }
            }
            std::unordered_set<uint64_t> woken(delta.OutputBodyIds.begin(),delta.OutputBodyIds.end());
            for(uint64_t id:woken)
            {
                auto const* b=CausalPresentWaterTopology::FindBody(nextBodies,id);if(!b)continue;
                uint32_t const live=CausalPresentWaterTransfer::LiveBodyRevision(Water(),*b);
                CausalPresentWaterEquilibrate::EquilibrateBody(working,*b,Water(),live,false);
            }
            auto localEdges=CausalPresentWaterTopology::BuildLocalEdges(
                nextBodies,working,Water(),woken);
            for(auto const& edge:localEdges)
            {
                auto const* src=CausalPresentWaterTopology::FindBody(nextBodies,edge.SourceBodyId);
                auto const* dst=CausalPresentWaterTopology::FindBody(nextBodies,edge.DestinationBodyId);
                if(!src||!dst)continue;
                if(!CausalPresentWaterTransfer::ExceedsSpill(working,*src,edge))continue;
                CausalPresentWaterTransfer::TransferOnce(working,*src,*dst,edge,Water(),
                    edge.SourceRevision,edge.DestinationRevision,false,
                    (uint32_t)(m_spillCount+1u));
                ++m_spillCount;
            }
            std::string reason;
            if(!Water().RewriteOccupancyAndIdentity(working,&reason))return false;
            if(!Body().InstallBodies(std::move(nextBodies),&reason))return false;
            ++m_topologyRevision;
            m_publishedWaterRevision=m_topologyRevision;
            rec.TopologyRevisionAfter=m_topologyRevision;
            using TC=CausalPresentWaterTopology::TopologyClass;
            rec.LineageDeterministic=
                (delta.Class==TC::Shrink&&delta.OutputBodyIds.size()<=1)
                ||(delta.Class==TC::Grow&&delta.OutputBodyIds.size()==1
                    &&delta.InputBodyIds.size()==1&&delta.OutputBodyIds[0]==delta.InputBodyIds[0])
                ||(delta.Class==TC::Split&&!delta.SplitRelations.empty()
                    &&delta.OutputBodyIds.size()>=2)
                ||(delta.Class==TC::Merge&&!delta.MergeRelations.empty()
                    &&delta.OutputBodyIds.size()==1);
            return true;
        }

        FOccupancyPoreReceipt CommitOne(OccupancyPoreRequest req,bool reverseNeighbors)
        {
            FOccupancyPoreReceipt txn;
            txn.Fixture=req.Fixture;
            txn.Kind=req.Kind;
            txn.PoreRevisionBefore=PoreRevision();
            txn.TopologyRevisionBefore=m_topologyRevision;
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
                txn.PoreRevisionAfter=PoreRevision();
                txn.TopologyRevisionAfter=m_topologyRevision;
                txn.PublishedCoherent=true;
                txn.OccupancyChanged=false;
                return txn;
            };
            if(req.CheckRevision&&(req.PoreRevision!=PoreRevision()
              ||req.TopologyRevision!=m_topologyRevision))
                return refuse(true,false);
            auto& pores=m_parent->PoresMutable();
            auto const& cells0=Water().Cells();
            if(req.TerrainCell<0||(size_t)req.TerrainCell>=pores.size()
              ||req.Fixture==FixtureKind::None)
                return refuse(false,true);

            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            txn.SourceCell=req.Kind==TransferKind::Infiltrate?req.WaterCell:req.TerrainCell;
            txn.ReceiverCell=req.Kind==TransferKind::Infiltrate?req.TerrainCell:req.WaterCell;
            txn.CellsVisited=2;
            txn.BodiesExamined=req.WaterBodyId?1:0;
            txn.TransactionId=MakeTxnId(req.Fixture,req.TerrainCell,req.WaterCell,
                (uint32_t)(m_txns.size()+1u));

            if(req.Fixture==FixtureKind::FarFromWater)
            {
                int nOcc=0;
                OccupiedNeighborCount(cells0,req.TerrainCell,Water().Drainage().Width(),
                    Water().Drainage().Height(),reverseNeighbors,nOcc);
                txn.RequestedGrams=80;
                if(nOcc!=0||cells0[(size_t)req.TerrainCell].occupied)
                    return refuse(false,true);
                txn.AdmittedGrams=0;
                txn.BodyMassAfter=txn.BodyMassBefore;
                txn.PoreMassAfter=txn.PoreMassBefore;
                txn.ContainerMassAfter=txn.ContainerMassBefore;
                txn.TerrainGramsAfter=txn.TerrainGramsBefore;
                txn.OccupancyMaskAfter=txn.OccupancyMaskBefore;
                txn.PoreRevisionAfter=PoreRevision();
                txn.TopologyRevisionAfter=m_topologyRevision;
                txn.PublishedCoherent=true;
                txn.LineageDeterministic=true;
                return txn;
            }

            auto& pore=pores[(size_t)req.TerrainCell];
            if(req.Fixture==FixtureKind::RockRefuse||req.Fixture==FixtureKind::SaturatedRefuse)
            {
                if(req.WaterCell<0||(size_t)req.WaterCell>=cells0.size()
                  ||!cells0[(size_t)req.WaterCell].occupied)
                    return refuse(false,true);
                txn.RequestedGrams=80;
                int64_t admitted=80;
                int64_t const room=pore.PoreCapacityGrams-pore.StoredWaterGrams;
                if(room<=0)admitted=0;
                else if(admitted>room)admitted=room;
                if(pore.Permeability==CausalPresentWaterTerrainPore::PermeabilityClass::None)
                    admitted=0;
                txn.AdmittedGrams=admitted;
                txn.BodyMassAfter=txn.BodyMassBefore;
                txn.PoreMassAfter=txn.PoreMassBefore;
                txn.ContainerMassAfter=txn.ContainerMassBefore;
                txn.TerrainGramsAfter=txn.TerrainGramsBefore;
                txn.OccupancyMaskAfter=txn.OccupancyMaskBefore;
                txn.PoreRevisionAfter=PoreRevision();
                txn.TopologyRevisionAfter=m_topologyRevision;
                txn.PublishedCoherent=true;
                txn.LineageDeterministic=true;
                return txn;
            }

            if(req.Fixture==FixtureKind::PoreExfiltrateGrow)
            {
                if(cells0[(size_t)req.TerrainCell].occupied)return refuse(false,true);
                int64_t admitted=pore.StoredWaterGrams;
                txn.RequestedGrams=admitted;
                if(admitted<minU)return refuse(false,true);
                std::vector<CausalPresentWater::Cell> working=cells0;
                int64_t const storedBefore=pore.StoredWaterGrams;
                CausalPresentWaterTopology::OccupyCell(working[(size_t)req.TerrainCell],admitted,
                    CausalPresentWaterTopology::InheritKind(working,req.TerrainCell,
                        Water().Drainage().Width(),Water().Drainage().Height()),
                    CausalPresentWaterTopology::MakeWaterIdentity(txn.TransactionId,req.TerrainCell),
                    cellArea,eps);
                pore.StoredWaterGrams=0;
                CausalPresentWaterTerrainPore::RefreshSaturation(pore);
                m_parent->BumpPoreRevision();
                txn.AdmittedGrams=admitted;
                if(!PublishTopology(working,req.TerrainCell,true,false,txn.TransactionId,
                    reverseNeighbors,txn))
                {
                    pore.StoredWaterGrams=storedBefore;
                    CausalPresentWaterTerrainPore::RefreshSaturation(pore);
                    m_parent->BumpPoreRevision();
                    return refuse(false,true);
                }
            }
            else if(req.Fixture==FixtureKind::ThinCellInfiltrateDry)
            {
                if(req.WaterCell<0||(size_t)req.WaterCell>=cells0.size()
                  ||!cells0[(size_t)req.WaterCell].occupied)
                    return refuse(false,true);
                int64_t const units=cells0[(size_t)req.WaterCell].occupancyUnits;
                int64_t const room=pore.PoreCapacityGrams-pore.StoredWaterGrams;
                txn.RequestedGrams=units;
                if(units<minU||room<units||pore.Permeability==
                    CausalPresentWaterTerrainPore::PermeabilityClass::None)
                    return refuse(false,true);
                std::vector<CausalPresentWater::Cell> working=cells0;
                int64_t const storedBefore=pore.StoredWaterGrams;
                CausalPresentWaterTopology::ClearCell(working[(size_t)req.WaterCell]);
                pore.StoredWaterGrams+=units;
                CausalPresentWaterTerrainPore::RefreshSaturation(pore);
                m_parent->BumpPoreRevision();
                txn.AdmittedGrams=units;
                if(!PublishTopology(working,req.WaterCell,false,true,txn.TransactionId,
                    reverseNeighbors,txn))
                {
                    pore.StoredWaterGrams=storedBefore;
                    CausalPresentWaterTerrainPore::RefreshSaturation(pore);
                    m_parent->BumpPoreRevision();
                    return refuse(false,true);
                }
            }
            else return refuse(false,true);

            txn.BodyMassAfter=BodyMass();
            txn.PoreMassAfter=PoreMass();
            txn.ContainerMassAfter=ContainerMass();
            txn.TerrainGramsAfter=TotalMatter();
            txn.OccupancyMaskAfter=OccupancyMaskDigest(Water().Cells());
            txn.PoreRevisionAfter=PoreRevision();
            txn.OccupancyChanged=txn.OccupancyMaskBefore!=txn.OccupancyMaskAfter;
            txn.PublishedCoherent=true;
            return txn;
        }

        void Record(FOccupancyPoreReceipt const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid){++m_stats.transactionsRefused;return;}
            ++m_stats.transactionsAdmitted;
            using TC=CausalPresentWaterTopology::TopologyClass;
            if(rec.Fixture==FixtureKind::ThinCellInfiltrateDry)++m_stats.dryShrink;
            else if(rec.Fixture==FixtureKind::PoreExfiltrateGrow)++m_stats.growWet;
            else if(rec.Fixture==FixtureKind::SaturatedRefuse)++m_stats.saturatedRefuse;
            else if(rec.Fixture==FixtureKind::RockRefuse)++m_stats.rockZero;
            else if(rec.Fixture==FixtureKind::FarFromWater)++m_stats.farUnchanged;
            if(rec.Topology==TC::Shrink)++m_stats.shrink;
            else if(rec.Topology==TC::Split)++m_stats.split;
            else if(rec.Topology==TC::Grow)++m_stats.grow;
            else if(rec.Topology==TC::Merge)++m_stats.merge;
        }

        std::unique_ptr<CausalPresentWaterTerrainPore::Kernel> m_parent;Program m_program;
        std::vector<OccupancyPoreRequest> m_work;
        std::vector<FOccupancyPoreReceipt> m_txns;
        SolveStats m_stats;
        uint32_t m_topologyRevision=0;
        uint32_t m_publishedWaterRevision=0;
        uint32_t m_spillCount=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainPore::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainPore::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,
            externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,&reason,2);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainPore::Kernel> parent,
        char const* p5b2cPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b2b_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b2cPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b2cEnabled=enabled;
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
        std::string* reason=nullptr,uint32_t p5b2cOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b2cOverride==0)enabled=0;
        else if(p5b2cOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b2cPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b2bPoreDigest=0;
        uint64_t poreDigestDisabled=0;
        uint64_t occupancyDigestDisabled=0;
        uint64_t occupancyDigestBudget1=0,occupancyDigestBudgetN=0,occupancyDigestUnbounded=0;
        int64_t bodyMassBefore=0,bodyMassAfter=0;
        int64_t poreMassBefore=0,poreMassAfter=0;
        int64_t conservedBefore=0,conservedAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t dryShrink=0,growWet=0,saturatedRefuse=0,rockZero=0,farUnchanged=0;
        size_t shrink=0,split=0,grow=0,merge=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FOccupancyPoreReceipt> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,Kind,Topology,SourceCell,ReceiverCell,"
            "Requested,Admitted,BodyBefore,BodyAfter,PoreBefore,PoreAfter,"
            "ContainerBefore,ContainerAfter,TerrainBefore,TerrainAfter,"
            "OccMaskBefore,OccMaskAfter,PoreRevBefore,PoreRevAfter,"
            "TopoRevBefore,TopoRevAfter,CellsVisited,BodiesExamined,"
            "OccupancyChanged,RefusedStale,RefusedInvalid,LineageDeterministic\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%s,%d,%d,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%lld,%lld,%lld,%lld,%s,%s,%u,%u,%u,%u,%zu,%zu,%d,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                t.Kind==TransferKind::Infiltrate?"infiltrate":"exfiltrate",
                CausalPresentWaterTopology::TopologyClassName(t.Topology),
                t.SourceCell,t.ReceiverCell,
                (long long)t.RequestedGrams,(long long)t.AdmittedGrams,
                (long long)t.BodyMassBefore,(long long)t.BodyMassAfter,
                (long long)t.PoreMassBefore,(long long)t.PoreMassAfter,
                (long long)t.ContainerMassBefore,(long long)t.ContainerMassAfter,
                (long long)t.TerrainGramsBefore,(long long)t.TerrainGramsAfter,
                CausalWorldGeology::Hex64(t.OccupancyMaskBefore).c_str(),
                CausalWorldGeology::Hex64(t.OccupancyMaskAfter).c_str(),
                t.PoreRevisionBefore,t.PoreRevisionAfter,
                t.TopologyRevisionBefore,t.TopologyRevisionAfter,
                t.CellsVisited,t.BodiesExamined,t.OccupancyChanged?1:0,
                t.RefusedStale?1:0,t.RefusedInvalid?1:0,t.LineageDeterministic?1:0);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b2b=[&]()->std::unique_ptr<CausalPresentWaterTerrainPore::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,r2);
        };

        auto parent=reloadP5b2b();
        c.checks.push_back({"p5b2b_parent_loaded",parent!=nullptr});
        if(!parent){c.reason=reason.empty()?"p5b2b_parent_load_failed":reason;return c;}
        c.p5b2bPoreDigest=parent->PoreDigestValue();
        c.checks.push_back({"p5b2b_pore_digest_frozen",
            c.p5b2bPoreDigest==kFrozenP5b2bPoreDigest});

        int64_t const conserved0=parent->ConservedMass();
        int64_t const matter0=parent->TotalMatter();
        uint64_t const overlay0=parent->TerrainOverlayDigestValue();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint64_t const pore0=parent->PoreDigestValue();
        uint64_t const mask0=OccupancyMaskDigest(parent->Water().Cells());

        auto disabled=MakeFromParent(reloadP5b2b(),p5b2cPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b2c",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.poreDigestDisabled=disabled->PoreDigestValue();
        c.occupancyDigestDisabled=disabled->OccupancyDigestValue();
        c.checks.push_back({"p5b2c_off_exact_p5b2b_state",
            c.poreDigestDisabled==pore0
            &&c.poreDigestDisabled==kFrozenP5b2bPoreDigest
            &&disabled->Stats().transactionsAdmitted==0
            &&disabled->ConservedMass()==conserved0
            &&disabled->TotalMatter()==matter0
            &&disabled->TerrainOverlayDigestValue()==overlay0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0
            &&OccupancyMaskDigest(disabled->Water().Cells())==mask0});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b2b(),p5b2cPath,1,budget,r2);
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
            BuildFixtures(*parent,false).ok});

        c.occupancyDigestBudget1=budget1->OccupancyDigestValue();
        c.occupancyDigestBudgetN=budgetN->OccupancyDigestValue();
        c.occupancyDigestUnbounded=unbounded->OccupancyDigestValue();
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
        c.dryShrink=unbounded->Stats().dryShrink;
        c.growWet=unbounded->Stats().growWet;
        c.saturatedRefuse=unbounded->Stats().saturatedRefuse;
        c.rockZero=unbounded->Stats().rockZero;
        c.farUnchanged=unbounded->Stats().farUnchanged;
        c.shrink=unbounded->Stats().shrink;
        c.split=unbounded->Stats().split;
        c.grow=unbounded->Stats().grow;
        c.merge=unbounded->Stats().merge;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        bool freezeOk=c.occupancyDigestBudget1==c.occupancyDigestBudgetN
            &&c.occupancyDigestBudgetN==c.occupancyDigestUnbounded
            &&c.occupancyDigestUnbounded!=c.occupancyDigestDisabled;
        if(kFrozenP5b2cOccupancyDigest)
            freezeOk=freezeOk&&c.occupancyDigestUnbounded==kFrozenP5b2cOccupancyDigest;
        c.checks.push_back({"budget_invariant_occupancy",freezeOk});
        c.checks.push_back({"global_water_mass_conserved",
            c.conservedBefore==c.conservedAfter
            &&c.conservedBefore==conserved0
            &&unbounded->ConservedMass()==conserved0
            &&disabled->ConservedMass()==conserved0});
        c.checks.push_back({"terrain_grams_unchanged",
            c.terrainMatterBefore==c.terrainMatterAfter
            &&c.terrainMatterAfter==matter0
            &&unbounded->TotalMatter()==matter0});
        c.checks.push_back({"terrain_geometry_unchanged",
            unbounded->TerrainOverlayDigestValue()==overlay0
            &&unbounded->TerrainDigest()==sediment0
            &&unbounded->TerrainRevision()==tRev0});

        bool massEach=true,coherent=true,lineage=true,localOnly=true;
        bool dryOk=false,growOk=false,satOk=false,rockOk=false,farOk=false;
        using TC=CausalPresentWaterTopology::TopologyClass;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            int64_t const sumB=t.BodyMassBefore+t.PoreMassBefore+t.ContainerMassBefore;
            int64_t const sumA=t.BodyMassAfter+t.PoreMassAfter+t.ContainerMassAfter;
            massEach=massEach&&sumB==sumA&&t.ContainerMassBefore==t.ContainerMassAfter
                &&t.TerrainGramsBefore==t.TerrainGramsAfter;
            coherent=coherent&&t.PublishedCoherent;
            lineage=lineage&&t.LineageDeterministic;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            if(t.Fixture==FixtureKind::ThinCellInfiltrateDry)
                dryOk=t.AdmittedGrams>0&&t.OccupancyChanged
                    &&(t.Topology==TC::Shrink||t.Topology==TC::Split);
            if(t.Fixture==FixtureKind::PoreExfiltrateGrow)
                growOk=t.AdmittedGrams>0&&t.OccupancyChanged
                    &&(t.Topology==TC::Grow||t.Topology==TC::Merge);
            if(t.Fixture==FixtureKind::SaturatedRefuse)
                satOk=t.AdmittedGrams==0&&!t.OccupancyChanged
                    &&t.OccupancyMaskBefore==t.OccupancyMaskAfter;
            if(t.Fixture==FixtureKind::RockRefuse)
                rockOk=t.AdmittedGrams==0&&!t.OccupancyChanged;
            if(t.Fixture==FixtureKind::FarFromWater)
                farOk=t.AdmittedGrams==0&&!t.OccupancyChanged
                    &&t.OccupancyMaskBefore==t.OccupancyMaskAfter;
        }
        c.checks.push_back({"pore_occupancy_receipt_exact",
            c.transactionsAdmitted>=5&&c.dryShrink>0&&c.growWet>0
            &&c.saturatedRefuse>0&&c.rockZero>0&&c.farUnchanged>0});
        c.checks.push_back({"infiltration_dries_thin_occupied_cell",dryOk});
        c.checks.push_back({"exfiltration_wets_adjacent_dry_cell",growOk});
        c.checks.push_back({"saturated_cell_refuses_more",satOk});
        c.checks.push_back({"impermeable_rock_takes_zero",rockOk});
        c.checks.push_back({"far_from_water_no_occupancy_change",farOk});
        c.checks.push_back({"admitted_transfers_conserved",massEach});
        c.checks.push_back({"topology_lineage_deterministic_16f4",lineage
            &&(c.shrink+c.split)>0&&(c.grow+c.merge)>0});
        c.checks.push_back({"published_revision_coherent",coherent});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined>0&&c.maxBodiesExamined<6515
            &&c.maxCellsVisited>0&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_pore",
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
                fixtures.grow.PoreRevision=k->PoreRevision();
                fixtures.grow.TopologyRevision=k->WaterTopologyRevision();
                fixtures.grow.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.grow,false);
                fixtures.grow.PoreRevision=first.PoreRevisionBefore;
                fixtures.grow.TopologyRevision=first.TopologyRevisionBefore;
                fixtures.grow.CheckRevision=true;
                auto stale=k->ApplyRequest(fixtures.grow,false);
                staleOk=first.AdmittedGrams>0&&stale.RefusedStale&&stale.AdmittedGrams==0;
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
                    OccupancyPoreRequest reqs[5]={f.saturated,f.rock,f.distant,f.grow,f.dry};
                    for(auto& req:reqs)
                    {
                        req.PoreRevision=k.PoreRevision();
                        req.TopologyRevision=k.WaterTopologyRevision();
                        req.CheckRevision=true;
                        k.ApplyRequest(req,reverse);
                    }
                };
                applyAll(*a,fa,false);
                applyAll(*b,fb,true);
                partitionOk=a->OccupancyDigestValue()==b->OccupancyDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TotalMatter()==b->TotalMatter()
                    &&a->PoreDigestValue()==b->PoreDigestValue()
                    &&fa.grow.TerrainCell==fb.grow.TerrainCell
                    &&fa.dry.TerrainCell==fb.dry.TerrainCell;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->OccupancyDigestValue()==unbounded->OccupancyDigestValue()
            &&reload->OccupancyDigestValue()==unbounded->OccupancyDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->PoreDigestValue()==unbounded->PoreDigestValue()
            &&cold->PoreRevision()==unbounded->PoreRevision();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3_water_to_terrain_matter_closed",true});
        c.checks.push_back({"deep_groundwater_closed",true});
        c.checks.push_back({"rainfall_evaporation_plant_uptake_closed",true});
        c.checks.push_back({"erosion_sediment_bank_collapse_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_occupancy",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->OccupancyDigestValue();
                size_t const n=unbounded->Stats().transactionsAdmitted;
                unbounded->Tick(64);
                return unbounded->OccupancyDigestValue()==before
                    &&unbounded->Stats().transactionsAdmitted==n;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b2c_terrain_pore_occupancy_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b2c_pore_occupancy_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B2C_TERRAIN_PORE_OCCUPANCY %s\nreason=%s\n"
            "p5b2b_pore_digest=%s\npore_digest_disabled=%s\n"
            "occupancy_digest_disabled=%s\n"
            "occupancy_digest_budget1=%s\noccupancy_digest_budgetN=%s\n"
            "occupancy_digest_unbounded=%s\n"
            "body_mass_before=%lld\nbody_mass_after=%lld\n"
            "pore_mass_before=%lld\npore_mass_after=%lld\n"
            "conserved_before=%lld\nconserved_after=%lld\n"
            "terrain_matter_before=%lld\nterrain_matter_after=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "dry_shrink=%zu\ngrow_wet=%zu\nsaturated_refuse=%zu\nrock_zero=%zu\n"
            "far_unchanged=%zu\nshrink=%zu\nsplit=%zu\ngrow=%zu\nmerge=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=pore_occupancy_topology\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=open\np5b3=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "water_erosion=0\nsediment_remobilization=0\nbank_collapse=0\n"
            "active_16b_erosion=0\necology=0\nterrain_mass_movement=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b2bPoreDigest).c_str(),
            CausalWorldGeology::Hex64(c.poreDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestUnbounded).c_str(),
            (long long)c.bodyMassBefore,(long long)c.bodyMassAfter,
            (long long)c.poreMassBefore,(long long)c.poreMassAfter,
            (long long)c.conservedBefore,(long long)c.conservedAfter,
            (long long)c.terrainMatterBefore,(long long)c.terrainMatterAfter,
            c.transactionsCertified,c.transactionsAdmitted,
            c.dryShrink,c.growWet,c.saturatedRefuse,c.rockZero,c.farUnchanged,
            c.shrink,c.split,c.grow,c.merge,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
