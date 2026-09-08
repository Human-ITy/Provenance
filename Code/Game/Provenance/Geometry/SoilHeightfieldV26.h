#pragma once
#include "GraniteContactCast.h"
#include "GraniteOutcropGround.h"
#include <set>
#include <chrono>

// Lab heightfield excavation: open scoops, not overhangs or soil collapse.
// Shared fine-grid vertices own both collision and rendering. Only touched
// coarse cells are refined; patch queries and meshes are rebuilt locally.
namespace EE::SoilHeightfieldV26
{
    namespace G=GraniteContactCast;
    namespace S=GraniteOutcropGround;
    using G::P;
    inline constexpr int Fine=8,Group=16,Columns=(S::NX+Group-1)/Group;
    inline constexpr double Step=S::Step/Fine,Bottom=-.12;
    inline constexpr double Diameter=.2159,Depth=.07239;
    // Provisional settled soil: mineral density 2650 kg/m3, solid fraction .60.
    inline constexpr double BulkDensity=2650*.60;
    struct Receipt {double volume=0;uint64_t massMg=0;size_t cells=0;bool blocked=false;};
    struct Body
    {
        std::map<std::pair<int,int>,double> heights;
        std::set<int> refined,dirty;
        std::map<int,G::SurfaceQuery> queries;
        uint32_t revision=0,scoops=0;double removedM3=0;uint64_t removedMg=0;
        static int Patch(int x,int y){return x/Group+(y/Group)*Columns;}
        double Node(int x,int y)const
        {
            auto it=heights.find({x,y});if(it!=heights.end())return it->second;
            double h=0;double px=S::X+x*Step,py=S::Y+y*Step;
            // Domain edge lies on the final original cell, too.
            int cx=std::clamp(x/Fine,0,S::NX-1),cy=std::clamp(y/Fine,0,S::NY-1);
            auto q=S::Quad(cx,cy);double u=(px-q[0][0])/S::Step,v=(py-q[0][1])/S::Step;
            h=u>=v?q[0][2]+(q[1][2]-q[0][2])*u+(q[2][2]-q[1][2])*v:q[0][2]+(q[2][2]-q[3][2])*u+(q[3][2]-q[0][2])*v;
            return h;
        }
        std::array<P,4> Quad(int x,int y)const
        {
            auto p=[&](int a,int b){return P{S::X+a*Step,S::Y+b*Step,Node(a,b)};};
            return {p(x,y),p(x+1,y),p(x+1,y+1),p(x,y+1)};
        }
        bool Surface(double x,double y,double& h)const
        {
            double gx=(x-S::X)/Step,gy=(y-S::Y)/Step;
            int ix=int(std::floor(gx)),iy=int(std::floor(gy));
            if(ix<0||iy<0||ix>=S::NX*Fine||iy>=S::NY*Fine)return false;
            auto q=Quad(ix,iy);double u=gx-ix,v=gy-iy;
            h=u>=v?q[0][2]+(q[1][2]-q[0][2])*u+(q[2][2]-q[1][2])*v:q[0][2]+(q[2][2]-q[3][2])*u+(q[3][2]-q[0][2])*v;
            return true;
        }
        template<class F> void CellTriangles(int x,int y,F emit)const
        {
            auto quad=[&](auto q){emit(q[0],q[1],q[2]);emit(q[0],q[2],q[3]);};
            if(!refined.count(y*S::NX+x)){quad(S::Quad(x,y));return;}
            for(int j=y*Fine;j<(y+1)*Fine;++j)for(int i=x*Fine;i<(x+1)*Fine;++i)quad(Quad(i,j));
        }
        G::Poly PatchFaces(int id)const
        {
            G::Poly faces;int sx=(id%Columns)*Group,sy=(id/Columns)*Group;
            for(int y=sy;y<std::min(sy+Group,S::NY);++y)for(int x=sx;x<std::min(sx+Group,S::NX);++x)
            {
                CellTriangles(x,y,[&](P a,P b,P c){faces.push_back({{a,b,c},0,0});});
                // Excavation cannot edit perimeter nodes; skirts stay intact.
                auto q=S::Quad(x,y);auto skirt=[&](P a,P b){P c=b,d=a;c[2]=d[2]=Bottom;faces.push_back({{a,b,c,d},0,0});};
                if(y==0)skirt(q[1],q[0]);if(y==S::NY-1)skirt(q[3],q[2]);
                if(x==0)skirt(q[0],q[3]);if(x==S::NX-1)skirt(q[2],q[1]);
            }return faces;
        }
        void Reset()
        {
            heights.clear();refined.clear();dirty.clear();queries.clear();revision=0;scoops=0;removedM3=0;removedMg=0;
            for(int y=0;y<S::NY;y+=Group)for(int x=0;x<S::NX;x+=Group)
            {int id=Patch(x,y);queries[id].Build(PatchFaces(id));dirty.insert(id);}
        }
        void Trace(P origin,P ray,double& nearest,G::Hit& hit)const
        {for(auto const& entry:queries)if(!entry.second.nodes.empty())entry.second.Trace(0,origin,ray,nearest,hit,-1);}
        bool Edited(double x,double y,double radius)const
        {
            int lx=std::max(0,int(std::floor((x-radius-S::X)/S::Step))),ux=std::min(S::NX-1,int(std::floor((x+radius-S::X)/S::Step)));
            int ly=std::max(0,int(std::floor((y-radius-S::Y)/S::Step))),uy=std::min(S::NY-1,int(std::floor((y+radius-S::Y)/S::Step)));
            for(int j=ly;j<=uy;++j)for(int i=lx;i<=ux;++i)if(refined.count(j*S::NX+i))return true;
            return false;
        }
    };
    inline Receipt Excavate(Body& soil,G::Body const& rock,G::Hit const& contact)
    {
        Receipt receipt;if(!contact.hit||!G::Finite(contact.position)||!G::Finite(contact.normal))return receipt;
        // A sphere tangent to the local ground creates the specified shallow
        // spherical cap on a level surface. Only exposed columns are lowered.
        double radius=(Diameter*Diameter*.25+Depth*Depth)/(2*Depth);
        P center=G::Add(contact.position,G::Mul(G::Unit(contact.normal),radius-Depth));
        int lx=std::max(1,int(std::floor((center[0]-radius-S::X)/Step))),ux=std::min(S::NX*Fine-1,int(std::ceil((center[0]+radius-S::X)/Step)));
        int ly=std::max(1,int(std::floor((center[1]-radius-S::Y)/Step))),uy=std::min(S::NY*Fine-1,int(std::ceil((center[1]+radius-S::Y)/Step)));
        std::map<std::pair<int,int>,double> proposed;
        for(int y=ly;y<=uy;++y)for(int x=lx;x<=ux;++x)
        {
            double dx=S::X+x*Step-center[0],dy=S::Y+y*Step-center[1],r2=radius*radius-dx*dx-dy*dy;
            if(r2<=0)continue;double old=soil.Node(x,y),root=std::sqrt(r2);
            if(old<center[2]-root||old>center[2]+root)continue;
            double h=std::max(Bottom,center[2]-root);
            if(h<old-1e-9)proposed[{x,y}]=h;
        }
        auto target=[&](int x,int y){auto it=proposed.find({x,y});return it==proposed.end()?soil.Node(x,y):it->second;};
        // Conservative column envelopes stop BEFORE any granite, including
        // hidden/intersecting original soil. No rock volume enters this ledger.
        // Revert all four shared nodes of blocked quads (at most a 15 mm rim).
        std::set<std::pair<int,int>> blocked;
        for(int y=std::max(0,ly-1);y<=uy;++y)for(int x=std::max(0,lx-1);x<=ux;++x)
        {
            auto q=soil.Quad(x,y);double lo=std::min({target(x,y),target(x+1,y),target(x+1,y+1),target(x,y+1)});
            double hi=std::max({q[0][2],q[1][2],q[2][2],q[3][2]});
            if(target(x,y)==q[0][2]&&target(x+1,y)==q[1][2]&&target(x+1,y+1)==q[2][2]&&target(x,y+1)==q[3][2])continue;
            double r=Step*.708;
            if(G::BlocksCapsule(rock,{S::X+(x+.5)*Step,S::Y+(y+.5)*Step,lo-r},r,hi-lo+2*r))
            {blocked.insert({x,y});blocked.insert({x+1,y});blocked.insert({x+1,y+1});blocked.insert({x,y+1});receipt.blocked=true;}
        }
        for(auto p:blocked)proposed.erase(p);
        std::set<int> patches;std::set<std::pair<int,int>> cells;
        for(auto const& entry:proposed)
        {
            int x=entry.first.first,y=entry.first.second;double delta=soil.Node(x,y)-entry.second;
            // Six adjacent triangles integrate a shared vertex's linear height
            // delta to exactly one fine-grid square times delta.
            receipt.volume+=delta*Step*Step;soil.heights[entry.first]=entry.second;
            for(int j:{y-1,y})for(int i:{x-1,x})
            {int cx=i/Fine,cy=j/Fine;soil.refined.insert(cy*S::NX+cx);patches.insert(Body::Patch(cx,cy));cells.insert({i,j});}
        }
        if(receipt.volume>0)
        {
            ++soil.revision;++soil.scoops;soil.removedM3+=receipt.volume;
            uint64_t total=uint64_t(std::llround(soil.removedM3*BulkDensity*1e6));receipt.massMg=total-soil.removedMg;soil.removedMg=total;
            for(int id:patches){soil.queries[id].Build(soil.PatchFaces(id));soil.dirty.insert(id);}
        }
        receipt.cells=cells.size();return receipt;
    }
    struct Transaction {Body terrain;Receipt receipt;double milliseconds=0;};
    inline Transaction Prepare(Body const& soil,G::Body const& rock,G::Hit contact)
    {
        auto start=std::chrono::steady_clock::now();Transaction result;result.terrain=soil;result.terrain.dirty.clear();
        result.receipt=Excavate(result.terrain,rock,contact);
        result.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        return result;
    }
}
