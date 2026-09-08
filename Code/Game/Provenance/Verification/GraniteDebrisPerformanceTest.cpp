#include "../Geometry/GraniteContactDebris.h"
#include <chrono>
#include <cstdio>
namespace G=EE::GraniteContactCast;
namespace D=EE::GraniteContactDebris;
int main()
{
    int checks=0,failures=0;
    auto check=[&](bool ok,char const* label){++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}};
    {
        G::Chip chip;chip.geometry=G::Box({0,0,0},{2,.02,.02});chip.mesh=G::Triangulate(chip.geometry);chip.center={1,.01,.01};chip.volume=G::Volume(chip.geometry);chip.massMg=uint64_t(std::llround(chip.volume*G::Density*1e6));
        auto slender=D::Launch(std::move(chip),{0,1,0});slender.settled=true;
        check(!D::Brush(slender,{1,-.1,1},{1,0,1},.3,1.8),"long shard radius cannot brush player a meter above its real geometry");
        check(!D::Brush(slender,{1,-.1,.020010},{1,0,.020010},.3,1.8),"ten-micron air gap stays asleep outside five-micron wake skin");
        check(D::Brush(slender,{1,-.1,.020004},{1,0,.020004},.3,1.8),"four-micron gap is inside explicit wake skin");slender.brushCooldown=0;slender.settled=true;
        check(D::Brush(slender,{1,-.1,0},{1,0,0},.3,1.8),"capsule contact with actual shard geometry wakes it");
    }
    G::Body body;std::vector<D::FallingChip> chips;
    for(int i=0;i<24;++i)
    {
        G::P o={.86+.055*(i%5),-.4,.18+.012*(i%20)};auto receipt=G::Strike(body,o,{0,1,0});
        if(receipt.removed)chips.push_back(D::Launch(receipt.chip,receipt.contact.normal));
    }
    EE::SoilScoop::Body terrain;terrain.Reset(&body);
    std::printf("prepared %zu chips\n",chips.size());std::fflush(stdout);
    double total=0,peak=0,idle=0;
    for(int frame=0;frame<420;++frame)
    {
        auto start=std::chrono::steady_clock::now();for(auto& chip:chips)D::Advance(chip,body,1./60,&terrain);
        double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();total+=ms;peak=std::max(peak,ms);if(frame>=360)idle+=ms;
    }
    int active=0;for(auto const& chip:chips)if(!chip.settled){++active;std::printf("active id %d offset %.3f %.3f %.3f speed %.5f spin %.5f rest %.5f\n",chip.chip.id,chip.offset[0],chip.offset[1],chip.offset[2],G::Dot(chip.velocity,chip.velocity),G::Dot(chip.spin,chip.spin),chip.restTime);}
    std::printf("debris %zu active %d frame avg %.3f peak %.3f idle %.3f ms\n",chips.size(),active,total/420,peak,idle/60);
    check(chips.size()==24,"fixture releases 24 chips");check(active==0,"all chips settle within seven seconds");
    check(total/420<3.,"twenty-four simultaneous slope chips stay below three milliseconds per frame");
    for(auto const& chip:chips)
    {
        for(double v:chip.offset)check(std::isfinite(v),"finite settled position");
        double lift=EE::GraniteOutcropGround::RequiredLift(D::Geometry(chip),G::P{});
        check(lift<=1e-7,"settled cast does not penetrate soil");
        auto copy=chip;check(!D::Advance(copy,body,1./60),"sleeping chip needs no pose update");
        check(copy.offset==chip.offset&&copy.orientation==chip.orientation,"sleep preserves pose");
    }
    if(!chips.empty())
    {
        auto localSleep=chips.front();localSleep.settled=localSleep.restingOnRock=true;
        localSleep.supportRevision=G::LocalRevision(body,G::Add(localSleep.chip.center,localSleep.offset),localSleep.radius);
        localSleep.checkedRockRevision=body.revision;++body.revision;
        check(!D::Advance(localSleep,body,1./60,&terrain)&&localSleep.settled,"distant rock revision does not wake a locally supported chip");
        G::Hit supportHit;for(auto const& triangle:body.collisionQuery.triangles)if(triangle.n[2]>.55)
        {supportHit={true,-1,0,G::Mul(G::Add(G::Add(triangle.a,triangle.b),triangle.c),1./3),triangle.n};break;}
        check(supportHit.hit&&supportHit.normal[2]>.55,"rock support fixture finds an upward face");
        auto exactSleep=chips.front();exactSleep.offset=G::Add(supportHit.position,G::Mul(exactSleep.chip.center,-1));exactSleep.settled=exactSleep.restingOnRock=true;
        exactSleep.rockSupportPoint=supportHit.position;exactSleep.rockSupportNormal=supportHit.normal;exactSleep.checkedRockRevision=body.revision-1;
        exactSleep.supportRevision=G::LocalRevision(body,supportHit.position,exactSleep.radius)+1;
        check(!D::Advance(exactSleep,body,1./60,&terrain)&&exactSleep.settled,"nearby revision keeps a chip asleep when its exact rock contact remains");
        auto deferred=exactSleep;deferred.rockSupportPoint={100,100,100};deferred.checkedRockRevision=body.revision-1;++deferred.supportRevision;
        check(!D::Advance(deferred,body,1./60,&terrain,false)&&deferred.settled,"unsupported rock chip stays queued when this frame's wake budget is spent");
        auto localRevision=G::LocalRevision(body,G::Add(deferred.chip.center,deferred.offset),deferred.radius);
        bool woke=D::Advance(deferred,body,1./60,&terrain,true);
        if(!woke||deferred.settled)std::printf("deferred wake checked %u body %u support %u local %u settled %d\n",deferred.checkedRockRevision,body.revision,deferred.supportRevision,localRevision,int(deferred.settled));
        check(woke&&!deferred.settled,"queued unsupported rock chip wakes when budget is available");
        auto quietSoil=chips.front();quietSoil.settled=true;quietSoil.restingOnRock=false;quietSoil.soilRevision=terrain.revision;++terrain.revision;
        check(!D::Advance(quietSoil,body,1./60,&terrain)&&quietSoil.settled,"terrain revision does not wake an entire settled granite pile");
        check(D::Nudge(quietSoil,{1,0,0},G::Add(quietSoil.chip.center,quietSoil.offset))&&!quietSoil.settled,"explicit tool tap still wakes a quiet soil-resting chip");
        auto trapped=chips.front();trapped.settled=trapped.restingOnRock=false;trapped.velocity={0,0,.5};trapped.spin={2,1,-1};
        trapped.watchdogPosition=G::Add(trapped.chip.center,trapped.offset);trapped.watchdogClock=.999;trapped.trappedTime=2.1;trapped.watchdogContact=true;
        D::Advance(trapped,body,1./60);
        check(trapped.settled&&G::Dot(trapped.velocity,trapped.velocity)==0,"contacted repeating chip reaches deterministic watchdog sleep");
    }
    // Compare the planar shortcut with independent full face/soil clipping,
    // including tilted casts, curved terrain transitions and patch boundaries.
    G::P positions[]={{0,-2,.5},{1.4,-.2,.8},{-1,1,1},{0,2,2},{2.8,1,2},{4,3,2},{-3.99,-3.99,1},{7.02,6.52,2}};
    for(size_t i=0;i<std::min(size_t(6),chips.size());++i)for(auto p:positions)for(int angle=0;angle<3;++angle)
    {
        auto pose=chips[i];double a=.37+angle*.73;auto axis=G::Unit(G::P{1.,double(i+1),.7});
        pose.orientation={std::cos(a),axis[0]*std::sin(a),axis[1]*std::sin(a),axis[2]*std::sin(a)};
        pose.offset=G::Add(p,G::Mul(pose.chip.center,-1));
        double fast=D::RequiredLift(pose),reference=EE::GraniteOutcropGround::RequiredLift(D::Geometry(pose),G::P{});
        check(std::isfinite(fast)&&std::abs(fast-reference)<1e-7,"fast support agrees with full clipping");
    }
    std::printf("Debris regression: %d checks %d failures\n",checks,failures);
    return failures?1:0;
}
