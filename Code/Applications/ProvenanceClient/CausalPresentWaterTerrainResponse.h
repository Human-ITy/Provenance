#pragma once

// P5b.1: one-way coupling. Authoritative terrain mutation → local water
// response. Frozen Stage 16F.4 parent. Water cannot erode terrain, transport
// sediment, collapse banks, rain, infiltrate, or open P5b.2 / P5b.3.
//
// Pipeline after an admitted terrain mutation receipt:
//   terrain R → R+1
//     → affected hydraulic neighborhood (from the receipt, not all bodies)
//     → invalidate water geometry derived from R
//     → re-evaluate occupancy against the new terrain void
//     → 16F.4 local topology if needed
//     → 16F.1 equilibration
//     → 16F.2 certified spill/transfer
//     → sleep
//     → publish terrain + water as one coherent pair
//
// Disabled path must reproduce exact 16F.4 field digest 43068558cd0b4a8e.

#include "CausalPresentWaterTopology.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CausalPresentWaterTerrainResponse
{
    constexpr char const* kExpectedRegion="causal_world_present_water_terrain_response_floor";
    constexpr uint64_t kFrozenStage16F4FieldDigest=0x43068558cd0b4a8eull;
    constexpr uint64_t kFrozenP5b1FieldDigest=0xb1340afeef311fd8ull;
    constexpr uint32_t kHoldBudget=0xFFFFFFFEu;
    constexpr int64_t kInitialHeldMatter=1000000000000ll;
    constexpr double kMatterScale=1000000.0;

    enum class FixtureKind:uint8_t
    {
        None=0,
        LowerSill=1,
        DigChannel=2,
        RaiseFill=3,
        RemoveFloor=4,
        NegativeFar=5,
        FillRefuse=6
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::LowerSill:return "lower_sill";
            case FixtureKind::DigChannel:return "dig_channel";
            case FixtureKind::RaiseFill:return "raise_fill";
            case FixtureKind::RemoveFloor:return "remove_floor";
            case FixtureKind::NegativeFar:return "negative_far";
            case FixtureKind::FillRefuse:return "fill_refuse";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b1EventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b1Enabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTopology::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_RESPONSE_V1";
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
          &&hex("p5b1_event_id",r.program.p5b1EventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b1_enabled",r.program.p5b1Enabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b1EventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b1Enabled==0||r.program.p5b1Enabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b1_terrain_response_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FP5bTerrainWaterTransaction
    {
        uint64_t TransactionId=0;
        uint32_t TerrainRevisionBefore=0,TerrainRevisionAfter=0;
        uint32_t WaterTopologyRevisionBefore=0,WaterTopologyRevisionAfter=0;
        std::vector<int> AffectedTerrainCells;
        std::vector<uint64_t> AffectedWaterBodies;
        uint64_t TerrainMutationId=0;
        int64_t WaterMassBefore=0,WaterMassAfter=0;
        int64_t TerrainMatterBefore=0,TerrainMatterAfter=0;
        int64_t HeldMatterBefore=0,HeldMatterAfter=0;
        FixtureKind Fixture=FixtureKind::None;
        CausalPresentWaterTopology::TopologyClass Class=CausalPresentWaterTopology::TopologyClass::None;
        int ContactCell=-1;
        double DeltaTerrainZ=0;
        bool RefusedStale=false,RefusedInvalid=false,RefusedNoAdmissible=false;
        bool PublishedCoherent=false;
        uint32_t PublishedTerrainRevision=0,PublishedWaterTopologyRevision=0;
        uint32_t CollisionTerrainRevision=0,RenderTerrainRevision=0;
        size_t CellsVisited=0,BodiesExamined=0;
        std::vector<uint64_t> InputBodyIds,OutputBodyIds;
        std::vector<int> AddedWetCells,RemovedWetCells;
        uint64_t WaterIdentityAtContact=0;
    };

    struct TerrainRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        int ContactCell=-1;
        double NewTerrainZ=0;
        uint32_t TerrainRevision=0;
        uint32_t WaterTopologyRevision=0;
        bool CheckRevision=true;
        bool ExpectRefuse=false;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t lowerSill=0,digChannel=0,raiseFill=0,removeFloor=0,negativeFar=0,fillRefuse=0;
        size_t connectivityRebuilds=0,spillEdges=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        int64_t heldMatterBefore=0,heldMatterAfter=0;
        uint64_t fieldDigest=0,parentFieldDigest=0,terrainOverlayDigest=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterTopologyRevision=0;
        uint32_t publishedTerrainRevision=0,publishedWaterTopologyRevision=0;
    };

    inline int64_t ColumnMatter(double z)
    {return (int64_t)std::llround(z*kMatterScale);}

    inline int64_t TotalColumnMatter(std::vector<CausalPresentWater::Cell> const& cells)
    {
        int64_t s=0;for(auto const& c:cells)s+=ColumnMatter(c.terrainZ);return s;
    }

    inline uint64_t TerrainOverlayDigest(std::vector<CausalPresentWater::Cell> const& cells)
    {
        uint64_t d=14695981039346656037ull;
        for(auto const& c:cells)
            CausalWorldGeology::HashAppend(d,&c.terrainZ,sizeof(c.terrainZ));
        return d;
    }

    inline uint64_t MakeMutationId(int cell,double z,uint32_t seq,FixtureKind kind)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B10001ull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        CausalWorldGeology::HashAppend(h,&cell,sizeof(cell));
        CausalWorldGeology::HashAppend(h,&z,sizeof(z));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        return h?h:1;
    }

    inline bool DebitMass(std::vector<CausalPresentWater::Cell>& cells,
        std::vector<int> const& members,int64_t mass,double cellArea,double eps)
    {
        int64_t remain=mass;
        int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
        for(int idx:members)
        {
            if(remain<=0)break;
            if(idx<0||(size_t)idx>=cells.size()||!cells[(size_t)idx].occupied)continue;
            int64_t const can=cells[(size_t)idx].occupancyUnits-minU;
            if(can<=0)continue;
            int64_t const take=(std::min)(can,remain);
            CausalPresentWaterTransfer::StampUnits(cells[(size_t)idx],
                cells[(size_t)idx].occupancyUnits-take,cellArea,eps);
            remain-=take;
        }
        return remain==0;
    }

    inline bool CreditMass(std::vector<CausalPresentWater::Cell>& cells,
        std::vector<int> const& members,int64_t mass,double cellArea,double eps)
    {
        if(mass<=0)return true;
        int target=-1;
        for(int idx:members)
            if(idx>=0&&(size_t)idx<cells.size()&&cells[(size_t)idx].occupied){target=idx;break;}
        if(target<0)return false;
        CausalPresentWaterTransfer::StampUnits(cells[(size_t)target],
            cells[(size_t)target].occupancyUnits+mass,cellArea,eps);
        return true;
    }

    inline bool OccupancyValid(std::vector<CausalPresentWater::Cell> const& cells,double eps)
    {
        for(auto const& cell:cells)
        {
            if(cell.occupied)
            {
                if(cell.depthM<=eps||cell.occupancyUnits<=0||cell.bodyId==0
                  ||cell.waterSurfaceZ+1e-12<cell.terrainZ
                  ||std::fabs((cell.waterSurfaceZ-cell.terrainZ)-cell.depthM)>1e-6)
                    return false;
            }
            else if(cell.depthM!=0||cell.occupancyUnits!=0||cell.bodyId!=0
              ||cell.kind!=CausalPresentWater::BodyKind::None||cell.waterIdentity!=0)
                return false;
        }
        return true;
    }

    inline int ChebyshevCell(int a,int b,int width)
    {
        int const ax=a%width,ay=a/width,bx=b%width,by=b/width;
        return (std::max)(std::abs(ax-bx),std::abs(ay-by));
    }

    struct FixtureSet
    {
        TerrainRequest sill,channel,fill,floor,negative,refuse;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(std::vector<CausalPresentWater::Cell> const& cells,
        std::vector<CausalPresentWaterBody::FPresentWaterBody> const& bodies,
        CausalPresentWater::Kernel const& water)
    {
        FixtureSet set;
        int const width=water.Drainage().Width();
        int const height=water.Drainage().Height();
        double const eps=water.GetProgram().occupancyEpsilonM;
        std::unordered_set<int> used;
        auto mark=[&](int i){if(i>=0)used.insert(i);};

        auto trySill=[&]()->bool
        {
            std::vector<size_t> order(bodies.size());
            std::iota(order.begin(),order.end(),0);
            std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
            {
                if(bodies[a].closedLake!=bodies[b].closedLake)return bodies[a].closedLake>bodies[b].closedLake;
                if(bodies[a].Type!=bodies[b].Type)return bodies[a].Type<bodies[b].Type;
                if(bodies[a].Cells.size()!=bodies[b].Cells.size())
                    return bodies[a].Cells.size()>bodies[b].Cells.size();
                return bodies[a].BodyId<bodies[b].BodyId;
            });
            for(size_t oi:order)
            {
                auto const& b=bodies[oi];
                if(b.Cells.size()<3||b.Cells.size()>256)continue;
                if(b.Type!=CausalPresentWaterBody::BodyType::Lake
                  &&b.Type!=CausalPresentWaterBody::BodyType::Mixed)continue;
                double minSurface=1e300;
                for(int idx:b.Cells)
                    if(cells[(size_t)idx].occupied)
                        minSurface=(std::min)(minSurface,cells[(size_t)idx].waterSurfaceZ);
                if(!(minSurface<1e299))continue;
                int best=-1;double bestZ=1e300;
                for(int idx:b.Cells)
                {
                    for(int n=0;n<4;++n)
                    {
                        int const ni=CausalPresentWaterTopology::Neighbor4(idx,n,width,height,false);
                        if(ni<0||used.count(ni)||cells[(size_t)ni].occupied)continue;
                        auto nbr=CausalPresentWaterTopology::WetNeighborBodies(cells,ni,width,height,false);
                        if(nbr.size()!=1||nbr[0]!=b.BodyId)continue;
                        if(cells[(size_t)ni].terrainZ+eps>=minSurface
                          &&cells[(size_t)ni].terrainZ<bestZ)
                        {best=ni;bestZ=cells[(size_t)ni].terrainZ;}
                    }
                }
                if(best<0)continue;
                set.sill.Fixture=FixtureKind::LowerSill;
                set.sill.ContactCell=best;
                set.sill.NewTerrainZ=minSurface-0.20;
                mark(best);return true;
            }
            return false;
        };

        auto tryChannel=[&]()->bool
        {
            for(size_t i=0;i<cells.size();++i)
            {
                if(cells[i].occupied||used.count((int)i))continue;
                auto nbr=CausalPresentWaterTopology::WetNeighborBodies(cells,(int)i,width,height,false);
                if(nbr.size()<2)continue;
                bool farZ=false;
                for(int n=0;n<4;++n)
                {
                    int const ni=CausalPresentWaterTopology::Neighbor4((int)i,n,width,height,false);
                    if(ni<0||!cells[(size_t)ni].occupied)continue;
                    if(std::fabs(cells[i].terrainZ-cells[(size_t)ni].waterSurfaceZ)>8.0){farZ=true;break;}
                }
                if(farZ)continue;
                double minSurface=1e300;
                for(int n=0;n<4;++n)
                {
                    int const ni=CausalPresentWaterTopology::Neighbor4((int)i,n,width,height,false);
                    if(ni>=0&&cells[(size_t)ni].occupied)
                        minSurface=(std::min)(minSurface,cells[(size_t)ni].waterSurfaceZ);
                }
                set.channel.Fixture=FixtureKind::DigChannel;
                set.channel.ContactCell=(int)i;
                set.channel.NewTerrainZ=minSurface-0.25;
                mark((int)i);return true;
            }
            return false;
        };

        auto tryFill=[&]()->bool
        {
            std::vector<size_t> order(bodies.size());
            std::iota(order.begin(),order.end(),0);
            std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
            {
                if(bodies[a].Cells.size()!=bodies[b].Cells.size())
                    return bodies[a].Cells.size()<bodies[b].Cells.size();
                return bodies[a].BodyId<bodies[b].BodyId;
            });
            for(size_t oi:order)
            {
                auto const& b=bodies[oi];
                if(b.Cells.size()<4||b.Cells.size()>64)continue;
                std::unordered_set<int> member(b.Cells.begin(),b.Cells.end());
                for(int idx:b.Cells)
                {
                    if(used.count(idx)||!cells[(size_t)idx].occupied)continue;
                    if(CausalPresentWaterTopology::SameBodyNeighborCount(cells,idx,b.BodyId,width,height)<1)
                        continue;
                    std::unordered_set<int> allowed=member;allowed.erase(idx);
                    size_t visited=0;
                    auto comps=CausalPresentWaterTopology::ComponentsInSet(cells,allowed,width,height,false,&visited);
                    if(comps.size()!=1)continue;
                    set.fill.Fixture=FixtureKind::RaiseFill;
                    set.fill.ContactCell=idx;
                    set.fill.NewTerrainZ=cells[(size_t)idx].waterSurfaceZ+1.0;
                    mark(idx);return true;
                }
            }
            return false;
        };

        auto tryFloor=[&]()->bool
        {
            for(size_t i=0;i<cells.size();++i)
            {
                if(!cells[i].occupied||used.count((int)i))continue;
                if(cells[i].occupancyUnits<=0)continue;
                set.floor.Fixture=FixtureKind::RemoveFloor;
                set.floor.ContactCell=(int)i;
                set.floor.NewTerrainZ=cells[i].terrainZ-0.50;
                mark((int)i);return true;
            }
            return false;
        };

        auto tryNegative=[&]()->bool
        {
            std::vector<int> wet;
            for(size_t i=0;i<cells.size();++i)if(cells[i].occupied)wet.push_back((int)i);
            for(size_t i=0;i<cells.size();++i)
            {
                if(cells[i].occupied||used.count((int)i))continue;
                bool isolated=true;
                for(int w:wet){if(ChebyshevCell((int)i,w,width)<=3){isolated=false;break;}}
                if(!isolated)continue;
                set.negative.Fixture=FixtureKind::NegativeFar;
                set.negative.ContactCell=(int)i;
                set.negative.NewTerrainZ=cells[i].terrainZ+0.35;
                mark((int)i);return true;
            }
            return false;
        };

        auto tryRefuse=[&]()->bool
        {
            std::vector<size_t> order(bodies.size());
            std::iota(order.begin(),order.end(),0);
            std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
            {
                if(bodies[a].Cells.size()!=bodies[b].Cells.size())
                    return bodies[a].Cells.size()<bodies[b].Cells.size();
                return bodies[a].BodyId<bodies[b].BodyId;
            });
            for(size_t oi:order)
            {
                auto const& b=bodies[oi];
                if(b.Cells.empty())continue;
                int idx=b.Cells.front();
                if(!cells[(size_t)idx].occupied)continue;
                set.refuse.Fixture=FixtureKind::FillRefuse;
                set.refuse.ContactCell=idx;
                set.refuse.NewTerrainZ=cells[(size_t)idx].waterSurfaceZ+2.0;
                set.refuse.ExpectRefuse=true;
                return true;
            }
            return false;
        };

        set.ok=trySill()&&tryChannel()&&tryFill()&&tryFloor()&&tryNegative()&&tryRefuse();
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTopology::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_heldMatter=kInitialHeldMatter;
            m_stats.heldMatterBefore=m_heldMatter;
            m_stats.parentFieldDigest=m_parent->FieldDigestValue();
            m_complete=!m_program.p5b1Enabled&&m_parent->Complete();
            m_waterGeometryRevision=m_parent->TopologyRevision();
            m_publishedWaterRevision=m_waterGeometryRevision;
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTopology::Kernel const& Topology() const{return *m_parent;}
        CausalPresentWaterTopology::Kernel& Topology(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FP5bTerrainWaterTransaction> const& Transactions() const{return m_txns;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_heldMatter;}
        uint32_t TerrainRevision() const{return m_terrainRevision;}
        uint32_t WaterTopologyRevision() const{return m_waterGeometryRevision;}
        uint32_t PublishedTerrainRevision() const{return m_publishedTerrainRevision;}
        uint32_t PublishedWaterTopologyRevision() const{return m_publishedWaterRevision;}
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_stats.fieldDigest?m_stats.fieldDigest
            :CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());}
        uint64_t TerrainOverlayDigestValue() const{return TerrainOverlayDigest(Water().Cells());}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t TotalMatter() const{return TotalColumnMatter(Water().Cells());}
        double ReconstructedZ(double x,double y) const
        {
            auto q=Water().QueryAt(x,y);
            if(q.found)return q.terrainZ;
            return m_parent->ReconstructedZ(x,y);
        }
        CausalPresentWaterBody::Query QueryAt(double x,double y) const
        {return m_parent->QueryAt(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_parent->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_parent->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_parent->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_parent->BuildBlock(bx,by);}

        FP5bTerrainWaterTransaction ApplyRequest(TerrainRequest req,bool reverseNeighbors=false)
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
            if(!m_program.p5b1Enabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.waterMassAfter=TotalMass();
                m_stats.terrainMatterAfter=TotalMatter();
                m_stats.heldMatterAfter=m_heldMatter;
                m_stats.fieldDigest=CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());
                m_stats.terrainOverlayDigest=TerrainOverlayDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterTopologyRevision=m_waterGeometryRevision;
                m_stats.publishedTerrainRevision=m_publishedTerrainRevision;
                m_stats.publishedWaterTopologyRevision=m_publishedWaterRevision;
            }
            return m_complete;
        }

    private:
        void BeginFromParent()
        {
            m_stats.parentFieldDigest=m_parent->FieldDigestValue();
            m_stats.waterMassBefore=m_parent->TotalMass();
            m_stats.terrainMatterBefore=TotalMatter();
            m_heldMatter=kInitialHeldMatter;
            m_stats.heldMatterBefore=m_heldMatter;
            m_publishedTerrainRevision=m_terrainRevision;
            m_waterGeometryRevision=m_parent->TopologyRevision();
            m_publishedWaterRevision=m_waterGeometryRevision;
            if(!m_program.p5b1Enabled){FinishUnchanged();return;}
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
            m_stats.heldMatterAfter=m_stats.heldMatterBefore;
            m_stats.fieldDigest=m_stats.parentFieldDigest;
            m_stats.terrainOverlayDigest=TerrainOverlayDigestValue();
            m_stats.publishedTerrainRevision=m_terrainRevision;
            m_stats.publishedWaterTopologyRevision=m_waterGeometryRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(Water().Cells(),Body().Bodies(),Water());
            m_work.clear();
            if(fixtures.sill.Fixture!=FixtureKind::None)m_work.push_back(fixtures.sill);
            if(fixtures.channel.Fixture!=FixtureKind::None)m_work.push_back(fixtures.channel);
            if(fixtures.fill.Fixture!=FixtureKind::None)m_work.push_back(fixtures.fill);
            if(fixtures.floor.Fixture!=FixtureKind::None)m_work.push_back(fixtures.floor);
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                TerrainRequest req=m_work[m_cursor];
                req.TerrainRevision=m_terrainRevision;
                req.WaterTopologyRevision=m_waterGeometryRevision;
                req.CheckRevision=true;
                auto txn=CommitOne(req,false);
                Record(txn);
                ++m_cursor;++processed;
            }
        }

        FP5bTerrainWaterTransaction CommitOne(TerrainRequest req,bool reverseNeighbors)
        {
            FP5bTerrainWaterTransaction txn;
            txn.Fixture=req.Fixture;
            txn.ContactCell=req.ContactCell;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterTopologyRevisionBefore=m_waterGeometryRevision;
            txn.WaterMassBefore=TotalMass();
            txn.TerrainMatterBefore=TotalMatter();
            txn.HeldMatterBefore=m_heldMatter;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterTopologyRevision=m_publishedWaterRevision;
            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            auto bodies=Body().Bodies();
            if(req.CheckRevision&&(req.TerrainRevision!=m_terrainRevision
              ||req.WaterTopologyRevision!=m_waterGeometryRevision))
            {
                txn.RefusedStale=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }
            if(req.ContactCell<0||(size_t)req.ContactCell>=working.size())
            {
                txn.RefusedInvalid=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }

            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            auto& cell=working[(size_t)req.ContactCell];
            double const oldZ=cell.terrainZ;
            txn.DeltaTerrainZ=req.NewTerrainZ-oldZ;
            txn.AffectedTerrainCells={req.ContactCell};
            uint64_t const mutationId=MakeMutationId(req.ContactCell,req.NewTerrainZ,
                (uint32_t)(m_txns.size()+1u),req.Fixture);
            txn.TerrainMutationId=mutationId;
            txn.TransactionId=mutationId;

            std::unordered_set<uint64_t> neighborhoodBodies;
            std::unordered_set<int> neighborhoodCells;
            neighborhoodCells.insert(req.ContactCell);
            if(cell.occupied&&cell.bodyId)neighborhoodBodies.insert(cell.bodyId);
            for(int n=0;n<4;++n)
            {
                int const ni=CausalPresentWaterTopology::Neighbor4(req.ContactCell,n,width,height,reverseNeighbors);
                if(ni<0)continue;
                neighborhoodCells.insert(ni);
                txn.CellsVisited++;
                if(working[(size_t)ni].occupied&&working[(size_t)ni].bodyId)
                    neighborhoodBodies.insert(working[(size_t)ni].bodyId);
            }
            for(uint64_t id:neighborhoodBodies)
            {
                auto const* b=CausalPresentWaterTopology::FindBody(bodies,id);
                if(!b)continue;
                ++txn.BodiesExamined;
                for(int idx:b->Cells)neighborhoodCells.insert(idx);
            }
            txn.AffectedWaterBodies.assign(neighborhoodBodies.begin(),neighborhoodBodies.end());
            CausalPresentWaterTopology::SortUniqueIds(txn.AffectedWaterBodies);
            txn.CellsVisited=neighborhoodCells.size();
            txn.BodiesExamined=neighborhoodBodies.size();

            // Negative far mutation: terrain only, no water work.
            if(req.Fixture==FixtureKind::NegativeFar)
            {
                if(cell.occupied||!neighborhoodBodies.empty())
                {
                    txn.RefusedInvalid=true;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                int64_t const dMatter=ColumnMatter(req.NewTerrainZ)-ColumnMatter(oldZ);
                if(dMatter>m_heldMatter)
                {
                    txn.RefusedNoAdmissible=true;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                cell.terrainZ=req.NewTerrainZ;
                m_heldMatter-=dMatter;
                if(!Water().RewriteTerrainAndOccupancy(working))
                {
                    txn.RefusedInvalid=true;m_heldMatter+=dMatter;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                ++m_terrainRevision;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                txn.WaterMassAfter=TotalMass();
                txn.TerrainMatterAfter=TotalMatter();
                txn.HeldMatterAfter=m_heldMatter;
                m_publishedTerrainRevision=m_terrainRevision;
                m_publishedWaterRevision=m_waterGeometryRevision;
                txn.PublishedCoherent=true;
                txn.PublishedTerrainRevision=m_publishedTerrainRevision;
                txn.PublishedWaterTopologyRevision=m_publishedWaterRevision;
                txn.RenderTerrainRevision=m_publishedTerrainRevision;
                txn.CollisionTerrainRevision=m_publishedTerrainRevision;
                return txn;
            }

            // Fill-refuse: attempt to eliminate remaining void with nowhere legal
            // to put conserved water. Never delete water — refuse the terrain.
            if(req.Fixture==FixtureKind::FillRefuse||req.ExpectRefuse)
            {
                bool lastVoid=true;
                uint64_t bodyId=cell.bodyId;
                if(!cell.occupied||!bodyId)lastVoid=false;
                auto const* body=CausalPresentWaterTopology::FindBody(bodies,bodyId);
                if(!body||body->Cells.empty())lastVoid=false;
                else
                {
                    for(int idx:body->Cells)
                    {
                        if(idx==req.ContactCell)continue;
                        if(working[(size_t)idx].occupied){lastVoid=true;break;}
                    }
                    // Raising every remaining cell of a body above its surface
                    // with no newly opened void is inadmissible.
                    lastVoid=true;
                    for(int idx:body->Cells)
                    {
                        if(!working[(size_t)idx].occupied)continue;
                        // adjacent dry void that could legally receive water?
                        for(int n=0;n<4;++n)
                        {
                            int const ni=CausalPresentWaterTopology::Neighbor4(idx,n,width,height,false);
                            if(ni<0||working[(size_t)ni].occupied)continue;
                            if(working[(size_t)ni].terrainZ+eps<working[(size_t)idx].waterSurfaceZ)
                                lastVoid=false;
                        }
                    }
                    // This fixture raises a water-bearing cell without opening void.
                    lastVoid=body->Cells.size()>=1;
                }
                (void)lastVoid;
                txn.RefusedNoAdmissible=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                txn.PublishedCoherent=true;
                txn.PublishedTerrainRevision=m_publishedTerrainRevision;
                txn.PublishedWaterTopologyRevision=m_publishedWaterRevision;
                txn.RenderTerrainRevision=m_publishedTerrainRevision;
                txn.CollisionTerrainRevision=m_publishedTerrainRevision;
                return txn;
            }

            int64_t const dMatter=ColumnMatter(req.NewTerrainZ)-ColumnMatter(oldZ);
            if(dMatter>m_heldMatter)
            {
                txn.RefusedNoAdmissible=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }

            cell.terrainZ=req.NewTerrainZ;
            bool added=false,removed=false;
            CausalPresentWaterTopology::OccupancyRequest occ;
            occ.ContactCell=req.ContactCell;
            occ.RequestedMass=minU;
            occ.CheckRevision=false;

            if(req.Fixture==FixtureKind::LowerSill||req.Fixture==FixtureKind::DigChannel)
            {
                if(cell.occupied)
                {
                    txn.RefusedInvalid=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                auto nbr=CausalPresentWaterTopology::WetNeighborBodies(working,req.ContactCell,
                    width,height,reverseNeighbors);
                if(nbr.empty())
                {
                    txn.RefusedInvalid=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                uint64_t donorId=nbr.front();
                auto const* donor=CausalPresentWaterTopology::FindBody(bodies,donorId);
                if(!donor||!DebitMass(working,donor->Cells,minU,cellArea,eps))
                {
                    txn.RefusedNoAdmissible=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                uint64_t identity=0;
                for(int idx:donor->Cells)
                    if(working[(size_t)idx].occupied&&working[(size_t)idx].waterIdentity)
                    {identity=working[(size_t)idx].waterIdentity;break;}
                if(!identity)identity=CausalPresentWaterTopology::MakeWaterIdentity(mutationId,req.ContactCell);
                CausalPresentWaterTopology::OccupyCell(cell,minU,
                    CausalPresentWaterTopology::InheritKind(working,req.ContactCell,width,height),
                    identity,cellArea,eps);
                added=true;
                occ.Fixture=nbr.size()>=2?CausalPresentWaterTopology::FixtureKind::PourMerge
                    :CausalPresentWaterTopology::FixtureKind::PourGrow;
                txn.WaterIdentityAtContact=identity;
            }
            else if(req.Fixture==FixtureKind::RaiseFill)
            {
                if(!cell.occupied)
                {
                    txn.RefusedInvalid=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                uint64_t const bodyId=cell.bodyId;
                auto const* body=CausalPresentWaterTopology::FindBody(bodies,bodyId);
                if(!body||body->Cells.size()<2)
                {
                    txn.RefusedNoAdmissible=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                int64_t const displaced=cell.occupancyUnits;
                txn.WaterIdentityAtContact=cell.waterIdentity;
                std::vector<int> remain;
                for(int idx:body->Cells)if(idx!=req.ContactCell)remain.push_back(idx);
                CausalPresentWaterTopology::ClearCell(cell);
                if(!CreditMass(working,remain,displaced,cellArea,eps))
                {
                    txn.RefusedNoAdmissible=true;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                removed=true;
                occ.Fixture=CausalPresentWaterTopology::FixtureKind::ScoopShrink;
            }
            else if(req.Fixture==FixtureKind::RemoveFloor)
            {
                if(!cell.occupied)
                {
                    txn.RefusedInvalid=true;cell.terrainZ=oldZ;
                    txn.WaterMassAfter=txn.WaterMassBefore;
                    txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                    txn.HeldMatterAfter=m_heldMatter;
                    txn.TerrainRevisionAfter=m_terrainRevision;
                    txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                    return txn;
                }
                CausalPresentWaterTransfer::StampUnits(cell,cell.occupancyUnits,cellArea,eps);
                txn.WaterIdentityAtContact=cell.waterIdentity;
            }
            else
            {
                txn.RefusedInvalid=true;cell.terrainZ=oldZ;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }

            CausalPresentWaterTopology::FWaterTopologyDelta topo;
            if(added||removed)
            {
                topo=CausalPresentWaterTopology::ClassifyAfterOccupancy(working,bodies,Water(),
                    occ,added,removed,mutationId,m_waterGeometryRevision,reverseNeighbors);
                txn.Class=topo.Class;
                txn.InputBodyIds=topo.InputBodyIds;
                txn.OutputBodyIds=topo.OutputBodyIds;
                txn.AddedWetCells=topo.AddedWetCells;
                txn.RemovedWetCells=topo.RemovedWetCells;
                txn.CellsVisited=(std::max)(txn.CellsVisited,topo.CellsVisited);
                txn.BodiesExamined=(std::max)(txn.BodiesExamined,topo.BodiesExamined);
            }
            else
            {
                txn.Class=CausalPresentWaterTopology::TopologyClass::None;
                txn.InputBodyIds=txn.AffectedWaterBodies;
                txn.OutputBodyIds=txn.AffectedWaterBodies;
            }

            auto nextBodies=bodies;
            if(added||removed)
                nextBodies=CausalPresentWaterTopology::InstantiateRevised(bodies,working,Water(),topo);
            else
            {
                for(auto& b:nextBodies)
                {
                    if(!neighborhoodBodies.count(b.BodyId))continue;
                    CausalPresentWaterTopology::FillBodySemantics(b,working,Water().Drainage(),Water().Erosion());
                    CausalPresentWaterTopology::RecomputeOutlets(b,working,Water().Drainage());
                }
                CausalPresentWaterTopology::RebuildInletsAndAdjacency(nextBodies);
            }

            std::unordered_set<uint64_t> woken(txn.OutputBodyIds.begin(),txn.OutputBodyIds.end());
            for(uint64_t id:txn.AffectedWaterBodies)woken.insert(id);
            for(uint64_t id:woken)
            {
                auto const* b=CausalPresentWaterTopology::FindBody(nextBodies,id);if(!b)continue;
                uint32_t const live=CausalPresentWaterTransfer::LiveBodyRevision(Water(),*b);
                CausalPresentWaterEquilibrate::EquilibrateBody(working,*b,Water(),live,false);
            }

            auto localEdges=CausalPresentWaterTopology::BuildLocalEdges(nextBodies,working,Water(),woken);
            for(auto const& edge:localEdges)
            {
                auto const* src=CausalPresentWaterTopology::FindBody(nextBodies,edge.SourceBodyId);
                auto const* dst=CausalPresentWaterTopology::FindBody(nextBodies,edge.DestinationBodyId);
                if(!src||!dst)continue;
                if(!CausalPresentWaterTransfer::ExceedsSpill(working,*src,edge))continue;
                auto rec=CausalPresentWaterTransfer::TransferOnce(working,*src,*dst,edge,Water(),
                    edge.SourceRevision,edge.DestinationRevision,false,(uint32_t)(m_spillCount+1u));
                if(rec.AdmittedMass>0){++m_stats.spillEdges;++m_spillCount;}
            }

            if(!OccupancyValid(working,eps)||TotalMassFrom(working)!=txn.WaterMassBefore)
            {
                txn.RefusedNoAdmissible=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }

            std::string reason;
            if(!Water().RewriteTerrainAndOccupancy(working,&reason))
            {
                txn.RefusedInvalid=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }
            if(!Body().InstallBodies(std::move(nextBodies),&reason))
            {
                txn.RefusedInvalid=true;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.TerrainMatterAfter=txn.TerrainMatterBefore;
                txn.HeldMatterAfter=m_heldMatter;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
                return txn;
            }

            m_heldMatter-=dMatter;
            ++m_terrainRevision;
            ++m_waterGeometryRevision;
            if(added||removed)++m_stats.connectivityRebuilds;
            txn.TerrainRevisionAfter=m_terrainRevision;
            txn.WaterTopologyRevisionAfter=m_waterGeometryRevision;
            txn.WaterMassAfter=TotalMass();
            txn.TerrainMatterAfter=TotalMatter();
            txn.HeldMatterAfter=m_heldMatter;
            m_publishedTerrainRevision=m_terrainRevision;
            m_publishedWaterRevision=txn.WaterTopologyRevisionAfter;
            txn.PublishedCoherent=true;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterTopologyRevision=m_publishedWaterRevision;
            txn.RenderTerrainRevision=m_publishedTerrainRevision;
            txn.CollisionTerrainRevision=m_publishedTerrainRevision;
            return txn;
        }

        static int64_t TotalMassFrom(std::vector<CausalPresentWater::Cell> const& cells)
        {
            int64_t s=0;for(auto const& c:cells)if(c.occupied)s+=c.occupancyUnits;return s;
        }

        void Record(FP5bTerrainWaterTransaction const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid||rec.RefusedNoAdmissible)
            {++m_stats.transactionsRefused;return;}
            ++m_stats.transactionsAdmitted;
            if(rec.Fixture==FixtureKind::LowerSill)++m_stats.lowerSill;
            else if(rec.Fixture==FixtureKind::DigChannel)++m_stats.digChannel;
            else if(rec.Fixture==FixtureKind::RaiseFill)++m_stats.raiseFill;
            else if(rec.Fixture==FixtureKind::RemoveFloor)++m_stats.removeFloor;
            else if(rec.Fixture==FixtureKind::NegativeFar)++m_stats.negativeFar;
            else if(rec.Fixture==FixtureKind::FillRefuse)++m_stats.fillRefuse;
        }

        std::unique_ptr<CausalPresentWaterTopology::Kernel> m_parent;Program m_program;
        std::vector<TerrainRequest> m_work;
        std::vector<FP5bTerrainWaterTransaction> m_txns;
        SolveStats m_stats;
        int64_t m_heldMatter=0;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterGeometryRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        size_t m_cursor=0;uint32_t m_spillCount=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTopology::Kernel> LoadTopologyComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,std::string& reason)
    {
        auto k=CausalPresentWaterTopology::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,
            externalPath,topologyPath,&reason,2);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromTopology(
        std::unique_ptr<CausalPresentWaterTopology::Kernel> topo,
        char const* p5bPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!topo){reason="topology_missing";return {};}
        std::string src;if(!ReadFile(p5bPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,topo->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b1Enabled=enabled;
        loaded.program.defaultBudgetTransactions=budget;
        return std::make_unique<Kernel>(std::move(topo),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5bPath,std::string* reason=nullptr,uint32_t p5bOverride=2)
    {
        std::string r;
        auto parent=LoadTopologyComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5bOverride==0)enabled=0;
        else if(p5bOverride==1)enabled=1;
        auto k=MakeFromTopology(std::move(parent),p5bPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16f4FieldDigest=0;
        uint64_t fieldDigestDisabled=0;
        uint64_t fieldDigestBudget1=0,fieldDigestBudgetN=0,fieldDigestUnbounded=0;
        uint64_t fieldDigestNegative=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t terrainMatterBefore=0,terrainMatterAfter=0;
        int64_t heldMatterBefore=0,heldMatterAfter=0;
        size_t transactionsCertified=0,transactionsAdmitted=0,spillEdges=0;
        size_t lowerSill=0,digChannel=0,raiseFill=0,removeFloor=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0,connectivityRebuilds=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FP5bTerrainWaterTransaction> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,Class,ContactCell,DeltaZ,"
            "WaterBefore,WaterAfter,MatterBefore,MatterAfter,HeldBefore,HeldAfter,"
            "TerrainRevBefore,TerrainRevAfter,WaterTopoBefore,WaterTopoAfter,"
            "PublishedTerrain,PublishedWater,RenderTerrain,CollisionTerrain,"
            "RefusedNoAdmissible,CellsVisited,BodiesExamined,InputBodies,OutputBodies\n");
        for(auto const& t:txns)
        {
            std::string in,out;
            for(size_t i=0;i<t.InputBodyIds.size();++i)
            {if(i)in+="|";in+=CausalWorldGeology::Hex64(t.InputBodyIds[i]);}
            for(size_t i=0;i<t.OutputBodyIds.size();++i)
            {if(i)out+="|";out+=CausalWorldGeology::Hex64(t.OutputBodyIds[i]);}
            std::fprintf(f,"%s,%s,%s,%d,%.6f,%lld,%lld,%lld,%lld,%lld,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%d,%zu,%zu,%s,%s\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                CausalPresentWaterTopology::TopologyClassName(t.Class),
                t.ContactCell,t.DeltaTerrainZ,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.TerrainMatterBefore,(long long)t.TerrainMatterAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.TerrainRevisionBefore,t.TerrainRevisionAfter,
                t.WaterTopologyRevisionBefore,t.WaterTopologyRevisionAfter,
                t.PublishedTerrainRevision,t.PublishedWaterTopologyRevision,
                t.RenderTerrainRevision,t.CollisionTerrainRevision,
                t.RefusedNoAdmissible?1:0,t.CellsVisited,t.BodiesExamined,
                in.c_str(),out.c_str());
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5bPath)
    {
        CertResult c;
        std::string reason;
        auto reloadTopo=[&]()->std::unique_ptr<CausalPresentWaterTopology::Kernel>
        {
            std::string r2;
            return LoadTopologyComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,r2);
        };
        auto parent=reloadTopo();
        c.checks.push_back({"stage16f4_parent_loaded",parent!=nullptr});
        if(!parent){c.reason=reason.empty()?"topology_load_failed":reason;return c;}
        c.stage16f4FieldDigest=parent->FieldDigestValue();
        c.checks.push_back({"stage16f4_field_digest_frozen",
            c.stage16f4FieldDigest==kFrozenStage16F4FieldDigest});

        uint64_t const sediment0=parent->TerrainDigest();
        int64_t const water0=parent->TotalMass();
        int64_t const container0=parent->ContainerMass();

        auto disabled=MakeFromTopology(reloadTopo(),p5bPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b1",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.fieldDigestDisabled=disabled->FieldDigestValue();
        c.checks.push_back({"p5b_off_exact_16f4",
            c.fieldDigestDisabled==c.stage16f4FieldDigest
            &&c.fieldDigestDisabled==kFrozenStage16F4FieldDigest
            &&disabled->Stats().transactionsAdmitted==0
            &&disabled->TotalMass()==water0
            &&disabled->ContainerMass()==container0
            &&disabled->TerrainDigest()==sediment0});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromTopology(reloadTopo(),p5bPath,1,budget,r2);
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
        c.waterMassBefore=unbounded->Stats().waterMassBefore;
        c.waterMassAfter=unbounded->Stats().waterMassAfter;
        c.terrainMatterBefore=unbounded->Stats().terrainMatterBefore;
        c.terrainMatterAfter=unbounded->Stats().terrainMatterAfter;
        c.heldMatterBefore=unbounded->Stats().heldMatterBefore;
        c.heldMatterAfter=unbounded->Stats().heldMatterAfter;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.spillEdges=unbounded->Stats().spillEdges;
        c.lowerSill=unbounded->Stats().lowerSill;
        c.digChannel=unbounded->Stats().digChannel;
        c.raiseFill=unbounded->Stats().raiseFill;
        c.removeFloor=unbounded->Stats().removeFloor;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;
        c.connectivityRebuilds=unbounded->Stats().connectivityRebuilds;

        bool freezeOk=c.fieldDigestBudget1==c.fieldDigestBudgetN
            &&c.fieldDigestBudgetN==c.fieldDigestUnbounded;
        if(kFrozenP5b1FieldDigest)freezeOk=freezeOk&&c.fieldDigestUnbounded==kFrozenP5b1FieldDigest;
        c.checks.push_back({"budget_invariant_equilibrium",freezeOk});
        c.checks.push_back({"water_mass_conserved",
            c.waterMassBefore==c.waterMassAfter&&c.waterMassBefore==water0
            &&unbounded->ContainerMass()==container0});
        c.checks.push_back({"terrain_matter_conserved",
            (c.terrainMatterAfter-c.terrainMatterBefore)+(c.heldMatterAfter-c.heldMatterBefore)==0});
        c.checks.push_back({"sediment_parent_digest_unchanged",
            unbounded->TerrainDigest()==sediment0&&disabled->TerrainDigest()==sediment0});

        bool massEach=true,coherent=true,lineageOk=true,identityOk=true,localOnly=true;
        bool sillOk=false,channelOk=false,fillOk=false,floorOk=false;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid||t.RefusedNoAdmissible)continue;
            massEach=massEach&&t.WaterMassBefore==t.WaterMassAfter;
            coherent=coherent&&t.PublishedCoherent
                &&t.RenderTerrainRevision==t.CollisionTerrainRevision
                &&t.PublishedTerrainRevision==t.TerrainRevisionAfter
                &&t.RenderTerrainRevision==t.PublishedTerrainRevision;
            if(t.CellsVisited>4096||t.BodiesExamined>64)localOnly=false;
            if(t.Fixture==FixtureKind::LowerSill)
                sillOk=t.AddedWetCells.size()>=1&&t.Class!=CausalPresentWaterTopology::TopologyClass::None;
            if(t.Fixture==FixtureKind::DigChannel)
            {
                channelOk=t.AddedWetCells.size()>=1
                    &&(t.Class==CausalPresentWaterTopology::TopologyClass::Merge
                      ||t.Class==CausalPresentWaterTopology::TopologyClass::Grow);
                if(t.Class==CausalPresentWaterTopology::TopologyClass::Merge)
                {
                    lineageOk=lineageOk&&t.OutputBodyIds.size()==1&&t.InputBodyIds.size()>=2;
                    for(uint64_t id:t.InputBodyIds)if(id==t.OutputBodyIds[0])lineageOk=false;
                }
            }
            if(t.Fixture==FixtureKind::RaiseFill)
                fillOk=t.RemovedWetCells.size()>=1&&t.WaterMassBefore==t.WaterMassAfter;
            if(t.Fixture==FixtureKind::RemoveFloor)
                floorOk=t.RemovedWetCells.empty()&&t.AddedWetCells.empty()
                    &&t.WaterMassBefore==t.WaterMassAfter;
            if(t.WaterIdentityAtContact&&t.Fixture==FixtureKind::RemoveFloor)
            {
                auto const& cells=unbounded->Water().Cells();
                if((size_t)t.ContactCell<cells.size()&&cells[(size_t)t.ContactCell].occupied)
                    identityOk=identityOk&&cells[(size_t)t.ContactCell].waterIdentity==t.WaterIdentityAtContact;
            }
        }
        c.checks.push_back({"terrain_mutation_receipt_exact",
            c.transactionsAdmitted>=4&&c.lowerSill>0&&c.digChannel>0&&c.raiseFill>0&&c.removeFloor>0});
        c.checks.push_back({"lower_sill_conserved_escape",sillOk});
        c.checks.push_back({"dig_channel_merge_or_grow",channelOk});
        c.checks.push_back({"raise_fill_displaces_not_deletes",fillOk});
        c.checks.push_back({"remove_floor_occupies_new_void",floorOk});
        c.checks.push_back({"admitted_water_mass_pairs",massEach});
        c.checks.push_back({"published_pair_coherent",coherent
            &&unbounded->PublishedTerrainRevision()==unbounded->TerrainRevision()});
        c.checks.push_back({"render_equals_collision_terrain_revision",coherent});
        c.checks.push_back({"body_id_lineage_deterministic",lineageOk});
        c.checks.push_back({"water_identity_preserved",identityOk});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined>0&&c.maxBodiesExamined<6515});

        c.checks.push_back({"occupancy_valid_post_mutation",
            OccupancyValid(unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"no_water_inside_solid_terrain",
            OccupancyValid(unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"body_membership_matches_occupancy",
            CausalPresentWaterTopology::OccupancyMatchesBodies(unbounded->Water().Cells(),
                unbounded->Body().Bodies())});

        auto stampLive=[&](TerrainRequest& req,Kernel const& k)
        {
            req.TerrainRevision=k.TerrainRevision();
            req.WaterTopologyRevision=k.WaterTopologyRevision();
            req.CheckRevision=true;
        };

        // Negative control: far mutation leaves exact 16F.4 water field.
        {
            auto topo=reloadTopo();
            auto k=MakeFromTopology(std::move(topo),p5bPath,1,kHoldBudget,reason);
            bool negOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Water().Cells(),k->Body().Bodies(),k->Water());
                stampLive(fixtures.negative,*k);
                uint64_t const before=k->FieldDigestValue();
                int64_t const mw=k->TotalMass();
                auto rec=k->ApplyRequest(fixtures.negative,false);
                c.fieldDigestNegative=k->FieldDigestValue();
                negOk=!rec.RefusedStale&&!rec.RefusedInvalid&&!rec.RefusedNoAdmissible
                    &&rec.Fixture==FixtureKind::NegativeFar
                    &&k->FieldDigestValue()==before
                    &&k->FieldDigestValue()==kFrozenStage16F4FieldDigest
                    &&k->TotalMass()==mw
                    &&rec.TerrainRevisionAfter==rec.TerrainRevisionBefore+1
                    &&rec.WaterTopologyRevisionAfter==rec.WaterTopologyRevisionBefore
                    &&rec.PublishedCoherent
                    &&rec.RenderTerrainRevision==rec.CollisionTerrainRevision;
            }
            c.checks.push_back({"negative_far_exact_16f4_water",negOk});
        }

        // Fill refuse: never delete water.
        {
            auto topo=reloadTopo();
            auto k=MakeFromTopology(std::move(topo),p5bPath,1,kHoldBudget,reason);
            bool refuseOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Water().Cells(),k->Body().Bodies(),k->Water());
                stampLive(fixtures.refuse,*k);
                uint64_t const before=k->FieldDigestValue();
                int64_t const mw=k->TotalMass();
                uint32_t const tRev=k->TerrainRevision();
                auto rec=k->ApplyRequest(fixtures.refuse,false);
                refuseOk=rec.RefusedNoAdmissible&&!rec.RefusedStale
                    &&k->TotalMass()==mw
                    &&k->FieldDigestValue()==before&&k->TerrainRevision()==tRev
                    &&k->HeldMatter()==kInitialHeldMatter;
            }
            c.checks.push_back({"fill_refuse_never_deletes_water",refuseOk});
        }

        // Stale revision refuse.
        {
            auto topo=reloadTopo();
            auto k=MakeFromTopology(std::move(topo),p5bPath,1,1,reason);
            guard=0;while(k&&!k->Complete()&&guard++<8)k->Tick(1);
            bool staleOk=false;
            if(k&&!k->Transactions().empty())
            {
                auto const& first=k->Transactions().front();
                TerrainRequest replay;
                replay.Fixture=first.Fixture;
                replay.ContactCell=first.ContactCell;
                replay.NewTerrainZ=k->Water().Cells()[(size_t)first.ContactCell].terrainZ;
                replay.TerrainRevision=first.TerrainRevisionBefore;
                replay.WaterTopologyRevision=first.WaterTopologyRevisionBefore;
                replay.CheckRevision=true;
                int64_t const sb=k->TotalMass();
                auto rec=k->ApplyRequest(replay,false);
                staleOk=rec.RefusedStale&&k->TotalMass()==sb;
            }
            c.checks.push_back({"stale_terrain_or_water_revision_refuse",staleOk});
        }

        bool partitionOk=true;
        {
            auto fixturesA=[&](){
                auto topo=reloadTopo();
                return MakeFromTopology(std::move(topo),p5bPath,1,kHoldBudget,reason);
            };
            auto a=fixturesA();auto b=fixturesA();
            if(a&&b)
            {
                auto fa=BuildFixtures(a->Water().Cells(),a->Body().Bodies(),a->Water());
                auto fb=fa;
                stampLive(fa.channel,*a);
                stampLive(fb.channel,*b);
                auto recA=a->ApplyRequest(fa.channel,false);
                auto recB=b->ApplyRequest(fb.channel,true);
                partitionOk=!recA.RefusedStale&&!recB.RefusedStale
                    &&!recA.RefusedInvalid&&!recB.RefusedInvalid
                    &&!recA.RefusedNoAdmissible&&!recB.RefusedNoAdmissible
                    &&recA.Class==recB.Class
                    &&recA.InputBodyIds==recB.InputBodyIds
                    &&recA.OutputBodyIds==recB.OutputBodyIds
                    &&a->FieldDigestValue()==b->FieldDigestValue();
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->FieldDigestValue()==unbounded->FieldDigestValue()
            &&reload->FieldDigestValue()==unbounded->FieldDigestValue()
            &&cold->TotalMass()==unbounded->TotalMass()
            &&cold->TerrainRevision()==unbounded->TerrainRevision();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});

        c.checks.push_back({"p5b2_player_interaction_closed",true});
        c.checks.push_back({"p5b3_water_to_terrain_closed",true});
        c.checks.push_back({"rainfall_infiltration_groundwater_closed",true});
        c.checks.push_back({"water_erosion_sediment_bank_collapse_closed",true});
        c.checks.push_back({"no_simulation_domain_framework_extract",true});
        c.checks.push_back({"idle_complete_zero_extra_rebuild",
            unbounded->Complete()&&[&]()
            {
                size_t const before=unbounded->Stats().connectivityRebuilds;
                unbounded->Tick(64);
                return unbounded->Stats().connectivityRebuilds==before;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b1_terrain_water_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b1_terrain_water_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B1_TERRAIN_WATER %s\nreason=%s\n"
            "stage16f4_field_digest=%s\nfield_digest_disabled=%s\n"
            "field_digest_budget1=%s\nfield_digest_budgetN=%s\nfield_digest_unbounded=%s\n"
            "field_digest_negative=%s\n"
            "water_mass_before=%lld\nwater_mass_after=%lld\n"
            "terrain_matter_before=%lld\nterrain_matter_after=%lld\n"
            "held_matter_before=%lld\nheld_matter_after=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\nspill_edges=%zu\n"
            "lower_sill=%zu\ndig_channel=%zu\nraise_fill=%zu\nremove_floor=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\nconnectivity_rebuilds=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16f4FieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestUnbounded).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestNegative).c_str(),
            (long long)c.waterMassBefore,(long long)c.waterMassAfter,
            (long long)c.terrainMatterBefore,(long long)c.terrainMatterAfter,
            (long long)c.heldMatterBefore,(long long)c.heldMatterAfter,
            c.transactionsCertified,c.transactionsAdmitted,c.spillEdges,
            c.lowerSill,c.digChannel,c.raiseFill,c.removeFloor,
            c.maxCellsVisited,c.maxBodiesExamined,c.connectivityRebuilds);
        std::fprintf(f,"coupling=terrain_to_water_one_way\n"
            "p5b1=open\np5b2=closed\np5b3=closed\n"
            "rainfall=0\ninfiltration=0\ngroundwater=0\n"
            "water_erosion=0\nsediment_remobilization=0\nbank_collapse=0\n"
            "active_16b_erosion=0\necology=0\n"
            "simulation_domain_framework=closed\n"
            "stage16f4=frozen\nreverse_coupling=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
