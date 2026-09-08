#include <cstdio>
#define GRANITE_CAST_DIAGNOSTICS
#include "../Geometry/GraniteContactCast.h"
#include "../Geometry/GraniteContactDebris.h"
#include <chrono>
#include <fstream>
#include <future>
namespace G=EE::GraniteContactCast;
namespace D=EE::GraniteContactDebris;
int checks=0,failures=0;
void Check(bool ok,char const* message){++checks;if(!ok){++failures;std::printf("FAIL %s\n",message);}}
double MeshVolume(std::vector<G::Triangle> const& mesh){double sum=0;for(auto const& t:mesh)sum+=G::Dot(t.a,G::Cross(t.b,t.c))/6;return sum;}
int main(int argc,char** argv)
{
    G::Body b;auto start=std::chrono::steady_clock::now();std::vector<G::Chip> chips;
    G::P firstOrigin={G::Width*.28,-.4,.22};
    auto first=G::Raycast(b,firstOrigin,{0,1,0});Check(first.hit,"initial contact");
    auto weak=G::Strike(b,firstOrigin,{0,1,0},400,0);Check(weak.contact.hit&&!weak.removed,"weak tool receipt");
    auto low=G::Strike(b,firstOrigin,{0,1,0},.1);Check(!low.removed,"local accumulation");
    int strikes=argc>3?std::atoi(argv[3]):80;
    for(int i=0;i<strikes;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));double x=.78+.52*double(h&255)/255,z=.18+.22*double((h>>8)&255)/255;
        auto r=G::Strike(b,{x,-.4,z},{0,1,0});Check(r.contact.hit,"contact during repeated excavation");
        if(!r.removed){std::printf("strike %d %s\n",i,r.status);continue;}
        Check(std::abs(G::Volume(r.chip.geometry)-r.chip.volume)<1e-9,"closed matching cast");
        G::P lo={1e9,1e9,1e9},hi={-1e9,-1e9,-1e9};for(auto const& f:r.chip.geometry)for(auto p:f.p)for(int k=0;k<3;++k){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}
        std::vector<G::P> points;for(auto const& f:r.chip.geometry)for(auto p:f.p)points.push_back(p);
        double diameter=0;for(auto p:points)for(auto q:points){auto d=G::Add(p,G::Mul(q,-1));diameter=std::max(diameter,G::Dot(d,d));}
        Check(diameter<=.127*.127,"bounded hand-sized cast");
        chips.push_back(std::move(r.chip));
        if(i%10==0)std::printf("strike %d faces %zu cells %zu ms %.1f\n",i,chips.back().geometry.size(),b.changed.size(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }
    auto mesh=G::Boundary(b);Check(std::abs(MeshVolume(mesh)+b.removedM3-b.initialM3)<1e-7,"remaining exterior plus removed equals initial volume");
    for(int i=0;i<32;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));G::P o={.74+.62*double(h&255)/255,-.4,.16+.26*double((h>>8)&255)/255},d={0,1,0};
        double nearest=1e30;for(auto const& t:mesh)nearest=std::min(nearest,G::Intersect(t.a,t.b,t.c,o,d));auto ray=G::Raycast(b,o,d);
        Check(ray.hit==(nearest<=2),"current cavity ray agrees with rendered geometry");if(ray.hit)Check(std::abs(ray.distance-nearest)<1e-8,"current cavity contact distance");
    }
    bool inherited=true;int exteriorFaces=0;
    for(auto const& chip:chips)for(auto const& f:chip.geometry)if(f.axis==G::Weathered)
    {
        ++exteriorFaces;auto const& source=b.substrate.pieces[(f.sign-1)/16].faces[(f.sign-1)%16];
        auto a=b.substrate.vertices[source.v[0]],c=b.substrate.vertices[source.v[1]],d=b.substrate.vertices[source.v[2]];
        auto n=G::Unit(G::Cross(G::Add(c,G::Mul(a,-1)),G::Add(d,G::Mul(a,-1))));
        for(auto p:f.p)inherited=inherited&&std::abs(G::Dot(n,G::Add(p,G::Mul(a,-1))))<1e-9;
    }
    Check(exteriorFaces>0&&inherited,"weathered outside face remains on original exterior");
    uint64_t mg=0;double volume=0;for(auto const& chip:chips){mg+=chip.massMg;volume+=chip.volume;}
    Check(mg==b.removedMg&&volume==b.removedM3,"recovered mass and volume ledger");Check(chips.size()>65,"no directional depth stall");
    Check(!G::BlocksCapsule(b,{G::Width*.5,-.5,.17}),"ground in front remains walkable");Check(G::BlocksCapsule(b,{G::Width*.28,.8,.17}),"remaining material blocks walking");
    Check(!G::BlocksCapsule(b,{G::Width*.28,1,b.high[2]+.05}),"standing above original crest is clear");
    auto walkQueryStart=std::chrono::steady_clock::now();int blockedSamples=0;
    for(int repeat=0;repeat<4;++repeat)for(int y=0;y<16;++y)for(int x=0;x<16;++x)
    {
        G::P feet={.72+.68*x/15.,-.28+.88*y/15.,(repeat&1)?.17:b.high[2]+.01};
        blockedSamples+=G::BlocksCapsule(b,feet);
    }
    double walkQueryMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-walkQueryStart).count();
    std::printf("1024 dense-cavity walk queries %.2f ms (%d blocked)\n",walkQueryMs,blockedSamples);
    Check(walkQueryMs<750.,"dense modified cavity remains interactive in an unoptimized build");
    for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
    {
        G::P o={G::Width*.28,G::Depth*.5,.42},d={};d[axis]=sign;o[axis]=sign>0?b.low[axis]-.15:b.high[axis]+.15;
        auto tx=G::PrepareStrike(b,o,d);Check(tx.receipt.contact.hit&&tx.receipt.removed,"six directions can prepare removal");
    }
    auto rev=b.revision;auto beforeMg=b.removedMg;
    G::P asyncOrigin={1.42,-.4,.24};auto job=std::async(std::launch::async,[&](){return G::PrepareStrike(b,asyncOrigin,{0,1,0});});
    for(int i=0;i<20;++i)Check(G::Raycast(b,asyncOrigin,{0,1,0}).hit,"camera queries remain valid during preparation");
    auto tx=job.get();Check(b.revision==rev&&b.removedMg==beforeMg,"background preparation never mutates world");
    auto stale=G::PrepareStrike(b,{1.58,-.4,.24},{0,1,0});auto committed=G::Commit(b,std::move(tx));Check(committed.removed&&b.revision==rev+1,"single atomic commit");
    Check(!G::Commit(b,std::move(stale)).removed,"stale transaction refused");
    std::map<uint64_t,int> signatures;
    for(uint32_t seed=0;seed<64;++seed)
    {
        auto cutter=G::MakeCutter(seed,{0,0,0},{0,-1,0},400);auto shape=G::Intersection(G::Box({-.2,-.2,-.2},{.2,.2,.2}),cutter.planes);
        Check(shape.size()>30,"real roundover geometry beyond macro planes");
        bool bounded=true;for(auto const& f:shape)for(auto p:f.p)for(int k=0;k<3;++k)bounded=bounded&&p[k]>=cutter.low[k]-1e-8&&p[k]<=cutter.high[k]+1e-8;
        Check(bounded,"cutter acceleration bounds contain curved geometry");
        signatures[uint64_t(std::llround(G::Volume(shape)*1e15))]++;
    }
    Check(signatures.size()==64,"seeded shape sample has no repeated volume signatures");
    if(argc>1){std::ofstream out(argv[1]);for(auto const& t:mesh){out<<"0";for(auto p:{t.a,t.b,t.c})for(double v:p)out<<' '<<v;out<<'\n';}}
    double maximumFlightMs=0;
    for(size_t i=0;i<std::min(size_t(12),chips.size());++i)
    {
        auto chip=D::Launch(chips[i],{0,-1,0});auto flightStart=std::chrono::steady_clock::now();
        for(int frame=0;frame<480&&!chip.settled;++frame)D::Advance(chip,b,1./60);
        maximumFlightMs=std::max(maximumFlightMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-flightStart).count());
        Check(chip.settled,"tumbling chip settles within eight seconds");
        Check(EE::GraniteOutcropGround::RequiredLift(D::Geometry(chip),G::P{})<=1e-8,"rotated cast rests above actual soil");
        Check(std::abs(G::Volume(D::Geometry(chip))-chip.chip.volume)<1e-9,"tumbling preserves removed volume");
        Check(chip.orientation!=D::Q{1,0,0,0},"chip actually rotates");
        if(!chip.settled)std::printf("unsettled chip %zu z %g speed %g rest %g\n",i,chip.offset[2],G::Dot(chip.velocity,chip.velocity),chip.restTime);
    }
    std::printf("maximum complete flight simulation %.2f ms\n",maximumFlightMs);
    if(!chips.empty())
    {
        auto a=D::Launch(chips[0],{0,-1,0}),c=a;
        for(int i=0;i<240;++i)D::Advance(a,b,1./30);for(int i=0;i<1152;++i)D::Advance(c,b,1./144);
        Check(G::Dot(G::Add(a.offset,G::Mul(c.offset,-1)),G::Add(a.offset,G::Mul(c.offset,-1)))<1e-14,"30 and 144 FPS agree on settling");
    }
    if(argc>2)
    {
        std::ofstream out(argv[2]);
        for(size_t i=0;i<std::min(size_t(12),chips.size());++i)
        {
            G::P offset=G::Add(G::P{1.1+.15*(i%4),-.05,.65+.15*(i/4)},G::Mul(chips[i].center,-1));
            for(auto const& t:G::Triangulate(chips[i].geometry)){out<<"0";for(auto p:{t.a,t.b,t.c})for(double v:G::Add(p,offset))out<<' '<<v;out<<'\n';}
        }
    }
    std::printf("Contact cast: %d checks %d failures; %zu chips; %.1f ms\n",checks,failures,chips.size(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());return failures?1:0;
}
