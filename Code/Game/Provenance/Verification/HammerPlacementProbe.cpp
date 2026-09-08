#include "../Geometry/GraniteLabContact.h"
#include "../Geometry/GrassCover.h"
#include <cstdio>
int main()
{
    namespace G=EE::GraniteContactCast;G::Body rock;EE::SoilScoop::Body soil;soil.Reset(&rock);
    // Upright selection display on the front granite ledge. The pommel rests
    // above stone; the head remains visible above surrounding living grass.
    EE::GrassCover::Model grass;grass.Reset();grass.ExposeRockMargin(*soil.originalRock);
    double best=1e20,bx=0,by=0,bz=0,variation=0,cover=0;
    for(int cx=0;cx<30;++cx)for(int cy=0;cy<24;++cy)
    {
        double px=.15+cx*.12,py=.10+cy*.10,top=-1e20,low=1e20;bool clear=true;double meanCover=0;
        for(int i=0;i<=4;++i)for(int j=0;j<=6;++j)
        {
            double x=px-.033+.066*i/4,y=py-.033+.066*j/6,z=0;
            if(!soil.SurfaceBelow(x,y,3,z,false)){clear=false;continue;}
            auto hit=G::Raycast(rock,{x,y,3},{0,0,-1},4);
            if(!hit.hit||hit.position[2]<=z+.05){clear=false;continue;}
            z=hit.position[2];
            top=std::max(top,z);low=std::min(low,z);
            int gx=int((x-EE::GraniteOutcropGround::X)*EE::GrassCover::Resolution/EE::GrassCover::Width),gy=int((y-EE::GraniteOutcropGround::Y)*EE::GrassCover::Resolution/EE::GrassCover::Length);
            if(gx>=0&&gy>=0&&gx<EE::GrassCover::Resolution&&gy<EE::GrassCover::Resolution)meanCover+=grass.cells[gy*EE::GrassCover::Resolution+gx].cover/35;
        }
        double distance=std::hypot(px-2.,py+1.8);
        if(distance<.55||distance>2.4)continue;
        double score=(top-low)*3+meanCover*.12+distance*.015;
        if(clear&&score<best){best=score;bx=px;by=py;bz=top;variation=top-low;cover=meanCover;}
    }
    if(best==1e20)return 1;
    std::printf("{\"world_position\":[%.9f,%.9f,%.9f],\"rotation_degrees\":[0,0,0],\"footprint_surface_range_m\":%.9f,\"support\":\"granite ledge, upright selection display\",\"terrain_clear\":true}\n",bx-1.4,by+35,bz+.12+.003,variation);
}
