#pragma once
#include "GraniteContactDebris.h"
#include "GraniteLabMovement.h"

namespace EE::GraniteLabContact
{
    namespace G=GraniteContactCast;
    namespace D=GraniteContactDebris;
    namespace S=GraniteOutcropGround;
    // Lab walking policy, not a granite friction/material constant. A tread
    // may slope up to 60 degrees from horizontal; walls remain ineligible.
    inline constexpr double RockTreadMinNormalZ=.5;
    enum class Material {None,Host,Soil,Loose};
    struct Contact {Material material=Material::None;G::Hit hit;int loose=-1;};
    inline G::SurfaceQuery const& SoilQuery()
    {
        static auto const query=[](){G::Poly faces;
            for(int y=0;y<S::NY;++y)for(int x=0;x<S::NX;++x)
            {
                auto q=S::Quad(x,y);faces.push_back({{q[0],q[1],q[2]},0,0});faces.push_back({{q[0],q[2],q[3]},0,0});
                auto skirt=[&](G::P a,G::P b){auto c=b,d=a;c[2]=d[2]=-.12;faces.push_back({{a,b,c,d},0,0});};
                if(y==0)skirt(q[1],q[0]);if(y==S::NY-1)skirt(q[3],q[2]);if(x==0)skirt(q[0],q[3]);if(x==S::NX-1)skirt(q[2],q[1]);
            }
            G::SurfaceQuery result;result.Build(faces);return result;}();
        return query;
    }
    // The nearest visible surface owns contact. The caller routes soil to its
    // scoop and host granite to the pick. Loose bodies still block either tool.
    inline Contact Trace(G::Body const& body,std::vector<D::FallingChip> const& debris,G::P origin,G::P direction,double reach=2,SoilScoop::Body const* terrain=nullptr)
    {
        Contact result;if(!G::Finite(origin)||!G::Finite(direction)||G::Dot(direction,direction)<1e-16||!std::isfinite(reach)||reach<=0)return result;
        auto ray=G::Unit(direction);result.hit=G::Raycast(body,origin,ray,reach);if(result.hit.hit)result.material=Material::Host;
        double nearest=result.hit.hit?result.hit.distance:reach+1e-8;
        G::Hit soil;if(terrain)terrain->Trace(origin,ray,nearest,soil);else SoilQuery().Trace(0,origin,ray,nearest,soil,-1);
        if(soil.hit){result.hit=soil;result.material=Material::Soil;}
        for(size_t i=0;i<debris.size();++i)
        {
            auto const& d=debris[i];auto center=G::Add(d.chip.center,d.offset);G::P radius={d.radius,d.radius,d.radius};
            if(!G::RayBox(origin,ray,G::Add(center,G::Mul(radius,-1)),G::Add(center,radius),nearest))continue;
            auto q=d.orientation;for(int k=1;k<4;++k)q[k]=-q[k];
            auto localOrigin=G::Add(d.chip.center,D::Rotate(q,G::Add(origin,G::Mul(center,-1))));auto localRay=D::Rotate(q,ray);
            for(auto const& t:d.chip.mesh)
            {
                double distance=G::Intersect(t.a,t.b,t.c,localOrigin,localRay);
                if(distance<nearest){nearest=distance;result.material=Material::Loose;result.loose=int(i);result.hit={true,d.chip.id,distance,G::Add(origin,G::Mul(ray,distance)),D::Rotate(d.orientation,G::Unit(G::Cross(G::Add(t.b,G::Mul(t.a,-1)),G::Add(t.c,G::Mul(t.a,-1)))))};}
            }
        }
        // Ground chips are small and the exact ray otherwise turns a near miss
        // into a terrain scoop. Give the pick a half-inch selection skin around
        // actual chip triangles (not the often-large bounding sphere).
        if(result.material!=Material::Loose)
        {
            constexpr double PickAssist=.0127;double limit=PickAssist*PickAssist;
            for(size_t i=0;i<debris.size();++i)
            {
                auto const& d=debris[i];auto center=G::Add(d.chip.center,d.offset);auto q=d.orientation;for(int k=1;k<4;++k)q[k]=-q[k];
                auto localOrigin=G::Add(d.chip.center,D::Rotate(q,G::Add(origin,G::Mul(center,-1))));auto localRay=D::Rotate(q,ray);auto localEnd=G::Add(localOrigin,G::Mul(localRay,nearest));bool touches=false;G::P low,high;
                for(int k=0;k<3;++k){low[k]=std::min(localOrigin[k],localEnd[k])-PickAssist;high[k]=std::max(localOrigin[k],localEnd[k])+PickAssist;}
                if(!d.collisionQuery.nodes.empty())d.collisionQuery.Visit(0,low,high,[&](G::SurfaceQuery::Tri const& triangle)
                {if(G::SegmentTriangleDistance2(localOrigin,localEnd,triangle.a,triangle.b,triangle.c)<=limit)touches=true;});
                if(!touches)continue;double distance=std::clamp(G::Dot(G::Add(center,G::Mul(origin,-1)),ray),0.,nearest);nearest=distance;result.material=Material::Loose;result.loose=int(i);result.hit={true,d.chip.id,distance,G::Add(origin,G::Mul(ray,distance)),G::Mul(ray,-1)};
            }
        }
        return result;
    }
    inline double StepSurface(G::Body const& body,double x,double y,double ceiling,SoilScoop::Body const* terrain=nullptr,bool includeLooseSoil=true)
    {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(ceiling))return -1e30;
        double top=-1e30,soil;
        if((terrain?terrain->SurfaceBelow(x,y,ceiling,soil,includeLooseSoil):S::Surface(x,y,soil))&&soil<=ceiling)top=soil;
        // Walking queries the same current boundary BVH as capsule collision,
        // not the tool ray's single-face/tetrahedron traversal. At a shared
        // edge, coincident tread and near-vertical faces otherwise alternate
        // as the winning hit with tiny origin-dependent rounding differences.
        // Resolve only coincident heights in favour of the upward tread; a
        // distinct higher steep face still occludes lower walkable surfaces.
        constexpr double Tie=1e-8;
        // Rounded feet can stand above the tread's local column on an incline.
        // Match the movement continuation window rather than clipping it at 2 cm.
        double high=ceiling+1e-5,low=high-(GraniteLabMovement::StepHeight+GraniteLabMovement::Radius);
        double rockTop=-1e30,normalZ=-1;
        if(!body.collisionQuery.nodes.empty())body.collisionQuery.Visit(0,{x-Tie,y-Tie,low},{x+Tie,y+Tie,high},[&](G::SurfaceQuery::Tri const& t)
        {
            double distance=G::Intersect(t.a,t.b,t.c,{x,y,high},{0,0,-1});
            if(distance>high-low)return;
            double z=high-distance;
            if(z>rockTop+Tie){rockTop=z;normalZ=t.n[2];}
            else if(std::abs(z-rockTop)<=Tie){rockTop=std::max(rockTop,z);normalZ=std::max(normalZ,t.n[2]);}
        });
        if(normalZ+1e-12>=RockTreadMinNormalZ)top=std::max(top,rockTop);
        return top;
    }
}
