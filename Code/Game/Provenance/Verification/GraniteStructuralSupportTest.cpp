#define NOMINMAX
#include "../Geometry/GraniteStructuralSupport.h"
#include "../Geometry/GraniteContactDebris.h"
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#endif
namespace S=EE::GraniteStructuralSupport;
namespace G=EE::GraniteContactCast;
int checks=0,failures=0;
void Check(bool ok,char const* message){++checks;if(!ok){++failures;std::printf("FAIL %s\n",message);}}
double PrivateMiB(){
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};counters.cb=sizeof(counters);if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))return double(counters.PrivateUsage)/1048576;
#endif
    return -1;
}
G::Body Fixture(std::vector<std::pair<G::P,G::P>> const& boxes)
{
    G::Body body(false);body.substrate.pieces.resize(boxes.size());std::map<std::pair<int,long long>,int> tokens;int nextToken=1,id=0;
    // Synthetic original-interface identities. Every solid is explicitly
    // changed, so these facets supply identity metadata, not geometry.
    for(size_t i=0;i<boxes.size();++i){body.substrate.pieces[i].faces.resize(16);for(auto& face:body.substrate.pieces[i].faces)face.neighbor=int((i+1)%boxes.size());}
    for(auto const& box:boxes){G::Cell cell;auto poly=G::Box(box.first,box.second);int face=0;for(auto& f:poly){int axis=face/2;auto key=std::make_pair(axis,std::llround(f.p[0][axis]*1e9));auto token=tokens.emplace(key,nextToken);if(token.second)++nextToken;f.sign=token.first->second;++face;}cell.solids.push_back(poly);cell.surface=poly;cell.query.Build(cell.surface);body.initialM3+=G::Volume(poly);body.substrate.pieces[id].low=box.first;body.substrate.pieces[id].high=box.second;body.renderGroups[G::RenderPatchID(body.substrate.pieces[id])].push_back(id);body.changed[id++]=std::move(cell);}
    body.initialMg=uint64_t(std::llround(body.initialM3*G::Density*1e6));return body;
}
void CheckBoundaryExpansion(G::Body const& body,G::Transaction const& tx)
{
    auto compact=S::Build(body,tx,{},false),full=S::Build(body,tx,{},true);
    S::CompleteBoundary(body,compact);
    bool same=compact.originalNode==full.originalNode&&compact.nodes.size()==full.nodes.size()&&compact.ports.size()==full.ports.size()&&compact.portals.size()==full.portals.size();
    if(!same)std::printf("boundary sizes compact %zu/%zu/%zu full %zu/%zu/%zu\n",compact.nodes.size(),compact.ports.size(),compact.portals.size(),full.nodes.size(),full.ports.size(),full.portals.size());
    for(size_t i=0;same&&i<full.nodes.size();++i){auto const& a=compact.nodes[i];auto const& b=full.nodes[i];same=a.volume==b.volume&&a.moment==b.moment&&a.anchor==b.anchor&&a.cell==b.cell&&a.solid==b.solid&&a.fractured==b.fractured;}
    for(size_t i=0;same&&i<full.ports.size();++i){auto const& a=compact.ports[i];auto const& b=full.ports[i];same=a.node==b.node&&a.cell==b.cell&&a.face->p==b.face->p&&a.face->sign==b.face->sign&&a.face->axis==b.face->axis&&a.n==b.n&&a.low==b.low&&a.high==b.high&&a.portals==b.portals;if(!same)std::printf("boundary port mismatch %zu cell %d/%d portal IDs equal %d normal equal %d\n",i,a.cell,b.cell,int(a.portals==b.portals),int(a.n==b.n));}
    for(size_t i=0;same&&i<full.portals.size();++i){auto const& a=compact.portals[i];auto const& b=full.portals[i];same=a.a==b.a&&a.b==b.b&&a.polygon==b.polygon;}
    Check(same,"expanded compact boundary exactly matches full rebuild");
}
G::Transaction Release(G::Body const& body)
{G::Transaction tx;tx.revision=body.revision;tx.receipt.removed=true;tx.receipt.chip.id=0;CheckBoundaryExpansion(body,tx);S::Apply(body,tx,{},false);return tx;}
void CheckTransaction(G::Body const& body,G::Transaction const& tx)
{
    double released=tx.receipt.chip.volume;uint64_t mass=tx.receipt.chip.massMg;
    for(auto const& chip:tx.receipt.detached){released+=chip.volume;mass+=chip.massMg;Check(std::abs(G::Volume(chip.geometry)+chip.absorbedM3-chip.volume)<1e-10,"released body closed volume plus explicit fines");}
    double remaining=0,surface=0;for(auto const& entry:body.changed){auto it=std::find_if(tx.updates.begin(),tx.updates.end(),[&](auto const& v){return v.first==entry.first;});auto const& cell=it==tx.updates.end()?entry.second:it->second;for(auto const& poly:cell.solids)remaining+=G::Volume(poly);surface+=G::Volume(cell.surface);}
    Check(std::abs(remaining+released-body.initialM3)<1e-10,"support release conserves owned volume");
    Check(mass==uint64_t(std::llround(released*G::Density*1e6)),"support release milligram ledger");
    Check(std::strstr(tx.receipt.status,"REFUSED")==nullptr,"support boundary accepted");
}
void LargeOutcropIsland()
{
    // A known closed excavation shell isolates many original tetrahedra at
    // once. This is a support regression fixture, not a pickaxe removal rule.
    G::Body body;auto const& base=body.substrate;std::vector<bool> cut(base.pieces.size(),false);G::P center={G::Width*.28,G::Depth*.5,.45};
    for(size_t id=0;id<cut.size();++id){G::P p=G::Mul(G::Add(base.pieces[id].low,base.pieces[id].high),.5);double r=0;for(int k=0;k<3;++k)r=std::max(r,std::abs(p[k]-center[k]));cut[id]=r>.25&&r<.55;}
    G::Transaction tx;tx.receipt.removed=true;
    for(int id=0;id<int(cut.size());++id)
    {
        if(cut[id]){tx.updates.emplace_back(id,G::Cell{});tx.receipt.chip.volume+=base.pieces[id].volume;for(auto const& f:base.pieces[id].faces)if(f.neighbor<0||!cut[f.neighbor])tx.receipt.chip.geometry.push_back({{base.vertices[f.v[0]],base.vertices[f.v[1]],base.vertices[f.v[2]]},G::Fresh,0});}
        else{bool boundary=false;for(auto const& f:base.pieces[id].faces)if(f.neighbor>=0&&cut[f.neighbor])boundary=true;if(boundary){auto cell=G::OriginalCell(body,id);for(auto const& f:base.pieces[id].faces)if(f.neighbor>=0&&cut[f.neighbor])cell.surface.push_back({{base.vertices[f.v[0]],base.vertices[f.v[1]],base.vertices[f.v[2]]},G::Fresh,0});cell.query.Build(cell.surface);tx.updates.emplace_back(id,std::move(cell));}}
    }
    Check(std::abs(G::Volume(tx.receipt.chip.geometry)-tx.receipt.chip.volume)<1e-8,"known shell excavation is closed");CheckBoundaryExpansion(body,tx);S::Apply(body,tx,{},false);
    // Selecting a shell of whole tetrahedral parcels can also isolate small
    // boundary shards. The connected central body must stay intact regardless.
    size_t large=0;for(auto const& chip:tx.receipt.detached)if(chip.massMg>5000000)++large;
    Check(large>=1,"large real outcrop central island released intact");Check(std::strstr(tx.receipt.status,"REFUSED")==nullptr,"large real outcrop boundary validates");
    auto receipt=G::Commit(body,std::move(tx));double volume=0;for(auto const& t:G::Boundary(body))volume+=G::Dot(t.a,G::Cross(t.b,t.c))/6;
    Check(std::abs(volume+body.removedM3-body.initialM3)<1e-7,"large real release leaves closed conserved host");
    for(auto const& chip:receipt.detached){Check(std::abs(G::Volume(chip.geometry)+chip.absorbedM3-chip.volume)<1e-8,"large real body preserves its exact cast");if(chip.massMg>5000000){auto falling=EE::GraniteContactDebris::Launch(chip,{0,-1,0});falling.offset[2]=2;falling.watchdogPosition=G::Add(falling.chip.center,falling.offset);falling.velocity={};falling.spin={};EE::GraniteContactDebris::Advance(falling,body,.05);Check(falling.offset[2]<1.995,"released large body begins falling under gravity");}}
}
int main(int argc,char** argv)
{
    {
        double minimum=1e30,maximum=0;G::P minimumExtent={};
        for(uint32_t seed=0;seed<4096;++seed)
        {
            auto cutter=G::MakeCutter(seed,{0,0,0},{0,1,0},400);auto shape=G::Box(cutter.low,cutter.high);
            for(auto const& plane:cutter.planes)shape=G::Clip(shape,plane);
            double volume=G::Volume(shape);if(volume<minimum){minimum=volume;G::P low={1e30,1e30,1e30},high={-1e30,-1e30,-1e30};for(auto const& face:shape)for(auto p:face.p)for(int k=0;k<3;++k){low[k]=std::min(low[k],p[k]);high[k]=std::max(high[k],p[k]);}minimumExtent=G::Add(high,G::Mul(low,-1));}maximum=std::max(maximum,volume);
        }
        std::printf("400 J cutter sample %.2f..%.2f cm3, minimum extents %.1f x %.1f x %.1f mm\n",minimum*1e6,maximum*1e6,minimumExtent[0]*1e3,minimumExtent[1]*1e3,minimumExtent[2]*1e3);Check(S::Calibration{}.minimumPickChipM3<minimum,"sub-pick remnant cutoff stays below sampled valid pick geometry");
    }
    {
        std::vector<S::Port> ports(300);std::vector<int> ids;std::vector<std::pair<int,int>> expected,actual;
        for(int i=0;i<300;++i){ids.push_back(i);uint32_t h=G::Hash(uint32_t(i));double x=double(h%31)*.1,y=double((h>>8)%11)*.1;ports[i].low={x,y,0};ports[i].high={x+.15,y+.15,0};}
        for(int a=0;a<300;++a)for(int b=a+1;b<300;++b){bool overlap=true;for(int k=0;k<3;++k)if(ports[a].low[k]>ports[b].high[k]+1e-10||ports[b].low[k]>ports[a].high[k]+1e-10)overlap=false;if(overlap)expected.emplace_back(a,b);}
        S::OverlappingPairs(ids,ports,[&](int a,int b){actual.emplace_back(a,b);});std::sort(actual.begin(),actual.end());
        Check(actual==expected,"support bounds sweep exactly matches all-pairs overlap search");
    }
    {
        G::Body rooted;auto graph=S::Build(rooted,{});
        Check(std::any_of(graph.nodes.begin(),graph.nodes.end(),[](auto const& node){return node.anchor;}),"buried natural outcrop retains a structural root below world zero");
        auto first=S::PrepareStrike(rooted,{G::Width*.28,-.4,.25},{0,1,0});
        Check(first.receipt.removed,"first natural-outcrop strike removes a pick-scale chip");
        Check(first.receipt.detached.empty(),"first natural-outcrop strike cannot release the rooted host body");
        auto receipt=G::Commit(rooted,std::move(first));
        Check(receipt.chip.volume>0&&receipt.chip.volume<.001&&rooted.removedM3<.001,"first strike leaves a visible cavity and the bulk host in place");
    }
    auto unsupported=Fixture({{{0,0,0},{.2,.2,.1}},{{0,0,.2},{.2,.2,.3}}});auto tx=Release(unsupported);Check(tx.receipt.detached.size()==1,"unconnected large volume falls intact");CheckTransaction(unsupported,tx);
    auto edge=Fixture({{{0,0,0},{.1,.1,.1}},{{.1,.1,.1},{.2,.2,.2}}});tx=Release(edge);Check(tx.receipt.detached.size()==1,"point contact is not structural support");CheckTransaction(edge,tx);
    auto tinyFixture=Fixture({{{0,0,0},{.2,.2,.1}},{{0,0,.2},{.005,.005,.205}}});tx=Release(tinyFixture);Check(tx.receipt.detached.empty()&&tx.receipt.chip.absorbedM3>0,"de minimis mass credited to last strike item");CheckTransaction(tinyFixture,tx);
    auto weak=Fixture({{{0,0,0},{.22,.2,.1492}},{{0,0,.1492},{.00635,.00635,.2}},{{0,0,.2},{.22,.2,.2573}}});tx=Release(weak);Check(!tx.receipt.detached.empty()&&tx.receipt.failedLinks>0,"approximately 15 lb cantilever fails quarter-inch two-inch neck");CheckTransaction(weak,tx);
    auto strong=Fixture({{{0,0,0},{.22,.2,.1492}},{{0,0,.1492},{.05,.05,.2}},{{0,0,.2},{.22,.2,.2573}}});tx=Release(strong);Check(tx.receipt.detached.empty()&&tx.receipt.chip.absorbedM3==0,"adequate neck does not collapse");
    Check(S::Utilization(.00635*.00635,{0,0,-1},{0,0,0},{.1,0,.05},6.8039)>1,"weak neck bending exceeds lab capacity");
    Check(S::Utilization(.00635*.00635,{0,0,-1},{0,0,0},{0,0,.05},6.8039)<1,"same mass axial compression differs from cantilever");
    S::Graph parallel;parallel.nodes={{0,{},true},{0,{},true},{6.8039/G::Density,G::Mul(G::P{.1,0,.2},6.8039/G::Density),false}};
    double area=.00635*.00635;parallel.edges={{0,2,area,G::Mul(G::P{0,0,.2},area),{0,0,area},false},{1,2,area,G::Mul(G::P{0,.01,.2},area),{0,0,area},false}};
    Check(S::Solve(parallel)[2]!=0,"multiple inadequate paths fail without requiring a graph bridge");
    auto finesAndBlock=Fixture({{{0,0,0},{.2,.2,.1}},{{0,0,.2},{.2,.2,.3}},{{.201,0,.2},{.206,.005,.205}}});tx=Release(finesAndBlock);Check(tx.receipt.detached.size()==1&&tx.receipt.chip.absorbedM3>0&&tx.receipt.detached[0].absorbedM3==0,"tiny remnant mass always credits the struck chip without changing falling geometry");CheckTransaction(finesAndBlock,tx);
    auto attachedRemnant=Fixture({{{0,0,0},{.2,.2,.1}},{{0,0,.1},{.02,.02,.11}}});for(auto& face:attachedRemnant.changed[1].solids[0])if(std::abs(G::Normal(face)[0])>.9){face.sign=1000;break;}tx=Release(attachedRemnant);Check(tx.receipt.detached.empty()&&tx.receipt.culledRemnants==1&&tx.receipt.chip.geometry.empty()&&tx.receipt.chip.absorbedM3>0,"fracture-created attached sub-pick remnant disappears as mass-only credit");CheckTransaction(attachedRemnant,tx);
    LargeOutcropIsland();
    // The same exact interface identity must survive clipping and pair with
    // the neighboring original volume. Never proximity-weld across a gap.
    G::Body body;auto before=S::MeasureStorage(body);size_t releases=0;double maxMs=0,totalMs=0;uint64_t ledger=0;double recovered=0;std::vector<EE::GraniteContactDebris::FallingChip> kept;
    std::printf("process private baseline %.2f MiB (standalone, not editor)\n",PrivateMiB());
    int strikes=argc>1?std::atoi(argv[1]):32;
    for(int i=0;i<strikes;++i)
    {
        uint32_t h=G::Hash(uint32_t(i));G::P origin={.78+.52*double(h&255)/255,-.4,.18+.22*double((h>>8)&255)/255};
        auto start=std::chrono::steady_clock::now();auto transaction=S::PrepareStrike(body,origin,{0,1,0},400,2,false);
        Check(std::strstr(transaction.receipt.status,"REFUSED")==nullptr,"real outcrop support boundary validates");
        auto receipt=G::Commit(body,std::move(transaction));double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();maxMs=std::max(maxMs,ms);totalMs+=ms;
        if(receipt.removed){ledger+=receipt.chip.massMg;recovered+=receipt.chip.volume;Check(std::abs(G::Volume(receipt.chip.geometry)+receipt.chip.absorbedM3-receipt.chip.volume)<1e-9,"struck chip geometry stays unchanged while remnant mass is explicit");kept.push_back(EE::GraniteContactDebris::Launch(std::move(receipt.chip),receipt.contact.normal));for(auto& chip:receipt.detached){ledger+=chip.massMg;recovered+=chip.volume;++releases;Check(std::abs(G::Volume(chip.geometry)+chip.absorbedM3-chip.volume)<1e-9,"actual detached cast volume");kept.push_back(EE::GraniteContactDebris::Launch(std::move(chip),receipt.contact.normal));}}
        Check(ledger==body.removedMg&&std::abs(recovered-body.removedM3)<1e-10,"multiple items per strike uniquely conserved");
        if(i%16==15){auto storage=S::MeasureStorage(body);std::printf("strikes %d support %.2f ms (cache %.2f graph %.2f solve/remnant %.2f) nodes %zu retained %.2f MiB solids %zu faces %zu released %zu\n",i+1,receipt.supportMs,receipt.supportCacheMs,receipt.supportGraphMs,receipt.supportSolveMs,receipt.supportNodes,double(storage.bytes)/1048576,storage.solids,storage.faces,releases);std::fflush(stdout);}
    }
    auto surface=G::Boundary(body);double volume=0;for(auto const& t:surface)volume+=G::Dot(t.a,G::Cross(t.b,t.c))/6;
    std::printf("post-release boundary %.12f + removed %.12f = %.12f (delta %.12g)\n",volume,body.removedM3,volume+body.removedM3,volume+body.removedM3-body.initialM3);
    Check(std::abs(volume+body.removedM3-body.initialM3)<1e-7,"post-release host boundary conserves full volume");
    {
        auto fast=S::Build(body,{},S::Calibration{},false),full=S::Build(body,{});
        auto order=[](S::Edge const& a,S::Edge const& b){return std::make_pair(a.a,a.b)<std::make_pair(b.a,b.b);};std::sort(fast.edges.begin(),fast.edges.end(),order);std::sort(full.edges.begin(),full.edges.end(),order);
        bool equal=fast.nodes.size()==full.nodes.size()&&fast.edges.size()==full.edges.size();
        size_t mismatch=std::numeric_limits<size_t>::max();
        for(size_t i=0;i<fast.edges.size()&&i<full.edges.size()&&equal;++i){auto const& a=fast.edges[i];auto const& b=full.edges[i];equal=a.a==b.a&&a.b==b.b&&std::abs(a.area-b.area)<1e-12;for(int k=0;k<3;++k)equal=equal&&std::abs(a.moment[k]-b.moment[k])<1e-12&&std::abs(a.normal[k]-b.normal[k])<1e-12;if(!equal)mismatch=i;}
        if(!equal)
        {
            std::printf("support graph fast %zu nodes/%zu edges; full %zu/%zu; first mismatch %zu\n",fast.nodes.size(),fast.edges.size(),full.nodes.size(),full.edges.size(),mismatch);
            if(mismatch<fast.edges.size()&&mismatch<full.edges.size())std::printf("  fast %d-%d area %.12g; full %d-%d area %.12g\n",fast.edges[mismatch].a,fast.edges[mismatch].b,fast.edges[mismatch].area,full.edges[mismatch].a,full.edges[mismatch].b,full.edges[mismatch].area);
            std::map<std::pair<int,int>,double> fastAreas,fullAreas;
            for(auto const& e:fast.edges)fastAreas[{e.a,e.b}]+=e.area;for(auto const& e:full.edges)fullAreas[{e.a,e.b}]+=e.area;
            int shown=0;for(auto const& entry:fullAreas)
            {
                auto found=fastAreas.find(entry.first);double fastArea=found==fastAreas.end()?0:found->second;
                if(std::abs(fastArea-entry.second)>1e-12&&shown++<5)
                {
                    auto const& a=full.nodes[entry.first.first];auto const& b=full.nodes[entry.first.second];
                    std::printf("  missing/different %d-%d cells %d/%d solids %d/%d area fast %.12g full %.12g\n",entry.first.first,entry.first.second,a.cell,b.cell,a.solid,b.solid,fastArea,entry.second);
                }
            }
        }
        Check(equal,"cached lightweight support graph matches full geometric contact graph");
    }
    CheckBoundaryExpansion(body,{});
    auto storage=S::MeasureStorage(body);std::printf("worker+commit mean %.2f max %.2f ms; retained %.2f MiB; releases %zu\n",totalMs/strikes,maxMs,double(storage.bytes)/1048576,releases);
    std::printf("all %zu moving/settled-capable bodies retained: process private %.2f MiB\n",kept.size(),PrivateMiB());
    std::vector<EE::GraniteContactDebris::FallingChip>().swap(kept);std::vector<G::Triangle>().swap(surface);body.Reset(body.seed);Check(S::MeasureStorage(body).bytes==before.bytes,"reset clears all modified geometry ownership");
    std::printf("after reset process private %.2f MiB (allocator caches may remain)\n",PrivateMiB());
    std::printf("%d checks %d failures\n",checks,failures);return failures?1:0;
}
