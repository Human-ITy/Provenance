#pragma once
#include "GraniteContactCast.h"
#include "PlayableLandscape.h"

namespace EE::GraniteBuildPlacement
{
    namespace G=GraniteContactCast;
    // The catalog release cannot also place an object. A catalog drag can
    // transfer its held gesture; a catalog click requires a fresh world press.
    struct Gesture
    {
        bool pending=false,armed=false;
        void Begin(bool dragging){pending=true;armed=dragging;}
        void Cancel(){pending=armed=false;}
        bool Update(bool world,bool valid,bool pressed,bool released,bool camera)
        {
            if(!pending)return false;
            if(pressed&&world&&!camera)armed=true;
            if(!released)return false;
            bool commit=armed&&world&&valid&&!camera;
            armed=false;
            if(commit)pending=false;
            return commit;
        }
    };
    inline double BoxDistance(G::P o,G::P d,G::P low,G::P high,double limit)
    {
        if(!G::Finite(o)||!G::Finite(d)||G::Dot(d,d)<1e-16||!G::Finite(low)||!G::Finite(high)||!std::isfinite(limit)||limit<0)return limit+1;
        double enter=0,leave=limit;
        for(int k=0;k<3;++k)
        {
            if(low[k]>high[k])return limit+1;
            if(std::abs(d[k])<1e-12){if(o[k]<low[k]||o[k]>high[k])return limit+1;continue;}
            double a=(low[k]-o[k])/d[k],b=(high[k]-o[k])/d[k];
            if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);
            if(enter>leave)return limit+1;
        }
        return enter;
    }
    // Split the ray at the actual grid planes, then test only crossed cells.
    // Uses the rendered triangles, with no analytic-height proxy or giant BVH.
    inline G::Hit LandscapeRay(G::P o,G::P ray,double reach)
    {
        G::Hit hit;
        if(!G::Finite(o)||!G::Finite(ray)||G::Dot(ray,ray)<1e-16||!std::isfinite(reach)||reach<=0)return hit;
        auto d=G::Unit(ray);auto const& grid=PlayableLandscape::Terrain();
        std::vector<double> cuts{0,reach};
        auto split=[&](auto const& axis,int k){if(std::abs(d[k])>1e-12)for(double p:axis){double t=(p-o[k])/d[k];if(t>0&&t<reach)cuts.push_back(t);}};
        split(grid.xs,0);split(grid.ys,1);std::sort(cuts.begin(),cuts.end());
        for(size_t i=1;i<cuts.size();++i)
        {
            auto p=G::Add(o,G::Mul(d,(cuts[i-1]+cuts[i])*.5));
            int x=int(std::upper_bound(grid.xs.begin(),grid.xs.end(),p[0])-grid.xs.begin())-1;
            int y=int(std::upper_bound(grid.ys.begin(),grid.ys.end(),p[1])-grid.ys.begin())-1;
            if(x<0||y<0||x>=grid.NX()||y>=grid.NY()||!grid.Cell(x,y))continue;
            auto a=grid.Vertex(x,y),b=grid.Vertex(x+1,y),c=grid.Vertex(x+1,y+1),e=grid.Vertex(x,y+1);
            auto test=[&](G::P v,G::P w,G::P z){double t=G::Intersect(v,w,z,o,d);if(t>=0&&t<=reach&&(!hit.hit||t<hit.distance))hit={true,-1,t,G::Add(o,G::Mul(d,t)),G::Unit(G::Cross(G::Add(w,G::Mul(v,-1)),G::Add(z,G::Mul(v,-1))))};};
            test(a,b,c);test(a,c,e);
            if(hit.hit)return hit;
        }
        return hit;
    }
}
