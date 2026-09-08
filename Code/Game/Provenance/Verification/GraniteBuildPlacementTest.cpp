#include "../Geometry/GraniteBuildPlacement.h"
#include "../Systems/GraniteLabGizmoMode.h"
#include <cstdio>
#include <limits>
namespace B=EE::GraniteBuildPlacement;
namespace G=EE::GraniteContactCast;
static int checks=0,failures=0;
static void Check(bool ok,char const* label){++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}}
int main()
{
    struct GizmoContract
    {
        enum class Mode { Translation, Rotation, Scale };
        Mode mode=Mode::Translation;
        bool dragging=false;
        int calls=0,illegal=0;
        bool IsManipulating()const{return dragging;}
        Mode GetMode()const{return mode;}
        void SetMode(Mode value){++calls;if(dragging)++illegal;mode=value;}
    } gizmo;
    using Mode=GizmoContract::Mode;
    EE::OutcropLab::SetIdleGizmoMode(gizmo,Mode::Translation);
    Check(gizmo.calls==0,"idle same mode does not reconfigure gizmo");
    gizmo.dragging=true;
    for(int frame=0;frame<120;++frame)EE::OutcropLab::SetIdleGizmoMode(gizmo,Mode::Translation);
    Check(gizmo.calls==0&&gizmo.illegal==0,"held granite handle never calls SetMode");
    EE::OutcropLab::SetIdleGizmoMode(gizmo,Mode::Rotation);
    Check(gizmo.mode==Mode::Translation&&gizmo.calls==0,"mode change cannot interrupt active drag");
    gizmo.dragging=false;
    EE::OutcropLab::SetIdleGizmoMode(gizmo,Mode::Rotation);
    Check(gizmo.mode==Mode::Rotation&&gizmo.calls==1,"released gizmo can switch modes");
    EE::OutcropLab::SetIdleGizmoMode(gizmo,Mode::Translation);
    Check(gizmo.mode==Mode::Translation&&gizmo.calls==2,"granite selection restores translation while idle");
    B::Gesture g;
    Check(!g.Update(true,true,true,true,false),"idle never spawns");
    g.Begin(false);
    Check(!g.Update(false,true,false,true,false)&&g.pending,"catalog release never places");
    Check(!g.Update(true,true,false,true,false),"world release needs fresh press");
    Check(!g.Update(true,true,true,false,false)&&g.armed,"world press arms");
    Check(g.Update(true,true,false,true,false)&&!g.pending,"release places once");
    Check(!g.Update(true,true,false,true,false),"repeated release cannot duplicate");
    for(bool world:{false,true})for(bool valid:{false,true})for(bool camera:{false,true})
    {
        g.Begin(true);bool expected=world&&valid&&!camera;
        Check(g.Update(world,valid,false,true,camera)==expected,"drag release gate");
        Check(g.pending==!expected&&!g.armed,"invalid drop disarms but preserves draft");
    }
    g.Begin(true);g.Cancel();Check(!g.pending&&!g.armed,"cancel clears pending gesture");
    Check(B::BoxDistance({0,0,3},{0,0,-1},{-1,-1,0},{1,1,1},10)==2,"front bounds pick");
    Check(B::BoxDistance({0,0,3},{0,0,-1},{-1,-1,0},{1,1,1},1)>1,"occluded bounds rejected");
    Check(B::BoxDistance({2,0,3},{0,0,-1},{-1,-1,0},{1,1,1},10)>10,"parallel miss");
    Check(B::BoxDistance({0,0,.5},{0,0,0},{-1,-1,0},{1,1,1},10)>10,"zero ray rejected");
    auto const& grid=EE::PlayableLandscape::Terrain();
    for(double x:{-40.,-18.123,-10.01,14.37,33.99})for(double y:{-70.,-40.,-10.,9.7})
    {
        double z=0;Check(grid.Surface(x,y,z),"sample in outer terrain");
        auto hit=B::LandscapeRay({x,y,10},{0,0,-1},20);
        Check(hit.hit&&std::abs(hit.position[2]-z)<1e-8,"vertical pick equals rendered grid surface");
        auto oblique=B::LandscapeRay({x-1,y-.7,z+2},{1,.7,-2},5);
        double actual=0;Check(oblique.hit&&grid.Surface(oblique.position[0],oblique.position[1],actual)&&std::abs(actual-oblique.position[2])<1e-8,"oblique pick on rendered triangle");
        Check(!B::LandscapeRay({x,y,10},{0,0,-1},1).hit,"reach enforced");
    }
    Check(!B::LandscapeRay({1,1,10},{0,0,-1},20).hit,"outer landscape excludes core soil");
    Check(!B::LandscapeRay({0,0,10},{0,0,0},20).hit,"invalid landscape direction");
    Check(!B::LandscapeRay({std::numeric_limits<double>::quiet_NaN(),0,0},{0,0,-1},20).hit,"nonfinite origin");
    std::printf("Build placement: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
