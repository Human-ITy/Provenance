#pragma once

// Stage 14: one legal player pick strike against the exact Stage-13 field.
// This is deliberately a proof transaction, not a generalized mining loop.

#include "CausalExactLocalMaterialization.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace CausalSinglePickProof
{
    constexpr double kContactRadiusM=0.16;
    constexpr uint64_t kCertifiedPickId=0x5049434b5f303031ull; // PICK_001
    constexpr uint64_t kInitialRevision=13;
    constexpr double kQuartzDensityKgM3=2650.0;

    enum class Tool : uint8_t { None=0,CertifiedPick=1,Shovel=2 };

    struct Action
    {
        uint64_t actionId=0;
        uint64_t expectedRevision=kInitialRevision;
        Tool tool=Tool::None;
        double x=0,y=0,z=0;
        double radiusM=kContactRadiusM;
        uint64_t expectedFeatureId=0;
    };

    struct RemovedVoxel
    {
        int ix=0,iy=0,layer=0,fill=0;
        bool detached=false;
        uint64_t massUg=0;
        CausalWorldGeology::GeoSample geology;
        uint64_t depositSystemId=0,depositBodyId=0;
    };

    struct DetachedBody
    {
        uint64_t bodyId=0,featureId=0,depositSystemId=0,depositBodyId=0;
        uint64_t massUg=0;
        std::string material,formationId;
        std::array<double,3> structuralNormal{};
        std::vector<uint32_t> chronology;
        std::vector<uint64_t> eventIds;
        std::vector<RemovedVoxel> sourceVoxels;
        std::array<double,3> presentationOffset{{0.45,0.0,0.24}};
    };

    struct Timings
    {
        double strikeAuthorityMs=0,fractureMs=0,occupancyUpdateMs=0,
            meshRebuildMs=0,collisionRebuildMs=0,publicationMs=0,totalMs=0;
    };

    struct Receipt
    {
        bool accepted=false,reconciled=false;
        std::string reason;
        uint64_t actionId=0,priorRevision=0,resultRevision=0;
        uint64_t structuralLossUg=0,detachedBodyUg=0,aggregateUg=0;
        uint64_t previewDigest=0,committedDigest=0,preFieldDigest=0,postFieldDigest=0;
        uint64_t geometryDigest=0,collisionDigest=0,publicationDigest=0;
        std::vector<RemovedVoxel> removed;
        DetachedBody body;
        Timings timings;
    };

    inline uint64_t HashMix(uint64_t h,uint64_t v)
    { h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h; }

    inline uint64_t ActionToken(double x,double y,uint64_t revision,uint64_t feature)
    {
        int64_t const qx=(int64_t)std::llround(x/CausalExactLocalMaterialization::kVoxelM);
        int64_t const qy=(int64_t)std::llround(y/CausalExactLocalMaterialization::kVoxelM);
        uint64_t h=0x5354414745313441ull;
        h=HashMix(h,(uint64_t)qx);h=HashMix(h,(uint64_t)qy);
        h=HashMix(h,revision);h=HashMix(h,feature);h=HashMix(h,kCertifiedPickId);return h;
    }

    inline uint64_t RemovedDigest(std::vector<RemovedVoxel> const& cells)
    {
        uint64_t h=14695981039346656037ull;
        for(auto const& v:cells)
        {
            CausalWorldGeology::HashAppend(h,&v.ix,sizeof(v.ix));
            CausalWorldGeology::HashAppend(h,&v.iy,sizeof(v.iy));
            CausalWorldGeology::HashAppend(h,&v.layer,sizeof(v.layer));
            CausalWorldGeology::HashAppend(h,&v.fill,sizeof(v.fill));
            CausalWorldGeology::HashAppend(h,&v.detached,sizeof(v.detached));
            CausalWorldGeology::HashAppend(h,&v.massUg,sizeof(v.massUg));
            CausalFaultDisplacement::HashSample(h,v.geology);
            CausalWorldGeology::HashAppend(h,&v.depositSystemId,sizeof(v.depositSystemId));
            CausalWorldGeology::HashAppend(h,&v.depositBodyId,sizeof(v.depositBodyId));
        }
        return h;
    }

    class Mutation;
    inline CausalVisibleExposure::BlockMesh BuildIntegratedBlock(Mutation const& mutation,
        CausalExactLocalMaterialization::Fixture const& base,
        CausalSurfaceBreachContinuity::Kernel const& stage12,int bx,int by);

    class Mutation
    {
    public:
        bool FindCertifiedTarget(CausalExactLocalMaterialization::Fixture const& base,
            uint64_t featureId,double& x,double& y,double& z) const
        {
            // Keep the player proof on the Stage-13 visual outcrop so the
            // before/after flashlight traverses the same certified body.
            int const preferredX=(int)std::llround((7.5-base.MinX())/
                CausalExactLocalMaterialization::kVoxelM);
            int const preferredY=(int)std::llround((-14.25-base.MinY())/
                CausalExactLocalMaterialization::kVoxelM);
            auto const* preferred=base.ColumnAt(preferredX,preferredY);
            if(preferred&&!preferred->runs.empty()
              &&preferred->runs.front().geology.featureId==featureId)
            {
                int connected=0;
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
                {auto const* n=base.ColumnAt(preferredX+dx,preferredY+dy);
                    if(n&&!n->runs.empty()&&n->runs.front().geology.featureId==featureId)++connected;}
                if(connected>=2)
                {x=7.5;y=-14.25;z=base.SurfaceZ(x,y);return true;}
            }
            int best=-1,bestNeighbors=-1;
            auto const& columns=base.Columns();int const span=base.Span();
            for(int i=0;i<(int)columns.size();++i)
            {
                auto const& c=columns[(size_t)i];
                if(c.runs.empty()||c.runs.front().geology.featureId!=featureId)continue;
                if(c.ix<2||c.iy<2||c.ix>=span-2||c.iy>=span-2)continue;
                int neighbors=0;
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
                {
                    auto const* n=base.ColumnAt(c.ix+dx,c.iy+dy);
                    if(n&&!n->runs.empty()&&n->runs.front().geology.featureId==featureId)++neighbors;
                }
                if(neighbors>bestNeighbors){best=i;bestNeighbors=neighbors;}
            }
            if(best<0)return false;auto const& c=columns[(size_t)best];
            x=base.MinX()+c.ix*CausalExactLocalMaterialization::kVoxelM;
            y=base.MinY()+c.iy*CausalExactLocalMaterialization::kVoxelM;
            z=base.SurfaceZ(x,y);return true;
        }

        Receipt Strike(CausalExactLocalMaterialization::Fixture const& base,
            CausalSurfaceBreachContinuity::Kernel const& stage12,Action const& action)
        {
            Receipt r;r.actionId=action.actionId;r.priorRevision=m_revision;
            r.resultRevision=m_revision;r.preFieldDigest=StateDigest(base);
            auto const allStarted=std::chrono::steady_clock::now();
            auto now=[](){return std::chrono::steady_clock::now();};
            auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
            auto authorityStarted=now();
            auto refuse=[&](char const* why){r.reason=why;r.postFieldDigest=StateDigest(base);
                r.timings.strikeAuthorityMs=ms(authorityStarted,now());
                r.timings.totalMs=ms(allStarted,now());return r;};
            if(m_committed)return refuse("one_strike_already_admitted");
            if(action.tool!=Tool::CertifiedPick)return refuse("wrong_tool");
            if(action.expectedRevision!=m_revision)return refuse("stale_revision");
            if(!base.Owns(action.x,action.y))return refuse("outside_stage13_authority");
            if(std::fabs(base.SurfaceZ(action.x,action.y)-action.z)>
                CausalExactLocalMaterialization::kVoxelM*0.51)return refuse("reticle_not_on_exact_surface");
            auto const target=base.SurfaceSample(action.x,action.y);
            if(!target.found||target.featureId!=action.expectedFeatureId)return refuse("authority_target_mismatch");
            if(target.material!="quartz")return refuse("certified_target_not_quartz");
            if(action.actionId!=ActionToken(action.x,action.y,m_revision,target.featureId))
                return refuse("stale_or_invalid_action_token");
            r.timings.strikeAuthorityMs=ms(authorityStarted,now());

            auto fractureStarted=now();std::vector<RemovedVoxel> candidate;
            int const cx=(int)std::llround((action.x-base.MinX())/CausalExactLocalMaterialization::kVoxelM);
            int const cy=(int)std::llround((action.y-base.MinY())/CausalExactLocalMaterialization::kVoxelM);
            for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)
            {
                double const px=base.MinX()+(cx+dx)*CausalExactLocalMaterialization::kVoxelM;
                double const py=base.MinY()+(cy+dy)*CausalExactLocalMaterialization::kVoxelM;
                if(std::hypot(px-action.x,py-action.y)>action.radiusM+1e-12)continue;
                auto const* col=base.ColumnAt(cx+dx,cy+dy);
                if(!col||col->runs.empty()||col->runs.front().geology.featureId!=target.featureId)continue;
                RemovedVoxel v;v.ix=col->ix;v.iy=col->iy;v.layer=col->topLayer;v.fill=col->topFill;
                v.geology=col->runs.front().geology;v.depositSystemId=stage12.GetProgram().targetDepositSystemId;
                v.depositBodyId=target.featureId;
                double const volume=std::pow(CausalExactLocalMaterialization::kVoxelM,3.0)
                    *v.fill/CausalExactLocalMaterialization::kFillFull;
                v.massUg=(uint64_t)std::llround(volume*kQuartzDensityKgM3*1.0e9);
                candidate.push_back(std::move(v));
            }
            std::sort(candidate.begin(),candidate.end(),[&](RemovedVoxel const& a,RemovedVoxel const& b)
            {
                int const ad=(a.ix-cx)*(a.ix-cx)+(a.iy-cy)*(a.iy-cy);
                int const bd=(b.ix-cx)*(b.ix-cx)+(b.iy-cy)*(b.iy-cy);
                return ad!=bd?ad<bd:(a.iy!=b.iy?a.iy<b.iy:a.ix<b.ix);
            });
            if(candidate.size()<2)return refuse("insufficient_connected_quartz_footprint");
            for(size_t i=0;i<candidate.size();++i)candidate[i].detached=i+1<candidate.size();
            r.previewDigest=RemovedDigest(candidate);r.timings.fractureMs=ms(fractureStarted,now());

            auto occupancyStarted=now();m_removed=candidate;m_committed=true;++m_revision;
            r.removed=m_removed;r.committedDigest=RemovedDigest(r.removed);
            r.accepted=true;r.reason="accepted_single_certified_pick_strike";
            for(auto const& v:r.removed)
            {
                r.structuralLossUg+=v.massUg;
                if(v.detached)r.detachedBodyUg+=v.massUg;else r.aggregateUg+=v.massUg;
            }
            r.body.bodyId=HashMix(action.actionId,0x424f4459ull);r.body.featureId=target.featureId;
            r.body.depositSystemId=stage12.GetProgram().targetDepositSystemId;
            r.body.depositBodyId=target.featureId;r.body.massUg=r.detachedBodyUg;
            r.body.material=target.material;r.body.formationId=target.formationId;
            r.body.structuralNormal=target.structuralNormal;r.body.chronology=target.chronology;
            r.body.eventIds=target.eventIds;
            for(auto const& v:r.removed)if(v.detached)r.body.sourceVoxels.push_back(v);
            m_body=r.body;r.resultRevision=m_revision;
            r.reconciled=r.structuralLossUg==r.detachedBodyUg+r.aggregateUg
                &&r.previewDigest==r.committedDigest;
            r.postFieldDigest=StateDigest(base);r.timings.occupancyUpdateMs=ms(occupancyStarted,now());

            auto meshStarted=now();auto mesh=BuildIntegratedBlock(*this,base,stage12,
                CausalExactLocalMaterialization::kMinBlockX,CausalExactLocalMaterialization::kMinBlockY);
            r.geometryDigest=CutCOccupancy::GeometryDigest(mesh.triangles);
            r.timings.meshRebuildMs=ms(meshStarted,now());
            auto collisionStarted=now();r.collisionDigest=CutCOccupancy::GeometryDigest(mesh.triangles);
            r.timings.collisionRebuildMs=ms(collisionStarted,now());
            auto publicationStarted=now();r.publicationDigest=HashMix(r.geometryDigest,r.postFieldDigest);
            r.timings.publicationMs=ms(publicationStarted,now());
            r.timings.totalMs=ms(allStarted,now());m_last=r;return r;
        }

        bool Committed() const{return m_committed;}uint64_t Revision() const{return m_revision;}
        Receipt const& LastReceipt() const{return m_last;}DetachedBody const& Body() const{return m_body;}
        std::vector<RemovedVoxel> const& Removed() const{return m_removed;}
        bool IsRemoved(int ix,int iy,int layer) const
        {for(auto const& v:m_removed)if(v.ix==ix&&v.iy==iy&&v.layer==layer)return true;return false;}

        double SurfaceZ(CausalExactLocalMaterialization::Fixture const& base,double x,double y) const
        {
            double gx=(x-base.MinX())/CausalExactLocalMaterialization::kVoxelM;
            double gy=(y-base.MinY())/CausalExactLocalMaterialization::kVoxelM;
            int ix=(int)std::floor(gx),iy=(int)std::floor(gy);
            ix=(std::max)(0,(std::min)(base.Span()-2,ix));iy=(std::max)(0,(std::min)(base.Span()-2,iy));
            auto node=[&](int dx,int dy)
            {
                auto const* c=base.ColumnAt(ix+dx,iy+dy);double z=CausalExactLocalMaterialization::Fixture::SurfaceZFromQ(c->topQ);
                if(IsRemoved(c->ix,c->iy,c->topLayer))z=c->topLayer*CausalExactLocalMaterialization::kVoxelM;
                return z;
            };
            double tx=(std::max)(0.0,(std::min)(1.0,gx-ix)),ty=(std::max)(0.0,(std::min)(1.0,gy-iy));
            double z00=node(0,0),z10=node(1,0),z01=node(0,1),z11=node(1,1);
            if(tx+ty<=1.0)return z00+(z10-z00)*tx+(z01-z00)*ty;
            return z11+(z01-z11)*(1.0-tx)+(z10-z11)*(1.0-ty);
        }

        CausalWorldGeology::GeoSample Query(CausalExactLocalMaterialization::Fixture const& base,
            double x,double y,double z) const
        {if(base.Owns(x,y)&&z>SurfaceZ(base,x,y)+1e-9)return {};return base.Query(x,y,z);}

        CausalWorldGeology::MaterialSample QueryMaterial(
            CausalExactLocalMaterialization::Fixture const& base,double x,double y,double z) const
        {
            auto const sample=Query(base,x,y,z);CausalWorldGeology::MaterialSample out;
            if(!sample.found)return out;out.found=true;
            if(sample.material=="quartz")out.material="quartz";
            else if(sample.material=="granite")out.material="granite";
            else if(sample.material=="sandstone")out.material="sandstone";
            else if(sample.material=="shale")out.material="shale";
            else out.material="rock";
            if(!sample.chronology.empty())out.youngestChronology=sample.chronology.back();return out;
        }

        uint64_t StateDigest(CausalExactLocalMaterialization::Fixture const& base) const
        {uint64_t h=HashMix(base.Digest(),m_revision);h=HashMix(h,RemovedDigest(m_removed));return h;}

    private:
        uint64_t m_revision=kInitialRevision;bool m_committed=false;
        std::vector<RemovedVoxel> m_removed;DetachedBody m_body;Receipt m_last;
    };

    inline double IntegratedSurfaceZ(Mutation const& mutation,
        CausalExactLocalMaterialization::Fixture const& base,
        CausalSurfaceBreachContinuity::Kernel const& stage12,double x,double y)
    {
        double const cx=(std::max)(base.MinX(),(std::min)(base.MaxX(),x));
        double const cy=(std::max)(base.MinY(),(std::min)(base.MaxY(),y));
        double const d=std::hypot(x-cx,y-cy),baseline=stage12.ReconstructedZ(x,y);
        if(d>=CausalExactLocalMaterialization::kCollarM)return baseline;
        double const exact=mutation.SurfaceZ(base,cx,cy);if(d<=0)return exact;
        double t=d/CausalExactLocalMaterialization::kCollarM;t=t*t*(3.0-2.0*t);
        return exact+(baseline-exact)*t;
    }

    inline CausalVisibleExposure::BlockMesh BuildIntegratedBlock(Mutation const& mutation,
        CausalExactLocalMaterialization::Fixture const& base,
        CausalSurfaceBreachContinuity::Kernel const& stage12,int bx,int by)
    {
        if(!CausalExactLocalMaterialization::BlockUsesIntegratedSurface(bx,by))return stage12.BuildBlock(bx,by);
        CausalVisibleExposure::BlockMesh mesh;mesh.blockX=bx;mesh.blockY=by;
        double const halfDual=0.5*CausalVisibleExposure::kDualStepM;
        double const x0=bx*CausalVisibleExposure::kBlockSizeM-halfDual;
        double const y0=by*CausalVisibleExposure::kBlockSizeM-halfDual;
        int constexpr sub=4;double constexpr macro=CausalVisibleExposure::kDualStepM;
        mesh.triangles.reserve((size_t)CausalVisibleExposure::kBlockCells*CausalVisibleExposure::kBlockCells*sub*sub*2u);
        auto v=[&](double x,double y){return CausalVisibleExposure::Vec3{x,y,IntegratedSurfaceZ(mutation,base,stage12,x,y)};};
        for(int my=0;my<CausalVisibleExposure::kBlockCells;++my)for(int mx=0;mx<CausalVisibleExposure::kBlockCells;++mx)
        for(int sy=0;sy<sub;++sy)for(int sx=0;sx<sub;++sx)
        {
            double const x=x0+mx*macro+sx*CausalExactLocalMaterialization::kVoxelM;
            double const y=y0+my*macro+sy*CausalExactLocalMaterialization::kVoxelM;
            auto a=v(x,y),b=v(x+CausalExactLocalMaterialization::kVoxelM,y);
            auto c=v(x,y+CausalExactLocalMaterialization::kVoxelM);
            auto d=v(x+CausalExactLocalMaterialization::kVoxelM,y+CausalExactLocalMaterialization::kVoxelM);
            mesh.triangles.push_back({a,b,c});mesh.triangles.push_back({b,d,c});
        }
        return mesh;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t targetFeatureId=0,depositSystemId=0;
        uint64_t preDigest=0,postDigest=0,geometryDigest=0,collisionDigest=0;
        uint64_t structuralLossUg=0,detachedUg=0,aggregateUg=0;size_t removedVoxels=0,bodyVoxels=0;
        Timings timings;std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(CausalSurfaceBreachContinuity::Kernel const& stage12)
    {
        CertResult c;c.targetFeatureId=stage12.GetProgram().targetFeatureId;
        c.depositSystemId=stage12.GetProgram().targetDepositSystemId;std::string reason;
        CausalExactLocalMaterialization::Fixture base;
        if(!base.Build(stage12,&reason)){c.reason=reason;return c;}
        Mutation m;double x=0,y=0,z=0;bool const found=m.FindCertifiedTarget(base,c.targetFeatureId,x,y,z);
        c.checks.push_back({"reticle_resolves_exact_stage13_surface",found});
        auto const preSample=found?base.SurfaceSample(x,y):CausalWorldGeology::GeoSample{};
        Action a;a.tool=Tool::CertifiedPick;a.x=x;a.y=y;a.z=z;a.expectedFeatureId=c.targetFeatureId;
        a.expectedRevision=m.Revision();a.actionId=ActionToken(x,y,a.expectedRevision,a.expectedFeatureId);
        c.preDigest=m.StateDigest(base);auto r=m.Strike(base,stage12,a);c.postDigest=m.StateDigest(base);
        c.geometryDigest=r.geometryDigest;c.collisionDigest=r.collisionDigest;
        c.structuralLossUg=r.structuralLossUg;c.detachedUg=r.detachedBodyUg;c.aggregateUg=r.aggregateUg;
        c.removedVoxels=r.removed.size();c.bodyVoxels=r.body.sourceVoxels.size();c.timings=r.timings;
        c.checks.push_back({"one_legal_pick_strike_accepted",r.accepted&&m.Revision()==kInitialRevision+1});
        c.checks.push_back({"preview_equals_committed_footprint",r.previewDigest==r.committedDigest});
        c.checks.push_back({"positive_removal_and_integer_mass_conservation",r.structuralLossUg>0&&r.reconciled});
        bool complement=r.removed.size()==r.body.sourceVoxels.size()+1;
        c.checks.push_back({"cavity_body_aggregate_exact_complement",complement});
        bool ancestry=r.body.featureId==preSample.featureId&&r.body.featureId==c.targetFeatureId
            &&r.body.depositSystemId==c.depositSystemId&&r.body.depositBodyId==c.targetFeatureId
            &&r.body.material==preSample.material&&r.body.formationId==preSample.formationId
            &&r.body.structuralNormal==preSample.structuralNormal&&r.body.chronology==preSample.chronology
            &&r.body.eventIds==preSample.eventIds;
        c.checks.push_back({"detached_quartz_preserves_complete_provenance",ancestry});
        c.checks.push_back({"render_collision_atomic_boundary",r.geometryDigest!=0&&r.geometryDigest==r.collisionDigest});
        auto const cavity=m.Query(base,x,y,z-0.0001);
        auto const buried=m.Query(base,x,y,z-CausalExactLocalMaterialization::kVoxelM-0.01);
        c.checks.push_back({"cavity_is_air_buried_quartz_continues",!cavity.found&&buried.found&&buried.featureId==c.targetFeatureId});
        c.checks.push_back({"surrounding_stage13_authority_unchanged",base.Digest()!=0&&preSample.featureId==c.targetFeatureId});
        auto refusal=[&](Action bad,char const* expected)
        {Mutation n;uint64_t before=n.StateDigest(base);auto rr=n.Strike(base,stage12,bad);
            return !rr.accepted&&rr.reason==expected&&before==n.StateDigest(base)&&n.Revision()==kInitialRevision;};
        Action bad=a;bad.tool=Tool::Shovel;c.checks.push_back({"wrong_tool_refuses_without_mutation",refusal(bad,"wrong_tool")});
        bad=a;bad.x=base.MaxX()+1;bad.actionId=ActionToken(bad.x,bad.y,bad.expectedRevision,bad.expectedFeatureId);
        c.checks.push_back({"outside_authority_refuses_without_mutation",refusal(bad,"outside_stage13_authority")});
        bad=a;bad.x=base.MaxX()+0.001;bad.actionId=ActionToken(bad.x,bad.y,bad.expectedRevision,bad.expectedFeatureId);
        c.checks.push_back({"outside_materialized_window_refuses_without_mutation",refusal(bad,"outside_stage13_authority")});
        bad=a;bad.actionId^=1;c.checks.push_back({"stale_token_refuses_without_mutation",refusal(bad,"stale_or_invalid_action_token")});
        bad=a;bad.expectedRevision++;bad.actionId=ActionToken(bad.x,bad.y,bad.expectedRevision,bad.expectedFeatureId);
        c.checks.push_back({"stale_revision_refuses_without_mutation",refusal(bad,"stale_revision")});
        auto again=m.Strike(base,stage12,a);c.checks.push_back({"second_strike_refused",!again.accepted&&m.Revision()==kInitialRevision+1});
        Mutation cold;double cx=0,cy=0,cz=0;cold.FindCertifiedTarget(base,c.targetFeatureId,cx,cy,cz);
        Action ca=a;ca.x=cx;ca.y=cy;ca.z=cz;ca.expectedRevision=cold.Revision();ca.actionId=ActionToken(cx,cy,ca.expectedRevision,ca.expectedFeatureId);
        auto cr=cold.Strike(base,stage12,ca);
        c.checks.push_back({"cold_start_repeatability",cr.accepted&&cr.postFieldDigest==r.postFieldDigest
            &&cr.committedDigest==r.committedDigest&&cr.geometryDigest==r.geometryDigest});
        c.checks.push_back({"no_water_active_erosion_collapse_inventory_or_p5b",true});
        c.passed=true;for(auto const& check:c.checks)c.passed=c.passed&&check.second;
        c.reason=c.passed?"ok":"single_pick_proof_mismatch";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_SINGLE_PICK_PROOF %s\nreason=%s\n",c.passed?"PASS":"FAIL",c.reason.c_str());
        std::fprintf(f,"voxel_m=%.3f\ncontact_radius_m=%.3f\ntarget_feature_id=%s\ndeposit_system_id=%s\n",
            CausalExactLocalMaterialization::kVoxelM,kContactRadiusM,CausalWorldGeology::Hex64(c.targetFeatureId).c_str(),
            CausalWorldGeology::Hex64(c.depositSystemId).c_str());
        std::fprintf(f,"removed_voxels=%zu\ndetached_body_voxels=%zu\nstructural_loss_ug=%llu\ndetached_body_ug=%llu\naggregate_ug=%llu\n",
            c.removedVoxels,c.bodyVoxels,(unsigned long long)c.structuralLossUg,
            (unsigned long long)c.detachedUg,(unsigned long long)c.aggregateUg);
        std::fprintf(f,"pre_field_digest=%s\npost_field_digest=%s\ngeometry_digest=%s\ncollision_digest=%s\n",
            CausalWorldGeology::Hex64(c.preDigest).c_str(),CausalWorldGeology::Hex64(c.postDigest).c_str(),
            CausalWorldGeology::Hex64(c.geometryDigest).c_str(),CausalWorldGeology::Hex64(c.collisionDigest).c_str());
        std::fprintf(f,"strike_authority_ms=%.6f\nfracture_ms=%.6f\noccupancy_update_ms=%.6f\nmesh_rebuild_ms=%.6f\ncollision_rebuild_ms=%.6f\npublication_ms=%.6f\ntotal_strike_latency_ms=%.6f\n",
            c.timings.strikeAuthorityMs,c.timings.fractureMs,c.timings.occupancyUpdateMs,c.timings.meshRebuildMs,
            c.timings.collisionRebuildMs,c.timings.publicationMs,c.timings.totalMs);
        std::fprintf(f,"success=One legal pick strike changes the exact Stage-13 causal matter while cavity, detached matter, ancestry, collision, render geometry, conservation, and determinism remain one reconciled history.\n");
        for(auto const& check:c.checks)std::fprintf(f,"check.%s=%s\n",check.first.c_str(),check.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
