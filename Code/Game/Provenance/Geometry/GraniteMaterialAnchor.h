#pragma once
#include "GraniteContactCast.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace EE::GraniteMaterialAnchor
{
    // Immutable material coordinates, not a simulated pose or a color snapshot.
    // UV0=source world XY, UV1=source world Z + opt-in marker. Color RGB stores
    // the source normal; alpha marks a valid payload, never material opacity.
    struct Payload { std::array<float,4> uv; uint32_t normal; };
    inline Payload Encode(GraniteContactCast::P sourceWorld,GraniteContactCast::P sourceNormal)
    {
        auto n=GraniteContactCast::Unit(sourceNormal);
        uint32_t packed=0xff000000u;
        for(int k=0;k<3;++k)
            packed|=uint32_t(std::lround(std::clamp(n[k]*.5+.5,0.,1.)*255.))<<(8*k);
        return {{{float(sourceWorld[0]),float(sourceWorld[1]),float(sourceWorld[2]),1.f}},packed};
    }
}
