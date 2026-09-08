#include "../Geometry/GraniteLabContact.h"
#include <cstdio>
#include <cmath>
#include <limits>
namespace G=EE::GraniteContactCast;
namespace C=EE::GraniteLabContact;
int checks=0,failures=0;
void Check(bool ok,char const* label){++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}}
int main()
{
    // Synthetic fixtures outside the soil patch isolate tread semantics from
    // this authored specimen. The current collision boundary is authoritative.
    G::Body fixture(false);auto platform=G::Box({49.9,49.9,-.1},{50.1,50.1,.05});
    fixture.collisionQuery.Build(platform);
    Check(std::abs(C::StepSurface(fixture,50,50,.2)-.05)<1e-8,"horizontal current-boundary tread");
    Check(C::StepSurface(fixture,50,50,.5)<-1e20,"tread outside downward reach is not offered");
    Check(C::StepSurface(fixture,50,50,.01)<-1e20,"surface above step ceiling is not offered");
    Check(C::StepSurface(fixture,51,50,.2)<-1e20,"no invented support outside surface");
    auto steep=platform;
    steep.push_back({{{49.9,49.9,-.08},{50.1,49.9,.32},{50.1,50.1,.32},{49.9,50.1,-.08}},0,0});
    fixture.collisionQuery.Build(steep);
    Check(C::StepSurface(fixture,50,50,.2)<-1e20,"distinct higher steep face hides lower tread");
    fixture.collisionQuery.Build(G::Box({49.9,49.9,-.1},{50.1,50.1,.02}));
    Check(std::abs(C::StepSurface(fixture,50,50,.2)-.02)<1e-8,"changed boundary does not reuse old tread");
    fixture.collisionQuery.Build({});
    Check(C::StepSurface(fixture,50,50,.2)<-1e20,"removed boundary has no phantom tread");
    Check(C::StepSurface(fixture,std::numeric_limits<double>::quiet_NaN(),50,.2)<-1e20,"nonfinite query refused");
    fixture.collisionQuery.Build(G::Box({49.9,49.9,-.4},{50.1,50.1,-.03}));
    Check(std::abs(C::StepSurface(fixture,50,50,.22)+.03)<1e-8,"rounded-foot support below the former two-centimetre window is visible");
    // Explicit lab slope policy, independent of the authored shape: up to
    // 60 degrees is eligible as a tread; steeper surfaces are not.
    for(double degrees:{0.,49.,55.,59.9,60.,60.1,70.,85.})
    {
        double dz=.1*std::tan(degrees*3.14159265358979323846/180.);
        fixture.collisionQuery.Build({{{{49.9,49.9,.05-dz},{50.1,49.9,.05+dz},{50.1,50.1,.05+dz},{49.9,50.1,.05-dz}},0,0}});
        double tread=C::StepSurface(fixture,50,50,.2);
        Check(degrees<=60?std::abs(tread-.05)<1e-8:tread<-1e20,"explicit slope boundary retains steep-face refusal");
    }
    G::Body rock;
    // Recorded fresh-route 03 crosses an authored triangle boundary. Changing
    // the query origin within reach must not turn that tread into a wall.
    for(double ceiling:{.90,.92,.94,.96,.98})
    {
        double top=C::StepSurface(rock,2.803,1.2,ceiling);
        std::printf("authored tread ceiling %.3f top %.9f\n",ceiling,top);
        Check(std::abs(top-.7792039)<1e-5,"recorded tread remains visible across query heights");
    }
    // Both directions of the recorded cross-crown route must now progress.
    EE::SoilScoop::Body soil;soil.Reset(&rock);
    namespace M=EE::GraniteLabMovement;
    auto advance=[&](M::Body& b,M::Input input,double dt=1./60)
    {
        M::Advance(b,input,dt,[&](G::P p,double h){return G::BlocksCapsule(rock,p,M::Radius,h)||soil.BlocksCapsule(p,M::Radius,h,false);},
            [](double,double){return -double(.12f);},[&](double x,double y,double z){return C::StepSurface(rock,x,y,z,&soil,false);});
    };
    for(int direction:{-1,1})
    {
        M::Body b{{direction>0?1.05:3.15,1.2,rock.high[2]+.4}};
        for(int i=0;i<180;++i)advance(b,{});
        double start=b.feet[0];int catchRun=0,worstCatch=0;bool clear=true;
        for(int i=0;i<79;++i){double old=b.feet[0];advance(b,{double(direction),0});catchRun=(b.feet[0]-old)*direction<.002?catchRun+1:0;worstCatch=std::max(worstCatch,catchRun);clear=clear&&!G::BlocksCapsule(rock,b.feet,M::Radius,b.Height())&&!soil.BlocksCapsule(b.feet,M::Radius,b.Height(),false);}
        Check((b.feet[0]-start)*direction>2&&worstCatch<12&&clear,"cross-crown tread route progresses without persistent catch or overlap");
    }
    // Retain the earlier small-clearance routes across update rates.
    for(int hz:{30,60,120,240})for(int route:{11,16})
    {
        double x=2.94,y=route==11?.6:.72,dx=route==11?0:-1.68,dy=route==11?1.:.96;
        double length=std::hypot(dx,dy);dx/=length;dy/=length;
        M::Body b{{x,y,rock.high[2]+.4}};for(int i=0;i<180;++i)advance(b,{});
        auto start=b.feet;int run=0,worst=0;bool clear=true;
        int frames=int(std::ceil((route==11?1.2:length)/M::WalkSpeed*hz));
        for(int i=0;i<frames;++i){auto old=b.feet;advance(b,{dx,dy},1./hz);double progress=(b.feet[0]-old[0])*dx+(b.feet[1]-old[1])*dy;run=progress<M::WalkSpeed/hz*.1?run+1:0;worst=std::max(worst,run);clear=clear&&!G::BlocksCapsule(rock,b.feet,M::Radius,b.Height())&&!soil.BlocksCapsule(b.feet,M::Radius,b.Height(),false);}
        double progress=(b.feet[0]-start[0])*dx+(b.feet[1]-start[1])*dy;
        std::printf("small-clearance route %d at %d Hz progress %.6f longest catch %d\n",route,hz,progress,worst);
        Check(progress>(route==11?.96:1.548)&&worst<hz/5&&clear,"small-clearance upper-contour traversal");
    }
    // Keep the near-vertical tall side blocked from the ground.
    M::Body side{{-.6,1.44,rock.high[2]+.4}};for(int i=0;i<180;++i)advance(side,{});
    for(int i=0;i<160;++i)advance(side,{1,0});
    Check(side.feet[0]<.6&&side.feet[2]<.4&&side.grounded&&!G::BlocksCapsule(rock,side.feet,M::Radius,side.Height()),"tall exterior face still blocks ground approach");
    for(int hz:{30,60,120,240})for(int route:{6,7,12})
    {
        double x=route==6?3.15:route==7?1.26:2.94;
        double y=route==6?1.56:route==7?.6:1.8;
        double dx=route==6?-1:0,dy=route==7?1:route==12?-1:0,length=route==6?2.1:1.2;
        M::Body b{{x,y,rock.high[2]+.4}};for(int i=0;i<180;++i)advance(b,{});
        auto start=b.feet;int run=0,worst=0;bool clear=true;
        for(int i=0;i<int(std::ceil(length/M::WalkSpeed*hz));++i)
        {
            auto old=b.feet;advance(b,{dx,dy},1./hz);
            double progress=(b.feet[0]-old[0])*dx+(b.feet[1]-old[1])*dy;
            run=progress<M::WalkSpeed/hz*.1?run+1:0;worst=std::max(worst,run);
            clear=clear&&!G::BlocksCapsule(rock,b.feet,M::Radius,b.Height())&&!soil.BlocksCapsule(b.feet,M::Radius,b.Height(),false);
        }
        double progress=(b.feet[0]-start[0])*dx+(b.feet[1]-start[1])*dy;
        std::printf("upper slope route %d at %d Hz progress %.6f longest catch %d\n",route,hz,progress,worst);
        if(worst>=hz/5)
        {
            double leadX=b.feet[0]+dx*(M::WalkSpeed/hz+M::Radius+M::Skin),leadY=b.feet[1]+dy*(M::WalkSpeed/hz+M::Radius+M::Skin);
            auto hit=G::Raycast(rock,{leadX,leadY,rock.high[2]+1},{0,0,-1},5);
            std::printf("  stop feet %.9f %.9f %.9f lead top %.9f normal %.9f tread %.9f\n",b.feet[0],b.feet[1],b.feet[2],hit.position[2],hit.normal[2],C::StepSurface(rock,leadX,leadY,b.feet[2]+M::StepHeight,&soil,false));
        }
        Check(progress>length*.8&&worst<hz/5&&clear,"upper slopes traverse without sustained catch or overlap");
    }
    // User-reported hotspot classes: upper contours in both stances, plus
    // the reachable front lip (stop before the later, steeper face).
    for(bool crouched:{false,true})for(int hz:{30,60,120,240})for(int route=0;route<6;++route)
    {
        double x=route==0?2.7:route==1||route==4?3.5:route==5?1.8:3.1;
        double y=route<2?1.3:route==5?-.6:1.7;
        double dx=route==0?1:route==1||route==4?-1:0;
        double dy=route==2||route==5?1:route==3||route==4?-1:0;
        double n=std::hypot(dx,dy);dx/=n;dy/=n;
        double length=route==5?.75:.55,speed=crouched?M::CrouchSpeed:M::WalkSpeed;
        M::Body b{{x,y,rock.high[2]+.4}};b.crouched=crouched;for(int i=0;i<180;++i)advance(b,{});
        auto start=b.feet;bool clear=b.grounded&&!G::BlocksCapsule(rock,b.feet,M::Radius,b.Height())&&!soil.BlocksCapsule(b.feet,M::Radius,b.Height(),false);int run=0,worst=0;
        for(int frame=0;frame<int(std::ceil(length/speed*hz));++frame)
        {
            auto old=b.feet;advance(b,{dx,dy},1./hz);
            run=(b.feet[0]-old[0])*dx+(b.feet[1]-old[1])*dy<speed/hz*.1?run+1:0;worst=std::max(worst,run);
            clear=clear&&!G::BlocksCapsule(rock,b.feet,M::Radius,b.Height())&&!soil.BlocksCapsule(b.feet,M::Radius,b.Height(),false);
        }
        double progress=(b.feet[0]-start[0])*dx+(b.feet[1]-start[1])*dy;
        std::printf("hotspot %d crouch %d hz %d progress %.6f catch %d\n",route,crouched,hz,progress,worst);
        Check(progress>length*.8&&worst<hz/5&&clear,"reported hotspot class progresses in both stances without sustained catch or overlap");
    }
    // Real closed ramp boundaries independently protect steep-face refusal.
    // Do not mistake a nearby floor for permission to scale a 70/85 degree face.
    for(bool crouched:{false,true})for(double degrees:{55.,70.,85.})
    {
        G::Body ramp(false);auto mesh=G::Box({50,49,-1},{52,51,.1});
        for(auto& face:mesh)for(auto& p:face.p)if(p[2]>.09)p[2]=.05+(p[0]-50)*std::tan(degrees*3.14159265358979323846/180.);
        ramp.collisionQuery.Build(mesh);M::Body b{{49.6,50,M::Skin}};b.crouched=crouched;bool clear=true;
        for(int i=0;i<120;++i)
        {
            M::Advance(b,{1,0},1./60,[&](M::P p,double h){return G::BlocksCapsule(ramp,p,M::Radius,h);},[](double,double){return 0.;},[&](double x,double y,double z){return C::StepSurface(ramp,x,y,z);});
            clear=clear&&!G::BlocksCapsule(ramp,b.feet,M::Radius,b.Height());
        }
        std::printf("ramp %.0f crouch %d feet %.6f %.6f\n",degrees,crouched,b.feet[0],b.feet[2]);
        Check(clear&&(degrees<60?b.feet[0]>50.3:b.feet[0]<50.1&&b.feet[2]<.3),"eligible ramp progresses but steep ramp remains blocked");
    }
    std::printf("Tread surface: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
