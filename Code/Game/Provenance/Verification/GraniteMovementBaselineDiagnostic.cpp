// Diagnostic only: no production movement/geometry edits, no replacement baseline.
#include "../Geometry/GraniteLabContact.h"
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace G=EE::GraniteContactCast;
namespace M=EE::GraniteLabMovement;
namespace C=EE::GraniteLabContact;
namespace S=EE::SoilScoop;
namespace Ground=EE::GraniteOutcropGround;

struct Lab
{
    G::Body rock;
    S::Body soil;
    Lab(){soil.Reset(&rock);}
    bool Blocks(M::P p,double h)const
    {return G::BlocksCapsule(rock,p,M::Radius,h)||soil.BlocksCapsule(p,M::Radius,h,false);}
    void Advance(M::Body& b,M::Input input,int& probes)const
    {
        // Match OutcropLab::UpdateCamera: float Z converted to double, static
        // soil collision only, and the combined granite/soil tread query.
        M::Advance(b,input,1./60,[&](M::P p,double h){++probes;return Blocks(p,h);},
            [](double,double){return -double(.12f);},
            [&](double x,double y,double z){return C::StepSurface(rock,x,y,z,&soil,false);});
    }
    M::Body Land(double x,double y)const
    {
        M::Body b{{x,y,rock.high[2]+.4}};
        int probes=0;for(int i=0;i<180;++i)Advance(b,{},probes);
        return b;
    }
};

uint64_t fingerprint=14695981039346656037ull;
void Hash(double v)
{
    // Exact same-build repeat evidence; timings never enter this fingerprint.
    uint64_t bits=0;std::memcpy(&bits,&v,sizeof(bits));
    for(int i=0;i<8;++i){fingerprint^=(bits>>(i*8))&255;fingerprint*=1099511628211ull;}
}
int unsafe=0,flagged=0,routes=0;

void Witness(Lab const& lab,M::Body const& b,double dx,double dy)
{
    double len=std::hypot(dx,dy);dx/=len;dy/=len;
    auto next=b.feet;next[0]+=dx*M::WalkSpeed/60;next[1]+=dy*M::WalkSpeed/60;
    auto lead=next;lead[0]+=dx*(M::Radius+M::Skin);lead[1]+=dy*(M::Radius+M::Skin);
    auto hit=G::Raycast(lab.rock,{lead[0],lead[1],lab.rock.high[2]+1},{0,0,-1},5);
    double tread=C::StepSurface(lab.rock,lead[0],lead[1],b.feet[2]+M::StepHeight,&lab.soil,false);
    std::printf("  witness feet=(%.6f,%.6f,%.6f) nextRock=%d nextSoil=%d leadSurfaceZ=%.6f normalZ=%.6f tread=%.6f grounded=%d\n",
        b.feet[0],b.feet[1],b.feet[2],G::BlocksCapsule(lab.rock,next,M::Radius,b.Height()),
        lab.soil.BlocksCapsule(next,M::Radius,b.Height(),false),hit.hit?hit.position[2]:-999.,hit.hit?hit.normal[2]:-999.,tread,b.grounded);
    auto local=G::Raycast(lab.rock,{lead[0],lead[1],b.feet[2]+M::StepHeight+1e-5},{0,0,-1},M::StepHeight+.02);
    // Brute-force diagnostic only: sampled clearance for a lift followed by
    // one frame's horizontal move. This proves neither walkable slope nor a
    // supported endpoint, and must not become a production wall-climbing rule.
    double sampledLift=-1;
    for(int mm=1;mm<=220;++mm)
    {
        double lift=mm*.001;auto up=b.feet;up[2]+=lift;
        if(lab.Blocks(up,b.Height()))break;
        auto end=next;end[2]+=lift;if(lab.Blocks(end,b.Height()))continue;
        bool clear=true;for(int j=1;j<9;++j){auto p=up;p[0]+=(next[0]-up[0])*j/9;p[1]+=(next[1]-up[1])*j/9;if(lab.Blocks(p,b.Height())){clear=false;break;}}
        if(clear){sampledLift=lift;break;}
    }
    std::printf("  localTreadRay hit=%d z=%.6f normalZ=%.6f sampledClearLift=%.3f (not a slope permission)\n",local.hit,local.hit?local.position[2]:-999.,local.hit?local.normal[2]:-999.,sampledLift);
}

