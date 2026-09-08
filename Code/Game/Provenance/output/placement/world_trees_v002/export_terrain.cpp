#include "../../../Geometry/PlayableLandscape.h"
#include "../../../Geometry/GraniteContactCast.h"
#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>

namespace L=EE::PlayableLandscape;
namespace G=EE::GraniteContactCast;
template<class T> void Write(std::ofstream& f,T const& x){f.write(reinterpret_cast<char const*>(&x),sizeof(x));}
void Point(std::ofstream& f,G::P p){p[0]+=L::OriginX;p[1]+=L::OriginY;p[2]+=L::OriginZ;for(double v:p)Write(f,v);}
int main(int argc,char** argv)
{
    auto const& grid=L::Terrain();
    if(argc>1)
    {
        std::ifstream in(argv[1]);std::string id;double x,y,z,r;
        size_t n=0;double error=0;
        while(in>>id>>x>>y>>z>>r)
        {
            double h=0;
            if(!grid.Surface(x-L::OriginX,y-L::OriginY,h)||L::CoreDistance(x-L::OriginX,y-L::OriginY)<r+3)return 2;
            error=std::max(error,std::abs(z-(h+L::OriginZ-.025)));
            for(int i=0;i<24;++i)
            {
                double angle=i*6.283185307179586/24,px=x+r*std::cos(angle),py=y+r*std::sin(angle);
                if(!grid.Surface(px-L::OriginX,py-L::OriginY,h))return 3;
                auto creek=L::Creek(px,py);
                if(std::abs(creek.across)<creek.half+1.5)return 4;
            }
            ++n;
        }
        if(!n||error>1e-7)return 5;
        std::cout<<"PASS "<<n<<" placements: exact rendered terrain collar error "<<std::setprecision(12)<<error<<" m; 24 root-footprint samples per tree clear of creek, floor edge and protected lab.\n";
        return 0;
    }
    std::vector<uint32_t> faces;
    for(int y=0;y<grid.NY();++y)for(int x=0;x<grid.NX();++x)if(grid.Cell(x,y))
    {
        auto a=uint32_t(grid.ID(x,y)),b=a+1,c=uint32_t(grid.ID(x+1,y+1)),d=c-1;
        faces.insert(faces.end(),{a,b,c,a,c,d});
    }
    std::ofstream f("terrain.bin",std::ios::binary);
    Write(f,uint32_t(grid.xs.size()));Write(f,uint32_t(grid.ys.size()));Write(f,uint32_t(faces.size()/3));
    for(double x:grid.xs)Write(f,x+L::OriginX);
    for(double y:grid.ys)Write(f,y+L::OriginY);
    for(auto v:grid.values){Write(f,v.height+L::OriginZ);Write(f,v.cover);Write(f,v.mud);}
    for(auto v:faces)Write(f,v);
    // Export the existing shared creek recipe at every landscape row.
    for(double y:grid.ys){auto c=L::Creek(0,y+L::OriginY);Write(f,L::CreekX(y+L::OriginY));Write(f,L::CreekSlope(y+L::OriginY));Write(f,c.width);}
    std::ofstream core("core.bin",std::ios::binary);
    Write(core,uint32_t(L::Core::NX*L::Core::NY*2));
    for(int y=0;y<L::Core::NY;++y)for(int x=0;x<L::Core::NX;++x)
    {auto q=L::Core::Quad(x,y);for(int k:{0,1,2,0,2,3})Point(core,q[k]);}
    G::Body body;auto boundary=G::Boundary(body);
    std::ofstream rock("granite.bin",std::ios::binary);Write(rock,uint32_t(boundary.size()));
    for(auto const& t:boundary){Point(rock,t.a);Point(rock,t.b);Point(rock,t.c);}
    if(!f||!core||!rock)return 1;
    std::cout<<"Exported actual terrain: "<<faces.size()/3<<" outer triangles, protected core and "<<boundary.size()<<" granite triangles.\n";
}
