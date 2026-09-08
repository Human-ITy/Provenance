#include "../Geometry/GraniteOutcropV2.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace G=EE::GraniteOutcropV2;

static bool ExportOBJ(char const* path)
{
    G::Body body;
    std::ofstream out(path);
    if(!out)return false;
    out<<"# Provenance granite authoring surface\n"
       <<"# Shape directly or remesh in Blender/Nomad; the baker samples the highest surface at each XY column.\n"
       <<"# Grid "<<G::SX+1<<" x "<<G::SY+1<<"; units are meters; local origin is the front-left corner.\n"
       <<"o GraniteOutcropAuthoringSurface\n"<<std::setprecision(12);
    for(int y=0;y<=G::SY;++y)for(int x=0;x<=G::SX;++x)
    {
        auto const& p=body.vertices[G::VertexID(x,y,G::SZ)];
        out<<"v "<<x*G::Spacing<<' '<<y*G::YSpacing<<' '<<p[2]<<'\n';
    }
    auto id=[](int x,int y){return 1+x+(G::SX+1)*y;};
    for(int y=0;y<G::SY;++y)for(int x=0;x<G::SX;++x)
    {
        // Quads make broad face selection and proportional editing easier.
        out<<"f "<<id(x,y)<<' '<<id(x+1,y)<<' '<<id(x+1,y+1)<<' '<<id(x,y+1)<<'\n';
    }
    return bool(out);
}

struct AuthoringMesh
{
    std::vector<G::P> vertices;
    std::vector<std::array<int,3>> triangles;
};

static bool ReadOBJ(char const* path,AuthoringMesh& mesh)
{
    std::ifstream in(path);if(!in)return false;std::string line;
    while(std::getline(in,line))
    {
        if(line.size()>2&&line[0]=='v'&&line[1]==' ')
        {
            std::istringstream stream(line.substr(2));G::P p{};
            if(!(stream>>p[0]>>p[1]>>p[2]))return false;mesh.vertices.push_back(p);
        }
        else if(line.size()>2&&line[0]=='f'&&line[1]==' ')
        {
            std::istringstream stream(line.substr(2));std::vector<int> polygon;std::string token;
            while(stream>>token)
            {
                auto slash=token.find('/');if(slash!=std::string::npos)token.resize(slash);
                int index=std::stoi(token);if(index<0)index=int(mesh.vertices.size())+index;else --index;
                if(index<0||index>=int(mesh.vertices.size()))return false;polygon.push_back(index);
            }
            for(size_t i=1;i+1<polygon.size();++i)mesh.triangles.push_back({polygon[0],polygon[i],polygon[i+1]});
        }
    }
    if(mesh.vertices.empty()||mesh.triangles.empty())return false;
    return true;
}

static double Cross2(G::P const& a,G::P const& b,G::P const& c)
{
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
}