void Route(Lab const& lab,char const* state,int id,double x0,double y0,double x1,double y1)
{
    M::Body b=lab.Land(x0,y0);auto start=b.feet;
    double dx=x1-x0,dy=y1-y0,length=std::hypot(dx,dy);dx/=length;dy/=length;
    int frames=int(std::ceil(length/M::WalkSpeed*60)),run=0,longest=0,worstProbes=0,embedded=0,air=0;
    double maxRise=0,maxFall=0,worstMs=0;M::Body firstCatch;bool caught=false;
    bool validStart=b.grounded&&!lab.Blocks(b.feet,b.Height());
    for(int i=0;i<frames;++i)
    {
        auto old=b.feet;int probes=0;auto begin=std::chrono::steady_clock::now();
        lab.Advance(b,{dx,dy},probes);
        worstMs=std::max(worstMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
        double progress=(b.feet[0]-old[0])*dx+(b.feet[1]-old[1])*dy;
        run=progress<M::WalkSpeed/60*.1?run+1:0;longest=std::max(longest,run);
        if(run==12&&!caught){firstCatch=b;caught=true;}
        worstProbes=std::max(worstProbes,probes);embedded+=lab.Blocks(b.feet,b.Height())?1:0;air+=!b.grounded;
        maxRise=std::max(maxRise,b.feet[2]-old[2]);maxFall=std::max(maxFall,old[2]-b.feet[2]);
        for(double p:b.feet)Hash(p);Hash(b.grounded?1.:0.);
    }
    double progress=(b.feet[0]-start[0])*dx+(b.feet[1]-start[1])*dy;
    bool flag=!validStart||longest>=12||progress<length*.8;
    ++routes;flagged+=flag;unsafe+=embedded>0||!validStart;
    std::printf("ROUTE %s/%02d start=(%.3f,%.3f,%.3f) end=(%.3f,%.3f,%.3f) requested=%.3f progress=%.3f longestCatch=%d embedded=%d air=%d rise=%.4f fall=%.4f probes=%d worstMs=%.3f %s\n",
        state,id,start[0],start[1],start[2],b.feet[0],b.feet[1],b.feet[2],length,progress,longest,embedded,air,maxRise,maxFall,worstProbes,worstMs,flag?"INVESTIGATE":"CLEAR");
    if(caught)Witness(lab,firstCatch,dx,dy);
}

void TopRoutes(Lab const& lab,char const* state)
{
    int id=0;
    for(double v:{.35,.5,.65})for(int reverse=0;reverse<2;++reverse)
        Route(lab,state,++id,G::Width*(reverse?.75:.25),G::Depth*v,G::Width*(reverse?.25:.75),G::Depth*v);
    for(double u:{.3,.5,.7})for(int reverse=0;reverse<2;++reverse)
        Route(lab,state,++id,G::Width*u,G::Depth*(reverse?.75:.25),G::Width*u,G::Depth*(reverse?.25:.75));
    for(int reverse=0;reverse<2;++reverse)for(int cross=0;cross<2;++cross)
        Route(lab,state,++id,G::Width*(reverse?.7:.3),G::Depth*((reverse!=cross)?.7:.3),G::Width*(reverse?.3:.7),G::Depth*((reverse!=cross)?.3:.7));
}

int main()
{
    Lab lab;
    std::printf("Movement diagnosis seed=%u dimensions=%.2f,%.2f crest=%.6f radius=%.3f step=%.3f normalGate=%.3f dt=1/60\n",lab.rock.seed,G::Width,G::Depth,lab.rock.high[2],M::Radius,M::StepHeight,C::RockTreadMinNormalZ);
    auto ground=[](double x,double y){double z=-.12;Ground::Surface(x,y,z);return z;};
    M::Body old{{G::Width*.5,-.6,M::Skin}},live=old,legacyWithSteps=old;
    std::printf("LEGACY-START ground=%.6f feetZ=%.6f liveSoilOverlap=%d\n",ground(old.feet[0],old.feet[1]),old.feet[2],lab.soil.BlocksCapsule(old.feet,M::Radius,old.Height(),false));
    for(int i=0;i<45;++i)
    {
        M::Advance(old,{0,1},1./60,[&](M::P p,double h){return G::BlocksCapsule(lab.rock,p,M::Radius,h);},ground);
        M::Advance(legacyWithSteps,{0,1},1./60,[&](M::P p,double h){return G::BlocksCapsule(lab.rock,p,M::Radius,h);},ground,
            [&](double x,double y,double z){return C::StepSurface(lab.rock,x,y,z);});
        int probes=0;lab.Advance(live,{0,1},probes);
    }
    std::printf("LEGACY feet=(%.6f,%.6f,%.6f) ground=%.6f Ygate=%d aboveGround=%d; LIVE-same-start feet=(%.6f,%.6f,%.6f)\n",old.feet[0],old.feet[1],old.feet[2],ground(old.feet[0],old.feet[1]),old.feet[1]>.05,old.feet[2]>=ground(old.feet[0],old.feet[1]),live.feet[0],live.feet[1],live.feet[2]);
    Witness(lab,old,0,1);
    std::printf("LEGACY-WITH-ROCK-STEPS feet=(%.6f,%.6f,%.6f) originalAssertion=%d\n",legacyWithSteps.feet[0],legacyWithSteps.feet[1],legacyWithSteps.feet[2],legacyWithSteps.feet[1]>.05&&legacyWithSteps.feet[2]>=ground(legacyWithSteps.feet[0],legacyWithSteps.feet[1]));
    // Land above the real soil first, unlike the legacy start at z=Skin.
    for(double u:{.25,.5,.75})
    {
        auto b=lab.Land(G::Width*u,-.6);auto start=b.feet;int embedded=0;
        for(int i=0;i<160;++i){int probes=0;lab.Advance(b,{0,1},probes);embedded+=lab.Blocks(b.feet,b.Height());}
        std::printf("GROUND u=%.2f startZ=%.6f end=(%.6f,%.6f,%.6f) grounded=%d embedded=%d\n",u,start[2],b.feet[0],b.feet[1],b.feet[2],b.grounded,embedded);
        unsafe+=embedded>0;Witness(lab,b,0,1);
    }
    for(int side=0;side<3;++side)
    {
        double x=side==0?-.6:side==1?G::Width+.6:G::Width*.25;
        double y=side<2?G::Depth*.6:G::Depth+.6;
        double dx=side==0?1:side==1?-1:0,dy=side==2?-1:0;
        auto b=lab.Land(x,y);auto start=b.feet;int embedded=0;
        for(int i=0;i<160;++i){int probes=0;lab.Advance(b,{dx,dy},probes);embedded+=lab.Blocks(b.feet,b.Height());}
        std::printf("GROUND-SIDE side=%d start=(%.6f,%.6f,%.6f) end=(%.6f,%.6f,%.6f) grounded=%d embedded=%d\n",side,start[0],start[1],start[2],b.feet[0],b.feet[1],b.feet[2],b.grounded,embedded);
        unsafe+=embedded>0;Witness(lab,b,dx,dy);
    }
    TopRoutes(lab,"fresh");
    // Edge departure is observational: report airborne descent and final
    // support, rather than treating falling off a legitimate edge as a snag.
    auto edge=lab.Land(G::Width*.5,G::Depth*.5);int airborne=0,embedded=0;double maxDrop=0;
    for(int i=0;i<210;++i){int probes=0;double z=edge.feet[2];lab.Advance(edge,{0,-1},probes);airborne+=!edge.grounded;embedded+=lab.Blocks(edge.feet,edge.Height());maxDrop=std::max(maxDrop,z-edge.feet[2]);}
    for(int i=0;i<90;++i){int probes=0;lab.Advance(edge,{},probes);embedded+=lab.Blocks(edge.feet,edge.Height());}
    std::printf("EDGE airborneDuringTravel=%d maxFrameDrop=%.6f settled=(%.6f,%.6f,%.6f) grounded=%d embedded=%d\n",airborne,maxDrop,edge.feet[0],edge.feet[1],edge.feet[2],edge.grounded,embedded);unsafe+=embedded>0||!edge.grounded;
    // A separate edited specimen, never the live editor's state. Geometry
    // edits use production strike/scoop operations; no artificial obstacles.
    auto top=G::Raycast(lab.rock,{G::Width*.5,G::Depth*.5,lab.rock.high[2]+1},{0,0,-1},5);
    int chips=0;for(int i=0;i<16;++i)chips+=G::Strike(lab.rock,{top.position[0],top.position[1],top.position[2]+.5},{0,0,-1}).removed;
    G::Hit soilHit;double soilReach=4;lab.soil.Trace({G::Width*.5,-.6,2},{0,0,-1},soilReach,soilHit);
    auto scoop=S::Prepare(lab.soil,lab.rock,soilHit,{0,0,-1});double scoopM3=scoop.receipt.volume;
    bool committed=S::Commit(lab.soil,std::move(scoop));
    std::printf("EDITED chips=%d rockRemovedM3=%.9f scoopM3=%.9f committed=%d (static collision; loose debris simulation excluded)\n",chips,lab.rock.removedM3,scoopM3,committed);
    TopRoutes(lab,"edited");
    auto dug=lab.Land(G::Width*.5,-1.);int probes=0;for(int i=0;i<100;++i)lab.Advance(dug,{0,1},probes);
    std::printf("DUG-APPROACH feet=(%.6f,%.6f,%.6f) grounded=%d embedded=%d\n",dug.feet[0],dug.feet[1],dug.feet[2],dug.grounded,lab.Blocks(dug.feet,dug.Height()));
    unsafe+=lab.Blocks(dug.feet,dug.Height());
    std::printf("DIAGNOSTIC routes=%d flagged=%d safetyFlags=%d trajectoryFingerprint=%016llx\n",routes,flagged,unsafe,(unsigned long long)fingerprint);
    // Flags remain visible/nonzero. These are investigation triggers, not a
    // claim that every straight route is guaranteed traversable by design.
    return unsafe||flagged?1:0;
}
