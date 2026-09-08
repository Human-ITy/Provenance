#pragma once
// Bounded walk-up fixture. Separate from the frozen excavation/bridge laws.
// Each grid parcel is bisected by a seeded plane. Recovered geometry is the
// actual removed convex parcel, not a separately generated decorative rock.
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace EE::GraniteOutcrop
{
    using P=std::array<double,3>;
    inline constexpr int NX=48, NY=20, NZ=26, Count=NX*NY*NZ;
    inline constexpr double Step=.06, Shear=.4, Density=2700, Height=NZ*Step;
    inline constexpr uint64_t ParcelMg=583200, InitialMg=uint64_t(Count)*ParcelMg;
    inline P Add(P a,P b) { for(int i=0;i<3;++i)a[i]+=b[i]; return a; }
    inline P Mul(P a,double s) { for(double& v:a)v*=s; return a; }
    inline double Dot(P a,P b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
    inline P Cross(P a,P b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
    inline P Unit(P a) { double n=std::sqrt(Dot(a,a)); return n>1e-12?Mul(a,1/n):P{}; }
    inline P Shape(P a) { a[1]+=Shear*a[2]; return a; }
    inline P Grid(P a) { a[1]-=Shear*a[2]; return a; }
    inline bool Finite(P p) { return std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]); }
    inline uint32_t Hash(uint32_t x) { x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; return x^(x>>16); }
    inline int ID(int x,int y,int z) { return x+NX*(y+NY*z); }
    inline P Low(int id) { return {(id%NX)*Step,((id/NX)%NY)*Step,(id/(NX*NY))*Step}; }
    struct Plane { P n; double d; };
    inline Plane Cut(uint32_t seed,int id,int part=0)
    {
        uint32_t h=Hash(seed^uint32_t(id)*0x9e3779b9u);
        P n={double(int(h&255)-127),double(int((h>>8)&255)-127),double(int((h>>16)&255)-127)};
        n=Unit(n); if(Dot(n,n)<.5)n={1,0,0};
        P c=Add(Low(id),P{Step*.5,Step*.5,Step*.5});
        double d=Dot(n,c)+Step*(double((h>>24)&255)/255-.5)*.24;
        return part?Plane{Mul(n,-1),-d}:Plane{n,d};
    }
    struct Face { std::vector<P> p; int axis=-1,sign=0; };
    using Poly=std::vector<Face>;
    inline std::vector<P> ClipPolygon(std::vector<P> const& in,Plane cut,std::vector<P>* crossings=nullptr)
    {
        std::vector<P> out;
        if(in.empty())return out;
        for(size_t i=0;i<in.size();++i)
        {
            P a=in[i],b=in[(i+1)%in.size()]; double da=Dot(cut.n,a)-cut.d,db=Dot(cut.n,b)-cut.d;
            if(da<=1e-11)out.push_back(a);
            if((da< -1e-11&&db>1e-11)||(da>1e-11&&db< -1e-11))
            { P p=Add(a,Mul(Add(b,Mul(a,-1)),da/(da-db))); out.push_back(p); if(crossings)crossings->push_back(p); }
        }
        return out;
    }
    inline Poly Geometry(uint32_t seed,int id,int part)
    {
        P low=Low(id); Poly result; std::vector<P> cap;
        Plane cut=Cut(seed,id,part);
        for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
        {
            int u=(axis+1)%3,v=(axis+2)%3;
            P a=low; if(sign>0)a[axis]+=Step;
            P b=a,c=a,d=a; b[u]+=Step; c[u]+=Step; c[v]+=Step; d[v]+=Step;
            std::vector<P> polygon=sign>0?std::vector<P>{a,b,c,d}:std::vector<P>{a,d,c,b};
            polygon=ClipPolygon(polygon,cut,&cap);
            if(polygon.size()>=3)result.push_back({polygon,axis,sign});
        }
        std::vector<P> unique;
        for(P p:cap)
        {
            bool found=false; for(P q:unique)if(Dot(Add(p,Mul(q,-1)),Add(p,Mul(q,-1)))<1e-20)found=true;
            if(!found)unique.push_back(p);
        }
        if(unique.size()>=3)
        {
            P center={}; for(P p:unique)center=Add(center,p); center=Mul(center,1.0/unique.size());
            P u=Unit(Cross(cut.n,std::abs(cut.n[2])<.8?P{0,0,1}:P{0,1,0})),v=Cross(cut.n,u);
            std::sort(unique.begin(),unique.end(),[&](P a,P b){a=Add(a,Mul(center,-1));b=Add(b,Mul(center,-1));return std::atan2(Dot(a,v),Dot(a,u))<std::atan2(Dot(b,v),Dot(b,u));});
            result.push_back({unique,-1,0});
        }
        return result;
    }
    inline double Volume(Poly const& poly)
    {
        if(poly.empty())return 0;
        P origin=poly[0].p[0]; double sum=0;
        for(auto const& f:poly)for(size_t i=1;i+1<f.p.size();++i)
            sum+=Dot(Add(f.p[0],Mul(origin,-1)),Cross(Add(f.p[i],Mul(origin,-1)),Add(f.p[i+1],Mul(origin,-1))))/6;
        return std::abs(sum);
    }
    inline uint64_t Mass(uint32_t seed,int id,int part)
    {
        uint64_t a=uint64_t(std::llround(Volume(Geometry(seed,id,0))*Density*1e6));
        return part?ParcelMg-a:a;
    }
    struct Parcel { uint8_t mask=3; double damage[2]={}; };
    struct Body { uint32_t seed=8675309; std::vector<Parcel> cells=std::vector<Parcel>(Count); uint64_t removedMg=0; double removedM3=0; uint32_t revision=0; };
    struct Hit { bool hit=false; int id=-1,part=-1; double distance=0; P position={},normal={}; };
    inline bool SegmentPart(uint32_t seed,int id,int part,P o,P d,double& enter,double& leave,P& normal,double inflate=0)
    {
        P low=Low(id); std::array<Plane,7> planes;
        for(int axis=0;axis<3;++axis)
        { P n={}; n[axis]=1; planes[axis*2]={n,low[axis]+Step}; planes[axis*2+1]={Mul(n,-1),-low[axis]}; }
        planes[6]=Cut(seed,id,part);
        for(Plane plane:planes)
        {
            // Plane normal in physical, sheared space; used for collision margin.
            P worldN={plane.n[0],plane.n[1],plane.n[2]-Shear*plane.n[1]};
            double bound=plane.d+inflate*std::sqrt(Dot(worldN,worldN));
            double numerator=bound-Dot(plane.n,o),denom=Dot(plane.n,d);
            if(std::abs(denom)<1e-14) { if(numerator< -1e-10)return false; continue; }
            double t=numerator/denom;
            if(denom<0) { if(t>enter){enter=t; normal=Unit(worldN);} } else leave=std::min(leave,t);
            if(leave<enter-1e-10)return false;
        }
        return leave>=enter;
    }
    inline Hit Raycast(Body const& body,P origin,P direction,double reach=2)
    {
        Hit result;
        if(body.cells.size()!=Count||!Finite(origin)||!Finite(direction)||!std::isfinite(reach)||reach<=0||reach>30||Dot(direction,direction)<1e-16)return result;
        direction=Unit(direction); P o=Grid(origin),d=Grid(direction);
        double enter=0,leave=reach; int counts[3]={NX,NY,NZ};
        for(int axis=0;axis<3;++axis)
        {
            if(std::abs(d[axis])<1e-14) { if(o[axis]<0||o[axis]>=counts[axis]*Step)return result; }
            else { double a=-o[axis]/d[axis],b=(counts[axis]*Step-o[axis])/d[axis]; if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b); }
        }
        if(leave<enter)return result;
        P p=Add(o,Mul(d,enter+1e-9)); int c[3];
        for(int axis=0;axis<3;++axis)c[axis]=std::clamp(int(std::floor(p[axis]/Step)),0,counts[axis]-1);
        for(int iter=0;iter<NX+NY+NZ+6;++iter)
        {
            int id=ID(c[0],c[1],c[2]);
            for(int part=0;part<2;++part)if(body.cells[id].mask&(1<<part))
            {
                double a=-1e30,b=leave; P n={};
                bool intersects=SegmentPart(body.seed,id,part,o,d,a,b,n);
                if(intersects&&a>=-1e-8&&a<=leave+1e-8&&(!result.hit||a<result.distance))
                    result={true,id,part,std::max(0.0,a),Add(origin,Mul(direction,std::max(0.0,a))),n};
                // Inside solid cannot tunnel through to a hidden next parcel.
                else if(intersects&&a< -1e-8&&b>1e-8&&enter==0)return {};
            }
            double next=1e30; int axis=-1;
            for(int k=0;k<3;++k)if(std::abs(d[k])>1e-14)
            { double t=((c[k]+(d[k]>0?1:0))*Step-o[k])/d[k]; if(t<next){next=t;axis=k;} }
            if(result.hit&&result.distance<=next+1e-8)return result;
            if(axis<0||next>leave)return result;
            c[axis]+=d[axis]>0?1:-1;
            if(c[axis]<0||c[axis]>=counts[axis])return result;
        }
        return result;
    }
    struct Chip { int id=-1,part=-1; Poly geometry; double volume=0; uint64_t massMg=0; P contact={}; };
    struct Receipt { Hit contact; bool removed=false; double transferredJ=0; Chip chip; char const* status="MISS"; };
    inline Receipt Strike(Body& body,P origin,P direction,double energyJ=400,uint32_t grade=2)
    {
        Receipt r;
        if(!std::isfinite(energyJ)||energyJ<0||energyJ>10000)return r;
        r.contact=Raycast(body,origin,direction,2);
        if(!r.contact.hit)return r;
        r.transferredJ=energyJ*std::max(0.0,-Dot(Unit(direction),r.contact.normal));
        if(grade<2){r.status="CONTACT: implement too soft; no removal";return r;}
        if(r.transferredJ<=0){r.status="CONTACT: glancing strike; no transfer";return r;}
        auto& parcel=body.cells[r.contact.id];
        Poly geometry=Geometry(body.seed,r.contact.id,r.contact.part);
        double volume=Volume(geometry),threshold=volume*1000000; // Test calibration, not measured pickaxe work.
        parcel.damage[r.contact.part]+=r.transferredJ;
        if(parcel.damage[r.contact.part]+1e-9<threshold){r.status="LOCAL DAMAGE: strike here again";return r;}
        r.removed=true; r.status="CHIP RELEASED: matching cavity / accounted granite";
        r.chip={r.contact.id,r.contact.part,geometry,volume,Mass(body.seed,r.contact.id,r.contact.part),r.contact.position};
        parcel.mask&=uint8_t(~(1<<r.contact.part)); parcel.damage[r.contact.part]=0;
        body.removedMg+=r.chip.massMg;body.removedM3+=volume;++body.revision;
        return r;
    }
    struct Triangle { P a,b,c; };
    inline std::vector<Triangle> Boundary(Body const& body)
    {
        std::vector<Triangle> mesh; int strides[3]={1,NX,NX*NY},counts[3]={NX,NY,NZ};
        for(int id=0;id<Count;++id)
        {
            int coordinates[3]={id%NX,(id/NX)%NY,id/(NX*NY)};
            auto mask=body.cells[id].mask;if(!mask)continue;
            bool exposed=mask!=3;
            for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
                if(coordinates[axis]+sign<0||coordinates[axis]+sign>=counts[axis]||body.cells[id+sign*strides[axis]].mask!=3)exposed=true;
            if(!exposed)continue;
            // Full parcels need only six cheap outer faces, not two complete
            // clipped polyhedra. This is the common path when rebuilding a wall.
            if(mask==3)
            {
                for(int axis=0;axis<3;++axis)for(int sign:{-1,1})
                {
                    int k=coordinates[axis]+sign,neighbor=-1;
                    if(k>=0&&k<counts[axis])
                    { neighbor=id+sign*strides[axis];if(body.cells[neighbor].mask==3)continue; }
                    int u=(axis+1)%3,v=(axis+2)%3;P a=Low(id);if(sign>0)a[axis]+=Step;
                    P b=a,c=a,d=a;b[u]+=Step;c[u]+=Step;c[v]+=Step;d[v]+=Step;
                    std::vector<P> polygon=sign>0?std::vector<P>{a,b,c,d}:std::vector<P>{a,d,c,b};
                    if(neighbor>=0&&body.cells[neighbor].mask)
                        polygon=ClipPolygon(polygon,Cut(body.seed,neighbor,body.cells[neighbor].mask==1?1:0));
                    for(size_t i=1;i+1<polygon.size();++i)mesh.push_back({Shape(polygon[0]),Shape(polygon[i]),Shape(polygon[i+1])});
                }
                continue;
            }
            for(int part=0;part<2;++part)if(mask&(1<<part))
            for(auto const& face:Geometry(body.seed,id,part))
            {
                auto polygon=face.p;
                if(face.axis<0){if(mask==3)continue;}
                else
                {
                    int k=coordinates[face.axis]+face.sign;
                    if(k>=0&&k<counts[face.axis])
                    {
                        int neighbor=id+face.sign*strides[face.axis];auto other=body.cells[neighbor].mask;
                        if(other==3)continue;
                        if(other)polygon=ClipPolygon(polygon,Cut(body.seed,neighbor,other==1?1:0));
                    }
                }
                for(size_t i=1;i+1<polygon.size();++i)mesh.push_back({Shape(polygon[0]),Shape(polygon[i]),Shape(polygon[i+1])});
            }
        }
        return mesh;
    }
    // Conservative vertical capsule against the SAME remaining convex matter.
    inline bool BlocksCapsule(Body const& body,P feet,double radius=.23,double height=1.65)
    {
        P a=Grid(Add(feet,P{0,0,radius})),b=Grid(Add(feet,P{0,0,height-radius}));
        int low[3],high[3],counts[3]={NX,NY,NZ};
        for(int k=0;k<3;++k){double margin=radius*(k==1?1+Shear:1);low[k]=std::max(0,int(std::floor((std::min(a[k],b[k])-margin)/Step)));high[k]=std::min(counts[k]-1,int(std::floor((std::max(a[k],b[k])+margin)/Step)));}
        for(int z=low[2];z<=high[2];++z)for(int y=low[1];y<=high[1];++y)for(int x=low[0];x<=high[0];++x)
        { int id=ID(x,y,z); for(int part=0;part<2;++part)if(body.cells[id].mask&(1<<part)){double e=0,l=1;P n; if(SegmentPart(body.seed,id,part,a,Add(b,Mul(a,-1)),e,l,n,radius))return true;} }
        return false;
    }
}
