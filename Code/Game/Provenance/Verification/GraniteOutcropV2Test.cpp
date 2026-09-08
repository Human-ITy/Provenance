#include "../Geometry/GraniteOutcropGround.h"
#include <cstdio>
#include <chrono>
#include <fstream>
namespace G=EE::GraniteOutcropV2;
namespace S=EE::GraniteOutcropGround;
using P=G::P;
int checks=0,failures=0;
void Check(bool ok,char const* name){++checks;if(!ok){++failures;std::printf("FAIL: %s\n",name);}}
double MeshVolume(std::vector<G::Triangle> const& mesh){double v=0;for(auto const& t:mesh)v+=G::Dot(t.a,G::Cross(t.b,t.c))/6;return v;}
void Export(G::Body const& body,char const* path)
{
    std::ofstream out(path);
    for(auto const& t:G::Boundary(body))out<<"0 "<<t.a[0]<<' '<<t.a[1]<<' '<<t.a[2]<<' '<<t.b[0]<<' '<<t.b[1]<<' '<<t.b[2]<<' '<<t.c[0]<<' '<<t.c[1]<<' '<<t.c[2]<<'\n';
    for(int y=0;y<S::NY;++y)for(int x=0;x<S::NX;++x)
    {
        auto q=S::Quad(x,y);for(auto indices:{std::array<int,3>{0,1,2},std::array<int,3>{0,2,3}})
        {out<<"1";for(int i:indices)for(double p:q[i])out<<' '<<p;out<<'\n';}
    }
}
int main(int argc,char** argv)
{
    auto start=std::chrono::steady_clock::now();G::Body body;
    double initMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::printf("Initial granite %.12f m3 / %.3f kg; bounds %.6f %.6f %.6f to %.6f %.6f %.6f\n",
        body.initialM3,double(body.initialMg)/1e6,body.low[0],body.low[1],body.low[2],body.high[0],body.high[1],body.high[2]);
    if(argc>1)
    {
        // Offline shape inspection artifact, not a claimed in-editor screenshot.
        Export(body,argv[1]);
    }
    uint64_t mass=0;double volume=0,maxDiameter=0,minDiameter=1e9;int maxDiameterPiece=-1;bool bounded=true,casts=true,paired=true;int faceCounts[20]={};
    for(int id=0;id<int(body.pieces.size());++id)
    {
        auto const& piece=body.pieces[id];mass+=piece.massMg;volume+=piece.volume;auto geometry=G::Geometry(body,id);
        casts=casts&&std::abs(G::Volume(geometry)-piece.volume)<1e-12;double diameter=0;
        for(auto const& f:geometry)for(P p:f.p)for(auto const& g:geometry)for(P q:g.p)diameter=std::max(diameter,std::sqrt(G::Dot(G::Add(p,G::Mul(q,-1)),G::Add(p,G::Mul(q,-1)))));
        if(diameter>maxDiameter){maxDiameter=diameter;maxDiameterPiece=id;}minDiameter=std::min(minDiameter,diameter);bounded=bounded&&diameter<=G::MaxPartitionDiameter;
        if(geometry.size()<20)++faceCounts[geometry.size()];
        for(auto const& f:piece.faces)if(f.neighbor>=0)
        {
            bool match=false;auto a=f.v;std::sort(a.begin(),a.end());
            for(auto const& other:body.pieces[f.neighbor].faces)if(other.neighbor==id){auto b=other.v;std::sort(b.begin(),b.end());if(a==b)match=true;}
            paired=paired&&match;
        }
    }
    if(maxDiameterPiece>=0){auto const& p=body.pieces[maxDiameterPiece];std::printf("Largest piece %d bounds %.4f %.4f %.4f to %.4f %.4f %.4f\n",maxDiameterPiece,p.low[0],p.low[1],p.low[2],p.high[0],p.high[1],p.high[2]);}
    Check(bounded,"every hidden acceleration partition remains local");Check(casts,"every partition volume matches tetrahedral matter");Check(paired,"shared internal faces paired by vertex identity");
    Check(mass==body.initialMg&&volume==body.initialM3,"initial matter sum");int variants=0;for(int n:faceCounts)if(n)++variants;Check(variants>=3,"tetrahedron and wedge shape diversity");
    Check(std::abs(body.initialM3-G::StartingM3)<1e-10&&body.initialM3>G::LegacyInitialM3,"seeded outcrop records its deliberately enlarged granite amount");
    Check(body.high[2]>1.20&&body.high[2]<1.53&&body.low[2]<-.4&&body.high[0]-body.low[0]>3.5&&body.high[1]-body.low[1]>1.9,"four-to-five-foot broad outcrop crest with buried substrate");
    Check(G::TopHeight(G::Width*.28,G::Depth*.51)>G::TopHeight(G::Width*.58,G::Depth*.52)+.2,"left crown rises above center saddle");
    Check(G::TopHeight(G::Width*.77,G::Depth*.55)>G::TopHeight(G::Width*.58,G::Depth*.52),"right shoulder rises above center saddle");
    // The artist may move or reshape satellite crowns, so certify the visible
    // relationship rather than obsolete procedural UV coordinates: multiple
    // exposed granite islands must remain separated by the one soil heightfield.
    constexpr int surfaceCount=(G::SX+1)*(G::SY+1);std::array<unsigned char,surfaceCount> exposed{};
    int exposedSamples=0,buriedSamples=0;
    for(int y=0;y<=G::SY;++y)for(int x=0;x<=G::SX;++x)
    {
        int id=x+(G::SX+1)*y;auto const& p=body.vertices[G::VertexID(x,y,G::SZ)];double clearance=p[2]-S::Height(p[0],p[1]);
        if(clearance>.025){exposed[id]=1;++exposedSamples;}else if(clearance<-.015)++buriedSamples;
    }
    int exposedIslands=0;std::array<unsigned char,surfaceCount> visited{};std::vector<int> queue;
    for(int islandStart=0;islandStart<surfaceCount;++islandStart)if(exposed[islandStart]&&!visited[islandStart])
    {
        queue.clear();queue.push_back(islandStart);visited[islandStart]=1;size_t head=0;
        while(head<queue.size())
        {
            int id=queue[head++],x=id%(G::SX+1),y=id/(G::SX+1);
            for(auto step:{std::array<int,2>{-1,0},std::array<int,2>{1,0},std::array<int,2>{0,-1},std::array<int,2>{0,1}})
            {
                int nx=x+step[0],ny=y+step[1];if(nx<0||ny<0||nx>G::SX||ny>G::SY)continue;
                int next=nx+(G::SX+1)*ny;if(exposed[next]&&!visited[next]){visited[next]=1;queue.push_back(next);}
            }
        }
        if(queue.size()>=2)++exposedIslands;
    }
    std::printf("Authored exposure %d samples / %d soil-buried / %d visible granite islands\n",exposedSamples,buriedSamples,exposedIslands);
    Check(exposedSamples>100&&buriedSamples>100,"authored granite remains both exposed and rooted beneath contiguous soil");
    Check(exposedIslands>=2,"soil visibly separates at least one satellite crown from the master outcrop");
    auto initialBoundaryVolume=MeshVolume(G::Boundary(body));
    std::printf("Initial boundary %.12f m3 (delta %.12g)\n",initialBoundaryVolume,initialBoundaryVolume-body.initialM3);
    Check(std::abs(initialBoundaryVolume-body.initialM3)<1e-8,"closed initial exterior volume");
    P origin={G::Width*.28,-.4,.22},direction={0,1,0};auto hit=G::Raycast(body,origin,direction);Check(hit.hit,"contact with shaped face");
    auto weak=G::Strike(body,origin,direction,400,0);Check(weak.contact.hit&&!weak.removed&&body.removedMg==0,"weak implement receipt");
    auto low=G::Strike(body,origin,direction,.01,2);Check(!low.removed&&body.pieces[hit.id].damage>0,"local cumulative damage");
    auto near=G::Raycast(body,P{origin[0]+.20,origin[1],origin[2]},direction);Check(near.hit&&near.id!=hit.id&&body.pieces[near.id].damage==0,"neighbor untouched");
    uint64_t removed=0;double removedVolume=0;int releases=0;G::Poly last;std::vector<S::FallingChip> falling;
    for(int i=0;i<35;++i)
    {
        auto r=G::Strike(body,origin,direction);if(!r.removed)continue;++releases;removed+=r.chip.massMg;removedVolume+=r.chip.volume;last=r.chip.geometry;
        Check(!body.pieces[r.chip.id].alive,"removed piece no longer host matter");
        Check(std::abs(G::Volume(r.chip.geometry)-r.chip.volume)<1e-12,"recovered cast geometric volume");
        falling.push_back(S::Launch(r.chip,r.contact.normal));
    }
    Check(releases>10,"same direction continues excavating");Check(removed==body.removedMg&&removedVolume==body.removedM3,"exact removed ledger");
    auto boundary=G::Boundary(body);auto excavatedBoundaryVolume=MeshVolume(boundary);
    std::printf("Excavated boundary %.12f + removed %.12f = %.12f (delta %.12g)\n",excavatedBoundaryVolume,removedVolume,excavatedBoundaryVolume+removedVolume,excavatedBoundaryVolume+removedVolume-body.initialM3);
    Check(std::abs(excavatedBoundaryVolume+removedVolume-body.initialM3)<1e-8,"excavated boundary plus chips equals original solid");
    uint64_t remaining=0;for(auto const& p:body.pieces)if(p.alive)remaining+=p.massMg;Check(remaining+removed==body.initialMg,"remaining plus recovered integer mass");
    // Direct exhaustive mesh intersection is independent of spatial bins.
    for(int i=0;i<128;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));P o={.65+double(h&255)/255*2.25,-.25,.18+double((h>>8)&255)/255*.10};
        P d=G::Unit(P{double(int((h>>16)&255)-127)/1000,1,double(int((h>>24)&255)-127)/1000});
        double distance=1e30;for(auto const& t:boundary)distance=std::min(distance,G::Intersect(t.a,t.b,t.c,o,d));auto contact=G::Raycast(body,o,d);
        Check(contact.hit==(distance<=2),"bin ray agrees with visible boundary");if(contact.hit)Check(std::abs(contact.distance-distance)<1e-7,"visible cavity hit distance");
    }
    Check(!G::BlocksCapsule(body,P{1.4,-.5,0}),"clear ground walk");Check(G::BlocksCapsule(body,P{1.4,.6,0}),"rock blocks walking");
    Check(!G::BlocksCapsule(body,P{1.4,1,body.high[2]+.05}),"clear standing above stone");
    for(int frame=0;frame<720;++frame)for(auto& chip:falling)S::Advance(chip,body,1./60);
    for(auto const& chip:falling)
    {
        Check(chip.settled,"released chip reaches stable support");
        if(!chip.settled)std::printf("unsettled offset %.3f %.3f %.3f velocity %.3f %.3f %.3f lift %.3f\n",chip.offset[0],chip.offset[1],chip.offset[2],chip.velocity[0],chip.velocity[1],chip.velocity[2],S::RequiredLift(chip.chip.geometry,chip.offset));
        Check(S::RequiredLift(chip.chip.geometry,chip.offset)<=1e-8,"settled cast never penetrates rendered soil");
        Check(G::Finite(chip.offset),"finite debris pose");
    }
    G::Body approachBody(body.seed);
    for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
    {
        P o={G::Width*.28,G::Depth*.51,.20},d={};d[axis]=sign;o[axis]=sign>0?approachBody.low[axis]-.15:approachBody.high[axis]+.15;
        auto r=G::Strike(approachBody,o,d);Check(r.contact.hit,"all six approach directions register material contact");
    }
    // Continuous terrain: no skipped triangles; samples agree with their plane.
    for(int y=0;y<S::NY;++y)for(int x=0;x<S::NX;++x)
    {
        auto q=S::Quad(x,y);double h=0;P p=G::Mul(G::Add(G::Add(q[0],q[1]),q[2]),1./3);
        Check(S::Surface(p[0],p[1],h)&&std::abs(h-p[2])<1e-10,"continuous soil triangle / walking agreement");
    }
    Check(!last.empty(),"fragment available for settling test");
    if(!last.empty())for(int i=0;i<60;++i)
    {
        P shift={-2+double(i%10)*.5,-1+double(i/10)*.45,-2};double lift=S::RequiredLift(last,shift);Check(lift> -1e20,"ground support exists across chip footprint");
        shift[2]+=lift+.002;Check(std::abs(S::RequiredLift(last,shift)+.002)<1e-9,"whole fragment settles above soil triangles");
    }
    if(argc>2)
    {
        for(int i=0;i<400;++i)
        {
            uint32_t h=G::Hash(uint32_t(i));P o={.65+2.25*double(h&255)/255,-.35,.18+.10*double((h>>8)&255)/255};G::Strike(body,o,P{0,1,0});
        }
        Export(body,argv[2]);
    }
    for(uint32_t seed:{0u,1u,12648430u,1592594996u})
    {
        body.Reset(seed);double maxPiece=0;
        for(auto const& piece:body.pieces)
        {
            std::vector<int> vertices;for(auto const& tet:piece.tets)for(int v:tet)if(std::find(vertices.begin(),vertices.end(),v)==vertices.end())vertices.push_back(v);
            for(size_t a=0;a<vertices.size();++a)for(size_t b=a+1;b<vertices.size();++b){P d=G::Add(body.vertices[vertices[a]],G::Mul(body.vertices[vertices[b]],-1));maxPiece=std::max(maxPiece,std::sqrt(G::Dot(d,d)));}
        }
        // These are hidden acceleration cells, never harvest templates. The
        // actual contact cutter remains pick-scale and independently bounded.
        Check(maxPiece<=G::MaxPartitionDiameter,"every acceleration partition bounded across additional seeds");
        std::printf("seed %u maximum piece %.4f inches\n",seed,maxPiece/.0254);
        Check(std::abs(MeshVolume(G::Boundary(body))-body.initialM3)<1e-8,"closed shaped host across additional seeds");
    }
    std::printf("Outcrop V2: %d checks / %d failures; diameter %.3f..%.3f inches; init %.1f ms\n",checks,failures,minDiameter/.0254,maxDiameter/.0254,initMs);
    return failures?1:0;
}
