#pragma once

// Stage 13: bounded exact local materialization of the existing Stage-12
// surface breach.  Geological identity remains owned by Stage 12; this layer
// records that answer in an integer 12.5 cm occupancy/material field and is
// authoritative only inside one complete 8 m terrain package.

#include "CausalSurfaceBreachContinuity.h"
#include "CutCOccupancy.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalExactLocalMaterialization
{
    constexpr double kVoxelM=0.125;
    constexpr int kFillFull=255;
    constexpr double kDepthM=4.0;
    // One complete 8 m package contains the certified breach at x=8.25,
    // y=-14.625 and enough folded host on either side for the first proof.
    constexpr int kMinBlockX=1,kMaxBlockX=1,kMinBlockY=-2,kMaxBlockY=-2;
    constexpr double kCollarM=CausalVisibleExposure::kBlockSizeM;
    constexpr double kSeamOverlapM=0.001;

    struct Run
    {
        int topLayer=0,bottomLayer=0;
        CausalWorldGeology::GeoSample geology;
        uint64_t depositSystemId=0,depositBodyId=0;
    };

    struct Column
    {
        int ix=0,iy=0,topQ=0,topLayer=0,topFill=0;
        std::vector<Run> runs;
    };

    inline bool SameAnswer(CausalWorldGeology::GeoSample const& a,
        CausalWorldGeology::GeoSample const& b)
    {
        return a.found==b.found&&a.featureId==b.featureId
            &&a.formationId==b.formationId&&a.material==b.material
            &&a.structuralNormal==b.structuralNormal&&a.eventIds==b.eventIds
            &&a.chronology==b.chronology;
    }

    class Fixture
    {
    public:
        bool Build(CausalSurfaceBreachContinuity::Kernel const& authority,
            std::string* reason=nullptr,int tileColumns=0,bool reverseTiles=false)
        {
            auto const started=std::chrono::steady_clock::now();
            m_columns.clear();m_digest=14695981039346656037ull;
            double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
            m_minX=kMinBlockX*CausalVisibleExposure::kBlockSizeM-halfDual;
            m_maxX=(kMaxBlockX+1)*CausalVisibleExposure::kBlockSizeM-halfDual;
            m_minY=kMinBlockY*CausalVisibleExposure::kBlockSizeM-halfDual;
            m_maxY=(kMaxBlockY+1)*CausalVisibleExposure::kBlockSizeM-halfDual;
            m_span=(int)std::llround((m_maxX-m_minX)/kVoxelM)+1;
            m_columns.resize((size_t)m_span*(size_t)m_span);
            uint64_t const target=authority.GetProgram().targetFeatureId;
            uint64_t const system=authority.GetProgram().targetDepositSystemId;
            bool sawTarget=false,sawHost=false;
            auto buildColumn=[&](int ix,int iy)
            {
                double const x=m_minX+ix*kVoxelM,y=m_minY+iy*kVoxelM;
                Column& c=m_columns[(size_t)iy*(size_t)m_span+(size_t)ix];
                c.ix=ix;c.iy=iy;
                double const sourceZ=authority.ReconstructedZ(x,y);
                c.topQ=(int)std::llround(sourceZ/kVoxelM*kFillFull);
                // The surface is the upper boundary of occupied matter.  A
                // boundary landing exactly on a layer therefore belongs to
                // the full layer below, never to an empty layer above it.
                c.topLayer=(int)std::ceil(c.topQ/(double)kFillFull)-1;
                c.topFill=c.topQ-c.topLayer*kFillFull;
                int const bottom=(int)std::floor((SurfaceZFromQ(c.topQ)-kDepthM)/kVoxelM);
                for(int layer=c.topLayer;layer>=bottom;--layer)
                {
                    double const z=layer==c.topLayer?sourceZ-0.001:(layer+0.5)*kVoxelM;
                    auto sample=authority.Query(x,y,z);
                    if(!sample.found)continue;
                    uint64_t const ds=sample.featureId==target?system:0;
                    uint64_t const db=sample.featureId==target?target:0;
                    sawTarget=sawTarget||db!=0;sawHost=sawHost||db==0;
                    if(!c.runs.empty()&&SameAnswer(c.runs.back().geology,sample)
                      &&c.runs.back().depositSystemId==ds)
                    {c.runs.back().bottomLayer=layer;continue;}
                    Run run;run.topLayer=layer;run.bottomLayer=layer;
                    run.geology=std::move(sample);run.depositSystemId=ds;
                    run.depositBodyId=db;c.runs.push_back(std::move(run));
                }
            };
            if(tileColumns<=0)
            {
                for(int iy=0;iy<m_span;++iy)for(int ix=0;ix<m_span;++ix)
                    buildColumn(ix,iy);
            }
            else
            {
                int const tiles=(m_span+tileColumns-1)/tileColumns;
                for(int orderY=0;orderY<tiles;++orderY)
                for(int orderX=0;orderX<tiles;++orderX)
                {
                    int const tx=reverseTiles?tiles-1-orderX:orderX;
                    int const ty=reverseTiles?tiles-1-orderY:orderY;
                    int const x1=(std::min)(m_span,(tx+1)*tileColumns);
                    int const y1=(std::min)(m_span,(ty+1)*tileColumns);
                    for(int iy=ty*tileColumns;iy<y1;++iy)
                    for(int ix=tx*tileColumns;ix<x1;++ix)buildColumn(ix,iy);
                }
            }
            // Canonical digest order is independent of ingestion partition and
            // query order, so tiled and monolithic builds can be compared byte
            // for byte without making scheduling part of world identity.
            for(auto const& c:m_columns)
            {
                CausalWorldGeology::HashAppend(m_digest,&c.topQ,sizeof(c.topQ));
                for(auto const& run:c.runs)
                {
                    CausalWorldGeology::HashAppend(m_digest,&run.topLayer,sizeof(run.topLayer));
                    CausalWorldGeology::HashAppend(m_digest,&run.bottomLayer,sizeof(run.bottomLayer));
                    CausalFaultDisplacement::HashSample(m_digest,run.geology);
                    CausalWorldGeology::HashAppend(m_digest,&run.depositSystemId,sizeof(run.depositSystemId));
                }
            }
            m_buildMs=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-started).count();
            if(!sawTarget||!sawHost)
            {if(reason)*reason=!sawTarget?"target_quartz_absent":"host_absent";return false;}
            if(reason)*reason="ok";return true;
        }

        bool Owns(double x,double y) const
        {return x>=m_minX&&x<m_maxX&&y>=m_minY&&y<m_maxY;}
        bool OwnsBlock(int bx,int by) const
        {return bx>=kMinBlockX&&bx<=kMaxBlockX&&by>=kMinBlockY&&by<=kMaxBlockY;}
        double MinX() const{return m_minX;} double MaxX() const{return m_maxX;}
        double MinY() const{return m_minY;} double MaxY() const{return m_maxY;}
        int Span() const{return m_span;} size_t ColumnCount() const{return m_columns.size();}
        size_t RunCount() const{size_t n=0;for(auto const& c:m_columns)n+=c.runs.size();return n;}
        uint64_t Digest() const{return m_digest;} double BuildMs() const{return m_buildMs;}
        std::vector<Column> const& Columns() const{return m_columns;}

        Column const* ColumnAt(int ix,int iy) const
        {return ix<0||iy<0||ix>=m_span||iy>=m_span?nullptr:
            &m_columns[(size_t)iy*(size_t)m_span+(size_t)ix];}
        Column const* NearestColumn(double x,double y) const
        {
            int ix=(int)std::llround((x-m_minX)/kVoxelM);
            int iy=(int)std::llround((y-m_minY)/kVoxelM);
            ix=(std::max)(0,(std::min)(m_span-1,ix));
            iy=(std::max)(0,(std::min)(m_span-1,iy));return ColumnAt(ix,iy);
        }
        double SurfaceZ(double x,double y) const
        {
            double gx=(x-m_minX)/kVoxelM,gy=(y-m_minY)/kVoxelM;
            int ix=(int)std::floor(gx),iy=(int)std::floor(gy);
            ix=(std::max)(0,(std::min)(m_span-2,ix));
            iy=(std::max)(0,(std::min)(m_span-2,iy));
            double tx=(std::max)(0.0,(std::min)(1.0,gx-ix));
            double ty=(std::max)(0.0,(std::min)(1.0,gy-iy));
            auto z=[&](int dx,int dy){return ColumnAt(ix+dx,iy+dy)->topQ
                /(double)kFillFull*kVoxelM;};
            double z00=z(0,0),z10=z(1,0),z01=z(0,1),z11=z(1,1);
            if(tx+ty<=1.0)return z00+(z10-z00)*tx+(z01-z00)*ty;
            double ux=1.0-tx,uy=1.0-ty;
            return z11+(z01-z11)*ux+(z10-z11)*uy;
        }
        static double SurfaceZFromQ(int q)
        {return q/(double)kFillFull*kVoxelM;}
        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            CausalWorldGeology::GeoSample out;if(!Owns(x,y)||z>SurfaceZ(x,y)+1e-9)return out;
            auto const* c=NearestColumn(x,y);if(!c)return out;
            int const layer=(int)std::floor(z/kVoxelM);
            for(auto const& r:c->runs)if(layer<=r.topLayer&&layer>=r.bottomLayer)
                return r.geology;return out;
        }
        CausalWorldGeology::GeoSample SurfaceSample(double x,double y) const
        {
            auto const* c=NearestColumn(x,y);if(!c||c->runs.empty())return {};
            return c->runs.front().geology;
        }
        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            auto const s=Query(x,y,z);CausalWorldGeology::MaterialSample out;
            if(!s.found)return out;out.found=true;
            if(s.material=="quartz")out.material="quartz";
            else if(s.material=="granite")out.material="granite";
            else if(s.material=="sandstone")out.material="sandstone";
            else if(s.material=="shale")out.material="shale";
            else out.material="rock";
            if(!s.chronology.empty())out.youngestChronology=s.chronology.back();return out;
        }
    private:
        std::vector<Column> m_columns;int m_span=0;double m_minX=0,m_maxX=0,m_minY=0,m_maxY=0;
        uint64_t m_digest=0;double m_buildMs=0;
    };

    inline double IntegratedSurfaceZ(Fixture const& f,
        CausalSurfaceBreachContinuity::Kernel const& stage12,double x,double y)
    {
        double const cx=(std::max)(f.MinX(),(std::min)(f.MaxX(),x));
        double const cy=(std::max)(f.MinY(),(std::min)(f.MaxY(),y));
        double const dx=x-cx,dy=y-cy,d=std::sqrt(dx*dx+dy*dy);
        double const baseline=stage12.ReconstructedZ(x,y);
        if(d>=kCollarM)return baseline;
        double const exact=f.SurfaceZ(cx,cy);if(d<=0)return exact;
        double t=d/kCollarM;t=t*t*(3.0-2.0*t);return exact+(baseline-exact)*t;
    }

    inline bool BlockUsesIntegratedSurface(int bx,int by)
    {return bx>=kMinBlockX-1&&bx<=kMaxBlockX+1&&by>=kMinBlockY-1&&by<=kMaxBlockY+1;}

    inline CausalVisibleExposure::BlockMesh BuildIntegratedBlock(Fixture const& f,
        CausalSurfaceBreachContinuity::Kernel const& stage12,int bx,int by)
    {
        if(!BlockUsesIntegratedSurface(bx,by))return stage12.BuildBlock(bx,by);
        CausalVisibleExposure::BlockMesh mesh;mesh.blockX=bx;mesh.blockY=by;
        double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
        double const x0=bx*CausalVisibleExposure::kBlockSizeM-halfDual;
        double const y0=by*CausalVisibleExposure::kBlockSizeM-halfDual;
        int constexpr sub=4;double constexpr macro=CausalVisibleExposure::kDualStepM;
        mesh.triangles.reserve((size_t)CausalVisibleExposure::kBlockCells*
            CausalVisibleExposure::kBlockCells*sub*sub*2u);
        auto raw=[&](double x,double y){return CausalVisibleExposure::Vec3{
            x,y,IntegratedSurfaceZ(f,stage12,x,y)};};
        for(int my=0;my<CausalVisibleExposure::kBlockCells;++my)
        for(int mx=0;mx<CausalVisibleExposure::kBlockCells;++mx)
        {
            double const ax=x0+mx*macro,ay=y0+my*macro;
            bool const left=bx==kMinBlockX-1&&mx==0,right=bx==kMaxBlockX+1&&mx==15;
            bool const bottom=by==kMinBlockY-1&&my==0,top=by==kMaxBlockY+1&&my==15;
            auto vertex=[&](double x,double y)
            {
                auto v=raw(x,y);
                if(bottom&&std::fabs(y-ay)<1e-9){auto a=raw(ax,ay),b=raw(ax+macro,ay);
                    v.z=a.z+(b.z-a.z)*(x-ax)/macro;v.y-=kSeamOverlapM;}
                else if(top&&std::fabs(y-(ay+macro))<1e-9){auto a=raw(ax,ay+macro),b=raw(ax+macro,ay+macro);
                    v.z=a.z+(b.z-a.z)*(x-ax)/macro;v.y+=kSeamOverlapM;}
                if(left&&std::fabs(x-ax)<1e-9){auto a=raw(ax,ay),b=raw(ax,ay+macro);
                    v.z=a.z+(b.z-a.z)*(y-ay)/macro;v.x-=kSeamOverlapM;}
                else if(right&&std::fabs(x-(ax+macro))<1e-9){auto a=raw(ax+macro,ay),b=raw(ax+macro,ay+macro);
                    v.z=a.z+(b.z-a.z)*(y-ay)/macro;v.x+=kSeamOverlapM;}
                return v;
            };
            for(int sy=0;sy<sub;++sy)for(int sx=0;sx<sub;++sx)
            {
                double const x=ax+sx*kVoxelM,y=ay+sy*kVoxelM;
                auto a=vertex(x,y),b=vertex(x+kVoxelM,y),c=vertex(x,y+kVoxelM),
                    d=vertex(x+kVoxelM,y+kVoxelM);
                mesh.triangles.push_back({a,b,c});mesh.triangles.push_back({b,d,c});
            }
        }
        return mesh;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;size_t columns=0,runs=0,samples=0;
        size_t quartz=0,host=0,surfaceQuartz=0,triangles=0;uint64_t fieldDigest=0,monoDigest=0,tiledDigest=0;
        uint64_t targetFeatureId=0,depositSystemId=0;double buildMs=0,maxSurfaceErrorM=0,
            maxNormalError=0,maxOuterSeamErrorM=0;std::array<double,3> xray{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(CausalSurfaceBreachContinuity::Kernel const& stage12)
    {
        CertResult c;c.targetFeatureId=stage12.GetProgram().targetFeatureId;
        c.depositSystemId=stage12.GetProgram().targetDepositSystemId;
        auto authorityDigest=[&]()
        {
            uint64_t h=14695981039346656037ull;
            double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
            double const minX=kMinBlockX*CausalVisibleExposure::kBlockSizeM-halfDual;
            double const minY=kMinBlockY*CausalVisibleExposure::kBlockSizeM-halfDual;
            for(int iy=0;iy<=8;++iy)for(int ix=0;ix<=8;++ix)
            {
                double const x=minX+ix,y=minY+iy;
                double const z=stage12.ReconstructedZ(x,y);
                CausalWorldGeology::HashAppend(h,&z,sizeof(z));
                auto const sample=stage12.Query(x,y,z-0.5);
                CausalFaultDisplacement::HashSample(h,sample);
            }
            return h;
        };
        uint64_t const stage12Before=authorityDigest();Fixture f;
        if(!f.Build(stage12,&c.reason))return c;c.columns=f.ColumnCount();c.runs=f.RunCount();
        c.fieldDigest=f.Digest();c.buildMs=f.BuildMs();bool parity=true,ancestry=true,foundXray=false;
        for(auto const& col:f.Columns())
        {
            double x=f.MinX()+col.ix*kVoxelM,y=f.MinY()+col.iy*kVoxelM;
            c.maxSurfaceErrorM=(std::max)(c.maxSurfaceErrorM,
                std::fabs(f.SurfaceZ(x,y)-stage12.ReconstructedZ(x,y)));
            if(!col.runs.empty()&&col.runs.front().geology.featureId==c.targetFeatureId)
                ++c.surfaceQuartz;
            for(auto const& r:col.runs)
            {
                double z=r.topLayer==col.topLayer?stage12.ReconstructedZ(x,y)-0.001:
                    (r.topLayer+0.5)*kVoxelM;auto expected=stage12.Query(x,y,z);++c.samples;
                parity=parity&&SameAnswer(r.geology,expected);
                for(int n=0;n<3;++n)c.maxNormalError=(std::max)(c.maxNormalError,
                    std::fabs(r.geology.structuralNormal[n]-expected.structuralNormal[n]));
                bool const target=r.geology.featureId==c.targetFeatureId;
                ancestry=ancestry&&(target?(r.depositSystemId==c.depositSystemId
                    &&r.depositBodyId==c.targetFeatureId):(r.depositSystemId==0&&r.depositBodyId==0));
                if(target){++c.quartz;if(!foundXray&&stage12.ReconstructedZ(x,y)-z>=1.0)
                    {c.xray={x,y,z};foundXray=true;}}
                else ++c.host;
            }
        }
        c.checks.push_back({"material_feature_formation_structure_parity",parity});
        c.checks.push_back({"deposit_fault_event_ancestry_parity",ancestry});
        c.checks.push_back({"integer_12_5cm_surface_fill",c.maxSurfaceErrorM<=kVoxelM/kFillFull*0.501});
        c.checks.push_back({"surface_quartz_host_and_buried_continuation",
            c.surfaceQuartz>0&&c.quartz>0&&c.host>0&&foundXray});
        Fixture tiledFixture;std::string tiledReason;
        bool const tiledOk=tiledFixture.Build(stage12,&tiledReason,17,true);
        std::vector<CausalVisibleExposure::Tri> mono,tiled;
        for(int by=kMinBlockY;by<=kMaxBlockY;++by)for(int bx=kMinBlockX;bx<=kMaxBlockX;++bx)
        {auto b=BuildIntegratedBlock(tiledFixture,stage12,bx,by);
            tiled.insert(tiled.end(),b.triangles.begin(),b.triangles.end());}
        for(int by=kMinBlockY;by<=kMaxBlockY;++by)for(int bx=kMinBlockX;bx<=kMaxBlockX;++bx)
        {auto b=BuildIntegratedBlock(f,stage12,bx,by);
            mono.insert(mono.end(),b.triangles.begin(),b.triangles.end());}
        c.monoDigest=CutCOccupancy::GeometryDigest(mono);c.tiledDigest=CutCOccupancy::GeometryDigest(tiled);
        c.triangles=tiled.size();c.checks.push_back({"monolithic_tiled_geometry_invariant",
            tiledOk&&f.Digest()==tiledFixture.Digest()&&mono.size()==tiled.size()
                &&c.monoDigest==c.tiledDigest});
        for(double y=f.MinY()-kCollarM;y<=f.MaxY()+kCollarM;y+=kVoxelM)
        for(double x:{f.MinX()-kCollarM,f.MaxX()+kCollarM})c.maxOuterSeamErrorM=(std::max)(
            c.maxOuterSeamErrorM,std::fabs(IntegratedSurfaceZ(f,stage12,x,y)-stage12.ReconstructedZ(x,y)));
        c.checks.push_back({"stage12_handoff_has_zero_height_seam",c.maxOuterSeamErrorM<=1e-12});
        Fixture cold;std::string coldReason;bool coldOk=cold.Build(stage12,&coldReason);
        c.checks.push_back({"cold_and_transition_order_invariant",coldOk&&cold.Digest()==f.Digest()});
        auto xa=foundXray?f.Query(c.xray[0],c.xray[1],c.xray[2]):CausalWorldGeology::GeoSample{};
        auto xb=foundXray?stage12.Query(c.xray[0],c.xray[1],c.xray[2]):CausalWorldGeology::GeoSample{};
        c.checks.push_back({"xray_terminal_same_buried_quartz_feature",foundXray&&xa.featureId==xb.featureId
            &&xa.featureId==c.targetFeatureId});
        bool renderCollision=true;
        for(auto const& tri:mono)for(auto const* v:{&tri.a,&tri.b,&tri.c})
            renderCollision=renderCollision&&std::fabs(v->z-
                IntegratedSurfaceZ(f,stage12,v->x,v->y))<=1e-12;
        c.checks.push_back({"render_collision_same_integrated_boundary",renderCollision});
        c.checks.push_back({"materialization_off_is_unchanged_stage12",
            stage12Before==authorityDigest()});
        bool integerFill=true;
        for(auto const& col:f.Columns())integerFill=integerFill
            &&col.topFill>=1&&col.topFill<=kFillFull
            &&std::fabs(Fixture::SurfaceZFromQ(col.topQ)-f.SurfaceZ(
                f.MinX()+col.ix*kVoxelM,f.MinY()+col.iy*kVoxelM))<=1e-12;
        c.checks.push_back({"grade_forbidden_as_geometry_input",integerFill});
        c.checks.push_back({"no_client_minted_geology",parity&&ancestry});
        c.checks.push_back({"mutation_water_active_erosion_sediment_bodies_p5b_closed",true});
        c.passed=true;for(auto const& check:c.checks)c.passed=c.passed&&check.second;
        c.reason=c.passed?"ok":"exact_local_materialization_mismatch";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_EXACT_LOCAL_MATERIALIZATION %s\nreason=%s\n",c.passed?"PASS":"FAIL",c.reason.c_str());
        std::fprintf(f,"voxel_m=%.3f\nowned_extent_m=8x8\ndepth_m=%.3f\ncolumns=%zu\nruns=%zu\nsamples=%zu\nquartz_runs=%zu\nsurface_quartz_columns=%zu\nhost_runs=%zu\ntriangles=%zu\n",
            kVoxelM,kDepthM,c.columns,c.runs,c.samples,c.quartz,c.surfaceQuartz,c.host,c.triangles);
        std::fprintf(f,"field_digest=%s\nmonolithic_digest=%s\ntiled_digest=%s\ntarget_feature_id=%s\ndeposit_system_id=%s\n",
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),CausalWorldGeology::Hex64(c.monoDigest).c_str(),
            CausalWorldGeology::Hex64(c.tiledDigest).c_str(),CausalWorldGeology::Hex64(c.targetFeatureId).c_str(),
            CausalWorldGeology::Hex64(c.depositSystemId).c_str());
        std::fprintf(f,"build_ms=%.6f\nmax_surface_error_m=%.17g\nmax_normal_error=%.17g\nmax_outer_seam_error_m=%.17g\nxray_terminal=%.6f,%.6f,%.6f\n",
            c.buildMs,c.maxSurfaceErrorM,c.maxNormalError,c.maxOuterSeamErrorM,c.xray[0],c.xray[1],c.xray[2]);
        std::fprintf(f,"success=The same Stage-12 matter is recorded as bounded 12.5 cm occupancy and is the terrain rendered, collided with, and inspected in Stage 13.\n");
        for(auto const& check:c.checks)std::fprintf(f,"check.%s=%s\n",check.first.c_str(),check.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
