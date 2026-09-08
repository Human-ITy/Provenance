// Shape-independent movement checks extracted unchanged from the existing
// integration test, so the deferred authored seam cannot hide these failures.
#include "../Geometry/GraniteLabMovement.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
namespace M=EE::GraniteLabMovement;
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
    // Tiny rounded-foot clearance is allowed only while supported, and must
    // still respect a ceiling and a tall obstruction at every tested rate.
    for(int hz:{30,60,120,240})
    {
        auto tiny=[](M::P p,double){return p[0]>=.3&&p[0]<.6&&p[2]<.004;};
        M::Body micro{{0,0,M::Skin}};
        double maxLift=0;for(int i=0;i<hz;++i){double z=micro.feet[2];M::Advance(micro,{1,0},1./hz,tiny,flat);maxLift=std::max(maxLift,micro.feet[2]-z);}
        Check(micro.feet[0]>1.5&&maxLift<=.0064,"supported micro-clearance traverses at varied frame rates");
        auto covered=[&](M::P p,double h){return tiny(p,h)||p[2]+h>M::StandingHeight+M::Skin+.0005;};
        micro={{0,0,M::Skin}};for(int i=0;i<hz;++i)M::Advance(micro,{1,0},1./hz,covered,flat);
        Check(micro.feet[0]<.3&&micro.feet[2]<=M::Skin,"micro-clearance cannot pass overhead obstruction");
        auto tallWall=[](M::P p,double){return p[0]>=.3&&p[2]<1;};
        micro={{0,0,M::Skin}};for(int i=0;i<hz;++i)M::Advance(micro,{1,0},1./hz,tallWall,flat);
        Check(micro.feet[0]<.3&&micro.feet[2]<=M::Skin,"micro-clearance cannot climb a tall wall");
    }
    auto tinyAir=[](M::P p,double){return p[0]>=.3&&p[2]<.004;};
    M::Body airborne{{.29,0,M::Skin}};auto farFloor=[](double,double){return -.2;};
    M::Advance(airborne,{1,0},1./60,tinyAir,farFloor);
    Check(airborne.feet[0]<.3&&!airborne.grounded&&airborne.verticalSpeed<0,"unsupported body cannot use micro-clearance to climb");
    std::printf("Movement core: %d checks, 0 failures\n",checks);
}
