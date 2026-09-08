#include "Engine/Render/Shaders/Materials/ProvenanceGrassWind.esh"
#include <algorithm>
#include <cstdio>
#include <cstdint>

int main()
{
    using namespace ProvenanceGrassWind;
    constexpr float tau=6.283185307179586f;
    unsigned checks=0,failed=0;
    auto check=[&](bool pass,char const* message){++checks;if(!pass){if(failed<10)std::printf("FAIL: %s\n",message);++failed;}};
    check(GrassWindWeight(0)==0,"roots remain exactly fixed");
    check(GrassWindWeight(.5f)==.25f,"middle bends less than the tips");
    check(GrassWindWeight(1)==1,"tip receives full wind");
    float maximum=0,wrapMaximum=0,temporalChange=0,neighborChange=0;
    for(int y=0;y<128;y+=3)for(int x=0;x<128;x+=3)
    {
        uint32_t h=uint32_t(x+y*128)+0x9e3779b9U;
        h^=h>>16;h*=0x7feb352dU;h^=h>>15;h*=0x846ca68bU;h^=h>>16;
        float spatial=float(x)*.081f+float(y)*.053f,tuft=tau*float(h&0xffffU)/65535.f;
        float wa=GrassWindAlong(tau,spatial,tuft)-GrassWindAlong(0,spatial,tuft);
        float wc=GrassWindAcross(tau,spatial,tuft)-GrassWindAcross(0,spatial,tuft);
        wrapMaximum=std::max(wrapMaximum,std::sqrt(wa*wa+wc*wc)*.05f);
        for(int t=0;t<256;++t)
        {
            float phase=tau*float(t)/256.f;
            float a=GrassWindAlong(phase,spatial,tuft),c=GrassWindAcross(phase,spatial,tuft);
            float displacement=.05f*std::sqrt(a*a+c*c);
            maximum=std::max(maximum,displacement);
            check(std::isfinite(displacement)&&displacement<.06f,"shader wind stays inside instance and cluster bounds");
            check(GrassWindWeight(0)*displacement==0,"root fixed at every sampled phase");
            check(GrassWindAlong(phase,spatial,tuft)==a,"same phase and cell reproduce the same wind");
            temporalChange=std::max(temporalChange,std::abs(a-GrassWindAlong(phase+.01f,spatial,tuft)));
            neighborChange=std::max(neighborChange,std::abs(a-GrassWindAlong(phase,spatial+.081f,tuft+.1f)));
        }
    }
    check(wrapMaximum<.00001f,"64-second phase wrap is continuous within 10 micrometers");
    check(temporalChange>.01f,"wind actually changes with time");
    check(neighborChange>.01f,"neighboring tufts are not locked in unison");
    std::printf("Grass wind: %u checks, %u failures; max displacement %.6f m (<0.060); max wrap error %.9f m\n",checks,failed,maximum,wrapMaximum);
    return failed?1:0;
}
