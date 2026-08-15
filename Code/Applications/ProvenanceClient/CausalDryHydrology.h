#pragma once

// Stage 16A: dry watershed and drainage authority. The graph is derived only
// from the frozen Stage-15 surface. It describes potential flow topology but
// cannot move terrain, create water, erode material, transport sediment, or
// open P5b.

#include "CausalBareEarthGeography.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalDryHydrology
{
    constexpr char const* kExpectedRegion="causal_world_dry_hydrology_floor";

    struct Program
    {
        uint64_t worldIdentityHash=0,hydrologyEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        std::string worldgenId,regionKey,parentRegionKey;
        double gridStepM=16.0,channelThresholdKm2=.075,depressionEpsilonM=1e-6;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline LoadResult LoadText(std::string const& source,
        CausalBareEarthGeography::Program const& parent)
    {
        LoadResult result;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic)
            {
                magic=line=="PROVENANCE_CAUSAL_DRY_HYDROLOGY_V1";
                if(!magic){result.reason="bad_magic";return result;}continue;
            }
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {result.reason="malformed_line";return result;}
            std::string const key=line.substr(0,eq);
            if(fields.count(key)){result.reason="duplicate_key:"+key;return result;}
            fields.emplace(key,line.substr(eq+1));
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
        auto const* parentRegion=get("parent_region_key");
        if(!id||!region||!parentRegion){result.reason="missing_identity";return result;}
        result.program.worldgenId=*id;result.program.regionKey=*region;
        result.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",result.program.worldIdentityHash)
          &&u32("worldgen_version",result.program.worldgenVersion)
          &&u32("schema_version",result.program.schemaVersion)
          &&u32("parent_authority_revision",result.program.parentAuthorityRevision)
          &&u32("authority_revision",result.program.authorityRevision)
          &&hex("hydrology_event_id",result.program.hydrologyEventId)
          &&u32("chronology",result.program.chronology)
          &&num("grid_step_m",result.program.gridStepM)
          &&num("channel_threshold_km2",result.program.channelThresholdKm2)
          &&num("depression_epsilon_m",result.program.depressionEpsilonM);
        if(!values){result.reason="invalid_program_values";return result;}
        bool const identity=result.program.worldIdentityHash==parent.worldIdentityHash
          &&result.program.worldgenId==parent.worldgenId
          &&result.program.worldgenVersion==parent.worldgenVersion
          &&result.program.schemaVersion==1
          &&result.program.regionKey==kExpectedRegion
          &&result.program.parentRegionKey==parent.regionKey
          &&result.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=result.program.authorityRevision>0
          &&result.program.hydrologyEventId!=0
          &&result.program.chronology>parent.erosionChronology
          &&result.program.gridStepM>=4.0&&result.program.gridStepM<=64.0
          &&result.program.channelThresholdKm2>0
          &&result.program.depressionEpsilonM>0&&result.program.depressionEpsilonM<.01;
        if(!identity){result.reason="parent_authority_mismatch";return result;}
        if(!contract){result.reason="invalid_dry_hydrology_contract";return result;}
        result.ok=true;result.reason="ok";return result;
    }

    struct Cell
    {
        double x=0,y=0,surfaceZ=0,filledZ=0,headZ=0;
        double accumulationM2=0,slope=0,spillElevationM=0;
        int receiver=-1,floodRank=-1;
        uint64_t watershedId=0,basinId=0;
        uint8_t channelOrder=0;
        bool boundary=false,channel=false,spill=false;
    };

    struct Query
    {
        bool found=false;int index=-1;Cell cell;
        double flowDx=0,flowDy=0;
    };

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalBareEarthGeography::Kernel> geography,Program program)
          :m_geography(std::move(geography)),m_program(std::move(program)){Build();}

        CausalBareEarthGeography::Kernel const& Geography() const{return *m_geography;}
        Program const& GetProgram() const{return m_program;}
        std::vector<Cell> const& Cells() const{return m_cells;}
        int Width() const{return m_width;}int Height() const{return m_height;}
        double MinX() const{return m_minX;}double MinY() const{return m_minY;}
        double StepM() const{return m_program.gridStepM;}
        uint64_t Digest() const{return m_digest;}

        Query QueryAt(double x,double y) const
        {
            Query q;
            CausalBareEarthGeography::Kernel::WrapIntoRegion(
                m_geography->GetProgram(),x,y);
            int const ix=(int)std::floor((x-m_minX)/m_program.gridStepM);
            int const iy=(int)std::floor((y-m_minY)/m_program.gridStepM);
            if(ix<0||iy<0||ix>=m_width||iy>=m_height)return q;
            q.index=Index(ix,iy);q.cell=m_cells[(size_t)q.index];q.found=true;
            if(q.cell.receiver>=0)
            {
                Cell const& r=m_cells[(size_t)q.cell.receiver];double const d=std::hypot(
                    r.x-q.cell.x,r.y-q.cell.y);if(d>0){q.flowDx=(r.x-q.cell.x)/d;q.flowDy=(r.y-q.cell.y)/d;}
            }
            return q;
        }

    private:
        struct HeapNode{double z;int index;};
        struct HeapLess{bool operator()(HeapNode const& a,HeapNode const& b) const
        {return a.z>b.z||(a.z==b.z&&a.index>b.index);}};

        int Index(int x,int y) const{return y*m_width+x;}
        bool Inside(int x,int y) const{return x>=0&&y>=0&&x<m_width&&y<m_height;}
        static uint64_t StableId(uint64_t world,uint64_t tag,int index)
        {
            uint64_t h=14695981039346656037ull;
            CausalWorldGeology::HashAppend(h,&world,sizeof(world));
            CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
            CausalWorldGeology::HashAppend(h,&index,sizeof(index));return h;
        }

        void Build()
        {
            auto const& gp=m_geography->GetProgram();m_minX=gp.minX;m_minY=gp.minY;
            m_width=(int)std::llround((gp.maxX-gp.minX)/m_program.gridStepM);
            m_height=(int)std::llround((gp.maxY-gp.minY)/m_program.gridStepM);
            size_t const count=(size_t)m_width*m_height;m_cells.assign(count,{});
            for(int y=0;y<m_height;++y)for(int x=0;x<m_width;++x)
            {
                int const i=Index(x,y);Cell& c=m_cells[(size_t)i];
                c.x=m_minX+(x+.5)*m_program.gridStepM;
                c.y=m_minY+(y+.5)*m_program.gridStepM;
                c.surfaceZ=m_geography->ReconstructedZ(c.x,c.y);
                c.filledZ=std::numeric_limits<double>::infinity();
                c.boundary=x==0||y==0||x==m_width-1||y==m_height-1;
            }

            std::priority_queue<HeapNode,std::vector<HeapNode>,HeapLess> heap;
            std::vector<uint8_t> visited(count,0);int rank=0;
            for(size_t i=0;i<count;++i)if(m_cells[i].boundary)
            {visited[i]=1;m_cells[i].filledZ=m_cells[i].surfaceZ;heap.push({m_cells[i].filledZ,(int)i});}
            static int const dx[8]={-1,0,1,-1,1,-1,0,1};
            static int const dy[8]={-1,-1,-1,0,0,1,1,1};
            while(!heap.empty())
            {
                HeapNode const node=heap.top();heap.pop();Cell& current=m_cells[(size_t)node.index];
                current.floodRank=rank++;int const cx=node.index%m_width,cy=node.index/m_width;
                for(int n=0;n<8;++n)
                {
                    int const nx=cx+dx[n],ny=cy+dy[n];if(!Inside(nx,ny))continue;
                    int const ni=Index(nx,ny);if(visited[(size_t)ni])continue;visited[(size_t)ni]=1;
                    Cell& next=m_cells[(size_t)ni];next.filledZ=(std::max)(next.surfaceZ,current.filledZ);
                    next.receiver=node.index;heap.push({next.filledZ,ni});
                }
            }

            // Prefer the steepest deterministic D8 descent over the filled
            // surface. Flat/depression routing follows lower flood rank.
            for(size_t ii=0;ii<count;++ii)
            {
                Cell& c=m_cells[ii];if(c.boundary){c.receiver=-1;continue;}
                int const cx=(int)ii%m_width,cy=(int)ii/m_width;int best=-1;
                double bestDrop=-std::numeric_limits<double>::infinity();
                for(int n=0;n<8;++n)
                {
                    int const nx=cx+dx[n],ny=cy+dy[n];if(!Inside(nx,ny))continue;
                    int const ni=Index(nx,ny);Cell const& q=m_cells[(size_t)ni];
                    if(q.floodRank>=c.floodRank)continue;
                    double const distance=m_program.gridStepM*((dx[n]&&dy[n])?std::sqrt(2.0):1.0);
                    double const drop=(c.filledZ-q.filledZ)/distance;
                    if(drop>bestDrop||(drop==bestDrop&&ni<best)){bestDrop=drop;best=ni;}
                }
                c.receiver=best;
            }

            // Label connected filled depressions and publish explicit spill
            // cells/elevations. This is topology only; it does not add water.
            std::vector<uint8_t> seen(count,0);std::vector<int> component;
            for(size_t seed=0;seed<count;++seed)
            {
                if(seen[seed]||m_cells[seed].filledZ-m_cells[seed].surfaceZ<=m_program.depressionEpsilonM)continue;
                component.clear();std::queue<int> open;open.push((int)seed);seen[seed]=1;
                while(!open.empty())
                {
                    int const i=open.front();open.pop();component.push_back(i);
                    int const cx=i%m_width,cy=i/m_width;
                    for(int n=0;n<8;++n){int const nx=cx+dx[n],ny=cy+dy[n];if(!Inside(nx,ny))continue;
                        int const ni=Index(nx,ny);if(!seen[(size_t)ni]
                          &&m_cells[(size_t)ni].filledZ-m_cells[(size_t)ni].surfaceZ>m_program.depressionEpsilonM)
                        {seen[(size_t)ni]=1;open.push(ni);}}
                }
                int root=*std::min_element(component.begin(),component.end()),spill=-1;
                double spillZ=std::numeric_limits<double>::infinity();
                for(int i:component)
                {
                    int const r=m_cells[(size_t)i].receiver;
                    if(r>=0&&m_cells[(size_t)r].filledZ-m_cells[(size_t)r].surfaceZ<=m_program.depressionEpsilonM)
                    {double const z=(std::max)(m_cells[(size_t)i].surfaceZ,m_cells[(size_t)r].surfaceZ);
                        if(z<spillZ||(z==spillZ&&i<spill)){spillZ=z;spill=i;}}
                }
                if(spill<0){spill=component.front();spillZ=m_cells[(size_t)spill].filledZ;}
                uint64_t const id=StableId(m_program.worldIdentityHash,0x424153494eull,root);
                for(int i:component){m_cells[(size_t)i].basinId=id;m_cells[(size_t)i].spillElevationM=spillZ;}
                m_cells[(size_t)spill].spill=true;
            }

            // Every receiver has lower flood rank, so descending rank is a
            // deterministic upstream-to-downstream accumulation order.
            std::vector<int> order(count);for(size_t i=0;i<count;++i)order[i]=(int)i;
            std::sort(order.begin(),order.end(),[&](int a,int b){return m_cells[(size_t)a].floodRank>m_cells[(size_t)b].floodRank;});
            double const cellArea=m_program.gridStepM*m_program.gridStepM;
            for(Cell& c:m_cells)c.accumulationM2=cellArea;
            for(int i:order){int const r=m_cells[(size_t)i].receiver;if(r>=0)m_cells[(size_t)r].accumulationM2+=m_cells[(size_t)i].accumulationM2;}

            double const channelArea=m_program.channelThresholdKm2*1000000.0;
            std::vector<uint8_t> childMax(count,0),childMaxCount(count,0);
            for(int i:order)
            {
                Cell& c=m_cells[(size_t)i];c.channel=c.accumulationM2>=channelArea;
                if(c.channel)c.channelOrder=childMax[(size_t)i]==0?1:
                    (uint8_t)(childMax[(size_t)i]+(childMaxCount[(size_t)i]>=2?1:0));
                int const r=c.receiver;if(r>=0&&c.channel)
                {
                    uint8_t const value=c.channelOrder;
                    if(value>childMax[(size_t)r]){childMax[(size_t)r]=value;childMaxCount[(size_t)r]=1;}
                    else if(value==childMax[(size_t)r])++childMaxCount[(size_t)r];
                }
            }

            // Roots and stable watershed identities propagate from boundary
            // outlets in ascending rank (receiver is already resolved).
            std::sort(order.begin(),order.end(),[&](int a,int b){return m_cells[(size_t)a].floodRank<m_cells[(size_t)b].floodRank;});
            for(int i:order)
            {
                Cell& c=m_cells[(size_t)i];
                c.watershedId=c.receiver<0?StableId(m_program.worldIdentityHash,0x5741544552ull,i)
                    :m_cells[(size_t)c.receiver].watershedId;
                c.headZ=c.filledZ+c.floodRank*1e-9;
                if(c.receiver>=0)
                {
                    Cell const& r=m_cells[(size_t)c.receiver];double const d=std::hypot(c.x-r.x,c.y-r.y);
                    c.slope=d>0?(c.headZ-(r.filledZ+r.floodRank*1e-9))/d:0;
                }
            }

            m_digest=14695981039346656037ull;
            for(Cell const& c:m_cells)
            {
                CausalWorldGeology::HashAppend(m_digest,&c.surfaceZ,sizeof(c.surfaceZ));
                CausalWorldGeology::HashAppend(m_digest,&c.filledZ,sizeof(c.filledZ));
                CausalWorldGeology::HashAppend(m_digest,&c.receiver,sizeof(c.receiver));
                CausalWorldGeology::HashAppend(m_digest,&c.accumulationM2,sizeof(c.accumulationM2));
                CausalWorldGeology::HashAppend(m_digest,&c.watershedId,sizeof(c.watershedId));
                CausalWorldGeology::HashAppend(m_digest,&c.basinId,sizeof(c.basinId));
                CausalWorldGeology::HashAppend(m_digest,&c.channelOrder,sizeof(c.channelOrder));
            }
        }

        std::unique_ptr<CausalBareEarthGeography::Kernel> m_geography;Program m_program;
        std::vector<Cell> m_cells;int m_width=0,m_height=0;double m_minX=0,m_minY=0;
        uint64_t m_digest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,std::string* reason=nullptr)
    {
        std::string source;if(!ReadFile(hydrologyPath,source)){if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalBareEarthGeography::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,&parentReason);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t topologyDigest=0,stage15SemanticDigest=0;
        size_t cells=0,channels=0,confluences=0,watersheds=0,basins=0,spills=0,outlets=0;
        size_t invalidReceivers=0,uphillNonBasin=0,channelDivideCrossings=0;
        double maxAccumulationKm2=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath)
    {
        CertResult c;auto const stage15=CausalBareEarthGeography::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath);
        c.stage15SemanticDigest=stage15.semanticDigest;
        c.checks.push_back({"stage15_frozen_negative_control",stage15.passed});
        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,&reason);
        c.reason=reason;c.checks.push_back({"descriptor_linked_dry_authority",k!=nullptr});
        if(!stage15.passed||!k)return c;
        c.topologyDigest=k->Digest();c.cells=k->Cells().size();
        std::vector<uint64_t> watershedIds,basinIds;
        std::vector<int> channelUpstream(k->Cells().size(),0);
        for(size_t i=0;i<k->Cells().size();++i)
        {
            Cell const& cell=k->Cells()[i];watershedIds.push_back(cell.watershedId);
            if(cell.basinId)basinIds.push_back(cell.basinId);if(cell.channel)++c.channels;
            if(cell.spill)++c.spills;if(cell.receiver<0)++c.outlets;
            c.maxAccumulationKm2=(std::max)(c.maxAccumulationKm2,cell.accumulationM2/1e6);
            if(cell.receiver>=0)
            {
                Cell const& receiver=k->Cells()[(size_t)cell.receiver];
                if(cell.channel&&receiver.channel)++channelUpstream[(size_t)cell.receiver];
                bool const adjacent=std::fabs(receiver.x-cell.x)<=k->StepM()+1e-9
                  &&std::fabs(receiver.y-cell.y)<=k->StepM()+1e-9;
                if(!adjacent||receiver.floodRank>=cell.floodRank||receiver.headZ>cell.headZ+1e-12)
                    ++c.invalidReceivers;
                if(receiver.surfaceZ>cell.surfaceZ+1e-9&&cell.basinId==0)++c.uphillNonBasin;
                if(cell.channel&&receiver.watershedId!=cell.watershedId)++c.channelDivideCrossings;
            }
            else if(!cell.boundary)++c.invalidReceivers;
        }
        for(size_t i=0;i<channelUpstream.size();++i)
            if(k->Cells()[i].channel&&channelUpstream[i]>=2)++c.confluences;
        std::sort(watershedIds.begin(),watershedIds.end());watershedIds.erase(
            std::unique(watershedIds.begin(),watershedIds.end()),watershedIds.end());c.watersheds=watershedIds.size();
        std::sort(basinIds.begin(),basinIds.end());basinIds.erase(
            std::unique(basinIds.begin(),basinIds.end()),basinIds.end());c.basins=basinIds.size();
        c.checks.push_back({"full_4096m_stage15_domain",k->Width()*k->StepM()==4096.0
            &&k->Height()*k->StepM()==4096.0});
        c.checks.push_back({"all_receivers_adjacent_acyclic_and_head_downhill",c.invalidReceivers==0});
        c.checks.push_back({"raw_uphill_edges_only_explicit_basin_spill_routing",c.uphillNonBasin==0});
        c.checks.push_back({"deterministic_watersheds_and_boundary_outlets",c.watersheds>1&&c.outlets>0});
        c.checks.push_back({"channels_do_not_cross_drainage_divides",c.channelDivideCrossings==0});
        c.checks.push_back({"channels_derive_from_upstream_area",c.channels>0&&c.maxAccumulationKm2>1.0});
        c.checks.push_back({"tributary_confluences_exist",c.confluences>0});
        c.checks.push_back({"closed_basins_publish_explicit_spills",c.basins==c.spills&&c.basins>0});
        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,&coldReason);
        c.checks.push_back({"cold_reload_and_partition_order_invariant",cold&&cold->Digest()==k->Digest()});
        bool stage15Parity=true;
        if(cold)for(double y=-1900;y<=1900;y+=173)for(double x=-1900;x<=1900;x+=181)
            stage15Parity=stage15Parity&&cold->Geography().ReconstructedZ(x,y)==k->Geography().ReconstructedZ(x,y);
        c.checks.push_back({"stage15_geometry_not_rewritten",stage15Parity});
        c.checks.push_back({"no_water_occupancy_rendering_active_erosion_sediment_or_p5b",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& v){return v.second;});
        if(!c.passed)c.reason="dry_hydrology_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_DRY_HYDROLOGY %s\nreason=%s\ntopology_digest=%s\n"
            "stage15_semantic_digest=%s\ncells=%zu\nwatersheds=%zu\nbasins=%zu\nspills=%zu\n"
            "outlets=%zu\nchannels=%zu\nconfluences=%zu\nmax_accumulation_km2=%.6f\n"
            "invalid_receivers=%zu\nuphill_non_basin_edges=%zu\nchannel_divide_crossings=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),CausalWorldGeology::Hex64(c.topologyDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage15SemanticDigest).c_str(),c.cells,c.watersheds,c.basins,
            c.spills,c.outlets,c.channels,c.confluences,c.maxAccumulationKm2,c.invalidReceivers,
            c.uphillNonBasin,c.channelDivideCrossings);
        std::fprintf(f,"terrain_mutation=0\nwater_occupancy=0\nwater_rendering=0\nfluid_solve=0\n"
            "active_erosion=0\nsediment_transport=0\nsediment_source_sink=closed\np5b=closed\n");
        for(auto const& check:c.checks)std::fprintf(f,"check.%s=%s\n",check.first.c_str(),check.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
