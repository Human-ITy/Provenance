#include "../Geometry/GraniteOutcrop.h"
#include <cstdio>
#include <set>
#include <chrono>
using namespace EE::GraniteOutcrop;
int checks=0,failures=0;
void Check(bool ok,char const* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}}
double MeshVolume(std::vector<Triangle> const& mesh){double v=0;for(auto t:mesh)v+=Dot(t.a,Cross(t.b,t.c))/6;return v;}
double TriangleDistance(Triangle t,P o,P d)
{
    P e1=Add(t.b,Mul(t.a,-1)),e2=Add(t.c,Mul(t.a,-1)),h=Cross(d,e2);
    double determinant=Dot(e1,h);if(std::abs(determinant)<1e-12)return 1e30;
    P s=Add(o,Mul(t.a,-1));double u=Dot(s,h)/determinant;if(u< -1e-9||u>1+1e-9)return 1e30;
    P q=Cross(s,e1);double v=Dot(d,q)/determinant;if(v< -1e-9||u+v>1+1e-9)return 1e30;
    double distance=Dot(e2,q)/determinant;return distance>=0?distance:1e30;
}
int main()
{
    Body body; double initial=Count*Step*Step*Step;
    Check(std::abs(MeshVolume(Boundary(body))-initial)<1e-8,"initial closed boundary volume");
    std::set<int> faceCounts;
    for(int id=0;id<Count;id+=137)
    {
        Poly a=Geometry(body.seed,id,0),b=Geometry(body.seed,id,1);
        Check(std::abs(Volume(a)+Volume(b)-Step*Step*Step)<1e-12,"complementary cast volumes");
        Check(Mass(body.seed,id,0)+Mass(body.seed,id,1)==ParcelMg,"paired exact mass");
        for(auto const* poly:{&a,&b})
        {
            faceCounts.insert(int(poly->size()));double diameter=0;
            for(auto const& f:*poly)for(P p:f.p)for(auto const& g:*poly)for(P q:g.p)
                diameter=std::max(diameter,std::sqrt(Dot(Shape(Add(p,Mul(q,-1))),Shape(Add(p,Mul(q,-1))))));
            Check(diameter<=.127,"basic chip no longer than five inches");
            Check(diameter>=.04,"no sliver chips in basic partition");
        }
    }
    Check(faceCounts.size()>=3,"variable wedge/block topology");
    P origin={1.411,-.8,.913},direction={0,1,0};
    auto hit=Raycast(body,origin,direction);
    Check(hit.hit&&hit.distance<2,"inclined face contact");
    auto weak=Strike(body,origin,direction,400,0);
    Check(weak.contact.hit&&!weak.removed&&body.removedMg==0,"weak implement contact without removal");
    auto low=Strike(body,origin,direction,.01,2);
    Check(low.contact.hit&&!low.removed&&body.cells[hit.id].damage[hit.part]>0,"localized accumulation");
    auto near=Raycast(body,P{1.61,-.8,.913},direction);
    Check(near.id!=hit.id&&body.cells[near.id].damage[near.part]==0,"neighbor damage independent");
    std::set<int> removed;uint64_t mass=0;double volume=0;
    for(int i=0;i<25;++i)
    {
        auto r=Strike(body,origin,direction);
        if(!r.removed)break;
        Check(removed.insert(r.chip.id*2+r.chip.part).second,"no duplicate matter");
        Check(std::abs(Volume(r.chip.geometry)-r.chip.volume)<1e-12,"chip is actual removed shape");
        Check(std::abs(double(r.chip.massMg)-r.chip.volume*Density*1e6)<=1.01,"mass matches cast volume within mg rounding");
        mass+=r.chip.massMg;volume+=r.chip.volume;
    }
    Check(removed.size()>8,"same direction keeps deepening cavity");
    Check(body.removedMg==mass&&std::abs(body.removedM3-volume)<1e-12,"receipt ledger exact");
    Check(std::abs(MeshVolume(Boundary(body)) + volume-initial)<1e-8,"host plus removed cast volume conserved");
    auto meshStart=std::chrono::steady_clock::now();auto boundary=Boundary(body);
    double meshMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-meshStart).count();
    for(int i=0;i<256;++i)
    {
        uint32_t hash=Hash(uint32_t(i));
        P o={1.1+double(hash&255)/255*.6,-.3,.65+double((hash>>8)&255)/255*.5};
        P d=Unit(P{double(int((hash>>16)&255)-127)/1000,1,double(int((hash>>24)&255)-127)/1000});
        double distance=1e30;for(auto triangle:boundary)distance=std::min(distance,TriangleDistance(triangle,o,d));
        auto contact=Raycast(body,o,d,2);
        Check(contact.hit==(distance<=2),"independent visible mesh / ray hit agreement");
        if(contact.hit)Check(std::abs(contact.distance-distance)<1e-7,"visible cavity depth matches contact");
    }
    auto inside=Raycast(body,Shape(P{.121,.121,.121}),P{0,1,0});
    Check(!inside.hit,"inside solid cannot tunnel");
    Check(!BlocksCapsule(body,P{1.4,-.5,0}),"walkable clear ground");
    Check(BlocksCapsule(body,P{1.4,.1,0}),"granite blocks capsule");
    Check(!BlocksCapsule(body,P{1.4,1,Height+.06}),"standing above stone is clear");
    Body replay;
    Strike(replay,origin,direction,.01,2);
    for(size_t i=0;i<removed.size();++i)Strike(replay,origin,direction);
    Check(replay.removedMg==body.removedMg&&replay.removedM3==body.removedM3,"deterministic replay");
    for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
    {
        Body test;P p={.711,.531,.831},d={};d[axis]=sign;
        p[axis]=sign>0?-.1:(axis==0?NX:axis==1?NY:NZ)*Step+.1;
        auto r=Strike(test,Shape(p),Shape(d));
        Check(r.contact.hit&&r.removed,"all exposed directions strikeable");
    }
    std::printf("Granite outcrop: %d checks, %d failures; boundary %.2f ms / %zu triangles\n",checks,failures,meshMs,boundary.size());
    return failures?1:0;
}
