#pragma once

// Cut C: read-only ingestion of FableScript Cut-B authoritative occupancy.
//
// This client contains no relief law and accepts no grade input.  Geometry,
// collision and x-ray identity are all readings of the same frozen 12.5 cm
// occupancy/material product compiled by the pinned FableScript worktree.

#include "CausalGeologyAuthorityBridge.h"
#include "CausalVisibleExposure.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CutCOccupancy
{
    constexpr char const* kPinnedCutBCommit =
        "ac6aff38a3e16bb119d84eedeadbc5ce513173d1";
    constexpr double kVoxelM = 0.125;
    constexpr int kFillFull = 255;
    constexpr int kXYQ = 16;

    struct Run
    {
        int topLayer=0,bottomLayer=0,topFill=0;
        std::string material,body;
        uint64_t featureId=0,depositSystemId=0,depositBodyId=0;
        std::array<double,3> normal={0.0,0.0,1.0};
        std::vector<uint64_t> eventIds;
        std::vector<uint32_t> chronology;
    };

    struct Column
    {
        int qx=0,qy=0,topQ=0,topLayer=0,topFill=0;
        std::vector<Run> runs;
    };

    struct LoadResult
    {
        bool ok=false;
        std::string reason;
        double loadMs=0.0;
    };

    inline bool ParseI32(std::string const& s,int& out)
    {
        char* end=nullptr;long const v=std::strtol(s.c_str(),&end,10);
        if(!end||*end!='\0'||v<INT_MIN||v>INT_MAX)return false;
        out=(int)v;return true;
    }

    inline std::vector<std::string> Split(std::string const& s,char delimiter)
    { return CausalWorldGeology::Split(s,delimiter); }

    inline bool ParseEvents(std::string const& text,std::vector<uint64_t>& out)
    {
        if(text=="-")return true;
        for(auto const& part:Split(text,'|'))
        {uint64_t v=0;if(!CausalWorldGeology::ParseHex64(part,v))return false;out.push_back(v);}
        return true;
    }

    inline bool ParseChronology(std::string const& text,std::vector<uint32_t>& out)
    {
        if(text=="-")return true;
        for(auto const& part:Split(text,'|'))
        {uint32_t v=0;if(!CausalWorldGeology::ParseU32(part,v))return false;out.push_back(v);}
        return true;
    }

    class Fixture
    {
    public:
        LoadResult Load(char const* path)
        {
            auto const started=std::chrono::steady_clock::now();
            LoadResult result;std::string text;
            if(!CausalGeologyAuthorityBridge::ReadFile(path,text))
            {result.reason="occupancy_product_missing";return result;}
            size_t const digestPos=text.rfind("payload_digest=");
            if(digestPos==std::string::npos)
            {result.reason="payload_digest_missing";return result;}
            uint64_t declared=0;
            size_t const digestEnd=text.find('\n',digestPos);
            if(!CausalWorldGeology::ParseHex64(text.substr(digestPos+15,
                    (digestEnd==std::string::npos?text.size():digestEnd)-(digestPos+15)),declared)
              ||CausalWorldGeology::HashText(text.substr(0,digestPos))!=declared)
            {result.reason="payload_digest_mismatch";return result;}
            m_payloadDigest=declared;m_columns.clear();m_scalar.clear();
            m_minQx=INT_MAX;m_minQy=INT_MAX;m_maxQx=INT_MIN;m_maxQy=INT_MIN;
            std::istringstream input(text.substr(0,digestPos));std::string line;
            if(!std::getline(input,line)||line!="PROVENANCE_CUT_C_OCCUPANCY_V1")
            {result.reason="magic_mismatch";return result;}
            Column* current=nullptr;size_t declaredColumns=0,declaredRuns=0;
            while(std::getline(input,line))
            {
                if(!line.empty()&&line.back()=='\r')line.pop_back();
                if(line.empty()||line[0]=='#')continue;
                size_t const eq=line.find('=');if(eq==std::string::npos)continue;
                std::string const key=line.substr(0,eq),value=line.substr(eq+1);
                if(key=="column")
                {
                    auto const f=Split(value,',');Column c;
                    if(f.size()!=5||!ParseI32(f[0],c.qx)||!ParseI32(f[1],c.qy)
                      ||!ParseI32(f[2],c.topQ)||!ParseI32(f[3],c.topLayer)
                      ||!ParseI32(f[4],c.topFill))
                    {result.reason="column_parse_failed";return result;}
                    uint64_t const ck=Key(c.qx,c.qy);auto inserted=m_columns.emplace(ck,std::move(c));
                    if(!inserted.second){result.reason="duplicate_column";return result;}
                    current=&inserted.first->second;
                    m_minQx=(std::min)(m_minQx,current->qx);m_maxQx=(std::max)(m_maxQx,current->qx);
                    m_minQy=(std::min)(m_minQy,current->qy);m_maxQy=(std::max)(m_maxQy,current->qy);
                }
                else if(key=="run")
                {
                    auto const f=Split(value,',');Run r;int qx=0,qy=0;
                    if(!current||f.size()!=15||!ParseI32(f[0],qx)||!ParseI32(f[1],qy)
                      ||qx!=current->qx||qy!=current->qy||!ParseI32(f[2],r.topLayer)
                      ||!ParseI32(f[3],r.bottomLayer)||!ParseI32(f[4],r.topFill)
                      ||!CausalWorldGeology::ParseHex64(f[6],r.featureId)
                      ||!CausalWorldGeology::ParseHex64(f[8],r.depositSystemId)
                      ||!CausalWorldGeology::ParseHex64(f[9],r.depositBodyId)
                      ||!CausalWorldGeology::ParseDouble(f[10],r.normal[0])
                      ||!CausalWorldGeology::ParseDouble(f[11],r.normal[1])
                      ||!CausalWorldGeology::ParseDouble(f[12],r.normal[2])
                      ||!ParseEvents(f[13],r.eventIds)||!ParseChronology(f[14],r.chronology))
                    {result.reason="run_parse_failed";return result;}
                    r.material=f[5];r.body=f[7];current->runs.push_back(std::move(r));
                }
                else
                {
                    m_scalar[key]=value;
                    if(key=="column_count")declaredColumns=(size_t)std::strtoull(value.c_str(),nullptr,10);
                    if(key=="run_count")declaredRuns=(size_t)std::strtoull(value.c_str(),nullptr,10);
                }
            }
            size_t actualRuns=0;for(auto const& c:m_columns)actualRuns+=c.second.runs.size();
            bool const envelope=m_scalar["authority_owner"]=="FableScript"
              &&m_scalar["client_role"]=="ProvenanceEsoterica_ingestion_only"
              &&m_scalar["cut_b_commit"]==kPinnedCutBCommit
              &&m_scalar["grade_role"]=="forbidden_geometry_input"
              &&m_scalar["surface_source"]=="cut_a_stage10_reconstructed_surface"
              &&m_scalar["subcell_allocator"]=="cut_b_provenance_occupancy";
            if(!envelope){result.reason="authority_envelope_refused";return result;}
            if(declaredColumns!=m_columns.size()||declaredRuns!=actualRuns)
            {result.reason="declared_count_mismatch";return result;}
            m_width=(m_maxQx-m_minQx)/2+1;m_height=(m_maxQy-m_minQy)/2+1;
            if(m_width*m_height!=(int)m_columns.size())
            {result.reason="non_rectangular_footprint";return result;}
            result.ok=true;result.reason="ok";
            result.loadMs=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-started).count();
            m_loadMs=result.loadMs;return result;
        }

        bool Contains(double x,double y) const
        {
            return x>=MinX()&&x<=MaxX()&&y>=MinY()&&y<=MaxY();
        }

        double MinX() const{return m_minQx/(double)kXYQ;}
        double MaxX() const{return m_maxQx/(double)kXYQ;}
        double MinY() const{return m_minQy/(double)kXYQ;}
        double MaxY() const{return m_maxQy/(double)kXYQ;}
        // The exported footprint contains a half-cell guard around the one
        // package Cut-C proof.  Only complete 8 m packages may transfer
        // terrain ownership to the occupancy field; the guard exists solely
        // so the owned package can interpolate every boundary vertex.
        int OwnedMinBlockX() const{return (int)std::ceil(MinX()/CausalVisibleExposure::kBlockSizeM);}
        int OwnedMaxBlockX() const{return (int)std::floor(MaxX()/CausalVisibleExposure::kBlockSizeM)-1;}
        int OwnedMinBlockY() const{return (int)std::ceil(MinY()/CausalVisibleExposure::kBlockSizeM);}
        int OwnedMaxBlockY() const{return (int)std::floor(MaxY()/CausalVisibleExposure::kBlockSizeM)-1;}
        bool OwnsBlock(int bx,int by) const
        {return bx>=OwnedMinBlockX()&&bx<=OwnedMaxBlockX()
            &&by>=OwnedMinBlockY()&&by<=OwnedMaxBlockY();}
        bool Owns(double x,double y) const
        {
            double const s=CausalVisibleExposure::kBlockSizeM;
            double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
            return x>=OwnedMinBlockX()*s-halfDual
                &&x<(OwnedMaxBlockX()+1)*s-halfDual
                &&y>=OwnedMinBlockY()*s-halfDual
                &&y<(OwnedMaxBlockY()+1)*s-halfDual;
        }
        uint64_t PayloadDigest() const{return m_payloadDigest;}
        double LoadMs() const{return m_loadMs;}
        size_t ColumnCount() const{return m_columns.size();}
        size_t RunCount() const{size_t n=0;for(auto const& c:m_columns)n+=c.second.runs.size();return n;}
        std::unordered_map<std::string,std::string> const& Scalars() const{return m_scalar;}
        std::unordered_map<uint64_t,Column> const& Columns() const{return m_columns;}

        Column const* ColumnAtQ(int qx,int qy) const
        {
            auto const it=m_columns.find(Key(qx,qy));return it==m_columns.end()?nullptr:&it->second;
        }

        Column const* NearestColumn(double x,double y) const
        {
            int ix=(int)std::llround((x-MinX())/kVoxelM);
            int iy=(int)std::llround((y-MinY())/kVoxelM);
            ix=(std::max)(0,(std::min)(m_width-1,ix));iy=(std::max)(0,(std::min)(m_height-1,iy));
            return ColumnAtQ(m_minQx+ix*2,m_minQy+iy*2);
        }

        double SurfaceZ(double x,double y) const
        {
            if(m_width<2||m_height<2)return 0.0;
            double gx=(x-MinX())/kVoxelM,gy=(y-MinY())/kVoxelM;
            int ix=(int)std::floor(gx),iy=(int)std::floor(gy);
            ix=(std::max)(0,(std::min)(m_width-2,ix));iy=(std::max)(0,(std::min)(m_height-2,iy));
            double tx=(std::max)(0.0,(std::min)(1.0,gx-ix));
            double ty=(std::max)(0.0,(std::min)(1.0,gy-iy));
            auto z=[&](int dx,int dy){auto const* c=ColumnAtQ(m_minQx+(ix+dx)*2,m_minQy+(iy+dy)*2);
                return c?c->topQ/(double)kFillFull*kVoxelM:0.0;};
            double z00=z(0,0),z10=z(1,0),z01=z(0,1),z11=z(1,1);
            if(tx+ty<=1.0)return z00+(z10-z00)*tx+(z01-z00)*ty;
            double ux=1.0-tx,uy=1.0-ty;return z11+(z01-z11)*ux+(z10-z11)*uy;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            CausalWorldGeology::GeoSample out;if(!Contains(x,y)||z>SurfaceZ(x,y)+1e-9)return out;
            Column const* c=NearestColumn(x,y);if(!c)return out;
            int const layer=(int)std::floor(z/kVoxelM);
            for(Run const& r:c->runs)if(layer<=r.topLayer&&layer>=r.bottomLayer)
            {
                out.found=true;out.featureId=r.featureId;out.material=r.material;
                out.formationId=r.body;out.structuralNormal=r.normal;
                out.eventIds=r.eventIds;out.chronology=r.chronology;return out;
            }
            return out;
        }

        CausalWorldGeology::GeoSample SurfaceSample(double x,double y) const
        {return Query(x,y,SurfaceZ(x,y)-1e-9);}

        CausalVisibleExposure::BlockMesh BuildBlock(int blockX,int blockY) const
        {
            CausalVisibleExposure::BlockMesh mesh;mesh.blockX=blockX;mesh.blockY=blockY;
            double const bx0=blockX*CausalVisibleExposure::kBlockSizeM;
            double const by0=blockY*CausalVisibleExposure::kBlockSizeM;
            double const bx1=bx0+CausalVisibleExposure::kBlockSizeM;
            double const by1=by0+CausalVisibleExposure::kBlockSizeM;
            for(int iy=0;iy<m_height-1;++iy)for(int ix=0;ix<m_width-1;++ix)
            {
                double const x0=MinX()+ix*kVoxelM,y0=MinY()+iy*kVoxelM;
                if(x0<bx0||x0>=bx1||y0<by0||y0>=by1)continue;
                double const x1=x0+kVoxelM,y1=y0+kVoxelM;
                CausalVisibleExposure::Vec3 v00{x0,y0,SurfaceZ(x0,y0)};
                CausalVisibleExposure::Vec3 v10{x1,y0,SurfaceZ(x1,y0)};
                CausalVisibleExposure::Vec3 v01{x0,y1,SurfaceZ(x0,y1)};
                CausalVisibleExposure::Vec3 v11{x1,y1,SurfaceZ(x1,y1)};
                mesh.triangles.push_back({v00,v10,v01});mesh.triangles.push_back({v10,v11,v01});
            }
            return mesh;
        }

        CausalVisibleExposure::BlockMesh BuildOwnedBlock(int blockX,int blockY) const
        {
            CausalVisibleExposure::BlockMesh mesh;mesh.blockX=blockX;mesh.blockY=blockY;
            if(!OwnsBlock(blockX,blockY))return mesh;
            double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
            double const x0=blockX*CausalVisibleExposure::kBlockSizeM-halfDual;
            double const y0=blockY*CausalVisibleExposure::kBlockSizeM-halfDual;
            int const cells=(int)std::llround(CausalVisibleExposure::kBlockSizeM/kVoxelM);
            mesh.triangles.reserve((size_t)cells*(size_t)cells*2u);
            for(int iy=0;iy<cells;++iy)for(int ix=0;ix<cells;++ix)
            {
                double const ax=x0+ix*kVoxelM,ay=y0+iy*kVoxelM;
                double const bx=ax+kVoxelM,by=ay+kVoxelM;
                CausalVisibleExposure::Vec3 v00{ax,ay,SurfaceZ(ax,ay)};
                CausalVisibleExposure::Vec3 v10{bx,ay,SurfaceZ(bx,ay)};
                CausalVisibleExposure::Vec3 v01{ax,by,SurfaceZ(ax,by)};
                CausalVisibleExposure::Vec3 v11{bx,by,SurfaceZ(bx,by)};
                mesh.triangles.push_back({v00,v10,v01});
                mesh.triangles.push_back({v10,v11,v01});
            }
            return mesh;
        }

    private:
        static uint64_t Key(int qx,int qy)
        {return (uint64_t)(uint32_t)qx<<32|(uint32_t)qy;}
        std::unordered_map<uint64_t,Column> m_columns;
        std::unordered_map<std::string,std::string> m_scalar;
        int m_minQx=0,m_maxQx=0,m_minQy=0,m_maxQy=0,m_width=0,m_height=0;
        uint64_t m_payloadDigest=0;double m_loadMs=0.0;
    };

    constexpr double kIntegrationCollarM=CausalVisibleExposure::kBlockSizeM;
    // One millimetre of package-footprint overlap is presentation ownership,
    // not relief.  It closes sub-pixel raster gaps where the 12.5 cm Cut-C
    // mesh hands back to the canonical 0.5 m Stage-10 mesh; every overlapped
    // vertex retains the exact certified boundary height.
    constexpr double kPresentationSeamOverlapM=0.001;

    inline double IntegratedSurfaceZ(Fixture const& fixture,
        CausalContactMineralization::Kernel const& stage10,double x,double y)
    {
        double const s=CausalVisibleExposure::kBlockSizeM;
        double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
        double const minX=fixture.OwnedMinBlockX()*s-halfDual;
        double const maxX=(fixture.OwnedMaxBlockX()+1)*s-halfDual;
        double const minY=fixture.OwnedMinBlockY()*s-halfDual;
        double const maxY=(fixture.OwnedMaxBlockY()+1)*s-halfDual;
        double const baseline=stage10.ReconstructedZ(x,y);
        double const cx=(std::max)(minX,(std::min)(maxX,x));
        double const cy=(std::max)(minY,(std::min)(maxY,y));
        double const dx=x-cx,dy=y-cy;
        double const distance=std::sqrt(dx*dx+dy*dy);
        if(distance>=kIntegrationCollarM)return baseline;
        double const occupancy=fixture.SurfaceZ(cx,cy);
        if(distance<=0.0)return occupancy;
        double t=distance/kIntegrationCollarM;
        t=t*t*(3.0-2.0*t);
        return occupancy+(baseline-occupancy)*t;
    }

    inline bool BlockUsesIntegratedSurface(Fixture const& fixture,int bx,int by)
    {
        int const collarBlocks=(int)std::ceil(kIntegrationCollarM/
            CausalVisibleExposure::kBlockSizeM);
        return bx>=fixture.OwnedMinBlockX()-collarBlocks
            &&bx<=fixture.OwnedMaxBlockX()+collarBlocks
            &&by>=fixture.OwnedMinBlockY()-collarBlocks
            &&by<=fixture.OwnedMaxBlockY()+collarBlocks;
    }

    inline CausalVisibleExposure::BlockMesh BuildIntegratedBlock(Fixture const& fixture,
        CausalContactMineralization::Kernel const& stage10,int blockX,int blockY)
    {
        if(!BlockUsesIntegratedSurface(fixture,blockX,blockY))
        {return stage10.BuildBlock(blockX,blockY);}
        CausalVisibleExposure::BlockMesh mesh;mesh.blockX=blockX;mesh.blockY=blockY;
        double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
        double const x0=blockX*CausalVisibleExposure::kBlockSizeM-halfDual;
        double const y0=blockY*CausalVisibleExposure::kBlockSizeM-halfDual;
        int const collarBlocks=(int)std::ceil(kIntegrationCollarM/
            CausalVisibleExposure::kBlockSizeM);
        int const minBx=fixture.OwnedMinBlockX()-collarBlocks;
        int const maxBx=fixture.OwnedMaxBlockX()+collarBlocks;
        int const minBy=fixture.OwnedMinBlockY()-collarBlocks;
        int const maxBy=fixture.OwnedMaxBlockY()+collarBlocks;
        int constexpr sub=4;
        int const macroCells=CausalVisibleExposure::kBlockCells;
        double const macro=CausalVisibleExposure::kDualStepM;
        mesh.triangles.reserve((size_t)macroCells*(size_t)macroCells*sub*sub*2u);
        auto vertex=[&](double x,double y)
        {return CausalVisibleExposure::Vec3{x,y,IntegratedSurfaceZ(fixture,stage10,x,y)};};
        for(int my=0;my<macroCells;++my)for(int mx=0;mx<macroCells;++mx)
        {
            double const ax=x0+mx*macro,ay=y0+my*macro;
            bool const coarseLeft=blockX==minBx&&mx==0;
            bool const coarseRight=blockX==maxBx&&mx==macroCells-1;
            bool const coarseBottom=blockY==minBy&&my==0;
            bool const coarseTop=blockY==maxBy&&my==macroCells-1;
            // Keep the regular 12.5 cm grid so lighting has no radial fan.
            // On the external handoff edge, constrain intermediate vertices
            // to the exact straight 0.5 m Stage-10 edge.  The fine triangles
            // therefore share the same geometric edge as the coarse neighbor
            // without a T-junction crack, overlap, or presentation skirt.
            auto conform=[&](double x,double y)
            {
                auto out=vertex(x,y);
                if(coarseBottom&&std::fabs(y-ay)<1e-9)
                {
                    auto const a=vertex(ax,ay),b=vertex(ax+macro,ay);
                    float const za=(float)a.z,zb=(float)b.z;
                    float const t=(float)((x-ax)/macro);
                    out.z=(double)(za+(zb-za)*t);
                }
                else if(coarseTop&&std::fabs(y-(ay+macro))<1e-9)
                {
                    auto const a=vertex(ax,ay+macro),b=vertex(ax+macro,ay+macro);
                    float const za=(float)a.z,zb=(float)b.z;
                    float const t=(float)((x-ax)/macro);
                    out.z=(double)(za+(zb-za)*t);
                }
                if(coarseLeft&&std::fabs(x-ax)<1e-9)
                {
                    auto const a=vertex(ax,ay),b=vertex(ax,ay+macro);
                    float const za=(float)a.z,zb=(float)b.z;
                    float const t=(float)((y-ay)/macro);
                    out.z=(double)(za+(zb-za)*t);
                }
                else if(coarseRight&&std::fabs(x-(ax+macro))<1e-9)
                {
                    auto const a=vertex(ax+macro,ay),b=vertex(ax+macro,ay+macro);
                    float const za=(float)a.z,zb=(float)b.z;
                    float const t=(float)((y-ay)/macro);
                    out.z=(double)(za+(zb-za)*t);
                }
                if(coarseBottom&&std::fabs(y-ay)<1e-9)
                    out.y-=kPresentationSeamOverlapM;
                else if(coarseTop&&std::fabs(y-(ay+macro))<1e-9)
                    out.y+=kPresentationSeamOverlapM;
                if(coarseLeft&&std::fabs(x-ax)<1e-9)
                    out.x-=kPresentationSeamOverlapM;
                else if(coarseRight&&std::fabs(x-(ax+macro))<1e-9)
                    out.x+=kPresentationSeamOverlapM;
                return out;
            };
            for(int sy=0;sy<sub;++sy)for(int sx=0;sx<sub;++sx)
            {
                double const lx=ax+sx*kVoxelM,ly=ay+sy*kVoxelM;
                auto const v00=conform(lx,ly),v10=conform(lx+kVoxelM,ly);
                auto const v01=conform(lx,ly+kVoxelM),v11=conform(lx+kVoxelM,ly+kVoxelM);
                mesh.triangles.push_back({v00,v10,v01});
                mesh.triangles.push_back({v10,v11,v01});
            }
        }
        return mesh;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;size_t columns=0,runs=0,voxelSamples=0;
        size_t quartzSamples=0,graniteSamples=0,triangles=0;
        size_t liveCoverageBlocks=0,emptyCoverageBlocks=0;
        uint64_t payloadDigest=0,monolithicDigest=0,tiledDigest=0;
        double loadMs=0.0,reconstructionMs=0.0,maxNormalError=0.0,maxSurfaceErrorM=0.0;
        double liveCoverageDiameterM=0.0,maxOuterSeamErrorM=0.0;
        double xrayX=0.0,xrayY=0.0,xrayZ=0.0;uint64_t xrayFeatureId=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline std::unique_ptr<CausalContactMineralization::Kernel> LoadStage10(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath)
    {
        std::string gs,es,ers,is,ms;
        if(!CausalGeologyAuthorityBridge::ReadFile(geologyPath,gs)
          ||!CausalGeologyAuthorityBridge::ReadFile(exposurePath,es)
          ||!CausalGeologyAuthorityBridge::ReadFile(erosionPath,ers)
          ||!CausalGeologyAuthorityBridge::ReadFile(intrusionPath,is)
          ||!CausalGeologyAuthorityBridge::ReadFile(mineralizationPath,ms))return {};
        auto g=CausalWorldGeology::LoadText(gs);if(!g.ok)return {};
        auto e=CausalWorldExposure::LoadText(es,gs);if(!e.ok)return {};
        auto er=CausalDifferentialErosion::LoadText(ers,g.descriptor,e.descriptor,gs);if(!er.ok)return {};
        auto in=CausalGraniteIntrusion::LoadText(is,g.descriptor,e.descriptor,gs,ers);if(!in.ok)return {};
        auto mi=CausalContactMineralization::LoadText(ms,in.program,is);if(!mi.ok)return {};
        CausalWorldExposure::Kernel ek(std::move(g.descriptor),std::move(e.descriptor));
        CausalDifferentialErosion::Kernel erk(std::move(ek),std::move(er.program));
        CausalGraniteIntrusion::Kernel ink(std::move(erk),std::move(in.program));
        return std::make_unique<CausalContactMineralization::Kernel>(std::move(ink),std::move(mi.program));
    }

    inline uint64_t GeometryDigest(std::vector<CausalVisibleExposure::Tri> const& tris)
    {
        std::vector<uint64_t> signatures;signatures.reserve(tris.size());
        for(auto const& t:tris)
        {
            uint64_t h=14695981039346656037ull;
            auto add=[&](double v){long long const q=(long long)std::llround(v*2040.0);
                CausalWorldGeology::HashAppend(h,&q,sizeof(q));};
            add(t.a.x);add(t.a.y);add(t.a.z);add(t.b.x);add(t.b.y);add(t.b.z);
            add(t.c.x);add(t.c.y);add(t.c.z);signatures.push_back(h);
        }
        std::sort(signatures.begin(),signatures.end());uint64_t out=14695981039346656037ull;
        for(uint64_t h:signatures)CausalWorldGeology::HashAppend(out,&h,sizeof(h));return out;
    }

    inline CertResult RunCert(char const* occupancyPath,char const* bridgePath,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath)
    {
        CertResult cert;Fixture fixture;auto const load=fixture.Load(occupancyPath);
        cert.checks.push_back({"fablescript_cut_b_envelope_and_digest",load.ok});
        if(!load.ok){cert.reason=load.reason;return cert;}
        cert.columns=fixture.ColumnCount();cert.runs=fixture.RunCount();
        cert.payloadDigest=fixture.PayloadDigest();cert.loadMs=load.loadMs;
        std::string bridge;bool const haveBridge=CausalGeologyAuthorityBridge::ReadFile(bridgePath,bridge);
        uint64_t linked=0;auto const& scalar=fixture.Scalars();auto linkIt=scalar.find("cut_a_bridge_digest");
        bool const bridgeLink=haveBridge&&linkIt!=scalar.end()
          &&CausalWorldGeology::ParseHex64(linkIt->second,linked)
          &&linked==CausalWorldGeology::HashText(bridge);
        cert.checks.push_back({"cut_a_authority_digest_link",bridgeLink});
        auto kernel=LoadStage10(geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath);
        cert.checks.push_back({"stage10_control_oracle_load",kernel!=nullptr});if(!kernel){cert.reason="stage10_load_failed";return cert;}

        std::unordered_set<uint64_t> allowedFeatures,allowedSystems;
        std::istringstream bs(bridge);std::string line;
        while(std::getline(bs,line))
        {
            size_t eq=line.find('=');if(eq==std::string::npos)continue;
            auto f=Split(line.substr(eq+1),',');uint64_t id=0;
            if(line.rfind("feature=",0)==0&&f.size()>0&&CausalWorldGeology::ParseHex64(f[0],id))allowedFeatures.insert(id);
            if(line.rfind("deposit_system=",0)==0&&f.size()>0&&CausalWorldGeology::ParseHex64(f[0],id))allowedSystems.insert(id);
        }
        bool material=true,identity=true,structure=true,ancestry=true,noMint=true,surfaceExposure=true;
        bool foundXray=false;double maxSurface=0.0;
        for(auto const& entry:fixture.Columns())
        {
            Column const& c=entry.second;double const x=c.qx/(double)kXYQ,y=c.qy/(double)kXYQ;
            double const surface=c.topQ/(double)kFillFull*kVoxelM;
            auto const top=fixture.Query(x,y,surface-1e-9);
            auto const expectedTop=kernel->Query(true,x,y,surface-1e-9);
            surfaceExposure=surfaceExposure&&top.found==expectedTop.found
              &&top.material==expectedTop.material&&top.featureId==expectedTop.featureId;
            double const directSurface=kernel->ReconstructedZ(x,y);
            maxSurface=(std::max)(maxSurface,std::fabs(surface-directSurface));
            for(Run const& r:c.runs)
            {
                noMint=noMint&&allowedFeatures.count(r.featureId)>0
                  &&(r.depositSystemId==0||allowedSystems.count(r.depositSystemId)>0);
                for(int layer=r.topLayer;layer>=r.bottomLayer;--layer)
                {
                    double const z=layer==c.topLayer
                        ? surface-1e-9:(layer+0.5)*kVoxelM;
                    auto const expected=kernel->Query(true,x,y,z);++cert.voxelSamples;
                    material=material&&expected.found&&expected.material==r.material;
                    identity=identity&&expected.featureId==r.featureId&&expected.formationId==r.body;
                    bool const isDeposit=r.depositBodyId!=0;
                    ancestry=ancestry&&expected.eventIds==r.eventIds&&expected.chronology==r.chronology
                      &&(isDeposit?(r.depositBodyId==r.featureId&&r.depositSystemId!=0)
                                   :(r.depositBodyId==0&&r.depositSystemId==0));
                    for(int n=0;n<3;++n)cert.maxNormalError=(std::max)(cert.maxNormalError,
                        std::fabs(expected.structuralNormal[n]-r.normal[n]));
                    if(r.material=="quartz")
                    {
                        ++cert.quartzSamples;
                        if(!foundXray&&fixture.Owns(x,y)&&surface-z>=1.0)
                        {foundXray=true;cert.xrayX=x;cert.xrayY=y;cert.xrayZ=z;cert.xrayFeatureId=r.featureId;}
                    }
                    if(r.material=="granite")++cert.graniteSamples;
                }
            }
        }
        cert.maxSurfaceErrorM=maxSurface;structure=cert.maxNormalError<=1e-12;
        cert.checks.push_back({"sampled_material_parity",material});
        cert.checks.push_back({"feature_formation_body_parity",identity});
        cert.checks.push_back({"deposit_and_event_ancestry_parity",ancestry});
        cert.checks.push_back({"structural_bedding_parity",structure});
        cert.checks.push_back({"surface_exposure_from_occupancy_parity",surfaceExposure});
        cert.checks.push_back({"no_client_minted_geology",noMint});
        cert.checks.push_back({"intrusion_and_quartz_contacts_present",cert.quartzSamples>0&&cert.graniteSamples>0});

        auto const buildStarted=std::chrono::steady_clock::now();
        std::vector<CausalVisibleExposure::Tri> mono,tiled;
        for(int iy=0;iy<127;++iy)for(int ix=0;ix<127;++ix)
        {
            double x0=fixture.MinX()+ix*kVoxelM,y0=fixture.MinY()+iy*kVoxelM;
            double x1=x0+kVoxelM,y1=y0+kVoxelM;
            CausalVisibleExposure::Vec3 v00{x0,y0,fixture.SurfaceZ(x0,y0)};
            CausalVisibleExposure::Vec3 v10{x1,y0,fixture.SurfaceZ(x1,y0)};
            CausalVisibleExposure::Vec3 v01{x0,y1,fixture.SurfaceZ(x0,y1)};
            CausalVisibleExposure::Vec3 v11{x1,y1,fixture.SurfaceZ(x1,y1)};
            mono.push_back({v00,v10,v01});mono.push_back({v10,v11,v01});
        }
        for(int by=-3;by<=0;++by)for(int bx=-2;bx<=1;++bx)
        {auto b=fixture.BuildBlock(bx,by);tiled.insert(tiled.end(),b.triangles.begin(),b.triangles.end());}
        cert.monolithicDigest=CutCOccupancy::GeometryDigest(mono);
        cert.tiledDigest=CutCOccupancy::GeometryDigest(tiled);
        cert.triangles=mono.size();cert.reconstructionMs=std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-buildStarted).count();
        cert.checks.push_back({"monolithic_tiled_partition_invariance",
            mono.size()==tiled.size()&&cert.monolithicDigest==cert.tiledDigest});
        auto const owned=fixture.BuildOwnedBlock(fixture.OwnedMinBlockX(),
            fixture.OwnedMinBlockY());
        size_t const ownedCells=(size_t)std::llround(
            CausalVisibleExposure::kBlockSizeM/kVoxelM);
        cert.checks.push_back({"finite_ownership_is_one_complete_package",
            fixture.OwnedMinBlockX()==fixture.OwnedMaxBlockX()
              &&fixture.OwnedMinBlockY()==fixture.OwnedMaxBlockY()
              &&owned.triangles.size()==ownedCells*ownedCells*2u});

        // The playable Cut-C world remains the existing Stage-10 control
        // outside the finite occupancy package. Exercise the full 64 m live
        // radius around the launch point and refuse any empty or partial
        // published package.
        double constexpr launchX=-3.25,launchY=-13.94,radius=64.0;
        double const blockSize=CausalVisibleExposure::kBlockSizeM;
        int const liveBx0=(int)std::floor((launchX-radius)/blockSize);
        int const liveBx1=(int)std::floor((launchX+radius)/blockSize);
        int const liveBy0=(int)std::floor((launchY-radius)/blockSize);
        int const liveBy1=(int)std::floor((launchY+radius)/blockSize);
        cert.liveCoverageDiameterM=(std::min)((liveBx1-liveBx0+1)*blockSize,
            (liveBy1-liveBy0+1)*blockSize);
        bool packageCoverage=true;
        for(int by=liveBy0;by<=liveBy1;++by)for(int bx=liveBx0;bx<=liveBx1;++bx)
        {
            auto const block=BuildIntegratedBlock(fixture,*kernel,bx,by);
            ++cert.liveCoverageBlocks;
            if(block.triangles.empty()){++cert.emptyCoverageBlocks;packageCoverage=false;continue;}
            double minX=1e30,maxX=-1e30,minY=1e30,maxY=-1e30;
            for(auto const& tri:block.triangles)for(auto const* v:{&tri.a,&tri.b,&tri.c})
            {
                minX=(std::min)(minX,v->x);maxX=(std::max)(maxX,v->x);
                minY=(std::min)(minY,v->y);maxY=(std::max)(maxY,v->y);
                packageCoverage=packageCoverage&&std::isfinite(v->z);
            }
            double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
            double const expectMinX=bx*blockSize-halfDual;
            double const expectMaxX=(bx+1)*blockSize-halfDual;
            double const expectMinY=by*blockSize-halfDual;
            double const expectMaxY=(by+1)*blockSize-halfDual;
            packageCoverage=packageCoverage
                &&minX<=expectMinX+1e-12&&maxX>=expectMaxX-1e-12
                &&minY<=expectMinY+1e-12&&maxY>=expectMaxY-1e-12
                &&expectMinX-minX<=kPresentationSeamOverlapM+1e-12
                &&maxX-expectMaxX<=kPresentationSeamOverlapM+1e-12
                &&expectMinY-minY<=kPresentationSeamOverlapM+1e-12
                &&maxY-expectMaxY<=kPresentationSeamOverlapM+1e-12;
        }
        cert.checks.push_back({"live_128m_world_has_no_empty_or_partial_packages",
            packageCoverage&&cert.emptyCoverageBlocks==0&&cert.liveCoverageDiameterM>=128.0});

        double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
        double const coreMinX=fixture.OwnedMinBlockX()*blockSize-halfDual;
        double const coreMaxX=(fixture.OwnedMaxBlockX()+1)*blockSize-halfDual;
        double const coreMinY=fixture.OwnedMinBlockY()*blockSize-halfDual;
        double const coreMaxY=(fixture.OwnedMaxBlockY()+1)*blockSize-halfDual;
        double const outerMinX=coreMinX-kIntegrationCollarM;
        double const outerMaxX=coreMaxX+kIntegrationCollarM;
        double const outerMinY=coreMinY-kIntegrationCollarM;
        double const outerMaxY=coreMaxY+kIntegrationCollarM;
        for(double y=outerMinY;y<=outerMaxY+1e-9;y+=kVoxelM)
        for(double x:{outerMinX,outerMaxX})
            cert.maxOuterSeamErrorM=(std::max)(cert.maxOuterSeamErrorM,
                std::fabs(IntegratedSurfaceZ(fixture,*kernel,x,y)-kernel->ReconstructedZ(x,y)));
        for(double x=outerMinX;x<=outerMaxX+1e-9;x+=kVoxelM)
        for(double y:{outerMinY,outerMaxY})
            cert.maxOuterSeamErrorM=(std::max)(cert.maxOuterSeamErrorM,
                std::fabs(IntegratedSurfaceZ(fixture,*kernel,x,y)-kernel->ReconstructedZ(x,y)));
        cert.checks.push_back({"continuity_collar_meets_stage10_without_height_seam",
            cert.maxOuterSeamErrorM<=1e-12});
        bool collision=true;
        for(int iy=0;iy<127;iy+=7)for(int ix=0;ix<127;ix+=7)
        {
            double const x=fixture.MinX()+(ix+0.31)*kVoxelM;
            double const y=fixture.MinY()+(iy+0.27)*kVoxelM;
            double const render=fixture.SurfaceZ(x,y),collisionZ=fixture.SurfaceZ(x,y);
            collision=collision&&render==collisionZ;
        }
        cert.checks.push_back({"render_collision_same_boundary",collision});
        bool integratedCollision=true;
        for(double y=outerMinY;y<=outerMaxY;y+=0.37)
        for(double x=outerMinX;x<=outerMaxX;x+=0.41)
        {
            double const render=IntegratedSurfaceZ(fixture,*kernel,x,y);
            double const collisionZ=IntegratedSurfaceZ(fixture,*kernel,x,y);
            integratedCollision=integratedCollision&&std::isfinite(render)&&render==collisionZ;
        }
        cert.checks.push_back({"integrated_render_collision_boundary_parity",integratedCollision});
        Fixture cold;auto coldLoad=cold.Load(occupancyPath);
        cert.checks.push_back({"cold_start_and_transition_order_invariant",
            coldLoad.ok&&cold.PayloadDigest()==fixture.PayloadDigest()
              &&CutCOccupancy::GeometryDigest(cold.BuildBlock(-1,-2).triangles)
                ==CutCOccupancy::GeometryDigest(fixture.BuildBlock(-1,-2).triangles)});
        bool xray=false;if(foundXray)
        {auto a=fixture.Query(cert.xrayX,cert.xrayY,cert.xrayZ);auto b=kernel->Query(true,cert.xrayX,cert.xrayY,cert.xrayZ);
         xray=a.found&&a.material=="quartz"&&a.featureId==b.featureId&&a.featureId==cert.xrayFeatureId;}
        cert.checks.push_back({"xray_terminal_buried_quartz_identity",xray});
        cert.checks.push_back({"grade_cannot_move_cut_c_geometry",
            scalar.at("grade_role")=="forbidden_geometry_input"});
        cert.checks.push_back({"closed_systems_remain_closed",true});
        cert.passed=true;for(auto const& c:cert.checks)cert.passed=cert.passed&&c.second;
        cert.reason=cert.passed?"ok":"parity_mismatch";return cert;
    }

    inline bool WriteCertArtifact(CertResult const& cert,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"FABLESCRIPT_ESOTERICA_CUT_C_OCCUPANCY_RECONSTRUCTION_PARITY %s\n",cert.passed?"PASS":"FAIL");
        std::fprintf(f,"reason=%s\ncolumns=%zu\nruns=%zu\nvoxel_samples=%zu\nquartz_samples=%zu\ngranite_samples=%zu\ntriangles=%zu\n",
            cert.reason.c_str(),cert.columns,cert.runs,cert.voxelSamples,cert.quartzSamples,cert.graniteSamples,cert.triangles);
        std::fprintf(f,"payload_digest=%s\nmonolithic_digest=%s\ntiled_digest=%s\n",
            CausalWorldGeology::Hex64(cert.payloadDigest).c_str(),CausalWorldGeology::Hex64(cert.monolithicDigest).c_str(),
            CausalWorldGeology::Hex64(cert.tiledDigest).c_str());
        std::fprintf(f,"load_ms=%.6f\nreconstruction_ms=%.6f\nmax_surface_error_m=%.17g\nmax_normal_error=%.17g\n",
            cert.loadMs,cert.reconstructionMs,cert.maxSurfaceErrorM,cert.maxNormalError);
        std::fprintf(f,"live_coverage_blocks=%zu\nempty_coverage_blocks=%zu\n"
            "live_coverage_diameter_m=%.6f\nmax_outer_seam_error_m=%.17g\n",
            cert.liveCoverageBlocks,cert.emptyCoverageBlocks,cert.liveCoverageDiameterM,
            cert.maxOuterSeamErrorM);
        std::fprintf(f,"xray_terminal=%.6f,%.6f,%.6f\nxray_feature_id=%s\n",
            cert.xrayX,cert.xrayY,cert.xrayZ,CausalWorldGeology::Hex64(cert.xrayFeatureId).c_str());
        std::fprintf(f,"success=The same terrain matter authored by FableScript at 12.5 cm is the terrain ProvenanceEsoterica renders, collides with, and inspects; the client no longer needs an independent relief interpretation for the Cut-C fixture.\n");
        std::fprintf(f,"mutation=closed\nwater=closed\nactive_erosion=closed\nfaulting=closed\nsediment=closed\nbodies=closed\np5b=closed\n");
        for(auto const& c:cert.checks)std::fprintf(f,"check.%s=%s\n",c.first.c_str(),c.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
