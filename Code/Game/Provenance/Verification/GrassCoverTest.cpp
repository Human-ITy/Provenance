#include "../Geometry/GrassCover.h"
#include <cstdio>
#include <cstdlib>
namespace C=EE::GrassCover;namespace S=EE::SoilScoop;namespace G=EE::GraniteContactCast;
int checks=0;void Check(bool value,char const* what){++checks;if(!value){std::printf("FAIL %s\n",what);std::exit(1);}}
int main()
{
    C::Model cover;cover.Reset();Check(cover.cells.size()==16384,"bounded coverage grid");
    for(auto const& c:cover.cells)Check(c.cover==1&&std::isfinite(c.height),"initial intact soil covered");
    G::Body seededRock;S::Body seededSoil;seededSoil.Reset(&seededRock);size_t emitted=0,rejected=0;double shortest=1,tallest=0,minVigor=1,maxVigor=0;
    for(int i=0;i<int(cover.cells.size());++i){auto tuft=cover.Blade(i);if(!tuft.emitted)continue;++emitted;if(!C::BaseClear(*seededSoil.originalRock,tuft))++rejected;shortest=std::min(shortest,tuft.height);tallest=std::max(tallest,tuft.height);minVigor=std::min(minVigor,tuft.vigor);maxVigor=std::max(maxVigor,tuft.vigor);}
    Check(emitted>7000&&rejected>50,"seeded grass evaluates jittered crossed-card roots against granite");
    Check(shortest>=.3048-1e-12&&tallest<=.4572+1e-12&&tallest-shortest>.10,"habitat-varying tufts remain twelve to eighteen inches tall");
    Check(minVigor<.20&&maxVigor>.85,"deterministic slope and moisture fields produce useful habitat variation");
    C::Model margin,marginRepeat;margin.Reset();marginRepeat.Reset();
    margin.ExposeRockMargin(*seededSoil.originalRock);marginRepeat.ExposeRockMargin(*seededSoil.originalRock);
    int exposedMargin=0,softMargin=0,intactHabitat=0;
    for(size_t i=0;i<margin.cells.size();++i)
    {
        Check(margin.cells[i].cover==marginRepeat.cells[i].cover,"rock-edge habitat margin is deterministic");
        if(margin.cells[i].cover<1){++exposedMargin;if(margin.cells[i].cover>0)++softMargin;}else ++intactHabitat;
    }
    Check(exposedMargin>50&&softMargin>10&&intactHabitat>12000,"patchy mineral pockets remain local and softly graded without a uniform moat");
    Check(margin.active.empty()&&margin.pending.empty(),"initial rock-edge habitat margin is not recorded as excavation or wear");
    double terrainLow=1e30,terrainHigh=-1e30,largestStep=0;
    for(int y=0;y<S::S::NY;++y)for(int x=0;x<S::S::NX;++x)
    {
        auto q=S::S::Quad(x,y);for(auto p:q){terrainLow=std::min(terrainLow,p[2]);terrainHigh=std::max(terrainHigh,p[2]);}
        largestStep=std::max({largestStep,std::abs(q[1][2]-q[0][2]),std::abs(q[3][2]-q[0][2])});
    }
    std::printf("rolling field %.3f..%.3f m (%.3f m range), largest 12 cm rise %.3f m\n",terrainLow,terrainHigh,terrainHigh-terrainLow,largestStep);
    Check(terrainHigh-terrainLow>.70&&terrainHigh-terrainLow<1.10,"rolling field has a bounded few feet of relief");
    Check(largestStep<.11,"rolling field remains smoothly walkable between heightfield samples");
    double bankLow=1e30,bankHigh=-1e30;
    for(double x=.18;x<G::Width-.18;x+=.37)for(double y:{.08,G::Depth-.08}){double z=S::S::Height(x,y);bankLow=std::min(bankLow,z);bankHigh=std::max(bankHigh,z);}
    for(double y=.18;y<G::Depth-.18;y+=.29)for(double x:{.08,G::Width-.08}){double z=S::S::Height(x,y);bankLow=std::min(bankLow,z);bankHigh=std::max(bankHigh,z);}
    Check(bankHigh-bankLow>.035&&bankHigh>.30,"real soil height banks vary around the granite perimeter");
    double leftFan=S::S::Height(.72,.72),leftFanNorth=S::S::Height(.72,1.04),leftFanSouth=S::S::Height(.72,.40);
    std::printf("left satellite soil fan %.3f m; flanks %.3f / %.3f m\n",leftFan,leftFanNorth,leftFanSouth);
    Check(leftFan>leftFanNorth+.05&&leftFan>leftFanSouth+.05,"localized soil fan climbs into the left satellite crease instead of forming a uniform band");
    int center=64+64*C::Resolution;auto p=cover.Position(center);p[2]=cover.cells[center].height;
    auto a=G::Add(p,{-.35,0,0}),b=G::Add(p,{.35,0,0});
    cover.Travel(a,b,false);cover.Travel(a,a,true);cover.Travel(a,G::Add(a,{2,0,0}),true);
    Check(cover.cells[center].cover==1,"airborne idle and teleport motion do not wear grass");
    cover.Travel(a,b,true);float first=cover.cells[center].cover;
    Check(first<1&&first>.8,"one crossing only partially wears grass");
    C::Model fine;fine.Reset();for(int i=0;i<70;++i)fine.Travel(G::Add(a,{i*.01,0,0}),G::Add(a,{(i+1)*.01,0,0}),true);
    Check(std::abs(fine.cells[center].cover-first)<.002,"wear approximately independent of frame subdivision");
    for(int i=0;i<30;++i)cover.Travel(a,b,true);
    Check(cover.cells[center].cover==0,"repeated traffic exposes soil");
    cover.Advance(119);Check(cover.cells[center].cover==0,"regrowth delay honored");
    cover.Advance(601);Check(std::abs(cover.cells[center].cover-.5)<1e-5,"gradual simulation-time recovery");
    cover.Travel(a,b,true);float worn=cover.cells[center].cover;cover.Advance(120);
    Check(cover.cells[center].cover==worn,"new traffic resets recovery delay");
    cover.Advance(1200);Check(cover.cells[center].cover==1,"abandoned path recovers fully");
    auto pixels=cover.Pixels(.12f);Check(pixels.size()==16384&&std::abs(pixels[center][1]-p[2]-.12)<1e-6&&pixels[center][2]==1,"coverage carries world height and eligibility");
    // Excavate at a map texel so the new topmost surface is unambiguous.
    S::Body soil;G::Body rock(false);soil.Reset();soil.dirty.clear();G::Hit hit;double reach=4;
    soil.Trace({p[0],p[1],S::Top+.01},{0,0,-1},reach,hit);Check(hit.hit,"soil target exists");
    auto result=S::Excavate(soil,rock,hit,{0,0,-1});Check(result.volume>0,"scoop removes volume");
    auto volume=soil.removedM3;cover.QueueTerrain(soil);Check(!cover.pending.empty(),"dig queues localized cover refresh");
    auto count=cover.pending.size();cover.Refresh(soil,1);Check(cover.pending.size()==count-1,"refresh obeys frame budget");
    while(!cover.pending.empty())cover.Refresh(soil);
    Check(cover.cells[center].cover==0&&cover.cells[center].height<p[2]-.02,"new excavated floor starts bare at new height");
    Check(cover.cells[center].eligible==1,"open upward-facing soil can recover");
    auto floor=cover.cells[center].height;cover.Advance(1320);
    Check(cover.cells[center].cover==1&&cover.cells[center].height==floor&&soil.removedM3==volume,"regrowth cannot refill holes or change soil accounting");
    // Publishing an otherwise untouched fine volume chunk changes triangle
    // interpolation slightly. It must not clear a rectangular grass footprint.
    C::Model remeshedCover;remeshedCover.Reset();S::Body remeshed;remeshed.Reset();remeshed.dirty.clear();
    S::Key key={int(std::floor((p[0]-S::S::X)/S::Span)),int(std::floor((p[1]-S::S::Y)/S::Span)),int(std::floor((p[2]-S::Bottom)/S::Span))};
    auto chunk=std::make_shared<S::Chunk>();chunk->key=key;
    for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x)chunk->values[S::Index(x,y,z)]=remeshed.Initial(S::Point(key,x,y,z));
    chunk->volume=chunk->Volume();chunk->Label();remeshed.chunks[key]=chunk;remeshed.queries[S::ID(key)]=std::make_shared<G::SurfaceQuery>(chunk->Mesh());
    int patch=S::Body::Patch(key);auto patchQuery=std::make_shared<G::SurfaceQuery>();patchQuery->Build(remeshed.PatchFaces(patch));remeshed.queries[patch]=patchQuery;remeshed.dirty.insert(S::ID(key));remeshed.dirty.insert(patch);
    remeshedCover.QueueTerrain(remeshed);auto affected=remeshedCover.pending;while(!remeshedCover.pending.empty())remeshedCover.Refresh(remeshed);
    for(int id:affected)Check(remeshedCover.cells[id].cover==1,"untouched remeshed top retains grass without square patch");
    // An overlying roof, not the hidden cavity floor, owns the coverage height.
    S::Body roof;auto query=std::make_shared<G::SurfaceQuery>();query->Build(G::Box({p[0]-.5,p[1]-.5,.8},{p[0]+.5,p[1]+.5,1.0}));roof.queries[0]=query;
    cover.pending.insert(center);cover.Refresh(roof);
    Check(std::abs(cover.cells[center].height-1)<1e-6,"topmost roof owns cover, not tunnel floor");
    roof.queries.clear();cover.pending.insert(center);cover.Refresh(roof);
    Check(cover.cells[center].cover==0&&cover.cells[center].eligible==0,"missing host soil cannot grow grass");
    cover.Reset();Check(cover.active.empty()&&cover.pending.empty()&&cover.time==0,"lab reset clears wear and recovery state");
    for(int i=0;i<16384;++i){cover.cells[i].cover=.2f;cover.cells[i].disturbed=-200;cover.active.insert(i);}
    auto start=std::chrono::steady_clock::now();for(int i=0;i<600;++i)cover.Advance(1./60);
    auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/600;
    std::printf("PASS %d grass checks; worst-case all-cell recovery %.3f ms/frame; map %zu bytes\n",checks,ms,pixels.size()*sizeof(pixels[0]));
}
