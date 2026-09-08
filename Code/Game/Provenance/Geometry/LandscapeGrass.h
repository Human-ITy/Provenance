#pragma once
#include "PlayableLandscape.h"
#include "GrassCover.h"

// Immutable exterior presentation, not a second editable cover ledger.
// World-stable candidates: streaming never changes seed, position or habitat.
namespace EE::LandscapeGrass
{
    namespace L=PlayableLandscape;
    inline constexpr double TileSize=4,Step=.125;
    inline constexpr int Cells=32,Radius=5,RetainRadius=6,MaxPatches=169;
    inline constexpr size_t MaxTuftsPerPatch=Cells*Cells;
    inline double Density(double x,double y)
    {
        // Continue the lab's ~70 tufts/m2 across its boundary, then transition
        // to a lighter meadow population. No inward rectangular bald collar.
        return 1.-.92*L::Core::Smooth(L::CoreDistance(x,y)/7.);
    }
    inline GrassCover::Tuft Candidate(int tileX,int tileY,int index)
    {
        GrassCover::Tuft t{};
        int ix=tileX*Cells+index%Cells,iy=tileY*Cells+index/Cells;
        uint32_t h=GrassCover::BladeHash(uint32_t(ix)*0x9e3779b9u^uint32_t(iy)*0x85ebca6bu);
        double x=(ix+.5+(GrassCover::BladeUnit(h)-.5)*.9)*Step;
        double y=(iy+.5+(GrassCover::BladeUnit(GrassCover::BladeHash(h))-.5)*.9)*Step;
        if(x<L::MinX||x>L::MaxX||y<L::MinY||y>L::MaxY||L::InCore(x,y))return t;
        auto recipe=L::Evaluate(x,y);
        if(GrassCover::BladeUnit(GrassCover::BladeHash(h^0x68bc21ebu))>=Density(x,y)*recipe.cover)return t;
        double height=0;if(!L::Terrain().Surface(x,y,height))return t;
        t.emitted=true;t.center={x,y,height};
        t.vigor=.25+.65*GrassCover::BladeUnit(GrassCover::BladeHash(h^0x23af129du));
        t.height=.3048+.1524*t.vigor;t.width=.115+.045*GrassCover::BladeUnit(h>>1);
        t.angle=6.283185307179586*GrassCover::BladeUnit(h>>2);
        return t;
    }
}
