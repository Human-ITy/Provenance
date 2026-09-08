#include "../Geometry/GraniteLabContact.h"
#include "../Geometry/GrassCover.h"
#include <cstdio>

// Read-only initial-soil placement probe. Does not change grass, rock or soil.
int main()
{
    namespace G=EE::GraniteContactCast;
    G::Body rock;EE::SoilScoop::Body soil;soil.Reset(&rock);
    EE::GrassCover::Model grass;grass.Reset();grass.ExposeRockMargin(*soil.originalRock);
    struct Candidate{char const* id;double x,y,r,yaw;};
    Candidate candidates[]={
        {"healing_bush",-1.4,-1.,.55,25}, {"stamina_bush",5.5,2.8,.55,-35},
        {"chestnut",-.35,.45,.10,15}, {"bell",.4,-.45,.10,-20},
        {"ivory",4.45,1.,.10,35}, {"ochre",3.4,2.9,.10,-10},
        {"cyan_pair",-.65,2.35,.18,25}, {"violet_lantern",4.65,2.45,.18,-25}};
    bool all=true;std::puts("[");
    for(size_t index=0;index<std::size(candidates);++index)
    {
        auto c=candidates[index];double best=1e30,bx=c.x,by=c.y,bz=0,variation=0,bestCover=1;
        int extent=c.r<.5?10:6;
        for(int ix=-extent;ix<=extent;++ix)for(int iy=-extent;iy<=extent;++iy)
        {
            double x=c.x+ix*.08,y=c.y+iy*.08,z;
            if(!soil.SurfaceBelow(x,y,3.,z,false))continue;
            bool clear=true;double low=z,high=z;
            for(int k=0;k<12;++k)
            {
                double angle=k*6.283185307179586/12,px=x+c.r*std::cos(angle),py=y+c.r*std::sin(angle),pz;
                if(!soil.SurfaceBelow(px,py,3.,pz,false)){clear=false;break;}
                low=std::min(low,pz);high=std::max(high,pz);
                auto hit=G::Raycast(rock,{px,py,3.},{0,0,-1},4.);
                if(hit.hit&&hit.position[2]>pz-.01)clear=false;
            }
            auto hit=G::Raycast(rock,{x,y,3.},{0,0,-1},4.);
            if(hit.hit&&hit.position[2]>z-.01)clear=false;
            int gx=int((x-EE::GraniteOutcropGround::X)*EE::GrassCover::Resolution/EE::GrassCover::Width),gy=int((y-EE::GraniteOutcropGround::Y)*EE::GrassCover::Resolution/EE::GrassCover::Length);
            if(gx<0||gy<0||gx>=EE::GrassCover::Resolution||gy>=EE::GrassCover::Resolution)continue;
            double cover=grass.cells[gy*EE::GrassCover::Resolution+gx].cover;
            // Prefer existing sparse cover for small mushrooms; no clearing,
            // wear or ecological state changes are introduced for decoration.
            double score=high-low+.025*std::hypot(x-c.x,y-c.y)+(c.r<.5?.35*cover:0);
            // Mushroom groups share a planting plane. Seat it at the lowest
            // sampled root support so a sloping bank buries roots slightly
            // instead of leaving the downhill stems suspended. Bushes have a
            // single trunk root and use the center soil height.
            if(clear&&score<best){best=score;bx=x;by=y;bz=c.r<.5?low:z;variation=high-low;bestCover=cover;}
        }
        if(best==1e30)all=false;
        std::printf("{\"id\":\"%s\",\"local\":[%.9f,%.9f,%.9f],\"world\":[%.9f,%.9f,%.9f],\"yaw\":%.1f,\"initial_grass_cover\":%.6f,\"footprint_height_range\":%.9f,\"clear\":%s}%s\n",c.id,bx,by,bz,bx-double(1.4f),by+35.,bz+double(.12f),c.yaw,bestCover,variation,best==1e30?"false":"true",index+1==std::size(candidates)?"":",");
    }
    std::puts("]");return all?0:1;
}
