#include "../Geometry/GraniteLabPlacement.h"
#include "../Geometry/GraniteStructuralSupport.h"
#include <cstdio>
#include <limits>
namespace G=EE::GraniteContactCast;
namespace P=EE::GraniteLabPlacement;
static int checks=0,failures=0;
static void Check(bool ok,char const* label){++checks;if(!ok){++failures;std::printf("FAIL: %s\n",label);}}
int main()
{
    G::Body original;
    auto moved=original;
    G::P delta={.7,-.45,.16};
    Check(P::Translate(moved,delta),"intact translation accepted");
    Check(moved.initialMg==original.initialMg&&moved.initialM3==original.initialM3,"exact mass and volume ledger preserved");
    Check(moved.substrate.bins==original.substrate.bins&&moved.renderGroups==original.renderGroups,"stable bin and render memberships");
    Check(moved.substrate.normals==original.substrate.normals,"translation preserves normals");
    Check(std::abs(EE::GraniteStructuralSupport::AnchorElevation(moved)-EE::GraniteStructuralSupport::AnchorElevation(original)-delta[2])<1e-10,"support basal elevation follows actual geometry");
    Check(std::abs(EE::GraniteOutcropV2::GridVolume(moved.substrate.vertices)-original.initialM3)<1e-8,"translated tetrahedra retain measured volume");
    for(double x:{.3,1.,2.,3.,3.8})for(double y:{.3,.8,1.4,2.1})
    {
        G::P start={x,y,3};
        auto a=G::Raycast(original,start,{0,0,-1},5);
        auto b=G::Raycast(moved,G::Add(start,delta),{0,0,-1},5);
        if(!(a.hit&&b.hit&&a.id==b.id&&std::abs(a.distance-b.distance)<1e-8))std::printf("ray %.3f %.3f: hits %d/%d ids %d/%d distances %.12f/%.12f\n",x,y,int(a.hit),int(b.hit),a.id,b.id,a.distance,b.distance);
        Check(a.hit&&b.hit&&a.id==b.id&&std::abs(a.distance-b.distance)<1e-8,"translated contact ray retains identity and distance");
        double nearA=5,nearB=5;G::Hit ca,cb;
        original.collisionQuery.Trace(0,start,{0,0,-1},nearA,ca,-1);
        moved.collisionQuery.Trace(0,G::Add(start,delta),{0,0,-1},nearB,cb,-1);
        Check(ca.hit&&cb.hit&&std::abs(nearA-nearB)<1e-8,"rebuilt walking surface matches translated boundary");
        G::P feet={x,y,.1};
        Check(G::BlocksCapsule(original,feet,.23,1.8)==G::BlocksCapsule(moved,G::Add(feet,delta),.23,1.8),"capsule collision follows translated body");
    }
    Check(P::Translate(moved,G::Mul(delta,-1)),"undo translation accepted");
    double error=0;for(size_t i=0;i<original.substrate.vertices.size();++i)for(int k=0;k<3;++k)error=std::max(error,std::abs(moved.substrate.vertices[i][k]-original.substrate.vertices[i][k]));
    Check(error<1e-12&&moved.initialMg==original.initialMg,"undo restores positions without mass drift");
    auto revision=moved.revision;
    Check(!P::Translate(moved,{std::numeric_limits<double>::quiet_NaN(),0,0})&&moved.revision==revision,"NaN rejected before mutation");
    Check(!P::Translate(moved,{9,0,0})&&moved.revision==revision,"unbounded translation rejected before mutation");
    moved.damage[{0,0,0}]=1;
    Check(!P::Translate(moved,delta)&&moved.revision==revision,"stored damage prevents authoring relocation");
    moved.damage.clear();moved.removedMg=1;
    Check(!P::Translate(moved,delta)&&moved.revision==revision,"recovered matter prevents authoring relocation");
    G::Body empty(false);Check(!P::Translate(empty,delta),"empty body refused");
    std::printf("Granite placement: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
