#include "Game/Provenance/Systems/GrassShadowBenchmark.h"
#include <limits>
int main()
{
    using B=EE::OutcropLab::GrassShadowBenchmark;
    int checks=0,failures=0;
    auto check=[&](bool ok){++checks;if(!ok)++failures;};
    auto empty=B::Summarize({});check(empty.count==0&&empty.mean==0);
    auto s=B::Summarize({4,1,2,3});check(s.count==4&&s.mean==2.5&&s.p95==4&&s.p99==4&&s.maximum==4);
    check(!B::Shadows(0)&&B::Shadows(1)&&B::Shadows(2)&&!B::Shadows(3));
    B b;b.Start();check(b.Running()&&!b.finished);
    check(!b.Advance(std::numeric_limits<double>::quiet_NaN(),true)&&b.elapsed==0);
    b.Advance(2,true);check(b.samples[0].empty());
    b.Advance(2,false);check(b.elapsed==0&&b.samples[0].empty());
    for(int i=0;i<4;++i){b.samples[i]={double(i+1)};}
    check(b.Combined(false).mean==2.5&&b.Combined(true).mean==2.5);
    b.Start();check(b.samples[0].empty()&&b.phase==0);
    // Completion writes only into the test output working directory.
    while(b.Running())b.Advance(B::Shadows(b.phase)?.0165:.016,true);
    check(b.finished&&b.pass&&b.offResult.count>120&&b.onResult.count>120);
    b.Start();while(b.Running())b.Advance(B::Shadows(b.phase)?.024:.016,true);
    check(b.finished&&!b.pass);
    std::printf("Grass shadow benchmark: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
