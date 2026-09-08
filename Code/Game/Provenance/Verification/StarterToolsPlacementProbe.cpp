#include "../Geometry/GraniteLabContact.h"
#include <cstdio>
int main()
{
    namespace G=EE::GraniteContactCast;G::Body rock;EE::SoilScoop::Body soil;soil.Reset(&rock);
    const char* ids[]={"stone_shovel","stone_axe","stone_pickaxe"};double desired[]={.65,1.50,3.15};
    std::puts("[");
    for(int tool=0;tool<3;++tool)
    {
        double best=1e20,bx=0,by=0,bz=0,variation=0;
        for(int ix=0;ix<11;++ix)for(int iy=0;iy<34;++iy)
        {
            double px=desired[tool]+(ix-5)*.035,py=.12+iy*.05,top=-1e20,low=1e20;bool clear=true;
            for(int i=0;i<5;++i)for(int j=0;j<5;++j)
            {
                double x=px-.035+i*.0175,y=py-.035+j*.0175,z;
                if(!soil.SurfaceBelow(x,y,3,z,false)){clear=false;continue;}
                auto hit=G::Raycast(rock,{x,y,3},{0,0,-1},4);
                if(!hit.hit||hit.position[2]<=z+.04){clear=false;continue;}
                top=std::max(top,hit.position[2]);low=std::min(low,hit.position[2]);
            }
            double score=(top-low)*4+py*.025+std::abs(px-desired[tool])*.15;
            if(clear&&score<best){best=score;bx=px;by=py;bz=top;variation=top-low;}
        }
        if(best==1e20)return 1;
        std::printf("{\"id\":\"%s\",\"support_world\":[%.9f,%.9f,%.9f],\"support_height_range_m\":%.9f,\"support\":\"granite ledge\"}%s\n",ids[tool],bx-1.4,by+35,bz+.12,variation,tool==2?"":",");
    }
    std::puts("]");
}
