#include "../Systems/MovementFrameCapture.h"
#include <cstdlib>
#include <cstdio>
#include <limits>
using C=EE::OutcropLab::MovementFrameCapture;
static int checks=0;
static void Check(bool x){++checks;if(!x)std::exit(1);}
int main()
{
    C c;c.Start();C::Context moving{true,false,true,C::Region::Creek,2,0},idle{false,false,true,C::Region::Outer,1,0};
    c.Observe(999,moving);Check(c.samples.empty()); // pre-capture delta excluded
    c.Observe(16,idle);Check(c.samples.size()==1&&c.samples[0].context.moving);
    c.Observe(18,moving);Check(c.samples.size()==2&&!c.samples[1].context.moving);
    auto busy=moving;busy.busy=true;c.Observe(20,busy);c.Observe(100,idle);
    c.Finish();Check(c.moving.count==2&&c.moving.mean==18&&c.moving.p99==20);Check(c.idle.count==1&&c.idle.mean==18);
    c.Start();auto setup=moving;setup.ready=false;c.Observe(1,setup);c.Observe(100,moving);Check(c.samples.empty());
    c.Observe(std::numeric_limits<double>::quiet_NaN(),moving);Check(c.samples.empty());
    for(int i=0;i<2000&&c.running;++i)c.Observe(20,moving);
    Check(c.finished&&!c.running&&c.samples.size()>=1499&&c.samples.size()<=1501);Check(c.moving.p95==20);
    c.Start();for(size_t i=0;i<C::Limit+20;++i)c.Observe(.01,moving);
    Check(c.finished&&c.samples.size()==C::Limit);Check(c.samples.capacity()>=C::Limit);
    Check(c.Write("MovementFrameCaptureTest.csv"));
    std::printf("Movement frame capture: %d checks PASS\n",checks);
}
