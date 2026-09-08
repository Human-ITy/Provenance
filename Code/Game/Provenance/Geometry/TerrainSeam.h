#pragma once
#include "GraniteOutcropGround.h"

namespace EE::TerrainSeam
{
    namespace S=GraniteOutcropGround;
    // Presentation only. Do not change the soil field, accounting, cover wear,
    // or walking surface to conceal the finite lab's boundary.
    inline double BladeBlend(double x,double y)
    {
        double edge=std::min({x-S::X,S::X+S::NX*S::Step-x,y-S::Y,S::Y+S::NY*S::Step-y});
        double feather=1.25+.25*std::sin(x*2.1+y*.73)+.20*std::sin(y*1.63-x*.51);
        return S::Smooth((edge-.10)/feather);
    }
    inline bool BuriedPerimeter(S::P a,S::P b,S::P c)
    {
        if(std::max({a[2],b[2],c[2]})>.110001)return false;
        for(int axis=0;axis<2;++axis)
            for(double edge:{axis==0?S::X:S::Y,axis==0?S::X+S::NX*S::Step:S::Y+S::NY*S::Step})
                if(std::abs(a[axis]-edge)<1e-7&&std::abs(b[axis]-edge)<1e-7&&std::abs(c[axis]-edge)<1e-7)return true;
        return false;
    }
    inline S::P TopNormal(S::P p,S::P normal)
    {
        double edge=std::min({p[0]-S::X,S::X+S::NX*S::Step-p[0],p[1]-S::Y,S::Y+S::NY*S::Step-p[1]});
        // Only the intact top. Excavated walls/clods must retain their normals.
        if(edge<0||edge>.24||normal[2]<=0||std::abs(p[2]-S::Height(p[0],p[1]))>.002)return normal;
        double t=S::Smooth(edge/.24);
        return S::Unit(S::Add(S::Mul(normal,t),S::P{0,0,1-t}));
    }
}
