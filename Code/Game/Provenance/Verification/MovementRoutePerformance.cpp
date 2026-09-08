#include "../Geometry/GraniteLabContact.h"
#include "../Geometry/PlayableLandscape.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
namespace G=EE::GraniteContactCast;namespace M=EE::GraniteLabMovement;namespace C=EE::GraniteLabContact;namespace L=EE::PlayableLandscape;
using Clock=std::chrono::steady_clock;
int main()
{
    auto begin=Clock::now();auto const& land=L::Terrain();
    std::printf("Immutable terrain setup %.3f ms; excluded from warmed route samples\n",std::chrono::duration<double,std::milli>(Clock::now()-begin).count());
    G::Body rock;EE::SoilScoop::Body soil;soil.Reset(&rock);
    auto floor=[&](double x,double y){double height;return land.Surface(x,y,height)?height:-.12;};
    auto run=[&](char const* name,double wx,double wy,M::Input input,int frames)
    {
        double x=wx-L::OriginX,y=wy-L::OriginY,z=floor(x,y);
        if(L::InCore(x,y))z=rock.high[2]+2; // settle onto the actual soil, never start beneath it
        M::Body body{{x,y,z+M::Skin}};std::vector<double> samples;size_t queries=0;int invalid=0;
        for(int i=-120;i<frames;++i)
        {
            auto start=Clock::now();size_t current=0;
            M::Advance(body,i<0?M::Input{}:input,1./60,
                [&](G::P p,double h){++current;return G::BlocksCapsule(rock,p,M::Radius,h)||soil.BlocksCapsule(p,M::Radius,h,false);},floor,
                [&](double px,double py,double h){return C::StepSurface(rock,px,py,h,&soil,false);});
            double ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
            if(i>=0){samples.push_back(ms);queries+=current;}
            invalid+=!G::Finite(body.feet);
        }
        double mean=0;for(double ms:samples)mean+=ms;mean/=samples.size();std::sort(samples.begin(),samples.end());
        std::printf("%s: n=%d mean=%.6f p95=%.6f p99=%.6f max=%.6f ms queries=%zu end=%.3f,%.3f,%.3f invalid=%d\n",name,frames,mean,samples[size_t(std::ceil(samples.size()*.95))-1],samples[size_t(std::ceil(samples.size()*.99))-1],samples.back(),queries,body.feet[0]+L::OriginX,body.feet[1]+L::OriginY,body.feet[2]+L::OriginZ,invalid);
        if(invalid)std::exit(1);
    };
    run("outer_idle",15,0,{},600);run("outer_walk",15,-10,{0,1},600);
    run("outer_sprint",20,-20,{0,1,true},600);
    run("creek_cross_east",L::CreekX(5)-5,5,{1,0},360);
    run("creek_cross_west",L::CreekX(-10)+5,-10,{-1,0},360);
    run("lab_apron",-3,31.4,{1,0},240);
    std::puts("Observational CPU routes, not graphics/FPS certification. Setup, rendering, grass wear/uploads and tools excluded.");
}
