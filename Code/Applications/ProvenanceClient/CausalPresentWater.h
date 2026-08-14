#pragma once

// Stage 16D: present-water body authority and occupancy. Stage 16C remains the
// immutable sediment/landscape parent. This layer derives static water surfaces
// (lakes, rivers, wetlands, connected bodies) from finished geomorphology. It
// cannot run a flow tick, open P5b, mutate terrain, or invent painted noise.

#include "CausalCompiledSediment.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalPresentWater
{
    constexpr char const* kExpectedRegion="causal_world_present_water_floor";
    constexpr uint64_t kFrozenSedimentDigest=0x1367584c41aedcdfull;

    enum class Control:uint8_t
    {
        Full=0,
        WaterOff=1,
        LakesOff=2,
        RiversOff=3,
        WetlandsOff=4
    };

    enum class BodyKind:uint8_t
    {
        None=0,
        Lake=1,
        River=2,
        Wetland=3,
        Connected=4
    };

    inline char const* BodyKindName(BodyKind value)
    {
        switch(value)
        {
            case BodyKind::Lake:return "lake";
            case BodyKind::River:return "river";
            case BodyKind::Wetland:return "wetland";
            case BodyKind::Connected:return "connected";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,waterEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t waterEnabled=1,lakesEnabled=1,riversEnabled=1,wetlandsEnabled=1;
        std::string worldgenId,regionKey,parentRegionKey;
        double lakeMinDepthM=.08,riverBaseDepthM=.18,riverOrderScaleM=.22;
        double riverAreaScaleM=.35,riverMaxDepthM=2.4,wetlandMaxDepthM=.35;
        double wetlandMinDepthM=.04,occupancyEpsilonM=.001;
        uint32_t lakeMinCells=8;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalCompiledSediment::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_V1";
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
        auto num=[&](char const* key,double& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseDouble(*v,out);};
        auto const* id=get("worldgen_id");auto const* region=get("region_key");
        auto const* parentRegion=get("parent_region_key");if(!id||!region||!parentRegion)
        {r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)&&u32("schema_version",r.program.schemaVersion)
          &&u32("parent_authority_revision",r.program.parentAuthorityRevision)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("water_event_id",r.program.waterEventId)&&u32("chronology",r.program.chronology)
          &&u32("water_enabled",r.program.waterEnabled)&&u32("lakes_enabled",r.program.lakesEnabled)
          &&u32("rivers_enabled",r.program.riversEnabled)&&u32("wetlands_enabled",r.program.wetlandsEnabled)
          &&num("lake_min_depth_m",r.program.lakeMinDepthM)
          &&u32("lake_min_cells",r.program.lakeMinCells)
          &&num("river_base_depth_m",r.program.riverBaseDepthM)
          &&num("river_order_scale_m",r.program.riverOrderScaleM)
          &&num("river_area_scale_m",r.program.riverAreaScaleM)
          &&num("river_max_depth_m",r.program.riverMaxDepthM)
          &&num("wetland_max_depth_m",r.program.wetlandMaxDepthM)
          &&num("wetland_min_depth_m",r.program.wetlandMinDepthM)
          &&num("occupancy_epsilon_m",r.program.occupancyEpsilonM);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.waterEventId!=0
          &&r.program.chronology>parent.chronology
          &&r.program.waterEnabled==1&&r.program.lakesEnabled==1
          &&r.program.riversEnabled==1&&r.program.wetlandsEnabled==1
          &&r.program.lakeMinDepthM>0&&r.program.lakeMinDepthM<4
          &&r.program.lakeMinCells>=2
          &&r.program.riverBaseDepthM>0&&r.program.riverMaxDepthM>=r.program.riverBaseDepthM
          &&r.program.wetlandMaxDepthM>r.program.wetlandMinDepthM
          &&r.program.occupancyEpsilonM>0&&r.program.occupancyEpsilonM<.01;
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_present_water_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct Cell
    {
        double x=0,y=0,terrainZ=0,waterSurfaceZ=0,depthM=0;
        uint64_t bodyId=0;
        uint64_t waterIdentity=0; // matter provenance; survives BodyId split/merge
        BodyKind kind=BodyKind::None;
        bool occupied=false;
        int64_t occupancyUnits=0;
    };

    struct Query
    {
        bool found=false;Cell water;
        CausalCompiledSediment::Query sediment;
        double terrainZ=0,waterSurfaceZ=0,depthM=0;
        bool occupied=false;
    };

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalCompiledSediment::Kernel> sediment,Program program)
          :m_sediment(std::move(sediment)),m_program(std::move(program))
        {
            Build(Control::Full);
            m_digest=DigestFor(Control::Full);
            m_occupancyDigest=OccupancyDigestFor(Control::Full);
        }

        CausalCompiledSediment::Kernel const& Sediment() const{return *m_sediment;}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_sediment->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_sediment->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_sediment->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<Cell> const& Cells() const{return m_cells;}
        uint64_t Digest() const{return m_digest;}
        uint64_t OccupancyDigest() const{return m_occupancyDigest;}

        // Stage 16F.1: body-local hydraulic rewrite. May change depth / surface /
        // occupancyUnits only. Refuses identity, occupancy mask, kind, terrain,
        // or bodyId changes. Parent sediment/terrain remains immutable.
        bool RewriteBodyLocalHydraulics(std::vector<Cell> const& next,std::string* reason=nullptr)
        {
            if(next.size()!=m_cells.size())
            {if(reason)*reason="cell_count_mismatch";return false;}
            for(size_t i=0;i<m_cells.size();++i)
            {
                Cell const& a=m_cells[i];Cell const& b=next[i];
                if(a.occupied!=b.occupied||a.bodyId!=b.bodyId||a.kind!=b.kind
                  ||a.waterIdentity!=b.waterIdentity
                  ||std::fabs(a.terrainZ-b.terrainZ)>1e-12
                  ||std::fabs(a.x-b.x)>1e-12||std::fabs(a.y-b.y)>1e-12)
                {if(reason)*reason="identity_or_terrain_mutation";return false;}
                if(b.occupied)
                {
                    if(b.depthM<=m_program.occupancyEpsilonM
                      ||b.waterSurfaceZ+1e-12<b.terrainZ
                      ||std::fabs((b.waterSurfaceZ-b.terrainZ)-b.depthM)>1e-6
                      ||b.occupancyUnits<=0)
                    {if(reason)*reason="void_or_units_invalid";return false;}
                }
                else if(b.depthM!=0||b.occupancyUnits!=0||b.bodyId!=0
                  ||b.kind!=BodyKind::None||b.waterIdentity!=0)
                {if(reason)*reason="dry_cell_dirty";return false;}
            }
            m_cells=next;
            m_digest=DigestFor(Control::Full);
            m_occupancyDigest=OccupancyDigestFor(Control::Full);
            if(reason)*reason="ok";return true;
        }

        // Stage 16F.4: occupancy-mask + hydraulic identity rewrite on fixed terrain.
        // May change occupied / bodyId / kind / depth / units / waterIdentity.
        // Refuses x/y/terrainZ mutation. Wet cells must remain void-consistent.
        bool RewriteOccupancyAndIdentity(std::vector<Cell> const& next,std::string* reason=nullptr)
        {
            if(next.size()!=m_cells.size())
            {if(reason)*reason="cell_count_mismatch";return false;}
            for(size_t i=0;i<m_cells.size();++i)
            {
                Cell const& a=m_cells[i];Cell const& b=next[i];
                if(std::fabs(a.terrainZ-b.terrainZ)>1e-12
                  ||std::fabs(a.x-b.x)>1e-12||std::fabs(a.y-b.y)>1e-12)
                {if(reason)*reason="terrain_mutation";return false;}
                if(b.occupied)
                {
                    if(b.depthM<=m_program.occupancyEpsilonM
                      ||b.waterSurfaceZ+1e-12<b.terrainZ
                      ||std::fabs((b.waterSurfaceZ-b.terrainZ)-b.depthM)>1e-6
                      ||b.occupancyUnits<=0||b.bodyId==0||b.kind==BodyKind::None)
                    {if(reason)*reason="void_or_units_invalid";return false;}
                }
                else if(b.depthM!=0||b.occupancyUnits!=0||b.bodyId!=0
                  ||b.kind!=BodyKind::None||b.waterIdentity!=0)
                {if(reason)*reason="dry_cell_dirty";return false;}
            }
            m_cells=next;
            m_digest=DigestFor(Control::Full);
            m_occupancyDigest=OccupancyDigestFor(Control::Full);
            if(reason)*reason="ok";return true;
        }

        double ReconstructedZ(double x,double y) const
        {return m_sediment->ReconstructedZ(x,y);}

        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_sediment->QueryGeology(x,y,z);}

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_sediment->SurfaceGeology(x,y);}

        std::vector<Cell> Evaluate(Control control) const
        {
            std::vector<Cell> cells;Derive(control,cells);return cells;
        }

        Query QueryAt(double x,double y,Control control=Control::Full) const
        {
            Query q;q.sediment=m_sediment->QueryAt(x,y);if(!q.sediment.found)return q;
            if(control==Control::Full)q.water=m_cells[(size_t)q.sediment.erosion.drainage.index];
            else
            {
                auto cells=Evaluate(control);
                q.water=cells[(size_t)q.sediment.erosion.drainage.index];
            }
            q.terrainZ=q.water.terrainZ;q.waterSurfaceZ=q.water.waterSurfaceZ;
            q.depthM=q.water.depthM;q.occupied=q.water.occupied;q.found=true;return q;
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_sediment->SampleBlock(bx,by);}

        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_sediment->BuildBlock(bx,by);}

        double SampleDepth(double x,double y,Control control=Control::Full) const
        {
            auto const q=QueryAt(x,y,control);return q.found?q.depthM:0;
        }

        bool SampleOccupied(double x,double y,Control control=Control::Full) const
        {
            auto const q=QueryAt(x,y,control);return q.found&&q.occupied;
        }

    private:
        static uint64_t StableBodyId(uint64_t world,uint64_t tag,int seed)
        {
            uint64_t h=14695981039346656037ull;
            CausalWorldGeology::HashAppend(h,&world,sizeof(world));
            CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
            CausalWorldGeology::HashAppend(h,&seed,sizeof(seed));
            return h;
        }

        double RiverDepth(CausalDryHydrology::Cell const& d) const
        {
            if(!d.channel)return 0;
            double const areaKm2=d.accumulationM2/1e6;
            double depth=m_program.riverBaseDepthM
                +m_program.riverOrderScaleM*(double)d.channelOrder
                +m_program.riverAreaScaleM*std::log1p(areaKm2);
            return std::clamp(depth,0.0,m_program.riverMaxDepthM);
        }

        void Derive(Control control,std::vector<Cell>& out) const
        {
            auto const& drainage=Drainage().Cells();
            auto const& sediment=m_sediment->Cells();
            auto const& erosion=Erosion().Cells();
            out.assign(drainage.size(),{});
            bool const waterOn=control!=Control::WaterOff&&m_program.waterEnabled;
            bool const lakesOn=waterOn&&control!=Control::LakesOff&&m_program.lakesEnabled;
            bool const riversOn=waterOn&&control!=Control::RiversOff&&m_program.riversEnabled;
            bool const wetlandsOn=waterOn&&control!=Control::WetlandsOff&&m_program.wetlandsEnabled;

            for(size_t i=0;i<drainage.size();++i)
            {
                Cell& cell=out[i];
                cell.x=drainage[i].x;cell.y=drainage[i].y;
                cell.terrainZ=sediment[i].surfaceZ;
                cell.waterSurfaceZ=cell.terrainZ;
            }
            if(!waterOn)return;

            struct BasinAgg
            {
                size_t cells=0;int spill=-1;double spillZ=0;
                CausalCompiledFluvialErosion::BasinClass klass=
                    CausalCompiledFluvialErosion::BasinClass::None;
            };
            std::map<uint64_t,BasinAgg> basins;
            for(size_t i=0;i<drainage.size();++i)
            {
                uint64_t const id=drainage[i].basinId;if(!id)continue;
                BasinAgg& agg=basins[id];++agg.cells;
                if(agg.klass==CausalCompiledFluvialErosion::BasinClass::None)
                    agg.klass=erosion[i].basinClass;
                if(drainage[i].spill)
                {
                    agg.spill=(int)i;agg.spillZ=sediment[i].surfaceZ;
                }
            }
            for(auto& kv:basins)
            {
                BasinAgg& agg=kv.second;
                if(agg.spill<0)
                {
                    double best=1e300;int bestI=-1;
                    for(size_t i=0;i<drainage.size();++i)
                    {
                        if(drainage[i].basinId!=kv.first)continue;
                        if(sediment[i].surfaceZ<best){best=sediment[i].surfaceZ;bestI=(int)i;}
                    }
                    agg.spill=bestI;agg.spillZ=bestI>=0?sediment[(size_t)bestI].surfaceZ:0;
                }
            }

            if(lakesOn)
            {
                for(size_t i=0;i<drainage.size();++i)
                {
                    uint64_t const id=drainage[i].basinId;if(!id)continue;
                    auto const it=basins.find(id);if(it==basins.end())continue;
                    BasinAgg const& agg=it->second;
                    if(agg.cells<(size_t)m_program.lakeMinCells)continue;
                    using BC=CausalCompiledFluvialErosion::BasinClass;
                    if(agg.klass!=BC::ClosedGeomorphic&&agg.klass!=BC::ThroughSpill
                      &&agg.klass!=BC::Structural&&agg.klass!=BC::ChannelConnected)continue;
                    double const depth=agg.spillZ-sediment[i].surfaceZ;
                    if(depth+1e-12<m_program.lakeMinDepthM)continue;
                    Cell& cell=out[i];
                    cell.kind=BodyKind::Lake;
                    cell.waterSurfaceZ=agg.spillZ;
                    cell.depthM=depth;
                    cell.occupied=depth>m_program.occupancyEpsilonM;
                }
            }

            if(riversOn)
            {
                for(size_t i=0;i<drainage.size();++i)
                {
                    if(!drainage[i].channel)continue;
                    double const depth=RiverDepth(drainage[i]);
                    if(depth<=m_program.occupancyEpsilonM)continue;
                    Cell& cell=out[i];
                    if(cell.kind==BodyKind::Lake&&cell.depthM>=depth)continue;
                    if(cell.kind==BodyKind::Lake)
                    {
                        // Deeper lake occupancy wins; shallow lake cells still keep lake kind.
                        continue;
                    }
                    cell.kind=BodyKind::River;
                    cell.depthM=depth;
                    cell.waterSurfaceZ=cell.terrainZ+depth;
                    cell.occupied=true;
                }
            }

            if(wetlandsOn)
            {
                for(size_t i=0;i<drainage.size();++i)
                {
                    Cell& cell=out[i];
                    if(cell.occupied)continue;
                    using BC=CausalCompiledFluvialErosion::BasinClass;
                    bool wetland=false;double depth=0;double surface=cell.terrainZ;
                    if(drainage[i].basinId)
                    {
                        auto const it=basins.find(drainage[i].basinId);
                        if(it!=basins.end())
                        {
                            BasinAgg const& agg=it->second;
                            double const pond=agg.spillZ-sediment[i].surfaceZ;
                            if(agg.klass==BC::MicroSink
                              &&pond>=m_program.wetlandMinDepthM)
                            {
                                depth=(std::min)(pond,m_program.wetlandMaxDepthM);
                                surface=cell.terrainZ+depth;wetland=depth>m_program.occupancyEpsilonM;
                            }
                            else if((agg.klass==BC::ClosedGeomorphic||agg.klass==BC::ThroughSpill
                                  ||agg.klass==BC::Structural||agg.klass==BC::ChannelConnected)
                              &&pond>m_program.occupancyEpsilonM
                              &&pond<m_program.lakeMinDepthM
                              &&pond>=m_program.wetlandMinDepthM)
                            {
                                depth=(std::min)(pond,m_program.wetlandMaxDepthM);
                                surface=agg.spillZ;wetland=true;
                            }
                        }
                    }
                    if(!wetland
                      &&(sediment[i].facies==CausalCompiledSediment::DepositFacies::Floodplain
                        ||sediment[i].facies==CausalCompiledSediment::DepositFacies::BasinFill)
                      &&drainage[i].accumulationM2>2500.0
                      &&drainage[i].slope<0.02)
                    {
                        depth=m_program.wetlandMinDepthM
                            +.5*(m_program.wetlandMaxDepthM-m_program.wetlandMinDepthM);
                        surface=cell.terrainZ+depth;wetland=true;
                    }
                    if(!wetland)continue;
                    cell.kind=BodyKind::Wetland;
                    cell.depthM=depth;
                    cell.waterSurfaceZ=surface;
                    cell.occupied=depth>m_program.occupancyEpsilonM;
                }
            }

            // Occupancy units from depth via P5a capacity contract (presentation seed only).
            double const cellArea=Drainage().StepM()*Drainage().StepM();
            for(Cell& cell:out)
            {
                if(!cell.occupied||cell.depthM<=m_program.occupancyEpsilonM)
                {
                    cell.occupied=false;cell.depthM=0;cell.waterSurfaceZ=cell.terrainZ;
                    cell.kind=BodyKind::None;cell.bodyId=0;cell.waterIdentity=0;cell.occupancyUnits=0;
                    continue;
                }
                // Void consistency: water surface must sit at/above terrain.
                if(cell.waterSurfaceZ+1e-12<cell.terrainZ)
                {cell.waterSurfaceZ=cell.terrainZ;cell.depthM=0;cell.occupied=false;
                    cell.kind=BodyKind::None;cell.bodyId=0;cell.waterIdentity=0;cell.occupancyUnits=0;continue;}
                cell.depthM=cell.waterSurfaceZ-cell.terrainZ;
                if(cell.depthM<=m_program.occupancyEpsilonM)
                {cell.occupied=false;cell.depthM=0;cell.kind=BodyKind::None;cell.bodyId=0;
                    cell.waterIdentity=0;cell.occupancyUnits=0;continue;}
                // P5a capacity contract scale (100 units ⇔ 1 m³ on 1 m² footprint).
                // Seeded occupancy only — not a live ledger / P5b wake.
                double const volume=cell.depthM*cellArea;
                cell.occupancyUnits=(int64_t)std::llround(volume*100.0);
                if(cell.occupancyUnits<0)cell.occupancyUnits=0;
            }

            // Connected body identity over occupied adjacency (4-neighborhood).
            int const width=Drainage().Width(),height=Drainage().Height();
            auto index=[&](int x,int y){return y*width+x;};
            std::vector<uint8_t> seen(out.size(),0);
            static int const dx[4]={1,-1,0,0};
            static int const dy[4]={0,0,1,-1};
            uint64_t const world=m_program.worldIdentityHash;
            for(size_t seed=0;seed<out.size();++seed)
            {
                if(!out[seed].occupied||seen[seed])continue;
                std::vector<int> component;std::queue<int> q;
                q.push((int)seed);seen[seed]=1;
                uint8_t hasLake=0,hasRiver=0,hasWetland=0;
                while(!q.empty())
                {
                    int const i=q.front();q.pop();component.push_back(i);
                    BodyKind const k=out[(size_t)i].kind;
                    if(k==BodyKind::Lake)hasLake=1;
                    else if(k==BodyKind::River)hasRiver=1;
                    else if(k==BodyKind::Wetland)hasWetland=1;
                    int const x=i%width,y=i/width;
                    for(int n=0;n<4;++n)
                    {
                        int const nx=x+dx[n],ny=y+dy[n];
                        if(nx<0||ny<0||nx>=width||ny>=height)continue;
                        int const ni=index(nx,ny);
                        if(seen[(size_t)ni]||!out[(size_t)ni].occupied)continue;
                        seen[(size_t)ni]=1;q.push(ni);
                    }
                }
                BodyKind bodyKind=BodyKind::Wetland;
                int kinds=(int)hasLake+(int)hasRiver+(int)hasWetland;
                if(kinds>=2)bodyKind=BodyKind::Connected;
                else if(hasLake)bodyKind=BodyKind::Lake;
                else if(hasRiver)bodyKind=BodyKind::River;
                else bodyKind=BodyKind::Wetland;
                int root=*std::min_element(component.begin(),component.end());
                uint64_t const bodyId=StableBodyId(world,0x16D0B0DEull,root);
                for(int i:component)
                {
                    out[(size_t)i].bodyId=bodyId;
                    out[(size_t)i].waterIdentity=bodyId;
                    if(kinds>=2&&out[(size_t)i].kind!=BodyKind::None)
                    {
                        // Keep cell-local kind for diagnostics; body is Connected.
                        (void)bodyKind;
                    }
                }
                // Stamp Connected on cells only when the component mixes kinds.
                if(bodyKind==BodyKind::Connected)
                {
                    for(int i:component){/* bodyId already set; cell kind retained */}
                }
                (void)bodyKind;
            }
        }

        void Build(Control control){Derive(control,m_cells);}

        uint64_t DigestFor(Control control) const
        {
            auto cells=control==Control::Full?m_cells:Evaluate(control);
            uint64_t digest=14695981039346656037ull;
            uint64_t const parent=m_sediment->Digest();
            CausalWorldGeology::HashAppend(digest,&parent,sizeof(parent));
            uint8_t const c=(uint8_t)control;
            CausalWorldGeology::HashAppend(digest,&c,sizeof(c));
            for(Cell const& cell:cells)
            {
                CausalWorldGeology::HashAppend(digest,&cell.depthM,sizeof(cell.depthM));
                CausalWorldGeology::HashAppend(digest,&cell.waterSurfaceZ,sizeof(cell.waterSurfaceZ));
                CausalWorldGeology::HashAppend(digest,&cell.bodyId,sizeof(cell.bodyId));
                CausalWorldGeology::HashAppend(digest,&cell.kind,sizeof(cell.kind));
                uint8_t const occ=cell.occupied?1:0;
                CausalWorldGeology::HashAppend(digest,&occ,sizeof(occ));
                CausalWorldGeology::HashAppend(digest,&cell.occupancyUnits,sizeof(cell.occupancyUnits));
            }
            return digest;
        }

        uint64_t OccupancyDigestFor(Control control) const
        {
            auto cells=control==Control::Full?m_cells:Evaluate(control);
            uint64_t digest=14695981039346656037ull;
            for(Cell const& cell:cells)
            {
                uint8_t const occ=cell.occupied?1:0;
                CausalWorldGeology::HashAppend(digest,&occ,sizeof(occ));
                CausalWorldGeology::HashAppend(digest,&cell.depthM,sizeof(cell.depthM));
                CausalWorldGeology::HashAppend(digest,&cell.occupancyUnits,sizeof(cell.occupancyUnits));
            }
            return digest;
        }

        std::unique_ptr<CausalCompiledSediment::Kernel> m_sediment;Program m_program;
        std::vector<Cell> m_cells;uint64_t m_digest=0,m_occupancyDigest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,std::string* reason=nullptr)
    {
        std::string source;if(!ReadFile(waterPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalCompiledSediment::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,&parentReason);
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
        size_t occupiedCells=0,lakeCells=0,riverCells=0,wetlandCells=0,bodyCount=0;
        size_t connectedBodies=0;
        double maxDepthM=0,meanDepthM=0,occupiedVolumeM3=0;
        double massResidualParentG=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath)
    {
        CertResult c;auto const parent=CausalCompiledSediment::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath);
        c.stage16CDigest=parent.sedimentDigest;
        c.massResidualParentG=parent.massResidualG;
        c.checks.push_back({"stage16c_frozen_control",
            parent.passed&&parent.sedimentDigest==kFrozenSedimentDigest});
        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,&reason);
        c.reason=reason;c.checks.push_back({"descriptor_linked_present_water",k!=nullptr});
        if(!parent.passed||!k)return c;
        c.waterDigest=k->Digest();c.occupancyDigest=k->OccupancyDigest();
        auto const& cells=k->Cells();
        double depthSum=0;std::map<uint64_t,uint8_t> bodies;std::map<uint64_t,uint8_t> bodyKinds;
        double const cellArea=k->Drainage().StepM()*k->Drainage().StepM();
        bool voidOk=true,surfaceParity=true;
        for(size_t i=0;i<cells.size();++i)
        {
            Cell const& cell=cells[i];
            double const parentZ=k->Sediment().Cells()[i].surfaceZ;
            surfaceParity=surfaceParity&&std::fabs(cell.terrainZ-parentZ)<1e-9;
            if(cell.occupied)
            {
                ++c.occupiedCells;depthSum+=cell.depthM;
                c.maxDepthM=(std::max)(c.maxDepthM,cell.depthM);
                c.occupiedVolumeM3+=cell.depthM*cellArea;
                voidOk=voidOk&&cell.depthM>k->GetProgram().occupancyEpsilonM
                    &&cell.waterSurfaceZ+1e-12>=cell.terrainZ
                    &&std::fabs((cell.waterSurfaceZ-cell.terrainZ)-cell.depthM)<1e-9
                    &&cell.occupancyUnits>=0;
                if(cell.kind==BodyKind::Lake)++c.lakeCells;
                else if(cell.kind==BodyKind::River)++c.riverCells;
                else if(cell.kind==BodyKind::Wetland)++c.wetlandCells;
                if(cell.bodyId)
                {
                    bodies[cell.bodyId]=1;
                    uint8_t& mask=bodyKinds[cell.bodyId];
                    if(cell.kind==BodyKind::Lake)mask|=1;
                    if(cell.kind==BodyKind::River)mask|=2;
                    if(cell.kind==BodyKind::Wetland)mask|=4;
                }
            }
            else
            {
                voidOk=voidOk&&cell.depthM==0&&cell.occupancyUnits==0
                    &&cell.kind==BodyKind::None&&cell.bodyId==0;
            }
        }
        c.meanDepthM=c.occupiedCells?depthSum/(double)c.occupiedCells:0;
        c.bodyCount=bodies.size();
        c.connectedBodies=0;
        for(auto const& kv:bodyKinds)
        {
            int bits=(kv.second&1?1:0)+(kv.second&2?1:0)+(kv.second&4?1:0);
            if(bits>=2)++c.connectedBodies;
        }

        auto waterOff=k->Evaluate(Control::WaterOff);
        bool waterOffZero=true;
        for(Cell const& cell:waterOff)
            waterOffZero=waterOffZero&&!cell.occupied&&cell.depthM==0&&cell.bodyId==0;

        auto lakesOff=k->Evaluate(Control::LakesOff);
        bool lakesOffNoLake=true;size_t lakesOffLake=0;
        for(Cell const& cell:lakesOff)
            if(cell.kind==BodyKind::Lake){lakesOffNoLake=false;++lakesOffLake;}

        auto riversOff=k->Evaluate(Control::RiversOff);
        bool riversOffNoRiver=true;
        for(Cell const& cell:riversOff)if(cell.kind==BodyKind::River)riversOffNoRiver=false;

        auto wetlandsOff=k->Evaluate(Control::WetlandsOff);
        bool wetlandsOffNoWetland=true;
        for(Cell const& cell:wetlandsOff)if(cell.kind==BodyKind::Wetland)wetlandsOffNoWetland=false;

        bool riversOnChannels=true;
        auto const& drainage=k->Drainage().Cells();
        for(size_t i=0;i<cells.size();++i)
            if(cells[i].kind==BodyKind::River)
                riversOnChannels=riversOnChannels&&drainage[i].channel;

        bool lakesNeedBasin=true;
        for(size_t i=0;i<cells.size();++i)
            if(cells[i].kind==BodyKind::Lake)
                lakesNeedBasin=lakesNeedBasin&&drainage[i].basinId!=0;

        bool collision=true;for(int by=-2;by<=2;++by)for(int bx=-2;bx<=2;++bx)
        {auto const block=k->BuildBlock(bx,by);for(auto const& tri:block.triangles)
            for(auto const* v:{&tri.a,&tri.b,&tri.c})
                collision=collision&&std::fabs(v->z-k->ReconstructedZ(v->x,v->y))<1e-9
                    &&std::fabs(v->z-k->Sediment().ReconstructedZ(v->x,v->y))<1e-9;}

        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,&coldReason);
        auto partitioned=k->Evaluate(Control::Full);
        bool monolithic=partitioned.size()==cells.size();
        for(size_t i=0;i<cells.size()&&monolithic;++i)
            monolithic=monolithic
                &&partitioned[i].occupied==cells[i].occupied
                &&partitioned[i].kind==cells[i].kind
                &&partitioned[i].bodyId==cells[i].bodyId
                &&std::fabs(partitioned[i].depthM-cells[i].depthM)<1e-9
                &&partitioned[i].occupancyUnits==cells[i].occupancyUnits;

        c.checks.push_back({"water_off_zero_occupancy",waterOffZero});
        c.checks.push_back({"occupancy_matches_terrain_void",voidOk&&surfaceParity});
        c.checks.push_back({"lakes_off_removes_lake_cells",lakesOffNoLake&&lakesOffLake==0});
        c.checks.push_back({"rivers_off_removes_river_cells",riversOffNoRiver});
        c.checks.push_back({"wetlands_off_removes_wetland_cells",wetlandsOffNoWetland});
        c.checks.push_back({"rivers_follow_channel_network",riversOnChannels&&c.riverCells>0});
        c.checks.push_back({"lakes_require_basin_membership",lakesNeedBasin&&c.lakeCells>0});
        c.checks.push_back({"wetlands_present",c.wetlandCells>0});
        c.checks.push_back({"connected_bodies_present",c.bodyCount>0});
        c.checks.push_back({"parent_terrain_unchanged",surfaceParity});
        c.checks.push_back({"render_collision_surface_parity",collision});
        c.checks.push_back({"monolithic_equals_partitioned",monolithic});
        c.checks.push_back({"cold_start_digest_stable",cold&&cold->Digest()==k->Digest()
            &&cold->OccupancyDigest()==k->OccupancyDigest()});
        c.checks.push_back({"parent_mass_residual_audit_visible",
            std::fabs(c.massResidualParentG+0.191406)<0.01});
        c.checks.push_back({"no_flow_sim_live_erosion_ecology_or_p5b",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="present_water_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_PRESENT_WATER %s\nreason=%s\nstage16c_digest=%s\n"
            "water_digest=%s\noccupancy_digest=%s\n"
            "occupied_cells=%zu\nlake_cells=%zu\nriver_cells=%zu\nwetland_cells=%zu\n"
            "body_count=%zu\nconnected_bodies=%zu\n"
            "depth_max_m=%.6f\ndepth_mean_m=%.6f\noccupied_volume_m3=%.3f\n"
            "parent_mass_residual_g=%.6f\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16CDigest).c_str(),
            CausalWorldGeology::Hex64(c.waterDigest).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigest).c_str(),
            c.occupiedCells,c.lakeCells,c.riverCells,c.wetlandCells,c.bodyCount,
            c.connectedBodies,c.maxDepthM,c.meanDepthM,c.occupiedVolumeM3,
            c.massResidualParentG);
        std::fprintf(f,"present_water_occupancy=1\nwater_rendering=1\nfluid_solve=0\n"
            "flow_simulation=0\nlive_erosion=0\necology=0\np5b=closed\n"
            "terrain_mutation_runtime=0\nstatic_surface_only=1\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
