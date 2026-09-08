#include "../Geometry/GraniteLabMovement.h"
#include "../Geometry/GraniteContactCast.h"
#include "../Geometry/GraniteOutcropGround.h"
#include "../Geometry/GraniteLabContact.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
namespace M=EE::GraniteLabMovement;
namespace G=EE::GraniteContactCast;
int checks=0;
void Check(bool result,char const* message){++checks;if(!result){std::printf("FAIL: %s\n",message);std::exit(1);}}
int main()
{
    auto empty=[](M::P,double){return false;};auto flat=[](double,double){return 0.;};
    auto run=[&](M::Body& b,M::Input input,int frames,auto blocks,auto floor){for(int i=0;i<frames;++i)M::Advance(b,input,1./60,blocks,floor);};
    M::Body walk{{0,0,M::Skin}},sprint=walk,diagonal=walk,crouch=walk;
    run(walk,{1,0},60,empty,flat);run(sprint,{1,0,true},60,empty,flat);run(diagonal,{1,1},60,empty,flat);run(crouch,{1,0,true,true},60,empty,flat);
    Check(std::abs(walk.feet[0]-M::WalkSpeed)<1e-8,"walk speed");
    Check(std::abs(sprint.feet[0]-M::RunSpeed)<1e-8,"sprint speed");
    Check(std::abs(std::hypot(diagonal.feet[0],diagonal.feet[1])-M::WalkSpeed)<1e-8,"no diagonal boost");
    Check(std::abs(crouch.feet[0]-M::CrouchSpeed)<1e-8&&crouch.crouched,"crouch overrides sprint");
    M::Body jumping{{0,0,M::Skin}};double peak=0;
    for(int i=0;i<120;++i){M::Advance(jumping,{0,0,false,false,true},1./60,empty,flat);peak=std::max(peak,jumping.feet[2]);}
    Check(peak>.8&&peak<1.,"jump height");Check(jumping.grounded&&jumping.feet[2]<.01,"hold jump does not auto bounce");
    M::Advance(jumping,{},1./60,empty,flat);M::Advance(jumping,{0,0,false,false,true},1./60,empty,flat);Check(jumping.verticalSpeed>0,"release then press jumps again");
    jumping={{0,0,2}};M::Advance(jumping,{0,0,false,false,true},1./60,empty,flat);Check(jumping.verticalSpeed<0,"no air jump");
    auto ledge=[](double x,double){return x<0?2.:0.;};M::Body falling{{-.01,0,2+M::Skin}};
    M::Advance(falling,{1,0},1./60,empty,ledge);
    Check(falling.feet[0]>0&&!falling.grounded&&falling.feet[2]>1.9,"walk off ledge without fence or downward teleport");
    run(falling,{},90,empty,ledge);Check(falling.grounded&&std::abs(falling.feet[2]-M::Skin)<1e-8,"land below ledge");
    auto wall=[](M::P p,double){return p[0]+M::Radius>1&&p[0]-M::Radius<1.1&&p[2]<3;};M::Body blocked{{0,0,M::Skin}};
    run(blocked,{1,1,true},60,wall,flat);Check(blocked.feet[0]<=1-M::Radius&&blocked.feet[1]>2,"capsule wall collision and sliding");
    M::Body slowFrame{{.75,0,M::Skin}};int slowFrameProbes=0;
    M::Advance(slowFrame,{1,1,true},.1,[&](M::P p,double h){++slowFrameProbes;return wall(p,h);},flat);
    std::printf("Slow blocked frame probes %d\n",slowFrameProbes);
    Check(slowFrameProbes<=24&&slowFrame.feet[1]>0,"slow blocked frame caps collision catch-up while preserving slide");
    auto diagonalWall=[](M::P p,double){return p[0]+p[1]+M::Radius*1.4142135623730951>1.;};
    M::Body diagonalSlide{{.5-M::Radius*.7071067811865476,.5-M::Radius*.7071067811865476,M::Skin}};
    M::P diagonalStart=diagonalSlide.feet;run(diagonalSlide,{1,.2},30,diagonalWall,flat);
    Check(diagonalSlide.feet[0]>diagonalStart[0]+.1&&diagonalSlide.feet[1]<diagonalStart[1]-.1,"diagonal wall redirects inward input along its tangent without catching");
    auto roof=[](M::P p,double height){return p[2]+height>1.4&&p[2]<1.6;};M::Body low{{0,0,M::Skin},0,true,true};
    M::Advance(low,{0,0,false,true},1./60,roof,flat);Check(low.crouched,"cannot stand inside ceiling");
    M::Advance(low,{},1./60,empty,flat);Check(low.crouched,"crouch remains latched on release and leaving roof");
    M::Advance(low,{0,0,false,true},1./60,empty,flat);Check(!low.crouched,"second press stands when clear");
    M::Advance(low,{0,0,false,true},1./60,empty,flat);Check(!low.crouched,"held C does not toggle repeatedly");
    auto highRoof=[](M::P p,double height){return p[2]+height>2.05&&p[2]<2.2;};M::Body head{{0,0,M::Skin}};double headPeak=0;
    for(int i=0;i<60;++i){M::Advance(head,{0,0,false,false,i==0},1./60,highRoof,flat);headPeak=std::max(headPeak,head.feet[2]);}
    Check(headPeak<=.250001&&head.grounded,"head collision stops ascent then lands");
    M::Body fast{{0,0,20},-100};run(fast,{},30,empty,flat);Check(fast.grounded&&fast.feet[2]>=0,"fast fall cannot tunnel through ground");
    M::Body unsupported{{0,0,M::Skin},0,true};auto lower=[](double,double){return -2.;};M::Advance(unsupported,{},1./60,empty,lower);Check(!unsupported.grounded&&unsupported.verticalSpeed<0,"removed support starts fall without movement");
    M::Body ramp{{0,0,M::Skin}};auto slope=[](double x,double){return x*.2;};run(ramp,{1,0},60,empty,slope);Check(ramp.feet[0]>1.5&&ramp.feet[2]>.3,"soil ramp climb");
    // Real remaining granite queries, not just mock boxes.
    G::Body rock;auto stone=[&](M::P p,double height){return G::BlocksCapsule(rock,p,M::Radius,height);};
    auto ground=[](double x,double y){double z=-.12;EE::GraniteOutcropGround::Surface(x,y,z);return z;};
    // Match the playable static environment: soil participates in capsule
    // collision and tread selection, with only the editor floor as fallback.
    // Spawn above the surface and settle instead of starting inside the bank.
    EE::SoilScoop::Body soil;soil.Reset(&rock);
    auto liveBlocks=[&](M::P p,double height){return stone(p,height)||soil.BlocksCapsule(p,M::Radius,height,false);};
    auto liveFloor=[](double,double){return -double(.12f);};
    auto liveTread=[&](double x,double y,double z){return EE::GraniteLabContact::StepSurface(rock,x,y,z,&soil,false);};
    auto liveAdvance=[&](M::Body& b,M::Input input){M::Advance(b,input,1./60,liveBlocks,liveFloor,liveTread);};
    auto land=[&](double x,double y){M::Body b{{x,y,rock.high[2]+.4}};for(int i=0;i<180;++i)liveAdvance(b,{});return b;};
    M::Body approach=land(G::Width*.5,-.6);
    Check(approach.grounded&&!liveBlocks(approach.feet,approach.Height()),"seam approach starts supported outside granite and soil");
    auto withoutTread=approach;
    for(int i=0;i<45;++i)M::Advance(withoutTread,{0,1},1./60,liveBlocks,liveFloor);
    Check(withoutTread.feet[1]<=.05&&!liveBlocks(withoutTread.feet,withoutTread.Height()),"negative control: missing playable tread callback cannot pass the apron seam");
    bool seamClear=true;for(int i=0;i<45;++i){liveAdvance(approach,{0,1});seamClear=seamClear&&!liveBlocks(approach.feet,approach.Height());}
    Check(seamClear,"real granite approach remains outside solid");
    std::printf("Real approach feet %.3f %.3f %.3f\n",approach.feet[0],approach.feet[1],approach.feet[2]);
    Check(approach.feet[1]>.05&&approach.feet[2]>=ground(approach.feet[0],approach.feet[1]),"continuous soil reaches the buried granite apron without a wall seam");
    // The successful apron approach now ends on an upper tread, not at a wall.
    // Independently approach the steep left exterior, then press inward while
    // moving along +Y. Preserve the progress, stall and probe limits.
    M::Body faceSlide=land(-.6,1.44);for(int i=0;i<160;++i)liveAdvance(faceSlide,{1,0});
    Check(faceSlide.feet[0]<.6&&faceSlide.feet[2]<.4&&faceSlide.grounded&&!liveBlocks(faceSlide.feet,faceSlide.Height()),"steep exterior blocks ground approach before wall slide");
    int faceSlideWorstProbes=0,faceSlideWorstFrame=0,faceSlideStalled=0,faceSlideTotalProbes=0;double faceSlideWorstMs=0,faceSlideStartY=faceSlide.feet[1];M::P faceSlideWorstFeet={};bool faceSlideClear=true;
    for(int frame=0;frame<90;++frame)
    {
        int probes=0;double oldY=faceSlide.feet[1];auto begin=std::chrono::steady_clock::now();
        M::Advance(faceSlide,{.18,1},1./60,[&](M::P p,double h){++probes;return liveBlocks(p,h);},liveFloor,liveTread);
        faceSlideWorstMs=std::max(faceSlideWorstMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
        faceSlideTotalProbes+=probes;if(probes>faceSlideWorstProbes){faceSlideWorstProbes=probes;faceSlideWorstFrame=frame;faceSlideWorstFeet=faceSlide.feet;}if(std::abs(faceSlide.feet[1]-oldY)<1e-5)++faceSlideStalled;
        faceSlideClear=faceSlideClear&&!liveBlocks(faceSlide.feet,faceSlide.Height());
    }
    std::printf("Fresh exterior slide dy %.3f avg probes %.1f worst %d at %d feet %.3f %.3f %.3f stalled %d worst %.3f ms\n",faceSlide.feet[1]-faceSlideStartY,double(faceSlideTotalProbes)/90,faceSlideWorstProbes,faceSlideWorstFrame,faceSlideWorstFeet[0],faceSlideWorstFeet[1],faceSlideWorstFeet[2],faceSlideStalled,faceSlideWorstMs);
    Check(faceSlide.feet[1]-faceSlideStartY>2&&faceSlideStalled==0&&faceSlideWorstProbes<=40&&faceSlideClear,"fresh exterior wall slide remains smooth and bounded");
    auto rockOnlyFloor=[](double,double){return -.12;};M::Body onRock{{G::Width*.5,.7,3.}};
    run(onRock,{},90,stone,rockOnlyFloor);Check(onRock.grounded&&onRock.feet[2]>.1&&!stone(onRock.feet,onRock.Height()),"land on real granite rather than soil datum");
    int freshRockProbes=0;auto freshRockStart=std::chrono::steady_clock::now();
    M::Advance(onRock,{1,1,true},.1,[&](M::P p,double h){++freshRockProbes;return G::BlocksCapsule(rock,p,M::Radius,h);},rockOnlyFloor,
        [&](double x,double y,double z){return EE::GraniteLabContact::StepSurface(rock,x,y,z);});
    double freshRockMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-freshRockStart).count();
    std::printf("Fresh-rock slow frame probes %d time %.3f ms\n",freshRockProbes,freshRockMs);
    Check(freshRockProbes<=32,"fresh untouched granite keeps slow-frame probes bounded");
    // Leave the finite soil patch; no invisible boundary. Land on editor floor.
    M::Body patchEdge{{-3.99,5,ground(-3.99,5)+M::Skin}};run(patchEdge,{-1,0},90,empty,ground);
    Check(patchEdge.feet[0]<-4&&patchEdge.grounded&&std::abs(patchEdge.feet[2]-(-.12+M::Skin))<1e-8,"fall off soil patch onto editor floor");
    auto step=[](M::P p,double){return p[0]+M::Radius>0&&p[0]-M::Radius<1&&p[2]<.12;};
    auto tread=[](double x,double,double){return x>=0&&x<=1?.12:0.;};M::Body stairs{{-.4,0,M::Skin}};
    for(int i=0;i<40;++i)M::Advance(stairs,{1,0},1./60,step,flat,tread);
    Check(stairs.feet[0]>.3&&stairs.feet[2]>=.12&&stairs.feet[2]<.13,"step across a five-inch rock riser");
    auto tall=[](M::P p,double){return p[0]+M::Radius>0&&p[0]-M::Radius<1&&p[2]<.5;};
    auto tallTread=[](double,double,double){return .5;};stairs={{-.4,0,M::Skin}};
    for(int i=0;i<40;++i)M::Advance(stairs,{1,0},1./60,tall,flat,tallTread);
    Check(stairs.feet[0]<0&&stairs.feet[2]<.01,"reject too-high step");
    auto stepRoof=[&](M::P p,double height){return step(p,height)||p[2]+height>1.86;};stairs={{-.4,0,M::Skin}};
    for(int i=0;i<40;++i)M::Advance(stairs,{1,0},1./60,stepRoof,flat,tread);
    Check(stairs.feet[0]<0&&stairs.feet[2]<.01,"step-up checks overhead path");
    // A four-foot natural crown need not be step-accessible on every rear
    // transect. At least one deterministic shoulder route must remain gently
    // walkable from the contiguous soil field.
    bool rearShoulderWalkable=false;M::Body shelf;
    for(double u:{.22,.38,.58,.76,.88})
    {
        shelf=land(G::Width*u,G::Depth+.55);
        bool clear=true;for(int i=0;i<120;++i){liveAdvance(shelf,{0,-1});clear=clear&&!liveBlocks(shelf.feet,shelf.Height());}
        bool crossed=shelf.feet[1]<G::Depth-.25&&clear;
        std::printf("Rear shoulder u %.2f feet %.3f %.3f %.3f crossed %d\n",u,shelf.feet[0],shelf.feet[1],shelf.feet[2],crossed?1:0);
        rearShoulderWalkable=rearShoulderWalkable||crossed;
    }
    Check(rearShoulderWalkable,"at least one rear-soil route walks onto the taller granite shoulder");
    // Height alone does not make the central route a wall: it has legal treads.
    // Independently test the recorded steep right and rear approaches instead.
    // The left exterior is checked above before its tangent-slide test.
    for(int side:{1,2})
    {
        auto b=side==1?land(G::Width+.6,1.44):land(1.05,G::Depth+.6);
        bool clear=true;for(int i=0;i<160;++i){liveAdvance(b,side==1?M::Input{-1,0}:M::Input{0,-1});clear=clear&&!liveBlocks(b.feet,b.Height());}
        std::printf("Steep exterior %d feet %.3f %.3f %.3f\n",side,b.feet[0],b.feet[1],b.feet[2]);
        Check((side==1?b.feet[0]>3.6:b.feet[1]>2.1)&&b.feet[2]<.4&&b.grounded&&clear,"steep exterior blocks cleanly without embedding or false step-up");
    }
    // Reproduce the live report: a short straight tunnel, then a slow render
    // frame while the player moves against the excavated face.
    G::Body tunnel;int tunnelChips=0;
    constexpr double tunnelX=G::Width*.28;
    for(int i=0;i<16;++i)if(G::Strike(tunnel,{tunnelX,-.5,.38},{0,1,0}).removed)++tunnelChips;
    M::Body tunnelWalker{{tunnelX,-.12,ground(tunnelX,-.12)+M::Skin}};int tunnelProbes=0;
    auto tunnelStart=std::chrono::steady_clock::now();
    M::Advance(tunnelWalker,{1,1,true},.1,[&](M::P p,double h){++tunnelProbes;return G::BlocksCapsule(tunnel,p,M::Radius,h);},ground,
        [&](double x,double y,double z){return EE::GraniteLabContact::StepSurface(tunnel,x,y,z);});
    double tunnelMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tunnelStart).count();
    std::printf("Dug-tunnel slow frame chips %d probes %d time %.3f ms\n",tunnelChips,tunnelProbes,tunnelMs);
    Check(tunnelChips>0&&tunnelProbes<=24,"dug-tunnel slow frame keeps collision work bounded");
    // Match the live clarification: the feet themselves are raised to the
    // chipped-hole elevation, so the capsule's rounded bottom traverses the
    // edited cavity lips rather than merely passing them at torso height.
    constexpr double holeElevation=.42;auto elevatedFloor=[=](double,double){return holeElevation;};
    M::Body elevated{{tunnelX,-.6,holeElevation+M::Skin}};
    run(elevated,{0,1},45,[&](M::P p,double h){return G::BlocksCapsule(tunnel,p,M::Radius,h);},elevatedFloor);
    int elevatedWorstProbes=0,elevatedStalls=0;double elevatedWorstMs=0,elevatedStartX=elevated.feet[0];
    for(int frame=0;frame<70;++frame)
    {
        int probes=0;double oldX=elevated.feet[0];auto begin=std::chrono::steady_clock::now();
        M::Advance(elevated,{1,.18},1./60,[&](M::P p,double h){++probes;return G::BlocksCapsule(tunnel,p,M::Radius,h);},elevatedFloor,
            [&](double x,double y,double z){return EE::GraniteLabContact::StepSurface(tunnel,x,y,z);});
        elevatedWorstMs=std::max(elevatedWorstMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
        elevatedWorstProbes=std::max(elevatedWorstProbes,probes);if(std::abs(elevated.feet[0]-oldX)<1e-5)++elevatedStalls;
    }
    std::printf("Elevated cavity-lip slide dx %.3f feet %.3f %.3f %.3f worst probes %d stalled %d worst %.3f ms\n",
        elevated.feet[0]-elevatedStartX,elevated.feet[0],elevated.feet[1],elevated.feet[2],elevatedWorstProbes,elevatedStalls,elevatedWorstMs);
    // A faceted lip may consume a few isolated contact frames while changing
    // supporting planes. Persistent catches are the regression: traversal
    // must continue without a direction change and keep probe work bounded.
    Check(elevated.feet[0]-elevatedStartX>1.7&&elevatedStalls<=4&&elevatedWorstProbes<=40,"feet-level cavity-lip traversal remains smooth and bounded without persistent catch");
    std::printf("Granite lab movement: %d checks, 0 failures\n",checks);
}