static bool RasterizeOBJ(char const* path,std::array<double,(G::SX+1)*(G::SY+1)>& heights)
{
    AuthoringMesh mesh;if(!ReadOBJ(path,mesh))return false;G::Body fallback;int covered=0;
    std::array<unsigned char,(G::SX+1)*(G::SY+1)> fromSculpt{};
    for(int gy=0;gy<=G::SY;++gy)for(int gx=0;gx<=G::SX;++gx)
    {
        G::P p={gx*G::Spacing,gy*G::YSpacing,0};double highest=-std::numeric_limits<double>::infinity();
        for(auto const& indices:mesh.triangles)
        {
            auto const& a=mesh.vertices[indices[0]];auto const& b=mesh.vertices[indices[1]];auto const& c=mesh.vertices[indices[2]];
            double area=Cross2(a,b,c);if(std::abs(area)<1e-12)continue;
            double wa=Cross2(p,b,c)/area,wb=Cross2(a,p,c)/area,wc=1.-wa-wb;
            if(wa>=-1e-7&&wb>=-1e-7&&wc>=-1e-7)highest=std::max(highest,wa*a[2]+wb*b[2]+wc*c[2]);
        }
        size_t id=size_t(gx+(G::SX+1)*gy);
        if(std::isfinite(highest)){heights[id]=highest;fromSculpt[id]=1;++covered;}
        else heights[id]=fallback.vertices[G::VertexID(gx,gy,G::SZ)][2];
    }
    std::printf("Rasterized %zu vertices / %zu triangles onto %d of %zu authoritative columns; %zu edge columns retained.\n",
        mesh.vertices.size(),mesh.triangles.size(),covered,heights.size(),heights.size()-covered);
    if(covered<int(heights.size()*.65))
    {std::fprintf(stderr,"The sculpt covers too little of the authoritative footprint. Check its scale and axes.\n");return false;}
    struct Jump{double rise;int ax,ay,bx,by;bool aSculpt,bSculpt;};
    std::vector<Jump> jumps;
    auto sample=[&](int x,int y){return size_t(x+(G::SX+1)*y);};
    for(int y=0;y<=G::SY;++y)for(int x=0;x<=G::SX;++x)
    {
        auto a=sample(x,y);
        if(x<G::SX)
        {
            auto b=sample(x+1,y);jumps.push_back({std::abs(heights[b]-heights[a]),x,y,x+1,y,fromSculpt[a]!=0,fromSculpt[b]!=0});
        }
        if(y<G::SY)
        {
            auto b=sample(x,y+1);jumps.push_back({std::abs(heights[b]-heights[a]),x,y,x,y+1,fromSculpt[a]!=0,fromSculpt[b]!=0});
        }
    }
    std::sort(jumps.begin(),jumps.end(),[](Jump const& a,Jump const& b){return a.rise>b.rise;});
    std::printf("Largest adjacent height jumps (S=sculpt, F=retained fallback):\n");
    for(size_t i=0;i<std::min<size_t>(12,jumps.size());++i)
    {
        auto const& j=jumps[i];
        std::printf("  %.4f m  (%d,%d,%c %.4f) -> (%d,%d,%c %.4f)\n",j.rise,
            j.ax,j.ay,j.aSculpt?'S':'F',heights[sample(j.ax,j.ay)],
            j.bx,j.by,j.bSculpt?'S':'F',heights[sample(j.bx,j.by)]);
    }
    return true;
}

static bool BakeHeader(char const* objPath,char const* headerPath)
{
    std::array<double,(G::SX+1)*(G::SY+1)> heights{};
    if(!RasterizeOBJ(objPath,heights))return false;
    std::ofstream out(headerPath);if(!out)return false;
    out<<"#pragma once\n#include <algorithm>\n#include <array>\n#include <cmath>\n\n"
       <<"// Generated by GraniteOutcropShapeTool from the artist-edited OBJ.\n"
       <<"// The values are the authoritative visible surface in local meters.\n"
       <<"namespace EE::GraniteOutcropAuthoredShape\n{\n"
       <<"    inline constexpr int SX="<<G::SX<<",SY="<<G::SY<<";\n"
       <<"    inline constexpr std::array<double,(SX+1)*(SY+1)> Heights={\n        "<<std::setprecision(12);
    for(size_t i=0;i<heights.size();++i)
    {
        if(i&&i%6==0)out<<"\n        ";
        out<<heights[i];if(i+1<heights.size())out<<',';
    }
    out<<"\n    };\n"
       <<"    inline double Sample(double x,double y,double width,double depth)\n"
       <<"    {\n"
       <<"        double gx=std::clamp(x/width,0.,1.)*SX,gy=std::clamp(y/depth,0.,1.)*SY;\n"
       <<"        int ix=std::min(SX-1,int(std::floor(gx))),iy=std::min(SY-1,int(std::floor(gy)));\n"
       <<"        double u=gx-ix,v=gy-iy;auto h=[&](int px,int py){return Heights[px+(SX+1)*py];};\n"
       <<"        return (1-u)*(1-v)*h(ix,iy)+u*(1-v)*h(ix+1,iy)+(1-u)*v*h(ix,iy+1)+u*v*h(ix+1,iy+1);\n"
       <<"    }\n}\n";
    return bool(out);
}

int main(int argc,char** argv)
{
    if(argc==3&&std::string(argv[1])=="export")
    {
        if(!ExportOBJ(argv[2])){std::fprintf(stderr,"Could not export %s\n",argv[2]);return 1;}
        std::printf("Exported editable granite surface: %s\n",argv[2]);return 0;
    }
    if(argc==4&&std::string(argv[1])=="bake")
    {
        if(!BakeHeader(argv[2],argv[3])){std::fprintf(stderr,"Could not bake %s\n",argv[2]);return 1;}
        std::printf("Baked authoritative granite surface: %s\n",argv[3]);return 0;
    }
    std::printf("Usage:\n  GraniteOutcropShapeTool export <surface.obj>\n  GraniteOutcropShapeTool bake <surface.obj> <GraniteOutcropAuthoredShape.h>\n");
    return 2;
}
