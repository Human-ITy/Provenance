#include "../Geometry/PlayableLandscape.h"
#include "../Geometry/TerrainSeam.h"
#include "../Geometry/GraniteStructuralSupport.h"
#include "../Geometry/SoilScoop.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>

namespace G=EE::GraniteContactCast;namespace S=EE::SoilScoop;namespace L=EE::PlayableLandscape;
using Clock=std::chrono::steady_clock;
static double Ms(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
static void Check(bool value,char const* label){if(!value){std::printf("FAIL %s\n",label);std::exit(1);}}
// Frozen pre-optimization implementation for exact differential validation.
static G::SurfaceQuery ReferenceCollision(G::Body const& body,std::vector<std::pair<int,G::Cell>> const& updates)
{
    G::SurfaceQuery query;
    for(int id=0;id<int(body.substrate.pieces.size());++id)
    {
        auto update=std::find_if(updates.begin(),updates.end(),[id](auto const& value){return value.first==id;});
        if(update!=updates.end()){query.Append(update->second.surface);continue;}
        auto changed=body.changed.find(id);if(changed!=body.changed.end()){query.Append(changed->second.surface);continue;}
        for(auto const& face:body.substrate.pieces[id].faces)if(face.neighbor<0)
            query.Append(body.substrate.vertices[face.v[0]],body.substrate.vertices[face.v[1]],body.substrate.vertices[face.v[2]]);
    }
    query.Finish();return query;
}
static void SameCollision(G::SurfaceQuery const& a,G::SurfaceQuery const& b)
{
    Check(a.triangles.size()==b.triangles.size()&&a.nodes.size()==b.nodes.size(),"collision sizes unchanged");
    for(size_t i=0;i<a.triangles.size();++i){auto const& x=a.triangles[i];auto const& y=b.triangles[i];Check(x.a==y.a&&x.b==y.b&&x.c==y.c&&x.n==y.n,"collision triangles and normals exactly unchanged");}
    for(size_t i=0;i<a.nodes.size();++i){auto const& x=a.nodes[i];auto const& y=b.nodes[i];Check(x.lo==y.lo&&x.hi==y.hi&&x.left==y.left&&x.right==y.right&&x.begin==y.begin&&x.end==y.end,"collision hierarchy exactly unchanged");}
}
int main()
{
    // Seam presentation must not alter the preserved editable footprint.
    for(int i=0;i<=100;++i)
    {
        double t=i/100.;
        for(auto p:{G::P{L::Core::X,L::Core::Y+t*L::Core::NY*L::Core::Step,0},G::P{L::Core::X+L::Core::NX*L::Core::Step,L::Core::Y+t*L::Core::NY*L::Core::Step,0},G::P{L::Core::X+t*L::Core::NX*L::Core::Step,L::Core::Y,0},G::P{L::Core::X+t*L::Core::NX*L::Core::Step,L::Core::Y+L::Core::NY*L::Core::Step,0}})
            Check(EE::TerrainSeam::BladeBlend(p[0],p[1])==0,"no blade wall at perimeter");
    }
    Check(EE::TerrainSeam::BladeBlend(1,1)==1,"interior grass unchanged");
    Check(EE::TerrainSeam::BuriedPerimeter({-4,-4,.11},{-4,-3,.11},{-4,-3,-.12}),"buried side excluded from rendering");
    Check(!EE::TerrainSeam::BuriedPerimeter({-4,-4,.11},{-3,-4,.11},{-3,-3,.11}),"top surface retained");
    Check(!EE::TerrainSeam::BuriedPerimeter({-4,-4,.3},{-4,-3,.11},{-4,-3,-.12}),"exposed raised face retained");
    std::puts("Seam presentation checks PASS");
    G::Body rock;
    std::puts("kind,index,prepare_ms,support_ms,commit_ms,patches,removed_mg");
    for(int i=0;i<8;++i)
    {
        auto start=Clock::now();auto tx=EE::GraniteStructuralSupport::PrepareStrike(rock,{1.15,-.4,.3},{0,1,0});double prepare=Ms(start);
        double support=tx.receipt.supportMs;size_t patches=tx.renderUpdates.size();start=Clock::now();auto r=G::Commit(rock,std::move(tx));double commit=Ms(start);
        Check(r.contact.hit,"pick baseline has contact");
        std::printf("pick,%d,%.3f,%.3f,%.3f,%zu,%llu\n",i,prepare,support,commit,patches,(unsigned long long)rock.removedMg);std::fflush(stdout);
    }
    // Repeat the same evolving fixture with explicit phases. The production
    // support wrapper calls these same operations in this same order.
    G::Body profiled;
    G::PreparationProfile early;early.complete=true;early.cutMs=1;
    auto rejected=G::PrepareStrike(profiled,{1.15,-.4,.3},{0,1,0},400,1,&early);
    Check(!rejected.receipt.removed&&!early.complete&&early.cutMs==0,"early refusal resets profile and reports incomplete phases");
    SameCollision(G::UpdatedCollisionQuery(profiled,{}),ReferenceCollision(profiled,{}));
    std::puts("phase,index,complete,cut_ms,patch_ms,collision_ms,support_ms,cache_ms,graph_ms,solve_ms,storage_ms,commit_ms,nodes,modified_bytes");
    for(int i=0;i<8;++i)
    {
        G::PreparationProfile p;
        auto revision=profiled.revision;auto mass=profiled.removedMg;
        auto tx=G::PrepareStrike(profiled,{1.15,-.4,.3},{0,1,0},400,2,&p);
        auto referenceStart=Clock::now();auto reference=ReferenceCollision(profiled,tx.updates);double referenceMs=Ms(referenceStart);
        SameCollision(tx.collisionQuery,reference);
        std::printf("collision_reference,%d,reference_ms,%.3f,profiled_ms,%.3f,assembly_ms,%.3f,tree_ms,%.3f,triangles,%zu\n",i,referenceMs,p.collisionMs,p.collision.assemblyMs,p.collision.treeMs,reference.triangles.size());
        auto start=Clock::now();EE::GraniteStructuralSupport::Apply(profiled,tx);double support=Ms(start);
        start=Clock::now();auto storage=EE::GraniteStructuralSupport::MeasureStorage(profiled,tx.receipt.removed?&tx:nullptr);double storageMs=Ms(start);
        Check(profiled.revision==revision&&profiled.removedMg==mass,"preparation leaves source revision and ledger unchanged");
        start=Clock::now();auto r=G::Commit(profiled,std::move(tx));double commit=Ms(start);
        Check(r.contact.hit,"profiled strike has contact");
        Check(!r.removed||p.complete,"successful cut has all timing phases");
        std::printf("phase,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%zu,%zu\n",i,int(p.complete),p.cutMs,p.patchMs,p.collisionMs,support,r.supportCacheMs,r.supportGraphMs,r.supportSolveMs,storageMs,commit,r.supportNodes,storage.bytes);std::fflush(stdout);
    }
    Check(profiled.revision==rock.revision&&profiled.removedMg==rock.removedMg&&profiled.removedM3==rock.removedM3,"profile observer preserves exact production ledger");
    auto a=G::Boundary(rock),b=G::Boundary(profiled);Check(a.size()==b.size(),"profile observer preserves boundary size");
    for(size_t i=0;i<a.size();++i)Check(a[i].a==b[i].a&&a[i].b==b[i].b&&a[i].c==b[i].c&&a[i].na==b[i].na&&a[i].nb==b[i].nb&&a[i].nc==b[i].nc,"profile observer preserves boundary positions and normals");
    std::puts("Preparation observer and exact collision/boundary/ledger differential checks PASS");
    S::Body soil;soil.Reset(&rock);soil.dirty.clear();
    // Eight successive cuts at each of four sites: repeated local complexity
    // plus accumulated prior edits, not 32 resets to an easy pristine scoop.
    std::puts("dig sequence: four sites, eight successive downward scoops each");
    uint64_t soilRecovered=0;auto rockRevision=rock.revision;auto rockMass=rock.removedMg;int digs=0,emptyAttempts=0;
    for(int i=0;i<32;++i)
    {
        G::P origin={-1.5+.8*(i/8),-1.5,2};G::Hit hit;double reach=4;soil.Trace(origin,{0,0,-1},reach,hit,false);
        if(!hit.hit)
        {
            // Finite soil can be exhausted. Independently sample the material
            // field along this column; never count a miss as a fast removal.
            double maximum=-1e30;
            for(double z=S::Bottom;z<=S::Top;z+=.001)maximum=std::max(maximum,soil.Field({origin[0],origin[1],z}));
            Check(maximum<=1e-8,"no-hit dig column has no positive sampled material");
            ++emptyAttempts;std::printf("dig_empty,%d,column_max_field,%.12g\n",i,maximum);continue;
        }
        auto tx=S::Prepare(soil,rock,hit,{0,0,-1});double prepare=tx.milliseconds,support=tx.receipt.supportMs;size_t patches=tx.terrain.dirty.size();
        Check(tx.receipt.volume>0,"repeat dig removes soil");soilRecovered+=tx.receipt.massMg;++digs;
        auto start=Clock::now();Check(S::Commit(soil,std::move(tx)),"dig commit accepted");double commit=Ms(start);soil.dirty.clear();
        Check(soil.removedMg==soilRecovered&&soil.removedMg==uint64_t(std::llround(soil.removedM3*S::BulkDensity*1e6)),"repeated digs conserve cumulative milligrams");
        Check(rock.revision==rockRevision&&rock.removedMg==rockMass,"dig sequence leaves granite untouched");
        std::printf("dig,%d,%.3f,%.3f,%.3f,%zu,%llu\n",i,prepare,support,commit,patches,(unsigned long long)soil.removedMg);std::fflush(stdout);
    }
    Check(digs>=24,"sustained dig fixture exercises at least 24 accepted removals");
    std::printf("Repeated digging checks PASS; accepted %d; empty column attempts %d (excluded from removal timings)\n",digs,emptyAttempts);
    std::puts("Headless timings exclude input queue, scheduling, GPU publication and frame presentation.");
}
