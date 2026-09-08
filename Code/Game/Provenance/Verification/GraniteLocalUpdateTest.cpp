#include "../Geometry/GraniteContactDebris.h"
#include <chrono>
#include <cstdio>
namespace G=EE::GraniteContactCast;
namespace D=EE::GraniteContactDebris;
int checks=0,failures=0;
void Check(bool ok,char const* label){++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}}
double Volume(std::vector<G::Triangle> const& triangles){double v=0;for(auto const& t:triangles)v+=G::Dot(t.a,G::Cross(t.b,t.c))/6;return v;}
G::Hit ReferenceRay(G::Body const& body,G::P o,G::P direction,double reach)
{
    auto d=G::Unit(direction),end=G::Add(o,G::Mul(d,reach));G::P lo,hi;for(int k=0;k<3;++k){lo[k]=std::min(o[k],end[k]);hi[k]=std::max(o[k],end[k]);}
    G::Hit result;double nearest=reach+1e-8;
    for(int id:G::Candidates(body,lo,hi))
    {
        auto it=body.changed.find(id);G::Poly original;if(it==body.changed.end())original=G::OriginalSurface(body,id);auto const& surface=it==body.changed.end()?original:it->second.surface;
        for(auto const& f:surface)for(size_t i=1;i+1<f.p.size();++i){double t=G::Intersect(f.p[0],f.p[i],f.p[i+1],o,d);if(t<nearest){nearest=t;result={true,id,t,G::Add(o,G::Mul(d,t)),G::Normal(f)};}}
    }if(result.hit&&G::Dot(result.normal,d)>1e-8)return {};return result;
}
int main()
{
    using Clock=std::chrono::steady_clock;auto ms=[](auto start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();};
    G::Body body;std::map<int,std::vector<G::Triangle>> displayed;
    for(auto const& p:body.renderGroups)displayed[p.first]=G::PatchBoundary(body,p.first);
    std::vector<G::Chip> chips;size_t maximumGroups=0;
    for(int i=0;i<128;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));G::P origin={.78+.52*double(h&255)/255,-.4,.18+.22*double((h>>8)&255)/255};
        auto tx=G::PrepareStrike(body,origin,{0,1,0});
        Check(tx.receipt.removed,"128 corner-region strikes remove material");
        maximumGroups=std::max(maximumGroups,tx.renderUpdates.size());
        Check(tx.renderUpdates.size()<12,"only bounded local groups rebuilt");
        for(auto const& update:tx.updates)Check(tx.renderUpdates.count(G::RenderPatchID(body.substrate.pieces[update.first]))==1,"every changed cell's display group is replaced");
        for(auto& p:tx.renderUpdates)displayed[p.first]=std::move(p.second);
        auto r=G::Commit(body,std::move(tx));if(r.removed)chips.push_back(std::move(r.chip));
        if(i%32==31){std::printf("prepared %d strikes\n",i+1);std::fflush(stdout);}
    }
    size_t patchTriangles=0;double patchVolume=0;
    for(auto const& p:displayed)
    {
        auto actual=G::PatchBoundary(body,p.first);Check(actual.size()==p.second.size(),"cached patch triangle count matches current matter");
        bool same=actual.size()==p.second.size();for(size_t i=0;i<actual.size()&&same;++i){auto const& a=actual[i];auto const& b=p.second[i];same=a.a==b.a&&a.b==b.b&&a.c==b.c&&a.na==b.na&&a.nb==b.nb&&a.nc==b.nc;}
        Check(same,"cached patch preserves exact positions and normals");patchTriangles+=p.second.size();patchVolume+=Volume(p.second);
    }
    auto whole=G::Boundary(body);Check(patchTriangles==whole.size(),"local groups contain full boundary exactly once");
    Check(displayed.size()<160,"rock renderer keeps a bounded balanced group count");
    Check(std::abs(patchVolume+body.removedM3-body.initialM3)<1e-7,"local meshes conserve full geometric volume");
    double oldRay=0,newRay=0;std::vector<G::P> origins,directions;
    for(int i=0;i<160;++i)
    {
        uint32_t h=G::Hash(uint32_t(i+900));G::P target={.74+.62*double(h&255)/255,.35,.16+.28*double((h>>8)&255)/255};
        G::P o=i%3==0?G::P{1.7,-.3,1.2}:i%3==1?G::P{1.2,-.5,.7}:G::P{1.1,.5,1.4};G::P d=G::Unit(G::Add(target,G::Mul(o,-1)));
        auto start=Clock::now();auto reference=ReferenceRay(body,o,d,2);oldRay+=ms(start);start=Clock::now();auto fast=G::Raycast(body,o,d,2);newRay+=ms(start);
        Check(fast.hit==reference.hit,"accelerated ray hit agrees with uncached surface");if(fast.hit)Check(std::abs(fast.distance-reference.distance)<1e-8,"accelerated ray distance unchanged");
        origins.push_back(o);directions.push_back(d);
    }
    std::printf("128-strike outcrop: max groups %zu / %zu; ray average old %.3f new %.3f ms\n",maximumGroups,displayed.size(),oldRay/160,newRay/160);
    double slowSupport=0,fastSupport=0;
    for(size_t i=0;i<12&&i<chips.size();++i)
    {
        auto d=D::Launch(chips[chips.size()-1-i],{1,0,0});double a=.21*double(i);d.orientation={std::cos(a),std::sin(a),0,0};
        G::P center={2.75+.025*double(i),.20+.01*double(i),1.};d.offset=G::Add(center,G::Mul(d.chip.center,-1));
        auto start=Clock::now();double reference=EE::GraniteOutcropGround::RequiredLift(D::Geometry(d),{});slowSupport+=ms(start);
        start=Clock::now();double fast=D::RequiredLift(d);fastSupport+=ms(start);Check(std::abs(reference-fast)<1e-7,"dense chip on sloped seam retains exact support");
    }
    std::printf("dense sloped support average old %.3f new %.3f ms\n",slowSupport/12,fastSupport/12);
    std::vector<D::FallingChip> moving;
    for(int i=0;i<3;++i)
    {
        auto d=D::Launch(chips[chips.size()-1-i],{1,0,0});G::P center={3.0+.06*i,.18+.05*i,1.4};d.offset=G::Add(center,G::Mul(d.chip.center,-1));d.velocity={-.1,-.12,0};moving.push_back(std::move(d));
    }
    double frameTotal=0,peak=0;for(int frame=0;frame<600;++frame){auto start=Clock::now();for(auto& d:moving)D::Advance(d,body,1./60);double dt=ms(start);frameTotal+=dt;peak=std::max(peak,dt);}
    int active=0;for(auto const& d:moving){if(!d.settled)++active;Check(EE::GraniteOutcropGround::RequiredLift(D::Geometry(d),{})<=1e-7,"seam flight finishes above soil");for(double p:d.offset)Check(std::isfinite(p),"finite seam pose");}
    std::printf("3 dense seam chips: active %d; avg %.3f peak %.3f ms\n",active,frameTotal/600,peak);
    Check(active==0,"seam chips settle within ten seconds");
    std::printf("Local update regression: %d checks %d failures\n",checks,failures);return failures?1:0;
}
