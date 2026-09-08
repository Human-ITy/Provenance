#include "../Geometry/GraniteStructuralSupport.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace G=EE::GraniteContactCast;
namespace S=EE::GraniteStructuralSupport;
using Clock=std::chrono::steady_clock;
static int checks=0;
static void Check(bool ok,char const* label){++checks;if(!ok){std::printf("FAIL %s\n",label);std::exit(1);}}
static double Ms(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
static double PendingBoundary(G::Body const& body,G::Transaction const& tx)
{
    double volume=0;std::map<int,G::Cell const*> cells;
    for(auto const& cell:body.changed)cells[cell.first]=&cell.second;
    if(tx.receipt.removed)for(auto const& cell:tx.updates)cells[cell.first]=&cell.second;
    for(int id=0;id<int(body.substrate.pieces.size());++id)
    {
        auto found=cells.find(id);G::Poly original;
        if(found==cells.end())original=G::OriginalSurface(body,id);
        auto const& surface=found==cells.end()?original:found->second->surface;
        for(auto const& f:surface)for(size_t i=1;i+1<f.p.size();++i)volume+=G::Dot(f.p[0],G::Cross(f.p[i],f.p[i+1]))/6;
    }
    return volume;
}
static void SamePoly(G::Poly const& a,G::Poly const& b)
{
    Check(a.size()==b.size(),"same polygon count");
    for(size_t i=0;i<a.size();++i)Check(a[i].p==b[i].p&&a[i].axis==b[i].axis&&a[i].sign==b[i].sign,"same ordered polygon vertices and identities");
}
static void SameMesh(std::vector<G::Triangle> const& a,std::vector<G::Triangle> const& b)
{
    Check(a.size()==b.size(),"same triangle count");
    for(size_t i=0;i<a.size();++i)Check(a[i].a==b[i].a&&a[i].b==b[i].b&&a[i].c==b[i].c&&a[i].na==b[i].na&&a[i].nb==b[i].nb&&a[i].nc==b[i].nc,"same ordered triangle positions and normals");
}
static void SameChip(G::Chip const& a,G::Chip const& b)
{
    Check(a.id==b.id&&a.massMg==b.massMg&&a.volume==b.volume&&a.absorbedM3==b.absorbedM3&&a.center==b.center,"same chip identity, mass, volume, fines and center");
    SamePoly(a.geometry,b.geometry);SameMesh(a.mesh,b.mesh);
}
static void SameTransaction(G::Transaction const& a,G::Transaction const& b)
{
    auto const& x=a.receipt;auto const& y=b.receipt;
    Check(x.removed==y.removed&&x.failedLinks==y.failedLinks&&x.culledRemnants==y.culledRemnants&&x.absorbedM3==y.absorbedM3&&std::strcmp(x.status,y.status)==0,"same release decisions and status");
    SameChip(x.chip,y.chip);Check(x.detached.size()==y.detached.size(),"same detached count");
    for(size_t i=0;i<x.detached.size();++i)SameChip(x.detached[i],y.detached[i]);
    Check(a.updates.size()==b.updates.size(),"same modified cell count");
    for(size_t i=0;i<a.updates.size();++i)
    {
        Check(a.updates[i].first==b.updates[i].first,"same modified IDs");auto const& p=a.updates[i].second;auto const& q=b.updates[i].second;
        Check(p.solids.size()==q.solids.size(),"same retained solid count");for(size_t j=0;j<p.solids.size();++j)SamePoly(p.solids[j],q.solids[j]);SamePoly(p.surface,q.surface);
    }
    Check(a.renderUpdates.size()==b.renderUpdates.size(),"same render patch count");
    for(auto const& patch:a.renderUpdates){auto found=b.renderUpdates.find(patch.first);Check(found!=b.renderUpdates.end(),"same patch IDs");SameMesh(patch.second,found->second);}
}
int main(int argc,char** argv)
{
    int count=argc>1?std::atoi(argv[1]):32;Check(count>0&&count<=128,"bounded repeated strike count");
    G::Body body;double oldTotal=0,newTotal=0;int releases=0,refusals=0;uint64_t recovered=0;bool firstVolumeDrift=true;
    std::puts("strike,reference_support_ms,reuse_support_ms,nodes,releases,fines");
    for(int i=0;i<count;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));G::P origin={.78+.52*double(h&255)/255,-.4,.18+.22*double((h>>8)&255)/255};
        auto tx=G::PrepareStrike(body,origin,{0,1,0});auto reference=tx;
        double cutResidual=PendingBoundary(body,tx)+body.removedM3+(tx.receipt.removed?tx.receipt.chip.volume:0)-body.initialM3;
        if(i==46)
        {
            auto graph=S::Build(body,tx);auto owners=S::Solve(graph);S::AssignSubPickRemnants(graph,owners);
            int zeroEdges=0;
            for(size_t pi=0;pi<graph.portals.size();++pi)
            {
                auto const& portal=graph.portals[pi];auto const& polygon=portal.polygon;
                for(size_t e=0;e<polygon.size();++e)
                {
                    auto edge=G::Add(polygon[(e+1)%polygon.size()],G::Mul(polygon[e],-1));
                    if(G::Dot(edge,edge)>1e-24)continue;++zeroEdges;
                    std::printf("DEGENERATE_PORTAL,%zu,edge,%zu,cells,%d,%d,owners,%d,%d,area,%.12g\n",pi,e,graph.ports[portal.a].cell,graph.ports[portal.b].cell,owners[graph.ports[portal.a].node],owners[graph.ports[portal.b].node],S::AreaMoment(polygon).first);
                }
            }
            std::printf("DIAGNOSTIC_ZERO_EDGES,%d\n",zeroEdges);
            auto overlap=[](G::Poly const& a,G::Poly const& b){std::vector<G::Plane> planes;for(auto const& f:b){auto n=G::Normal(f);planes.push_back({n,G::Dot(n,f.p[0])});}return G::Volume(G::Intersection(a,planes));};
            auto ca=G::OriginalCell(body,114472),cb=G::OriginalCell(body,114165);double originalOverlap=0;
            for(auto const& a:ca.solids)for(auto const& b:cb.solids)originalOverlap+=overlap(a,b);
            std::printf("ORIGINAL_CELL_OVERLAP,%.15g\n",originalOverlap);
            std::set<int> replaced;
            for(auto const& cell:graph.cellNodes)for(int n:cell.second)if(owners[n])replaced.insert(cell.first);
            for(auto const& p:graph.portals)if(owners[graph.ports[p.a].node]!=owners[graph.ports[p.b].node]){replaced.insert(graph.ports[p.a].cell);replaced.insert(graph.ports[p.b].cell);}
            std::map<int,double> oldFaces,newFaces;auto add=[](auto& sums,G::Poly const& poly){for(auto const& f:poly)for(size_t j=1;j+1<f.p.size();++j)sums[f.sign]+=G::Dot(f.p[0],G::Cross(f.p[j],f.p[j+1]))/6;};
            for(int id:replaced){auto found=graph.cells.find(id);if(found!=graph.cells.end())add(oldFaces,found->second->surface);else {auto original=G::OriginalSurface(body,id);add(oldFaces,original);add(newFaces,original);}}
            for(int p=0;p<int(graph.ports.size());++p)if(replaced.count(graph.ports[p].cell))add(newFaces,S::PortBoundary(body,graph,p,owners));
            for(auto const& face:oldFaces)newFaces[face.first]-=face.second;
            double total=0;for(auto const& face:newFaces)total+=face.second;std::printf("RECONSTRUCTION_DELTA,%.15g\n",total);
            oldFaces.clear();newFaces.clear();
            for(int p=0;p<int(graph.ports.size());++p){add(oldFaces,G::Poly{*graph.ports[p].face});add(newFaces,S::PortBoundary(body,graph,p,owners));}
            for(auto const& face:oldFaces)newFaces[face.first]-=face.second;
            for(auto const& face:newFaces)if(std::abs(face.second)>1e-10)std::printf("PORT_CANCELLATION_DRIFT,%d,delta,%.15g\n",face.first,face.second);
            for(int p=0;p<int(graph.ports.size());++p)if(graph.ports[p].face->sign==1919345)
            {
                auto const& port=graph.ports[p];double expected=S::AreaMoment(port.face->p).first,actual=0;
                for(int id:port.portals){auto const& portal=graph.portals[id];int other=portal.a==p?portal.b:portal.a;if(owners[port.node]==owners[graph.ports[other].node])expected-=S::AreaMoment(portal.polygon).first;}
                for(auto const& f:S::PortBoundary(body,graph,p,owners))actual+=S::AreaMoment(f.p).first;
                if(std::abs(actual-expected)<1e-10)continue;
                std::printf("BAD_PORT,%d,cell,%d,owner,%d,expected_area,%.15g,actual_area,%.15g\n",p,port.cell,owners[port.node],expected,actual);
                auto dump=[](char const* name,std::vector<G::P> const& ps){std::printf("%s {",name);for(auto v:ps)std::printf("{%.17g,%.17g,%.17g},",v[0],v[1],v[2]);std::puts("}");};
                dump("FACE",port.face->p);for(int id:port.portals){auto const& portal=graph.portals[id];auto const& other=graph.ports[portal.a==p?portal.b:portal.a];std::printf("OTHER_PORT,%d,cell,%d,solid,%d,normal_dot,%.15g\n",portal.a==p?portal.b:portal.a,other.cell,graph.nodes[other.node].solid,G::Dot(port.n,other.n));dump("OTHER_FACE",other.face->p);dump("MASK",portal.polygon);}
                for(auto const& f:S::PortBoundary(body,graph,p,owners))dump("RESULT",f.p);
            }
        }
        double oldMs=0,newMs=0;
        // Alternate first-run order; copies and cutting are outside timing.
        auto runOld=[&](){auto start=Clock::now();S::Apply(body,reference,{},true);oldMs=Ms(start);};
        auto runNew=[&](){auto start=Clock::now();S::Apply(body,tx,{},false);newMs=Ms(start);};
        if(i%2){runNew();runOld();}else{runOld();runNew();}
        SameTransaction(tx,reference);
        // The long reference sequence can conservatively refuse a cut or
        // support release. Do not hide it, or attribute a baseline refusal
        // to the candidate. Exact status/geometry parity is checked above.
        if(std::strstr(tx.receipt.status,"REFUSED")){++refusals;std::printf("MATCHED_REFERENCE_REFUSAL,%d,%s\n",i,tx.receipt.status);}
        if(tx.receipt.culledRemnants||!tx.receipt.detached.empty())++releases;
        oldTotal+=oldMs;newTotal+=newMs;
        std::printf("%d,%.3f,%.3f,%zu,%zu,%zu\n",i,oldMs,newMs,tx.receipt.supportNodes,tx.receipt.detached.size(),tx.receipt.culledRemnants);std::fflush(stdout);
        auto r=G::Commit(body,std::move(tx));recovered+=r.chip.massMg;for(auto const& chip:r.detached)recovered+=chip.massMg;
        Check(recovered==body.removedMg,"cumulative chip identities conserve milligrams");
        // Diagnostic only here, outside performance timings. Retain the final
        // strict failure so extended sequences cannot silently bless drift.
        double boundary=0;for(auto const& t:G::Boundary(body))boundary+=G::Dot(t.a,G::Cross(t.b,t.c))/6;
        double residual=boundary+body.removedM3-body.initialM3;
        if(firstVolumeDrift&&std::abs(residual)>=1e-7){std::printf("FIRST_VOLUME_DRIFT,%d,cut_residual_m3,%.15g,after_support_residual_m3,%.15g\n",i,cutResidual,residual);firstVolumeDrift=false;}
    }
    double volume=0;for(auto const& t:G::Boundary(body))volume+=G::Dot(t.a,G::Cross(t.b,t.c))/6;
    std::printf("TOTAL reference_support_ms %.3f reuse_support_ms %.3f; refusals %d; checks %d\n",oldTotal,newTotal,refusals,checks);
    std::printf("VOLUME boundary %.15g recovered %.15g initial %.15g residual %.15g\n",volume,body.removedM3,body.initialM3,volume+body.removedM3-body.initialM3);
    Check(std::abs(volume+body.removedM3-body.initialM3)<1e-7,"closed host plus recovered volume conserved");
    Check(releases>0,"sequence exercises boundary expansion");
    std::printf("PASS %d differential checks; %d strikes; %d release events; %d matched reference refusals; reference support %.3f ms; reuse %.3f ms\n",checks,count,releases,refusals,oldTotal,newTotal);
    std::puts("CPU support only; no claim about end-to-end action latency or GPU publication.");
}
