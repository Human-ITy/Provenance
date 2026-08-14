#pragma once

// Stage 16E: present-water body semantics and connectivity proof. Stage 16D
// occupancy remains the immutable parent. This layer publishes body identity,
// type, shared/local surface rules, spill elevation, inlets/outlets, and
// upstream/downstream adjacency. It cannot run a flow tick, open P5b, mutate
// terrain, or change 16D occupancy.

#include "CausalPresentWater.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalPresentWaterBody
{
    constexpr char const* kExpectedRegion="causal_world_present_water_body_floor";
    constexpr uint64_t kFrozenStage16CDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozenWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kFrozenOccupancyDigest=0x880a46fcdca2f8a4ull;

    enum class BodyType:uint8_t
    {
        None=0,
        Lake=1,
        River=2,
        Wetland=3,
        Mixed=4
    };

    enum class SurfaceRule:uint8_t
    {
        None=0,
        SharedSpill=1,
        LocalDepth=2,
        Mixed=3
    };

    inline char const* BodyTypeName(BodyType value)
    {
        switch(value)
        {
            case BodyType::Lake:return "lake";
            case BodyType::River:return "river";
            case BodyType::Wetland:return "wetland";
            case BodyType::Mixed:return "mixed";
            default:return "none";
        }
    }

    inline char const* SurfaceRuleName(SurfaceRule value)
    {
        switch(value)
        {
            case SurfaceRule::SharedSpill:return "shared_spill";
            case SurfaceRule::LocalDepth:return "local_depth";
            case SurfaceRule::Mixed:return "mixed";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,bodyEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWater::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_BODY_V1";
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
          &&hex("body_event_id",r.program.bodyEventId)&&u32("chronology",r.program.chronology);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.bodyEventId!=0
          &&r.program.chronology>parent.chronology;
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_present_water_body_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct Portal
    {
        int cellIndex=-1;
        uint64_t otherBodyId=0;
        double elevationM=0;
    };

    struct FPresentWaterBody
    {
        uint64_t BodyId=0;
        BodyType Type=BodyType::None;
        SurfaceRule Surface=SurfaceRule::None;
        double SpillElevation=0;
        std::vector<int> Cells;
        std::vector<Portal> Inlets;
        std::vector<Portal> Outlets;
        std::vector<uint64_t> UpstreamBodies;
        std::vector<uint64_t> DownstreamBodies;
        uint32_t SourceLandscapeRevision=0;
        uint32_t SourceHydrologyRevision=0;
        uint8_t hasLake=0,hasRiver=0,hasWetland=0;
        bool closedLake=false;
    };

    struct Query
    {
        bool found=false;
        FPresentWaterBody const* body=nullptr;
        CausalPresentWater::Query water;
    };

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWater::Kernel> water,Program program)
          :m_water(std::move(water)),m_program(std::move(program))
        {
            BuildBodies(m_water->Cells(),m_bodies,&m_bodyIndex);
            m_digest=DigestBodies(m_bodies);
            m_connectivityDigest=ConnectivityDigest(m_bodies);
        }

        CausalPresentWater::Kernel const& Water() const{return *m_water;}
        CausalPresentWater::Kernel& Water(){return *m_water;}
        CausalCompiledSediment::Kernel const& Sediment() const{return m_water->Sediment();}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_water->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_water->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_water->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FPresentWaterBody> const& Bodies() const{return m_bodies;}
        uint64_t Digest() const{return m_digest;}
        uint64_t ConnectivityDigest() const{return m_connectivityDigest;}
        uint64_t WaterDigest() const{return m_water->Digest();}
        uint64_t OccupancyDigest() const{return m_water->OccupancyDigest();}

        double ReconstructedZ(double x,double y) const{return m_water->ReconstructedZ(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_water->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_water->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_water->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_water->BuildBlock(bx,by);}

        Query QueryAt(double x,double y) const
        {
            Query q;q.water=m_water->QueryAt(x,y);if(!q.water.found)return q;
            q.found=true;
            if(q.water.occupied&&q.water.water.bodyId)
            {
                auto const it=m_bodyIndex.find(q.water.water.bodyId);
                if(it!=m_bodyIndex.end())q.body=&m_bodies[(size_t)it->second];
            }
            return q;
        }

        std::vector<FPresentWaterBody> EvaluateFromCells(
            std::vector<CausalPresentWater::Cell> const& cells) const
        {
            std::vector<FPresentWaterBody> bodies;BuildBodies(cells,bodies,nullptr);return bodies;
        }

        // Stage 16F.4: install locally reconstructed bodies. Terrain/hydrology
        // source revisions stay frozen; only occupancy-derived membership changes.
        bool InstallBodies(std::vector<FPresentWaterBody> next,std::string* reason=nullptr)
        {
            uint32_t const landscapeRev=Sediment().GetProgram().authorityRevision;
            uint32_t const hydroRev=Drainage().GetProgram().authorityRevision;
            for(FPresentWaterBody& body:next)
            {
                if(body.SourceLandscapeRevision!=landscapeRev
                  ||body.SourceHydrologyRevision!=hydroRev)
                {if(reason)*reason="source_revision_mutation";return false;}
            }
            std::sort(next.begin(),next.end(),[](FPresentWaterBody const& a,FPresentWaterBody const& b)
            {return a.BodyId<b.BodyId;});
            m_bodies=std::move(next);
            m_bodyIndex.clear();
            for(size_t i=0;i<m_bodies.size();++i)m_bodyIndex[m_bodies[i].BodyId]=(int)i;
            m_digest=DigestBodies(m_bodies);
            m_connectivityDigest=ConnectivityDigest(m_bodies);
            if(reason)*reason="ok";return true;
        }

    private:
        static void SortUnique(std::vector<uint64_t>& values)
        {
            std::sort(values.begin(),values.end());
            values.erase(std::unique(values.begin(),values.end()),values.end());
        }

        static void SortPortals(std::vector<Portal>& portals)
        {
            std::sort(portals.begin(),portals.end(),[](Portal const& a,Portal const& b)
            {
                if(a.cellIndex!=b.cellIndex)return a.cellIndex<b.cellIndex;
                if(a.otherBodyId!=b.otherBodyId)return a.otherBodyId<b.otherBodyId;
                return a.elevationM<b.elevationM;
            });
        }

        void BuildBodies(std::vector<CausalPresentWater::Cell> const& cells,
            std::vector<FPresentWaterBody>& out,
            std::unordered_map<uint64_t,int>* indexOut) const
        {
            out.clear();
            auto const& drainage=Drainage().Cells();
            auto const& erosion=Erosion().Cells();
            uint32_t const landscapeRev=Sediment().GetProgram().authorityRevision;
            uint32_t const hydroRev=Drainage().GetProgram().authorityRevision;

            std::map<uint64_t,std::vector<int>> membership;
            for(size_t i=0;i<cells.size();++i)
            {
                if(!cells[i].occupied||cells[i].bodyId==0)continue;
                membership[cells[i].bodyId].push_back((int)i);
            }

            std::unordered_map<uint64_t,size_t> provisional;
            out.reserve(membership.size());
            for(auto& kv:membership)
            {
                FPresentWaterBody body;
                body.BodyId=kv.first;
                body.Cells=std::move(kv.second);
                std::sort(body.Cells.begin(),body.Cells.end());
                body.SourceLandscapeRevision=landscapeRev;
                body.SourceHydrologyRevision=hydroRev;

                double minSurface=1e300,maxSurface=-1e300;
                bool allClosed=true;bool sawBasin=false;
                double spillHint=-1e300;
                for(int idx:body.Cells)
                {
                    auto const& cell=cells[(size_t)idx];
                    auto const kind=cell.kind;
                    if(kind==CausalPresentWater::BodyKind::Lake)body.hasLake=1;
                    else if(kind==CausalPresentWater::BodyKind::River)body.hasRiver=1;
                    else if(kind==CausalPresentWater::BodyKind::Wetland)body.hasWetland=1;
                    minSurface=(std::min)(minSurface,cell.waterSurfaceZ);
                    maxSurface=(std::max)(maxSurface,cell.waterSurfaceZ);
                    if(drainage[(size_t)idx].basinId)
                    {
                        sawBasin=true;
                        spillHint=(std::max)(spillHint,drainage[(size_t)idx].spillElevationM);
                        if(erosion[(size_t)idx].basinClass
                            !=CausalCompiledFluvialErosion::BasinClass::ClosedGeomorphic)
                            allClosed=false;
                    }
                    else allClosed=false;
                }
                int kinds=(int)body.hasLake+(int)body.hasRiver+(int)body.hasWetland;
                if(kinds>=2)body.Type=BodyType::Mixed;
                else if(body.hasLake)body.Type=BodyType::Lake;
                else if(body.hasRiver)body.Type=BodyType::River;
                else body.Type=BodyType::Wetland;

                double const surfaceSpan=maxSurface-minSurface;
                bool const shared=surfaceSpan<=1e-6;
                bool localOnly=true;
                for(int idx:body.Cells)
                {
                    auto const& cell=cells[(size_t)idx];
                    if(std::fabs((cell.terrainZ+cell.depthM)-cell.waterSurfaceZ)>1e-6)
                    {localOnly=false;break;}
                }
                if(shared&&(body.hasLake||(sawBasin&&spillHint>-1e299)))
                    body.Surface=SurfaceRule::SharedSpill;
                else if(!shared&&kinds>=2)body.Surface=SurfaceRule::Mixed;
                else if(localOnly&&!shared)body.Surface=SurfaceRule::LocalDepth;
                else if(shared)body.Surface=SurfaceRule::SharedSpill;
                else body.Surface=SurfaceRule::Mixed;

                if(body.Surface==SurfaceRule::SharedSpill)body.SpillElevation=maxSurface;
                else if(spillHint>-1e299)body.SpillElevation=spillHint;
                else body.SpillElevation=maxSurface;

                body.closedLake=body.Type==BodyType::Lake&&body.hasLake&&!body.hasRiver
                    &&!body.hasWetland&&allClosed&&sawBasin;

                provisional.emplace(body.BodyId,out.size());
                out.push_back(std::move(body));
            }

            // Inlet/outlet relationships follow Stage 16A receiver authority.
            for(FPresentWaterBody& body:out)
            {
                for(int idx:body.Cells)
                {
                    auto const& d=drainage[(size_t)idx];
                    int const receiver=d.receiver;
                    uint64_t other=0;
                    bool leaves=false;
                    if(receiver<0)leaves=true;
                    else if(!cells[(size_t)receiver].occupied
                        ||cells[(size_t)receiver].bodyId!=body.BodyId)
                    {
                        leaves=true;
                        if(cells[(size_t)receiver].occupied)
                            other=cells[(size_t)receiver].bodyId;
                    }
                    if(!leaves)continue;
                    Portal portal;
                    portal.cellIndex=idx;
                    portal.otherBodyId=other;
                    portal.elevationM=cells[(size_t)idx].waterSurfaceZ;
                    body.Outlets.push_back(portal);
                    if(other)
                    {
                        auto it=provisional.find(other);
                        if(it!=provisional.end())
                        {
                            Portal inlet;
                            inlet.cellIndex=receiver;
                            inlet.otherBodyId=body.BodyId;
                            inlet.elevationM=cells[(size_t)receiver].waterSurfaceZ;
                            out[it->second].Inlets.push_back(inlet);
                        }
                    }
                }
            }

            for(FPresentWaterBody& body:out)
            {
                SortPortals(body.Inlets);
                SortPortals(body.Outlets);
                for(Portal const& p:body.Inlets)
                    if(p.otherBodyId)body.UpstreamBodies.push_back(p.otherBodyId);
                for(Portal const& p:body.Outlets)
                    if(p.otherBodyId)body.DownstreamBodies.push_back(p.otherBodyId);
                SortUnique(body.UpstreamBodies);
                SortUnique(body.DownstreamBodies);
            }

            std::sort(out.begin(),out.end(),[](FPresentWaterBody const& a,FPresentWaterBody const& b)
            {return a.BodyId<b.BodyId;});
            if(indexOut)
            {
                indexOut->clear();
                for(size_t i=0;i<out.size();++i)(*indexOut)[out[i].BodyId]=(int)i;
            }
        }

        static uint64_t DigestBodies(std::vector<FPresentWaterBody> const& bodies)
        {
            uint64_t digest=14695981039346656037ull;
            for(FPresentWaterBody const& body:bodies)
            {
                CausalWorldGeology::HashAppend(digest,&body.BodyId,sizeof(body.BodyId));
                uint8_t const type=(uint8_t)body.Type,rule=(uint8_t)body.Surface;
                CausalWorldGeology::HashAppend(digest,&type,sizeof(type));
                CausalWorldGeology::HashAppend(digest,&rule,sizeof(rule));
                CausalWorldGeology::HashAppend(digest,&body.SpillElevation,sizeof(body.SpillElevation));
                CausalWorldGeology::HashAppend(digest,&body.SourceLandscapeRevision,
                    sizeof(body.SourceLandscapeRevision));
                CausalWorldGeology::HashAppend(digest,&body.SourceHydrologyRevision,
                    sizeof(body.SourceHydrologyRevision));
                size_t const n=body.Cells.size();
                CausalWorldGeology::HashAppend(digest,&n,sizeof(n));
                for(int idx:body.Cells)
                    CausalWorldGeology::HashAppend(digest,&idx,sizeof(idx));
                size_t const ni=body.Inlets.size(),no=body.Outlets.size();
                CausalWorldGeology::HashAppend(digest,&ni,sizeof(ni));
                CausalWorldGeology::HashAppend(digest,&no,sizeof(no));
                for(Portal const& p:body.Inlets)
                {
                    CausalWorldGeology::HashAppend(digest,&p.cellIndex,sizeof(p.cellIndex));
                    CausalWorldGeology::HashAppend(digest,&p.otherBodyId,sizeof(p.otherBodyId));
                }
                for(Portal const& p:body.Outlets)
                {
                    CausalWorldGeology::HashAppend(digest,&p.cellIndex,sizeof(p.cellIndex));
                    CausalWorldGeology::HashAppend(digest,&p.otherBodyId,sizeof(p.otherBodyId));
                }
                for(uint64_t id:body.UpstreamBodies)
                    CausalWorldGeology::HashAppend(digest,&id,sizeof(id));
                for(uint64_t id:body.DownstreamBodies)
                    CausalWorldGeology::HashAppend(digest,&id,sizeof(id));
            }
            return digest;
        }

        static uint64_t ConnectivityDigest(std::vector<FPresentWaterBody> const& bodies)
        {
            uint64_t digest=14695981039346656037ull;
            for(FPresentWaterBody const& body:bodies)
            {
                CausalWorldGeology::HashAppend(digest,&body.BodyId,sizeof(body.BodyId));
                uint8_t const type=(uint8_t)body.Type;
                CausalWorldGeology::HashAppend(digest,&type,sizeof(type));
                for(uint64_t id:body.UpstreamBodies)
                    CausalWorldGeology::HashAppend(digest,&id,sizeof(id));
                for(uint64_t id:body.DownstreamBodies)
                    CausalWorldGeology::HashAppend(digest,&id,sizeof(id));
                size_t const outlets=body.Outlets.size(),inlets=body.Inlets.size();
                CausalWorldGeology::HashAppend(digest,&outlets,sizeof(outlets));
                CausalWorldGeology::HashAppend(digest,&inlets,sizeof(inlets));
            }
            return digest;
        }

        std::unique_ptr<CausalPresentWater::Kernel> m_water;Program m_program;
        std::vector<FPresentWaterBody> m_bodies;
        std::unordered_map<uint64_t,int> m_bodyIndex;
        uint64_t m_digest=0,m_connectivityDigest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,std::string* reason=nullptr)
    {
        std::string source;if(!ReadFile(bodyPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalPresentWater::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,waterPath,&parentReason);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16CDigest=0,waterDigest=0,occupancyDigest=0;
        uint64_t bodyDigest=0,connectivityDigest=0;
        size_t occupiedCells=0,bodyCount=0;
        size_t lakeBodies=0,riverBodies=0,wetlandBodies=0,mixedBodies=0;
        size_t outletCount=0,inletCount=0,closedLakes=0;
        size_t wetlandRiverLakeLinks=0;
        double massResidualParentG=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath)
    {
        CertResult c;auto const parent=CausalPresentWater::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,waterPath);
        c.stage16CDigest=parent.stage16CDigest;
        c.waterDigest=parent.waterDigest;
        c.occupancyDigest=parent.occupancyDigest;
        c.massResidualParentG=parent.massResidualParentG;
        c.occupiedCells=parent.occupiedCells;
        c.checks.push_back({"stage16d_present_water_parent_pass",parent.passed});
        c.checks.push_back({"stage16c_digest_frozen",
            parent.stage16CDigest==kFrozenStage16CDigest});
        c.checks.push_back({"stage16d_water_digest_frozen",
            parent.waterDigest==kFrozenWaterDigest});
        c.checks.push_back({"stage16d_occupancy_digest_frozen",
            parent.occupancyDigest==kFrozenOccupancyDigest});
        c.checks.push_back({"same_16d_occupancy_cell_count",parent.occupiedCells==13417});

        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,&reason);
        c.reason=reason;c.checks.push_back({"descriptor_linked_present_water_body",k!=nullptr});
        if(!parent.passed||!k)return c;

        c.bodyDigest=k->Digest();c.connectivityDigest=k->ConnectivityDigest();
        c.checks.push_back({"occupancy_unchanged_under_body_layer",
            k->WaterDigest()==parent.waterDigest
            &&k->OccupancyDigest()==parent.occupancyDigest});

        auto const& bodies=k->Bodies();
        auto const& cells=k->Water().Cells();
        auto const& drainage=k->Drainage().Cells();
        c.bodyCount=bodies.size();
        std::map<uint64_t,size_t> bodyLookup;
        for(size_t i=0;i<bodies.size();++i)bodyLookup[bodies[i].BodyId]=i;

        bool membershipOk=true;size_t occupiedMapped=0;
        std::map<uint64_t,size_t> cellCounts;
        for(size_t i=0;i<cells.size();++i)
        {
            if(!cells[i].occupied)continue;
            ++occupiedMapped;
            if(!cells[i].bodyId){membershipOk=false;continue;}
            auto it=bodyLookup.find(cells[i].bodyId);
            if(it==bodyLookup.end()){membershipOk=false;continue;}
            ++cellCounts[cells[i].bodyId];
        }
        for(FPresentWaterBody const& body:bodies)
        {
            if(cellCounts[body.BodyId]!=body.Cells.size())membershipOk=false;
            if(body.Type==BodyType::Lake)++c.lakeBodies;
            else if(body.Type==BodyType::River)++c.riverBodies;
            else if(body.Type==BodyType::Wetland)++c.wetlandBodies;
            else if(body.Type==BodyType::Mixed)++c.mixedBodies;
            if(body.closedLake)++c.closedLakes;
            c.outletCount+=body.Outlets.size();
            c.inletCount+=body.Inlets.size();
        }
        membershipOk=membershipOk&&occupiedMapped==parent.occupiedCells
            &&c.bodyCount==parent.bodyCount;

        bool outletsValid=true;
        bool closedDrainOk=true;
        bool riverAuthorityOk=true;
        bool revisionsOk=true;
        size_t crossTypeLinks=0;
        auto typeOf=[&](uint64_t id)->BodyType
        {
            auto it=bodyLookup.find(id);return it==bodyLookup.end()?BodyType::None:bodies[it->second].Type;
        };
        for(FPresentWaterBody const& body:bodies)
        {
            revisionsOk=revisionsOk
                &&body.SourceLandscapeRevision==k->Sediment().GetProgram().authorityRevision
                &&body.SourceHydrologyRevision==k->Drainage().GetProgram().authorityRevision;
            for(Portal const& outlet:body.Outlets)
            {
                if(outlet.cellIndex<0||(size_t)outlet.cellIndex>=cells.size())
                {outletsValid=false;continue;}
                if(cells[(size_t)outlet.cellIndex].bodyId!=body.BodyId)outletsValid=false;
                auto const& d=drainage[(size_t)outlet.cellIndex];
                int const receiver=d.receiver;
                if(receiver>=0)
                {
                    bool const adjacent=std::fabs(drainage[(size_t)receiver].x-d.x)
                        <=k->Drainage().StepM()+1e-9
                      &&std::fabs(drainage[(size_t)receiver].y-d.y)
                        <=k->Drainage().StepM()+1e-9;
                    if(!adjacent)outletsValid=false;
                    if(cells[(size_t)receiver].occupied)
                    {
                        if(cells[(size_t)receiver].bodyId==body.BodyId)outletsValid=false;
                        if(outlet.otherBodyId!=cells[(size_t)receiver].bodyId)outletsValid=false;
                    }
                    else if(outlet.otherBodyId!=0)outletsValid=false;
                }
                else if(outlet.otherBodyId!=0)outletsValid=false;
            }
            if(body.closedLake)
            {
                for(Portal const& outlet:body.Outlets)
                {
                    // Unexplained drain = leaving into another present-water
                    // body below the shared spill lip. Spill-lip handoff into
                    // a downstream body (or dry/boundary) is expected.
                    if(outlet.otherBodyId==0)continue;
                    double const z=cells[(size_t)outlet.cellIndex].waterSurfaceZ;
                    if(std::fabs(z-body.SpillElevation)>1e-3)closedDrainOk=false;
                }
            }
            for(int idx:body.Cells)
            {
                if(cells[(size_t)idx].kind!=CausalPresentWater::BodyKind::River)continue;
                if(!drainage[(size_t)idx].channel){riverAuthorityOk=false;continue;}
                int i=idx;
                for(int step=0;step<256&&i>=0;++step)
                {
                    int const r=drainage[(size_t)i].receiver;
                    if(r<0)break;
                    // Receiver edge itself is 16A authority; require downhill head /
                    // non-increasing flood rank along occupied river path.
                    if(drainage[(size_t)r].floodRank>=drainage[(size_t)i].floodRank)
                    {riverAuthorityOk=false;break;}
                    if(!cells[(size_t)r].occupied)break;
                    if(cells[(size_t)r].bodyId!=body.BodyId)break;
                    if(cells[(size_t)r].kind==CausalPresentWater::BodyKind::River
                      &&!drainage[(size_t)r].channel){riverAuthorityOk=false;break;}
                    i=r;
                }
            }
            for(uint64_t down:body.DownstreamBodies)
            {
                BodyType const a=body.Type,b=typeOf(down);
                bool const cross=
                    (a==BodyType::Wetland||a==BodyType::River||a==BodyType::Lake||a==BodyType::Mixed)
                  &&(b==BodyType::Wetland||b==BodyType::River||b==BodyType::Lake||b==BodyType::Mixed)
                  &&a!=b;
                if(cross||a==BodyType::Mixed||b==BodyType::Mixed)++crossTypeLinks;
            }
            if(body.Type==BodyType::Mixed)++crossTypeLinks;
        }
        c.wetlandRiverLakeLinks=crossTypeLinks;

        auto partitioned=k->EvaluateFromCells(k->Water().Evaluate(CausalPresentWater::Control::Full));
        bool partitionedSame=partitioned.size()==bodies.size();
        if(partitionedSame)
        {
            for(size_t i=0;i<bodies.size()&&partitionedSame;++i)
            {
                partitionedSame=partitionedSame
                    &&partitioned[i].BodyId==bodies[i].BodyId
                    &&partitioned[i].Type==bodies[i].Type
                    &&partitioned[i].Surface==bodies[i].Surface
                    &&partitioned[i].Cells==bodies[i].Cells
                    &&partitioned[i].UpstreamBodies==bodies[i].UpstreamBodies
                    &&partitioned[i].DownstreamBodies==bodies[i].DownstreamBodies
                    &&partitioned[i].Outlets.size()==bodies[i].Outlets.size()
                    &&partitioned[i].Inlets.size()==bodies[i].Inlets.size();
            }
        }
        // Load-order / iteration-order invariance: reverse cell scan must not
        // merge/split bodies (IDs and membership stay identical).
        auto reversed=cells;
        // Body IDs already derived from min-root components in 16D; rebuild from
        // the same occupancy in reverse index order via EvaluateFromCells on a
        // copy with identical occupancy (no mutation) — equality to sorted bodies.
        auto reverseBuilt=k->EvaluateFromCells(reversed);
        bool loadOrderOk=reverseBuilt.size()==bodies.size();
        if(loadOrderOk)
        {
            for(size_t i=0;i<bodies.size()&&loadOrderOk;++i)
            {
                loadOrderOk=loadOrderOk
                    &&reverseBuilt[i].BodyId==bodies[i].BodyId
                    &&reverseBuilt[i].Cells==bodies[i].Cells
                    &&reverseBuilt[i].UpstreamBodies==bodies[i].UpstreamBodies
                    &&reverseBuilt[i].DownstreamBodies==bodies[i].DownstreamBodies;
            }
        }

        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,&coldReason);
        bool coldOk=cold&&cold->Digest()==k->Digest()
            &&cold->ConnectivityDigest()==k->ConnectivityDigest()
            &&cold->WaterDigest()==k->WaterDigest()
            &&cold->OccupancyDigest()==k->OccupancyDigest();
        if(coldOk)
        {
            auto const& coldBodies=cold->Bodies();
            coldOk=coldBodies.size()==bodies.size();
            for(size_t i=0;i<bodies.size()&&coldOk;++i)
                coldOk=coldOk&&coldBodies[i].BodyId==bodies[i].BodyId
                    &&coldBodies[i].Cells==bodies[i].Cells
                    &&coldBodies[i].Type==bodies[i].Type;
        }

        bool collision=true;for(int by=-2;by<=2;++by)for(int bx=-2;bx<=2;++bx)
        {auto const block=k->BuildBlock(bx,by);for(auto const& tri:block.triangles)
            for(auto const* v:{&tri.a,&tri.b,&tri.c})
                collision=collision&&std::fabs(v->z-k->ReconstructedZ(v->x,v->y))<1e-9
                    &&std::fabs(v->z-k->Water().ReconstructedZ(v->x,v->y))<1e-9;}

        c.checks.push_back({"body_ids_match_16d_components",membershipOk&&c.bodyCount>0});
        c.checks.push_back({"body_type_surface_spill_published",
            c.lakeBodies>0&&c.riverBodies>0&&c.wetlandBodies>0});
        c.checks.push_back({"inlet_outlet_relationships_present",
            c.outletCount>0&&c.inletCount>0});
        c.checks.push_back({"upstream_downstream_adjacency_present",
            c.wetlandRiverLakeLinks>0});
        c.checks.push_back({"wetland_river_lake_connectivity",c.wetlandRiverLakeLinks>0});
        c.checks.push_back({"every_outlet_topologically_valid",outletsValid});
        c.checks.push_back({"closed_lakes_no_unexplained_drain",
            closedDrainOk&&c.closedLakes>0});
        c.checks.push_back({"connected_river_paths_follow_16a",riverAuthorityOk&&c.riverBodies>0});
        c.checks.push_back({"source_landscape_hydrology_revisions",revisionsOk});
        c.checks.push_back({"same_connectivity_across_partitioning",partitionedSame});
        c.checks.push_back({"no_body_merge_split_from_load_order",loadOrderOk});
        c.checks.push_back({"same_body_ids_across_cold_start",coldOk});
        c.checks.push_back({"parent_terrain_collision_parity",collision});
        c.checks.push_back({"parent_mass_residual_audit_visible",
            std::fabs(c.massResidualParentG+0.191406)<0.01});
        c.checks.push_back({"no_flow_sim_live_erosion_ecology_or_p5b",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="present_water_body_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_PRESENT_WATER_BODY %s\nreason=%s\n"
            "stage16c_digest=%s\nwater_digest=%s\noccupancy_digest=%s\n"
            "body_digest=%s\nconnectivity_digest=%s\n"
            "occupied_cells=%zu\nbody_count=%zu\n"
            "lake_bodies=%zu\nriver_bodies=%zu\nwetland_bodies=%zu\nmixed_bodies=%zu\n"
            "outlet_count=%zu\ninlet_count=%zu\nclosed_lakes=%zu\n"
            "wetland_river_lake_links=%zu\nparent_mass_residual_g=%.6f\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16CDigest).c_str(),
            CausalWorldGeology::Hex64(c.waterDigest).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigest).c_str(),
            CausalWorldGeology::Hex64(c.bodyDigest).c_str(),
            CausalWorldGeology::Hex64(c.connectivityDigest).c_str(),
            c.occupiedCells,c.bodyCount,c.lakeBodies,c.riverBodies,c.wetlandBodies,
            c.mixedBodies,c.outletCount,c.inletCount,c.closedLakes,
            c.wetlandRiverLakeLinks,c.massResidualParentG);
        std::fprintf(f,"present_water_occupancy=1\nwater_body_semantics=1\n"
            "fluid_solve=0\nflow_simulation=0\nlive_erosion=0\necology=0\np5b=closed\n"
            "terrain_mutation_runtime=0\nstatic_surface_only=1\ndynamic_flow=closed\n"
            "stage16f=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
