#pragma once
#include "GraniteOutcrop.h"
#ifndef GRANITE_OUTCROP_AUTHORED_SHAPE_HEADER
#define GRANITE_OUTCROP_AUTHORED_SHAPE_HEADER "GraniteOutcropAuthoredShape.h"
#endif
#include GRANITE_OUTCROP_AUTHORED_SHAPE_HEADER
#include <unordered_map>

// Version 2: shared, jittered volumetric vertices replace square parcel faces.
// Triangular faces are paired by identity, not proximity. Surface, collision,
// damage, recovered casts and mass all consume the same partition.
namespace EE::GraniteOutcropV2
{
    using namespace GraniteOutcrop;
    using GraniteOutcrop::Hash;
    inline constexpr int AlgorithmVersion=10;
    inline constexpr int SX=60,SY=24,SZ=20;
    // The former wall contained 3.303840347385 m3. This test-space outcrop
    // intentionally adds a taller connected core, shoulders and buried
    // aprons, then records their complete volume in the same authoritative
    // excavation ledger.
    inline constexpr double LegacyInitialM3=3.303840347385;
    inline constexpr double StartingM3=9.00;
    inline constexpr double Width=4.20,Depth=2.40,Rise=1.45,Base=-.38;
    inline constexpr double Spacing=Width/SX,YSpacing=Depth/SY,ZSpacing=Rise/SZ;
    inline constexpr double BinSize=.13;
    // A hidden acceleration partition is not a harvestable chip template.
    // Tall surface cells may follow the outcrop's angular grade transition,
    // while every contact cutter remains independently pick-scale bounded.
    inline constexpr double MaxPartitionDiameter=.52;
    using Tet=std::array<int,4>;
    struct Facet { std::array<int,3> v; int neighbor=-1; };
    struct Piece
    {
        std::vector<Facet> faces;std::vector<Tet> tets;
        P low={},high={};double volume=0,damage=0;uint64_t massMg=0;bool alive=true;
    };
    struct FaceKey
    {
        std::array<int,3> v;
        bool operator==(FaceKey const& other)const{return v==other.v;}
    };
    struct FaceHash
    {
        size_t operator()(FaceKey const& key)const
        {return size_t(Hash(uint32_t(key.v[0])))^(size_t(Hash(uint32_t(key.v[1])))<<1)^(size_t(Hash(uint32_t(key.v[2])))<<2);}
    };
    inline int VertexID(int x,int y,int z){return x+(SX+1)*(y+(SY+1)*z);}
    inline double Gaussian(double x,double y,double cx,double cy,double sx,double sy)
    {
        double dx=(x-cx)/sx,dy=(y-cy)/sy;return std::exp(-(dx*dx+dy*dy));
    }
    inline double AngularCrown(double x,double y,double cx,double cy,double sx,double sy,double angle,double exponent=1.16)
    {
        double dx=(x-cx)/sx,dy=(y-cy)/sy,c=std::cos(angle),s=std::sin(angle);
        double rx=c*dx-s*dy,ry=s*dx+c*dy;
        // A high-order norm of three opposing plane pairs produces a rotated,
        // six-sided crown. It retains broad planar-looking sectors and clear
        // ridge breaks without the derivative discontinuity of a hard max,
        // which can invert a tetrahedron at the fixed authoritative sampling.
        double a=std::abs(rx),b=std::abs(.5*rx+.866025403784*ry),d=std::abs(.5*rx-.866025403784*ry);
        double radius=std::pow(std::pow(a,6)+std::pow(b,6)+std::pow(d,6),1./6);
        return std::pow(std::max(0.,1-radius),exponent);
    }
    inline double Groove(double x,double y,double ax,double ay,double bx,double by,double width)
    {
        double vx=bx-ax,vy=by-ay,px=x-ax,py=y-ay;
        double t=std::clamp((px*vx+py*vy)/(vx*vx+vy*vy),0.,1.);
        double dx=x-(ax+t*vx),dy=y-(ay+t*vy);
        return std::exp(-(dx*dx+dy*dy)/(width*width));
    }
    inline double ProceduralReferenceTopHeight(double x,double y)
    {
        double u=std::clamp(x/Width,0.,1.),v=std::clamp(y/Depth,0.,1.);
        auto smooth=[](double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);};
        // Composed rock masses, not a rounded spline envelope: a steep left
        // wall rises into the dominant crown, a lower right crown breaks the
        // skyline, and an angular front slab projects from the long face.
        double broad=.13*Gaussian(u,v,.48,.53,.53,.58);
        // The cliff is steep enough to read as a face, but spans slightly
        // more than one authoritative Y cell so the closed tetrahedral
        // partition cannot fold across it.
        double leftCliff=.70+.30*smooth((v-.18)/.12);
        double leftSmooth=std::pow(Gaussian(u,v,.29,.54,.235,.39),.82);
        double leftAngular=AngularCrown(u,v,.285,.54,.31,.47,-.18,1.04);
        double left=leftCliff*(.22*leftSmooth+.70*leftAngular);
        double rightSmooth=std::pow(Gaussian(u,v,.73,.56,.235,.35),.78);
        double rightAngular=AngularCrown(u,v,.735,.57,.29,.38,.22,1.07);
        double right=.18*rightSmooth+.43*rightAngular;
        double apron=.06*std::pow(Gaussian(u,v,.58,.25,.38,.18),.78)
            +.25*AngularCrown(u,v,.66,.27,.36,.17,-.09,1.03);
        double saddle=.14*Gaussian(u,v,.54,.52,.12,.17);
        // Centimeter-scale dark centers are supplied by the material. At this
        // fixed grid the authoritative grooves occupy one vertex row and are
        // deep enough to cast a real shadow without broadening the depression.
        double creaseA=.028*Groove(u,v,.36,.19,.44,.73,.022);
        double creaseB=.025*Groove(u,v,.55,.53,.79,.59,.020);
        double planes=.030*(std::abs(std::sin(8.1*u+3.7*v+.4))-.52)
            +.018*(std::abs(std::sin(5.3*u-7.9*v-1.1))-.48);
        // Satellite crowns are narrow, slightly asymmetric rock ridges. Their
        // roof is intentionally below the soil between each crown and the
        // main mass, while their common granite body remains connected below.
        double satelliteLeft=.46*(.78*std::pow(Gaussian(u,v,.105,.28,.052,.105),.68)
            +.22*AngularCrown(u,v,.105,.28,.100,.175,-.52,1.10))
            *(.88+.12*std::sin(19*u+7*v));
        double satelliteRight=.36*(.78*std::pow(Gaussian(u,v,.905,.34,.048,.112),.70)
            +.22*AngularCrown(u,v,.905,.34,.095,.175,.43,1.12))
            *(.90+.10*std::sin(17*u-9*v));
        double satelliteRear=.41*(.78*std::pow(Gaussian(u,v,.73,.86,.084,.078),.72)
            +.22*AngularCrown(u,v,.73,.86,.145,.135,-.66,1.14))
            *(.90+.10*std::sin(13*u+21*v));
        double weather=.019*std::sin(11.2*u+1.1)*std::sin(8.7*v-.4)+.011*std::sin(20.3*u+5.1*v);
        double buriedEdge=smooth(std::min(u,1-u)/.24)*smooth(std::min(v,1-v)/.30);
        double satelliteEdge=smooth(std::min(u,1-u)/.045)*smooth(std::min(v,1-v)/.055);
        double main=buriedEdge*std::max(.025,.035+broad+left+right+apron-saddle-creaseA-creaseB+planes+weather);
        // Broad soil-covered saddles sever the visible necks without severing
        // the authoritative granite root below grade.
        double separation=.30*Gaussian(u,v,.20,.32,.100,.175)
            +.22*Gaussian(u,v,.81,.35,.095,.125)+.26*Gaussian(u,v,.73,.74,.165,.105);
        double exposed=main+satelliteEdge*(satelliteLeft+satelliteRight+satelliteRear)-separation;
        return .015+std::max(0.,exposed);
    }
    inline double TopHeight(double x,double y)
    {
        return GraniteOutcropAuthoredShape::Sample(x,y,Width,Depth);
    }
    inline P Exterior(P p)
    {
        double t=std::clamp(p[2]/Rise,0.,1.);
        p[2]=Base+t*(TopHeight(p[0],p[1])-Base);
        return p;
    }
    // Each XY quad is split into two triangular prisms; each prism becomes
    // three tetrahedra along its vertical edges. Unlike a five-tet cube split,
    // these tetrahedra remain orientation-preserving even when an authored
    // surface has a near-vertical height change across one grid interval.
    inline constexpr int CellTets[6][4]={
        {0,1,3,4},{1,3,4,5},{3,4,5,7},
        {0,3,2,4},{3,2,4,7},{2,4,7,6}};
    inline double TetrahedronVolume(P a,P b,P c,P d)
    {return std::abs(Dot(Add(b,Mul(a,-1)),Cross(Add(c,Mul(a,-1)),Add(d,Mul(a,-1)))))/6;}
    inline double GridVolume(std::vector<P> const& vertices)
    {
        double volume=0;
        for(int z=0;z<SZ;++z)for(int y=0;y<SY;++y)for(int x=0;x<SX;++x)for(int i=0;i<6;++i)
        {
            int ids[4];for(int j=0;j<4;++j){int v=CellTets[i][j];ids[j]=VertexID(x+(v&1),y+((v>>1)&1),z+((v>>2)&1));}
            volume+=TetrahedronVolume(vertices[ids[0]],vertices[ids[1]],vertices[ids[2]],vertices[ids[3]]);
        }
        return volume;
    }
    struct Body
    {
        uint32_t seed=8675309,revision=0;std::vector<P> vertices,normals;std::vector<Piece> pieces;
        P low={},high={};std::array<int,3> binCount={};std::vector<std::vector<int>> bins;
        double initialM3=0,removedM3=0;uint64_t initialMg=0,removedMg=0;
        explicit Body(bool initialize=true){if(initialize)Reset(seed);}
        int BinID(int x,int y,int z)const{return x+binCount[0]*(y+binCount[1]*z);}
        void Reset(uint32_t newSeed)
        {
            seed=newSeed;revision=0;initialM3=removedM3=0;initialMg=removedMg=0;pieces.clear();bins.clear();
            vertices.resize((SX+1)*(SY+1)*(SZ+1));normals.assign(vertices.size(),P{});
            low={1e9,1e9,1e9};high={-1e9,-1e9,-1e9};
            for(int z=0;z<=SZ;++z)for(int y=0;y<=SY;++y)for(int x=0;x<=SX;++x)
            {
                int id=VertexID(x,y,z);uint32_t h=Hash(seed^uint32_t(id)*0x9e3779b9u);
                P p={x*Spacing,y*YSpacing,z*ZSpacing};
                for(int k=0;k<3;++k)
                {
                    // Boundary vertices retain a smooth weathered envelope;
                    // internal vertices break all regular excavation squares.
                    int c=k==0?x:k==1?y:z,extent=k==0?SX:k==1?SY:SZ;
                    if(c>0&&c<extent&&!(z==SZ&&k<2))
                    {
                        // A small three-axis fracture jitter still breaks the
                        // regular grid, while keeping the hidden partition
                        // orientation-preserving beneath deliberate cliffs.
                        double amplitude=.001;
                        p[k]+=(double((h>>(k*8))&255)/255-.5)*amplitude;
                    }
                }
                vertices[id]=Exterior(p);
            }
            // The artist-authored top surface is inviolable. Reconcile its
            // predetermined mass by moving only the buried layers/root; never
            // globally rescale the crest after the artist has shaped it.
            double rawVolume=GridVolume(vertices);auto meterDeeper=vertices;
            for(int z=0;z<=SZ;++z)for(int y=0;y<=SY;++y)for(int x=0;x<=SX;++x)
                meterDeeper[VertexID(x,y,z)][2]-=1.-double(z)/SZ;
            double volumePerRootMeter=GridVolume(meterDeeper)-rawVolume;
            double rootDepth=(StartingM3-rawVolume)/volumePerRootMeter;
            for(int z=0;z<=SZ;++z)for(int y=0;y<=SY;++y)for(int x=0;x<=SX;++x)
                vertices[VertexID(x,y,z)][2]-=rootDepth*(1.-double(z)/SZ);
            for(auto const& p:vertices)for(int k=0;k<3;++k){low[k]=std::min(low[k],p[k]);high[k]=std::max(high[k],p[k]);}
            // The two prism chains share exactly the same diagonal on every
            // neighboring face, so steep artist-authored planes stay closed.
            std::unordered_map<FaceKey,std::pair<int,int>,FaceHash> pending;
            for(int z=0;z<SZ;++z)for(int y=0;y<SY;++y)for(int x=0;x<SX;++x)
            {
                uint32_t h=Hash(seed^uint32_t(x+SX*(y+SY*z)));Tet tets[6];
                for(int i=0;i<6;++i)
                {
                    for(int j=0;j<4;++j){int v=CellTets[i][j];tets[i][j]=VertexID(x+(v&1),y+((v>>1)&1),z+((v>>2)&1));}
                }
                for(int prism=0;prism<2;++prism)
                {
                    int consumed=prism*3,end=consumed+3;
                    while(consumed<end)
                    {
                        int count=std::min(end-consumed,1+int((h>>(4+prism*3+consumed))%3));
                        // Joined prism neighbors stay small where the surface
                        // bends; a single steep tetrahedron keeps its authored
                        // geometry rather than rescaling the visible face.
                        while(count>1)
                        {
                            std::vector<int> group;
                            for(int j=0;j<count;++j)for(int v:tets[consumed+j])
                                if(std::find(group.begin(),group.end(),v)==group.end())group.push_back(v);
                            double diameter2=0;
                            for(int a:group)for(int b:group){P d=Add(vertices[a],Mul(vertices[b],-1));diameter2=std::max(diameter2,Dot(d,d));}
                            if(diameter2<=.16*.16)break;--count;
                        }
                        int pieceID=int(pieces.size());pieces.emplace_back();auto& piece=pieces.back();
                        piece.low={1e9,1e9,1e9};piece.high={-1e9,-1e9,-1e9};
                        for(int j=0;j<count;++j)
                        {
                            Tet tet=tets[consumed+j];piece.tets.push_back(tet);
                            P a=vertices[tet[0]],b=vertices[tet[1]],c=vertices[tet[2]],d=vertices[tet[3]];
                            piece.volume+=TetrahedronVolume(a,b,c,d);
                            for(int v:tet)for(int k=0;k<3;++k){piece.low[k]=std::min(piece.low[k],vertices[v][k]);piece.high[k]=std::max(piece.high[k],vertices[v][k]);}
                            for(int opposite=0;opposite<4;++opposite)
                            {
                                Facet face;int f=0;for(int k=0;k<4;++k)if(k!=opposite)face.v[f++]=tet[k];
                                P p=vertices[face.v[0]],q=vertices[face.v[1]],r=vertices[face.v[2]];
                                if(Dot(Cross(Add(q,Mul(p,-1)),Add(r,Mul(p,-1))),Add(vertices[tet[opposite]],Mul(p,-1)))>0)std::swap(face.v[1],face.v[2]);
                                FaceKey key{face.v};std::sort(key.v.begin(),key.v.end());int faceID=int(piece.faces.size());
                                auto found=pending.find(key);
                                if(found!=pending.end())
                                {
                                    face.neighbor=found->second.first;
                                    pieces[face.neighbor].faces[found->second.second].neighbor=pieceID;
                                    pending.erase(found);
                                }
                                else pending.emplace(key,std::make_pair(pieceID,faceID));
                                piece.faces.push_back(face);
                            }
                        }
                        piece.massMg=uint64_t(std::llround(piece.volume*Density*1e6));
                        initialMg+=piece.massMg;initialM3+=piece.volume;consumed+=count;
                    }
                }
            }
            for(auto const& piece:pieces)for(auto const& f:piece.faces)if(f.neighbor<0)
            {
                P n=Cross(Add(vertices[f.v[1]],Mul(vertices[f.v[0]],-1)),Add(vertices[f.v[2]],Mul(vertices[f.v[0]],-1)));
                for(int v:f.v)normals[v]=Add(normals[v],n);
            }
            for(auto& n:normals)n=Unit(n);
            for(int k=0;k<3;++k){low[k]-=.001;high[k]+=.001;binCount[k]=int(std::ceil((high[k]-low[k])/BinSize));}
            bins.resize(binCount[0]*binCount[1]*binCount[2]);
            for(int id=0;id<int(pieces.size());++id)
            {
                int l[3],u[3];for(int k=0;k<3;++k){l[k]=int((pieces[id].low[k]-low[k])/BinSize);u[k]=std::min(binCount[k]-1,int((pieces[id].high[k]-low[k])/BinSize));}
                for(int z=l[2];z<=u[2];++z)for(int y=l[1];y<=u[1];++y)for(int x=l[0];x<=u[0];++x)bins[BinID(x,y,z)].push_back(id);
            }
        }
    };
    inline Poly Geometry(Body const& body,int id)
    {
        Poly result;for(auto const& f:body.pieces[id].faces)if(f.neighbor!=id)
            result.push_back({{body.vertices[f.v[0]],body.vertices[f.v[1]],body.vertices[f.v[2]]},-1,0});
        return result;
    }
    struct Triangle {P a,b,c,na,nb,nc;};
    inline std::vector<Triangle> Boundary(Body const& body)
    {
        std::vector<Triangle> mesh;
        for(auto const& piece:body.pieces)if(piece.alive)for(auto const& f:piece.faces)
        {
            if(f.neighbor>=0&&body.pieces[f.neighbor].alive)continue;
            P a=body.vertices[f.v[0]],b=body.vertices[f.v[1]],c=body.vertices[f.v[2]],n=Unit(Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1))));
            mesh.push_back({a,b,c,f.neighbor<0?body.normals[f.v[0]]:n,f.neighbor<0?body.normals[f.v[1]]:n,f.neighbor<0?body.normals[f.v[2]]:n});
        }
        return mesh;
    }
    inline double Intersect(P a,P b,P c,P o,P d)
    {
        P e=Add(b,Mul(a,-1)),g=Add(c,Mul(a,-1)),h=Cross(d,g);double det=Dot(e,h);
        if(std::abs(det)<1e-13)return 1e30;
        P s=Add(o,Mul(a,-1));double u=Dot(s,h)/det;if(u< -1e-8||u>1+1e-8)return 1e30;
        P q=Cross(s,e);double v=Dot(d,q)/det;if(v< -1e-8||u+v>1+1e-8)return 1e30;
        double t=Dot(g,q)/det;return t>=-1e-8?std::max(0.,t):1e30;
    }
    struct Hit{bool hit=false;int id=-1;double distance=0;P position={},normal={};};
    inline Hit Raycast(Body const& body,P o,P d,double reach=2)
    {
        Hit result;if(!Finite(o)||!Finite(d)||!std::isfinite(reach)||reach<=0||Dot(d,d)<1e-16)return result;
        d=Unit(d);double enter=0,leave=reach;
        for(int k=0;k<3;++k)
        {
            if(std::abs(d[k])<1e-13){if(o[k]<body.low[k]||o[k]>body.high[k])return result;}
            else{double a=(body.low[k]-o[k])/d[k],b=(body.high[k]-o[k])/d[k];if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);}
        }
        if(leave<enter)return result;
        int bin[3];for(int k=0;k<3;++k)bin[k]=std::clamp(int((o[k]+d[k]*(enter+1e-9)-body.low[k])/BinSize),0,body.binCount[k]-1);
        double nearest=reach+1e-8;
        for(int iteration=0;iteration<body.binCount[0]+body.binCount[1]+body.binCount[2]+6;++iteration)
        {
            for(int id:body.bins[body.BinID(bin[0],bin[1],bin[2])])if(body.pieces[id].alive)
            for(auto const& f:body.pieces[id].faces)
            {
                if(f.neighbor>=0&&body.pieces[f.neighbor].alive)continue;
                P a=body.vertices[f.v[0]],b=body.vertices[f.v[1]],c=body.vertices[f.v[2]];
                double t=Intersect(a,b,c,o,d);if(t<nearest){nearest=t;result={true,id,t,Add(o,Mul(d,t)),Unit(Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1))))};}
            }
            double next=1e30;int axis=-1;
            for(int k=0;k<3;++k)if(std::abs(d[k])>1e-13){double t=(body.low[k]+(bin[k]+(d[k]>0?1:0))*BinSize-o[k])/d[k];if(t<next){next=t;axis=k;}}
            if((result.hit&&nearest<=next+1e-8)||axis<0||next>leave)break;
            bin[axis]+=d[axis]>0?1:-1;if(bin[axis]<0||bin[axis]>=body.binCount[axis])break;
        }
        // An exit surface means the camera began inside solid matter.
        if(result.hit&&Dot(result.normal,d)>1e-8)return {};
        return result;
    }
    struct Chip{int id=-1;Poly geometry;P center={};double volume=0;uint64_t massMg=0;};
    struct Receipt{Hit contact;bool removed=false;double transferredJ=0;Chip chip;char const* status="READY";};
    inline Receipt Strike(Body& body,P origin,P direction,double energyJ=400,uint32_t grade=2)
    {
        Receipt r;if(!std::isfinite(energyJ)||energyJ<0)return r;r.contact=Raycast(body,origin,direction);
        if(!r.contact.hit){r.status="MISS";return r;}r.transferredJ=energyJ*std::max(0.,-Dot(Unit(direction),r.contact.normal));
        if(grade<2){r.status="CONTACT: implement too soft; no removal";return r;}
        auto& piece=body.pieces[r.contact.id];piece.damage+=r.transferredJ;
        if(piece.damage+1e-9<piece.volume*1e6){r.status="LOCAL DAMAGE: strike here again";return r;}
        r.removed=true;r.status="CHIP RELEASED: irregular cast / accounted granite";
        r.chip.id=r.contact.id;r.chip.geometry=Geometry(body,r.contact.id);r.chip.volume=piece.volume;r.chip.massMg=piece.massMg;
        int count=0;for(auto const& face:r.chip.geometry)for(P p:face.p){r.chip.center=Add(r.chip.center,p);++count;}
        r.chip.center=Mul(r.chip.center,1.0/count);piece.alive=false;body.removedMg+=piece.massMg;body.removedM3+=piece.volume;++body.revision;return r;
    }
    inline bool BlocksCapsule(Body const& body,P feet,double radius=.23,double height=1.8)
    {
        P a=Add(feet,P{0,0,radius}),d={0,0,height-2*radius};int l[3],u[3];
        for(int k=0;k<3;++k){l[k]=std::max(0,int(std::floor((feet[k]-radius-body.low[k])/BinSize)));u[k]=std::min(body.binCount[k]-1,int(std::floor((feet[k]+(k==2?height:radius)-body.low[k])/BinSize)));}
        for(int z=l[2];z<=u[2];++z)for(int y=l[1];y<=u[1];++y)for(int x=l[0];x<=u[0];++x)
        for(int id:body.bins[body.BinID(x,y,z)])if(body.pieces[id].alive
            &&feet[2]<body.pieces[id].high[2]&&feet[2]+height>body.pieces[id].low[2]
            &&feet[0]+radius>body.pieces[id].low[0]&&feet[0]-radius<body.pieces[id].high[0]
            &&feet[1]+radius>body.pieces[id].low[1]&&feet[1]-radius<body.pieces[id].high[1])for(auto const& tet:body.pieces[id].tets)
        {
            double enter=0,leave=1;bool intersects=true;
            for(int opposite=0;opposite<4;++opposite)
            {
                P p[3];int count=0;for(int k=0;k<4;++k)if(k!=opposite)p[count++]=body.vertices[tet[k]];
                P n=Unit(Cross(Add(p[1],Mul(p[0],-1)),Add(p[2],Mul(p[0],-1))));if(Dot(n,Add(body.vertices[tet[opposite]],Mul(p[0],-1)))>0)n=Mul(n,-1);
                double numerator=Dot(n,Add(p[0],Mul(a,-1)))+radius,denom=Dot(n,d);
                if(std::abs(denom)<1e-12){if(numerator<0){intersects=false;break;}}
                else{double t=numerator/denom;if(denom<0)enter=std::max(enter,t);else leave=std::min(leave,t);}
                if(leave<enter){intersects=false;break;}
            }
            if(intersects)return true;
        }
        return false;
    }
}
