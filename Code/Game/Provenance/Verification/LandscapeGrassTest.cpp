#include "../Geometry/LandscapeGrass.h"
#include "../Geometry/TerrainSeam.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
namespace M=EE::LandscapeGrass;namespace L=EE::PlayableLandscape;
static int checks=0;
static void Check(bool ok,char const* s){++checks;if(!ok){std::printf("FAIL %s\n",s);std::exit(1);}}
int main()
{
    auto started=std::chrono::steady_clock::now();size_t total=0,maximum=0,near=0,far=0;
    for(int y=-22;y<=4;++y)for(int x=-13;x<=13;++x)
    {
        size_t count=0;
        for(int i=0;i<M::Cells*M::Cells;++i)
        {
            auto a=M::Candidate(x,y,i),b=M::Candidate(x,y,i);
            Check(a.center==b.center&&a.emitted==b.emitted,"stable streaming identity");
            if(!a.emitted)continue;
            double h=0;Check(L::Terrain().Surface(a.center[0],a.center[1],h),"outside editable core, inside floor");
            Check(std::abs(h-a.center[2])<1e-10,"root on rendered triangle");
            Check(L::Evaluate(a.center[0],a.center[1]).cover>0,"no grass in bare creek/dirt");
            Check(a.height>=.3048&&a.height<=.4572,"retains grass scale");
            if(L::CoreDistance(a.center[0],a.center[1])<.25)++near;else ++far;
            ++count;
        }
        Check(count<=M::MaxTuftsPerPatch,"bounded patch upload");maximum=std::max(maximum,count);total+=count;
    }
    Check(near>300&&far>10000,"grass extends across former rectangle and world");
    Check(total<100000,"whole-floor candidate budget");
    for(int side=0;side<4;++side)for(int i=0;i<=100;++i)
    {
        double t=i*.01;L::P p={L::Core::X+t*L::Core::NX*L::Core::Step,L::Core::Y+t*L::Core::NY*L::Core::Step,.11};
        if(side<2)p[0]=side?L::Core::X+L::Core::NX*L::Core::Step:L::Core::X;
        else p[1]=side==3?L::Core::Y+L::Core::NY*L::Core::Step:L::Core::Y;
        auto n=EE::TerrainSeam::TopNormal(p,{.4,.3,.8660254});
        Check(n[2]>.999999,"intact edge normal meets world-up terrain");
        p[2]-=.05;Check(EE::TerrainSeam::TopNormal(p,{1,0,0})==L::P{1,0,0},"excavation walls untouched");
    }
    std::printf("PASS %d checks | full-floor tufts %zu | largest patch %zu | seam-near %zu | %.2f ms\n",checks,total,maximum,near,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count());
}
