#pragma once
#include "GraniteOutcropV2.h"
#include <map>
#include <memory>
#include <chrono>

// Contact-driven constructive fracture. The old tetrahedra are ONLY an
// acceleration/volume decomposition; none is a harvestable piece template.
// Every new cut is shared by the retained host and recovered material.
namespace EE::GraniteContactCast
{
    using namespace GraniteOutcrop;
    using GraniteOutcrop::Hash;
    inline constexpr double Width=GraniteOutcropV2::Width,Depth=GraniteOutcropV2::Depth,Rise=GraniteOutcropV2::Rise;
    using Triangle=GraniteOutcropV2::Triangle;
    using Hit=GraniteOutcropV2::Hit;
    using GraniteOutcropV2::Intersect;
    inline constexpr int Hidden=-2,Fresh=-1,Weathered=0;
    inline double Random(uint32_t& state){state=Hash(state+0x9e3779b9u);return double(state)/4294967295.;}
    inline P Normal(Face const& f){P sum={};for(size_t i=1;i+1<f.p.size();++i)sum=Add(sum,Cross(Add(f.p[i],Mul(f.p[0],-1)),Add(f.p[i+1],Mul(f.p[0],-1))));return Unit(sum);}
    inline Poly Box(P lo,P hi,int tag=Hidden)
    {
        Poly p;for(int k=0;k<3;++k)for(int s:{-1,1})
        {
            int u=(k+1)%3,v=(k+2)%3;P a=lo;if(s>0)a[k]=hi[k];P b=a,c=a,d=a;b[u]=c[u]=hi[u];c[v]=d[v]=hi[v];
            p.push_back({s>0?std::vector<P>{a,b,c,d}:std::vector<P>{a,d,c,b},tag,0});
        }return p;
    }
    inline Poly Clip(Poly const& p,Plane plane,int capTag=Hidden,int capSign=0)
    {
        double lo=1e30,hi=-1e30;for(auto const& f:p)for(P v:f.p){double d=Dot(plane.n,v)-plane.d;lo=std::min(lo,d);hi=std::max(hi,d);}
        if(hi<=1e-10)return p;if(lo>=-1e-10)return {};
        Poly out;std::vector<P> crossings;
        for(auto const& f:p){auto q=ClipPolygon(f.p,plane,&crossings);for(P v:q)if(std::abs(Dot(plane.n,v)-plane.d)<1e-10)crossings.push_back(v);if(q.size()>=3)out.push_back({std::move(q),f.axis,f.sign});}
        std::vector<P> unique;for(P v:crossings){bool exists=false;for(P q:unique)if(Dot(Add(v,Mul(q,-1)),Add(v,Mul(q,-1)))<1e-18){exists=true;break;}if(!exists)unique.push_back(v);}
        if(unique.size()>=3)
        {
            P center={};for(P v:unique)center=Add(center,v);center=Mul(center,1./unique.size());
            P u=Unit(Cross(plane.n,std::abs(plane.n[2])<.8?P{0,0,1}:P{0,1,0})),v=Cross(plane.n,u);
            std::sort(unique.begin(),unique.end(),[&](P a,P b){a=Add(a,Mul(center,-1));b=Add(b,Mul(center,-1));return std::atan2(Dot(a,v),Dot(a,u))<std::atan2(Dot(b,v),Dot(b,u));});
            out.push_back({std::move(unique),capTag,capSign});
        }return out;
    }
    inline Poly Intersection(Poly p,std::vector<Plane> const& planes,int capTag=Fresh)
    {for(auto plane:planes){p=Clip(p,plane,capTag);if(p.empty())break;}return p;}
    inline bool Overlaps(Poly const& p,std::vector<Plane> const& planes)
    {
        for(auto plane:planes){bool inside=false;for(auto const& f:p){for(P v:f.p)if(Dot(plane.n,v)<=plane.d+1e-10){inside=true;break;}if(inside)break;}if(!inside)return false;}return true;
    }
    inline Poly ClipSurface(Poly const& surface,std::vector<Plane> const& planes,bool inside)
    {
        Poly out;for(auto const& face:surface)
        {
            auto p=face.p;
            for(auto plane:planes)
            {
                if(!inside){auto q=ClipPolygon(p,{Mul(plane.n,-1),-plane.d});if(q.size()>=3)out.push_back({std::move(q),face.axis,face.sign});}
                p=ClipPolygon(p,plane);if(p.size()<3)break;
            }
            if(inside&&p.size()>=3)out.push_back({std::move(p),face.axis,face.sign});
        }return out;
    }
    struct Cutter {std::vector<Plane> planes;P low,high;double radius=0;};
    inline Cutter MakeCutter(uint32_t seed,P contact,P outward,double energy)
    {
        // Test calibration: anisotropic joint-like macro planes, not tetrahedra.
        // The Minkowski offset below creates actual narrow roundover strips.
        uint32_t h=seed;double scale=std::clamp(std::cbrt(std::max(1.,energy)/400),.3,1.);
        P extent={scale*(.026+.014*Random(h)),scale*(.020+.011*Random(h)),scale*(.015+.010*Random(h))};
        double radius=scale*(.001+.0017*Random(h));
        Poly core=Box(Mul(extent,-2),Mul(extent,2));
        int planeCount=12+int(Random(h)*5);double phase=Random(h)*6.283185307179586;
        for(int i=0;i<planeCount;++i)
        {
            double z=1-2*(i+.35+.30*Random(h))/planeCount,angle=phase+i*2.399963229728653+(Random(h)-.5)*.35;
            double radial=std::sqrt(std::max(0.,1-z*z));P n={radial*std::cos(angle),radial*std::sin(angle),z};
            double support=std::sqrt(n[0]*n[0]*extent[0]*extent[0]+n[1]*n[1]*extent[1]*extent[1]+n[2]*n[2]*extent[2]*extent[2]);
            core=Clip(core,{n,support*(.86+.22*Random(h))});
        }
        double coreRadius=0;for(auto const& f:core)for(P p:f.p)coreRadius=std::max(coreRadius,std::sqrt(Dot(p,p)));
        if(coreRadius>.055){double s=.055/coreRadius;for(auto& f:core)for(P& p:f.p)p=Mul(p,s);coreRadius=.055;}
        std::vector<P> directions;for(auto const& f:core)directions.push_back(Normal(f));size_t macro=directions.size();
        // Only adjacent plane normals need intermediate directions. Their
        // tangent planes sample a circular fillet while broad centers stay flat.
        for(size_t a=0;a<macro;++a)for(size_t b=a+1;b<macro;++b)
        {
            int shared=0;for(P p:core[a].p)for(P q:core[b].p)if(Dot(Add(p,Mul(q,-1)),Add(p,Mul(q,-1)))<1e-18)++shared;
            if(shared>=2)for(double t:{.25,.5,.75})directions.push_back(Unit(Add(Mul(directions[a],1-t),Mul(directions[b],t))));
        }
        P n=Unit(outward),u=Unit(Cross(n,std::abs(n[2])<.8?P{0,0,1}:P{0,1,0})),v=Cross(n,u);
        double angle=Random(h)*6.283185307179586;P ru=Add(Mul(u,std::cos(angle)),Mul(v,std::sin(angle))),rv=Cross(n,ru);
        auto rotate=[&](P p){return Add(Add(Mul(ru,p[0]),Mul(rv,p[1])),Mul(n,p[2]));};
        P center=Add(contact,Mul(n,-extent[2]*.30));Cutter result;result.radius=radius;
        // Bounding sphere is conservative even at corners of fillet facets.
        double bound=coreRadius+radius*2;
        result.low=Add(center,P{-bound,-bound,-bound});result.high=Add(center,P{bound,bound,bound});
        for(P d:directions)
        {
            double support=-1e30;for(auto const& f:core)for(P p:f.p)support=std::max(support,Dot(d,p));
            P world=rotate(d);result.planes.push_back({world,Dot(world,center)+support+radius});
        }return result;
    }
    inline bool RayBox(P o,P d,P lo,P hi,double reach,double* entry=nullptr)
    {
        double enter=0,leave=reach;
        for(int k=0;k<3;++k)
        {
            if(std::abs(d[k])<1e-14){if(o[k]<lo[k]-1e-9||o[k]>hi[k]+1e-9)return false;}
            else{double a=(lo[k]-1e-9-o[k])/d[k],b=(hi[k]+1e-9-o[k])/d[k];if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);}
        }if(entry)*entry=enter;return leave>=enter;
    }
    // Immutable per-cell surface acceleration, prepared alongside the cut.
    // Queries never allocate/reconstruct face geometry or recompute normals.
    struct SurfaceQuery
    {
        struct Tri{P a,b,c,n;};
        struct Node{P lo={1e30,1e30,1e30},hi={-1e30,-1e30,-1e30};int begin=0,end=0,left=-1,right=-1;};
        std::vector<Tri> triangles;std::vector<Node> nodes;
        int BuildNode(int begin,int end)
        {
            int index=int(nodes.size());nodes.emplace_back();Node node;node.begin=begin;node.end=end;
            for(int i=begin;i<end;++i)for(P p:{triangles[i].a,triangles[i].b,triangles[i].c})for(int k=0;k<3;++k){node.lo[k]=std::min(node.lo[k],p[k]);node.hi[k]=std::max(node.hi[k],p[k]);}
            if(end-begin>8)
            {
                int axis=0;for(int k=1;k<3;++k)if(node.hi[k]-node.lo[k]>node.hi[axis]-node.lo[axis])axis=k;
                int mid=(begin+end)/2;
                std::nth_element(triangles.begin()+begin,triangles.begin()+mid,triangles.begin()+end,[axis](Tri const& a,Tri const& b){return a.a[axis]+a.b[axis]+a.c[axis]<b.a[axis]+b.b[axis]+b.c[axis];});
                node.left=BuildNode(begin,mid);node.right=BuildNode(mid,end);
            }nodes[index]=node;return index;
        }
        void Build(Poly const& surface)
        {
            triangles.clear();nodes.clear();Append(surface);Finish();
        }
        void Append(Poly const& surface)
        {
            for(auto const& f:surface){P n=Normal(f);for(size_t i=1;i+1<f.p.size();++i)triangles.push_back({f.p[0],f.p[i],f.p[i+1],n});}
        }
        void Append(P a,P b,P c)
        {
            triangles.push_back({a,b,c,Unit(Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1))))});
        }
        void Finish()
        {
            nodes.clear();
            if(!triangles.empty())BuildNode(0,int(triangles.size()));
        }
        void Trace(int index,P o,P d,double& nearest,Hit& result,int id)const
        {
            auto const& node=nodes[index];if(!RayBox(o,d,node.lo,node.hi,nearest))return;
            if(node.left>=0)
            {
                // Visit the nearer child first.  Containment rays previously
                // entered whichever half happened to be stored first, then
                // walked much of a detailed cavity before discovering the
                // nearby boundary that could have pruned the rest.
                double leftEntry=0,rightEntry=0;auto const& left=nodes[node.left];auto const& right=nodes[node.right];
                bool hasLeft=RayBox(o,d,left.lo,left.hi,nearest,&leftEntry),hasRight=RayBox(o,d,right.lo,right.hi,nearest,&rightEntry);
                int first=node.left,second=node.right;if(rightEntry<leftEntry)std::swap(first,second);
                if(hasLeft&&hasRight){Trace(first,o,d,nearest,result,id);Trace(second,o,d,nearest,result,id);}
                else if(hasLeft)Trace(node.left,o,d,nearest,result,id);else if(hasRight)Trace(node.right,o,d,nearest,result,id);
                return;
            }
            for(int i=node.begin;i<node.end;++i){auto const& f=triangles[i];double t=Intersect(f.a,f.b,f.c,o,d);if(t<nearest){nearest=t;result={true,id,t,Add(o,Mul(d,t)),f.n};}}
        }
        template<class F>void Visit(int index,P low,P high,F&& visit)const
        {
            auto const& node=nodes[index];for(int k=0;k<3;++k)if(node.hi[k]<low[k]||node.lo[k]>high[k])return;
            if(node.left>=0){Visit(node.left,low,high,visit);Visit(node.right,low,high,visit);return;}
            for(int i=node.begin;i<node.end;++i)visit(triangles[i]);
        }
        template<class F>bool VisitCapsule(int index,P a,P b,double radius2,F&& visit)const
        {
            auto const& node=nodes[index];double distance2=0;
            for(int k=0;k<2;++k){double delta=a[k]<node.lo[k]?node.lo[k]-a[k]:a[k]>node.hi[k]?a[k]-node.hi[k]:0;distance2+=delta*delta;}
            double dz=b[2]<node.lo[2]?node.lo[2]-b[2]:a[2]>node.hi[2]?a[2]-node.hi[2]:0;distance2+=dz*dz;
            if(distance2>radius2)return true;
            if(node.left>=0){if(!VisitCapsule(node.left,a,b,radius2,visit))return false;return VisitCapsule(node.right,a,b,radius2,visit);}
            for(int i=node.begin;i<node.end;++i)if(!visit(triangles[i]))return false;
            return true;
        }
    };
    struct SupportCache;
    struct Cell {std::vector<Poly> solids;Poly surface;SurfaceQuery query;std::shared_ptr<SupportCache const> support;uint32_t revision=0;};
    inline int RenderPatchID(GraniteOutcropV2::Piece const& piece)
    {
        // The former two-bin (.26 m) groups produced 311 procedural objects,
        // while six-bin groups collapsed the body to 25 oversized objects and
        // made one edited group take hundreds of milliseconds to rebuild. The
        // taller V5.5 crown needs four-bin groups to keep object count bounded
        // without returning to those oversized six-bin rebuilds.
        constexpr double patchSize=GraniteOutcropV2::BinSize*4;
        P c=Mul(Add(piece.low,piece.high),.5);
        int x=int(std::floor(c[0]/patchSize))+8,y=int(std::floor(c[1]/patchSize))+8,z=int(std::floor(c[2]/patchSize))+8;
        return x+32*(y+32*z);
    }
    struct Body
    {
        GraniteOutcropV2::Body substrate{false};std::map<int,Cell> changed;
        SurfaceQuery collisionQuery;
        std::map<int,std::vector<int>> renderGroups;
        std::map<std::array<int,3>,double> damage;
        uint32_t seed=8675309,revision=0,nextChipID=0;P low={},high={};double initialM3=0,removedM3=0;uint64_t initialMg=0,removedMg=0;
        explicit Body(bool initialize=true){if(initialize)Reset(seed);}
        void Reset(uint32_t s)
        {
            substrate.Reset(s);changed.clear();damage.clear();renderGroups.clear();Poly exterior;
            for(int id=0;id<int(substrate.pieces.size());++id)
            {
                renderGroups[RenderPatchID(substrate.pieces[id])].push_back(id);
                for(auto const& face:substrate.pieces[id].faces)if(face.neighbor<0)
                    exterior.push_back({{substrate.vertices[face.v[0]],substrate.vertices[face.v[1]],substrate.vertices[face.v[2]]},Weathered,0});
            }
            collisionQuery.Build(exterior);seed=s;revision=nextChipID=0;low=substrate.low;high=substrate.high;initialM3=substrate.initialM3;initialMg=uint64_t(std::llround(initialM3*Density*1e6));removedM3=0;removedMg=0;
        }
    };
    inline Poly OriginalSurface(Body const& body,int id)
    {
        Poly out;int index=0;for(auto const& f:body.substrate.pieces[id].faces){if(f.neighbor<0)out.push_back({{body.substrate.vertices[f.v[0]],body.substrate.vertices[f.v[1]],body.substrate.vertices[f.v[2]]},Weathered,id*16+index+1});++index;}return out;
    }
    // Both sides of an original interface share an exact identity. Subsequent
    // clipping preserves it, so support never depends on rounded coordinates.
    inline int SupportToken(Body const& body,int id,int faceIndex)
    {
        auto const& f=body.substrate.pieces[id].faces[faceIndex];int token=id*16+faceIndex+1;
        if(f.neighbor>=0){auto key=f.v;std::sort(key.begin(),key.end());int j=0;
            for(auto const& other:body.substrate.pieces[f.neighbor].faces){auto v=other.v;std::sort(v.begin(),v.end());if(v==key)token=std::min(token,f.neighbor*16+j+1);++j;}}
        return token;
    }
    inline Cell OriginalCell(Body const& body,int id,bool supportIdentity=true)
    {
        Cell cell;auto const& piece=body.substrate.pieces[id];
        for(size_t i=0;i<piece.tets.size();++i)
        {
            Poly p;for(int j=0;j<4;++j){int index=int(i*4+j);auto const& f=piece.faces[index];p.push_back({{body.substrate.vertices[f.v[0]],body.substrate.vertices[f.v[1]],body.substrate.vertices[f.v[2]]},Hidden,supportIdentity?SupportToken(body,id,index):0});}cell.solids.push_back(std::move(p));
        }cell.surface=OriginalSurface(body,id);return cell;
    }
    inline std::vector<int> Candidates(Body const& body,P low,P high)
    {
        auto const& base=body.substrate;int lo[3],hi[3];for(int k=0;k<3;++k){lo[k]=std::max(0,int(std::floor((low[k]-base.low[k])/GraniteOutcropV2::BinSize)));hi[k]=std::min(base.binCount[k]-1,int(std::floor((high[k]-base.low[k])/GraniteOutcropV2::BinSize)));}
        std::vector<int> ids;for(int z=lo[2];z<=hi[2];++z)for(int y=lo[1];y<=hi[1];++y)for(int x=lo[0];x<=hi[0];++x)
            for(int id:base.bins[base.BinID(x,y,z)]){auto const& p=base.pieces[id];if(p.low[0]<=high[0]&&p.high[0]>=low[0]&&p.low[1]<=high[1]&&p.high[1]>=low[1]&&p.low[2]<=high[2]&&p.high[2]>=low[2])ids.push_back(id);}
        std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());return ids;
    }
    inline uint32_t LocalRevision(Body const& body,P center,double radius)
    {
        P extent={radius+.01,radius+.01,radius+.01};uint32_t revision=0;
        for(int id:Candidates(body,Add(center,Mul(extent,-1)),Add(center,extent)))
        {
            auto changed=body.changed.find(id);if(changed!=body.changed.end())revision=std::max(revision,changed->second.revision);
        }
        return revision;
    }
    inline Hit Raycast(Body const& body,P o,P direction,double reach=2)
    {
        if(body.substrate.pieces.empty())return {};
        if(!Finite(o)||!Finite(direction)||!std::isfinite(reach)||reach<=0||Dot(direction,direction)<1e-16)return {};
        P d=Unit(direction);Hit result;double nearest=reach+1e-8,enter=0,leave=reach;auto const& base=body.substrate;
        for(int k=0;k<3;++k)
        {
            if(std::abs(d[k])<1e-14){if(o[k]<body.low[k]||o[k]>body.high[k])return {};}
            else{double a=(body.low[k]-o[k])/d[k],b=(body.high[k]-o[k])/d[k];if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);}
        }if(leave<enter)return {};
        int bin[3];for(int k=0;k<3;++k)bin[k]=std::clamp(int((o[k]+d[k]*(enter+1e-10)-body.low[k])/GraniteOutcropV2::BinSize),0,base.binCount[k]-1);
        // Walk only the bins touched by the ray, rather than its enclosing box.
        for(int iteration=0;iteration<base.binCount[0]+base.binCount[1]+base.binCount[2]+6;++iteration)
        {
          for(int id:base.bins[base.BinID(bin[0],bin[1],bin[2])])
          {
            auto const& piece=body.substrate.pieces[id];if(!RayBox(o,d,piece.low,piece.high,nearest))continue;
            auto it=body.changed.find(id);
            if(it!=body.changed.end()){if(!it->second.query.nodes.empty())it->second.query.Trace(0,o,d,nearest,result,id);}
            else for(auto const& f:piece.faces)if(f.neighbor<0)
            {
                P a=body.substrate.vertices[f.v[0]],b=body.substrate.vertices[f.v[1]],c=body.substrate.vertices[f.v[2]];
                double t=Intersect(a,b,c,o,d);if(t>nearest+1e-9)continue;
                P normal=Unit(Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1))));
                // At a shared edge several exterior faces can report the same
                // contact. Floating-point translation must not select an exit
                // face and turn a valid entry into a miss. Prefer the strongest
                // entry normal on ties, then stable source identity.
                bool nearer=t<nearest-1e-9;
                double facing=Dot(normal,d),oldFacing=Dot(result.normal,d);
                bool tied=result.hit&&std::abs(t-nearest)<=1e-9;
                if(nearer||(tied&&(facing<oldFacing-1e-9||(std::abs(facing-oldFacing)<=1e-9&&id<result.id))))
                {nearest=t;result={true,id,t,Add(o,Mul(d,t)),normal};}
            }
          }
          double next=1e30;int axis=-1;
          for(int k=0;k<3;++k)if(std::abs(d[k])>=1e-14){double t=(body.low[k]+(bin[k]+(d[k]>0?1:0))*GraniteOutcropV2::BinSize-o[k])/d[k];if(t<next){next=t;axis=k;}}
          if((result.hit&&nearest<=next+1e-8)||axis<0||next>leave)break;
          bin[axis]+=d[axis]>0?1:-1;if(bin[axis]<0||bin[axis]>=base.binCount[axis])break;
        }if(result.hit&&Dot(result.normal,d)>1e-8)return {};return result;
    }
    inline std::vector<Triangle> Triangulate(Poly const& surface,Body const* body=nullptr)
    {
        std::vector<Triangle> mesh;for(auto const& f:surface)
        {
            P faceNormal=Normal(f);
            auto normal=[&](P p)
            {
                P n=faceNormal;if(!body||f.axis!=Weathered||f.sign<=0)return n;
                auto const& base=body->substrate;auto const& triangle=base.pieces[(f.sign-1)/16].faces[(f.sign-1)%16];
                P a=base.vertices[triangle.v[0]],u=Add(base.vertices[triangle.v[1]],Mul(a,-1)),v=Add(base.vertices[triangle.v[2]],Mul(a,-1)),w=Add(p,Mul(a,-1));
                double uu=Dot(u,u),uv=Dot(u,v),vv=Dot(v,v),det=uu*vv-uv*uv;if(det<1e-20)return n;
                double b=(Dot(w,u)*vv-Dot(w,v)*uv)/det,c=(Dot(w,v)*uu-Dot(w,u)*uv)/det;
                return Unit(Add(Add(Mul(base.normals[triangle.v[0]],1-b-c),Mul(base.normals[triangle.v[1]],b)),Mul(base.normals[triangle.v[2]],c)));
            };
            for(size_t i=1;i+1<f.p.size();++i)mesh.push_back({f.p[0],f.p[i],f.p[i+1],normal(f.p[0]),normal(f.p[i]),normal(f.p[i+1])});
        }return mesh;
    }
    inline std::vector<Triangle> Boundary(Body const& body)
    {
        std::vector<Triangle> mesh;for(int id=0;id<int(body.substrate.pieces.size());++id)
        {auto it=body.changed.find(id);auto triangles=Triangulate(it==body.changed.end()?OriginalSurface(body,id):it->second.surface,&body);mesh.insert(mesh.end(),triangles.begin(),triangles.end());}return mesh;
    }
    // Rendering ownership groups do not clip or alter any material faces.
    // A local edit replaces only its groups; untouched groups retain their GPU meshes.
    inline std::vector<Triangle> PatchBoundary(Body const& body,int patch,std::vector<std::pair<int,Cell>> const& updates={})
    {
        std::vector<Triangle> mesh;auto group=body.renderGroups.find(patch);if(group==body.renderGroups.end())return mesh;
        for(int id:group->second)
        {
            auto update=std::find_if(updates.begin(),updates.end(),[id](auto const& v){return v.first==id;});
            auto old=body.changed.find(id);Poly original;Poly const* surface=nullptr;
            if(update!=updates.end())surface=&update->second.surface;
            else if(old!=body.changed.end())surface=&old->second.surface;
            else{original=OriginalSurface(body,id);surface=&original;}
            auto triangles=Triangulate(*surface,&body);mesh.insert(mesh.end(),triangles.begin(),triangles.end());
        }return mesh;
    }
    struct Chip:GraniteOutcropV2::Chip {std::vector<Triangle> mesh;double absorbedM3=0;};
    struct Receipt {Hit contact;bool removed=false;double transferredJ=0;Chip chip;std::vector<Chip> detached;double supportMs=0,prepareMs=0,absorbedM3=0,supportCacheMs=0,supportGraphMs=0,supportSolveMs=0;size_t supportNodes=0,failedLinks=0,culledRemnants=0,modifiedBytes=0;char const* status="READY";};
    struct Transaction {Receipt receipt;std::vector<std::pair<int,Cell>> updates;std::map<int,std::vector<Triangle>> renderUpdates;SurfaceQuery collisionQuery;std::array<int,3> damageKey={};double damage=0;bool hasDamage=false;uint32_t revision=0;};
    struct CollisionPreparationProfile { double assemblyMs=0,treeMs=0; };
    inline SurfaceQuery UpdatedCollisionQuery(Body const& body,std::vector<std::pair<int,Cell>> const& updates,CollisionPreparationProfile* profile=nullptr)
    {
        using Clock=std::chrono::steady_clock;
        auto start=profile?Clock::now():Clock::time_point{};
        SurfaceQuery query;
        for(int id=0;id<int(body.substrate.pieces.size());++id)
        {
            auto update=std::find_if(updates.begin(),updates.end(),[id](auto const& value){return value.first==id;});
            if(update!=updates.end()){query.Append(update->second.surface);continue;}
            auto changed=body.changed.find(id);if(changed!=body.changed.end()){query.Append(changed->second.surface);continue;}
            for(auto const& face:body.substrate.pieces[id].faces)if(face.neighbor<0)
                query.Append(body.substrate.vertices[face.v[0]],body.substrate.vertices[face.v[1]],body.substrate.vertices[face.v[2]]);
        }
        if(profile){auto now=Clock::now();profile->assemblyMs=std::chrono::duration<double,std::milli>(now-start).count();start=now;}
        query.Finish();
        if(profile)profile->treeMs=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        return query;
    }
    inline double ContactThickness(Body const& body,P contact,P inward,double reach)
    {
        P end=Add(contact,Mul(inward,reach)),lo,hi;for(int k=0;k<3;++k){lo[k]=std::min(contact[k],end[k])-1e-8;hi[k]=std::max(contact[k],end[k])+1e-8;}
        std::vector<std::pair<double,double>> intervals;
        for(int id:Candidates(body,lo,hi))
        {
            auto it=body.changed.find(id);Cell original;if(it==body.changed.end())original=OriginalCell(body,id,false);auto const& cell=it==body.changed.end()?original:it->second;
            for(auto const& solid:cell.solids)
            {
                double enter=0,leave=reach;bool hit=true;
                for(auto const& f:solid){P n=Normal(f);double num=Dot(n,Add(f.p[0],Mul(contact,-1))),den=Dot(n,inward);
                    if(std::abs(den)<1e-12){if(num< -1e-9){hit=false;break;}}
                    else if(den<0)enter=std::max(enter,num/den);else leave=std::min(leave,num/den);
                    if(leave<enter-1e-9){hit=false;break;}}
                if(hit&&leave>0)intervals.emplace_back(std::max(0.,enter),leave);
            }
        }
        std::sort(intervals.begin(),intervals.end());double depth=0;
        for(auto interval:intervals){if(interval.first>depth+1e-8)break;depth=std::max(depth,interval.second);}
        return depth;
    }
    // Optional headless observer. Normal gameplay callers take no clock samples.
    // These are CPU preparation phases, not GPU publication or frame latency.
    struct PreparationProfile { double cutMs=0,patchMs=0,collisionMs=0; CollisionPreparationProfile collision; bool complete=false; };
    inline Transaction PrepareStrike(Body const& body,P origin,P direction,double energyJ=400,uint32_t grade=2,PreparationProfile* profile=nullptr)
    {
        using ProfileClock=std::chrono::steady_clock;
        auto phaseStart=ProfileClock::time_point{};
        if(profile){*profile={};phaseStart=ProfileClock::now();}
        auto endPhase=[&](double& output){auto now=ProfileClock::now();output=std::chrono::duration<double,std::milli>(now-phaseStart).count();phaseStart=now;};
        Transaction tx;tx.revision=body.revision;auto& r=tx.receipt;if(!std::isfinite(energyJ)||energyJ<0||energyJ>10000)return tx;r.contact=Raycast(body,origin,direction);
        if(!r.contact.hit){r.status="MISS";return tx;}r.transferredJ=energyJ*std::max(0.,-Dot(Unit(direction),r.contact.normal));
        if(grade<2){r.status="CONTACT: implement too soft; no removal";return tx;}
        std::array<int,3> key;for(int k=0;k<3;++k)key[k]=int(std::floor(r.contact.position[k]/.012));
        auto foundDamage=body.damage.find(key);double damage=(foundDamage==body.damage.end()?0:foundDamage->second)+r.transferredJ;
        tx.hasDamage=true;tx.damageKey=key;tx.damage=damage;
        if(damage<12){r.status="LOCAL DAMAGE: strike here again";return tx;}
        uint32_t shapeSeed=Hash(body.seed^uint32_t(key[0])*73856093u^uint32_t(key[1])*19349663u^uint32_t(key[2])*83492791u);
        auto cutter=MakeCutter(shapeSeed,r.contact.position,r.contact.normal,damage);
        // A thin contacted lip must not let the cutter jump through air and
        // remove a second wall behind it. Bound depth at the first material
        // exit along the cutter's inward axis; internal partition seams merge.
        P inward=Mul(r.contact.normal,-1);double reach=std::sqrt(Dot(Add(cutter.high,Mul(cutter.low,-1)),Add(cutter.high,Mul(cutter.low,-1))));
        double thickness=ContactThickness(body,r.contact.position,inward,reach);
        if(thickness<reach-1e-8)cutter.planes.push_back({inward,Dot(inward,r.contact.position)+thickness+1e-9});
        double removed=0;
        for(int id:Candidates(body,cutter.low,cutter.high))
        {
            auto it=body.changed.find(id);Cell storage;if(it==body.changed.end())storage=OriginalCell(body,id);Cell const& original=it==body.changed.end()?storage:it->second;Cell next;
            Poly caps;double cellVolume=0;
            for(auto const& solid:original.solids)
            {
                if(!Overlaps(solid,cutter.planes)){next.solids.push_back(solid);continue;}
                auto cut=Intersection(solid,cutter.planes);double volume=Volume(cut);
                if(volume<1e-14){next.solids.push_back(solid);continue;}
                cellVolume+=volume;
                for(auto const& f:cut)if(f.axis==Fresh)caps.push_back(f);
                Poly remainder=solid;
                int planeIndex=0;for(auto plane:cutter.planes)
                {
                    // 256 identities per strike; reject overflow instead of aliasing bonds.
                    uint64_t identity=body.substrate.pieces.size()*16ull+1+uint64_t(body.revision)*256+planeIndex++;
                    if(identity>2147483647ull||cutter.planes.size()>256){r.status="REFUSED: support identity capacity";return Transaction{r};}
                    auto outside=Clip(remainder,{Mul(plane.n,-1),-plane.d},Hidden,int(identity));if(Volume(outside)>1e-14)next.solids.push_back(std::move(outside));
                    remainder=Clip(remainder,plane,Hidden,int(identity));if(remainder.empty())break;
                }
            }
            if(cellVolume<1e-14)continue;
            auto exterior=ClipSurface(original.surface,cutter.planes,true);r.chip.geometry.insert(r.chip.geometry.end(),exterior.begin(),exterior.end());
            r.chip.geometry.insert(r.chip.geometry.end(),caps.begin(),caps.end());
            next.surface=ClipSurface(original.surface,cutter.planes,false);
            for(auto f:caps){std::reverse(f.p.begin(),f.p.end());next.surface.push_back(std::move(f));}
            next.query.Build(next.surface);
            removed+=cellVolume;tx.updates.emplace_back(id,std::move(next));
        }
        if(removed<1e-12){r.chip={};r.status="CONTACT: no removable volume";return tx;}
        // Reject rather than publish a malformed geometric transaction.
        if(std::abs(Volume(r.chip.geometry)-removed)>std::max(1e-10,removed*1e-5)){
#if defined(GRANITE_CAST_DIAGNOSTICS)
            std::printf("cast %.12g parts %.12g delta %.12g\n",Volume(r.chip.geometry),removed,Volume(r.chip.geometry)-removed);
#endif
            r.chip={};r.status="REFUSED: cavity/cast volume mismatch";return tx;}
        r.chip.id=int(body.nextChipID);r.chip.volume=removed;
        uint64_t total=uint64_t(std::llround((body.removedM3+removed)*Density*1e6));r.chip.massMg=total-body.removedMg;
        P center={};double signedVolume=0;P ref=r.contact.position;
        for(auto const& f:r.chip.geometry)for(size_t i=1;i+1<f.p.size();++i){P a=Add(f.p[0],Mul(ref,-1)),b=Add(f.p[i],Mul(ref,-1)),c=Add(f.p[i+1],Mul(ref,-1));double v=Dot(a,Cross(b,c))/6;signedVolume+=v;center=Add(center,Mul(Add(Add(a,b),c),v*.25));}
        r.chip.center=Add(ref,Mul(center,1/signedVolume));r.chip.mesh=Triangulate(r.chip.geometry,&body);tx.damage=0;r.removed=true;r.status="CHIP RELEASED: rounded contact cast / accounted granite";
        if(profile)endPhase(profile->cutMs);
        for(auto const& update:tx.updates)tx.renderUpdates.emplace(RenderPatchID(body.substrate.pieces[update.first]),std::vector<Triangle>{});
        for(auto& patch:tx.renderUpdates)patch.second=PatchBoundary(body,patch.first,tx.updates);
        if(profile)endPhase(profile->patchMs);
        // Prepare one closed walking authority for the future revision on the
        // fracture worker. Movement across cavity lips should not revisit each
        // edited cell or reconstruct untouched tetrahedra per capsule probe.
        tx.collisionQuery=UpdatedCollisionQuery(body,tx.updates,profile?&profile->collision:nullptr);
        if(profile){endPhase(profile->collisionMs);profile->complete=true;}
        return tx;
    }
    inline Receipt Commit(Body& body,Transaction tx)
    {
        if(body.revision!=tx.revision){Receipt refused;refused.status="REFUSED: stale fracture transaction";return refused;}
        if(tx.hasDamage){if(tx.damage>0)body.damage[tx.damageKey]=tx.damage;else body.damage.erase(tx.damageKey);}
        if(tx.receipt.removed)
        {
            uint32_t revision=body.revision+1;
            for(auto& update:tx.updates){update.second.revision=revision;body.changed[update.first]=std::move(update.second);}
            body.collisionQuery=std::move(tx.collisionQuery);
            body.removedM3+=tx.receipt.chip.volume;body.removedMg+=tx.receipt.chip.massMg;++body.nextChipID;
            for(auto const& chip:tx.receipt.detached){body.removedM3+=chip.volume;body.removedMg+=chip.massMg;++body.nextChipID;}
            body.revision=revision;
        }
        return std::move(tx.receipt);
    }
    inline Receipt Strike(Body& body,P origin,P direction,double energyJ=400,uint32_t grade=2)
    {return Commit(body,PrepareStrike(body,origin,direction,energyJ,grade));}
    inline double SegmentDistance2(P a,P b,P c,P d)
    {
        P u=Add(b,Mul(a,-1)),v=Add(d,Mul(c,-1)),w=Add(a,Mul(c,-1));
        double aa=Dot(u,u),bb=Dot(u,v),cc=Dot(v,v),dd=Dot(u,w),ee=Dot(v,w),s=0,t=0;
        if(aa<1e-24){t=cc>1e-24?std::clamp(ee/cc,0.,1.):0;}
        else if(cc<1e-24)s=std::clamp(-dd/aa,0.,1.);
        else{double den=aa*cc-bb*bb;s=den>1e-24?std::clamp((bb*ee-cc*dd)/den,0.,1.):0;t=(bb*s+ee)/cc;if(t<0){t=0;s=std::clamp(-dd/aa,0.,1.);}else if(t>1){t=1;s=std::clamp((bb-dd)/aa,0.,1.);}}
        P delta=Add(Add(w,Mul(u,s)),Mul(v,-t));return Dot(delta,delta);
    }
    inline double PointTriangleDistance2(P p,P a,P b,P c)
    {
        P u=Add(b,Mul(a,-1)),v=Add(c,Mul(a,-1)),w=Add(p,Mul(a,-1));
        double uu=Dot(u,u),uv=Dot(u,v),vv=Dot(v,v),wu=Dot(w,u),wv=Dot(w,v),det=uu*vv-uv*uv;
        if(det>1e-30){double s=(wu*vv-wv*uv)/det,t=(wv*uu-wu*uv)/det;if(s>=0&&t>=0&&s+t<=1){P delta=Add(w,Mul(Add(Mul(u,s),Mul(v,t)),-1));return Dot(delta,delta);}}
        return std::min({SegmentDistance2(p,p,a,b),SegmentDistance2(p,p,b,c),SegmentDistance2(p,p,c,a)});
    }
    inline double SegmentTriangleDistance2(P a,P b,P x,P y,P z)
    {
        P direction=Add(b,Mul(a,-1));double t=Intersect(x,y,z,a,direction);if(t<=1)return 0;
        return std::min({PointTriangleDistance2(a,x,y,z),PointTriangleDistance2(b,x,y,z),SegmentDistance2(a,b,x,y),SegmentDistance2(a,b,y,z),SegmentDistance2(a,b,z,x)});
    }
    inline bool Contains(SurfaceQuery const& query,P point)
    {
        if(query.nodes.empty())return false;
        auto const& root=query.nodes[0];for(int k=0;k<3;++k)if(point[k]<root.lo[k]||point[k]>root.hi[k])return false;
        // The published surface is a closed, consistently outward-oriented
        // boundary.  From inside it, the first crossing of an arbitrary ray
        // is an exit; from outside it is an entry.  This avoids revisiting the
        // proliferating convex clipping remnants solely for containment.
        P direction=Unit(P{.7548776662466927,.5698402909980532,.3263518223330696});
        Hit hit;double nearest=1e30;query.Trace(0,point,direction,nearest,hit,-1);
        return hit.hit&&Dot(hit.normal,direction)>1e-9;
    }
    inline bool QueryBlocksCapsule(SurfaceQuery const& query,P a,P end,double radius)
    {
        if(query.nodes.empty())return false;
        double limit=radius*radius-1e-12;bool blocked=false;
        query.VisitCapsule(0,a,end,radius*radius,[&](SurfaceQuery::Tri const& triangle)
        {
            P lo,hi;for(int k=0;k<3;++k){lo[k]=std::min({triangle.a[k],triangle.b[k],triangle.c[k]});hi[k]=std::max({triangle.a[k],triangle.b[k],triangle.c[k]});}
            double boxDistance2=0;for(int k=0;k<2;++k){double delta=a[k]<lo[k]?lo[k]-a[k]:a[k]>hi[k]?a[k]-hi[k]:0;boxDistance2+=delta*delta;}
            double dz=end[2]<lo[2]?lo[2]-end[2]:a[2]>hi[2]?a[2]-hi[2]:0;boxDistance2+=dz*dz;
            if(boxDistance2<=radius*radius&&SegmentTriangleDistance2(a,end,triangle.a,triangle.b,triangle.c)<limit)blocked=true;
            return !blocked;
        });
        if(blocked)return true;
        // If the axis crosses a boundary the triangle test above catches it;
        // otherwise one interior witness is sufficient for full containment.
        return Contains(query,Mul(Add(a,end),.5));
    }
    inline bool OriginalTetBlocksCapsule(GraniteOutcropV2::Body const& base,GraniteOutcropV2::Piece const& piece,size_t tetIndex,P a,P end,double radius)
    {
        P d=Add(end,Mul(a,-1)),points[4][3],normals[4];
        for(int j=0;j<4;++j)
        {
            auto const& face=piece.faces[tetIndex*4+j];
            for(int k=0;k<3;++k)points[j][k]=base.vertices[face.v[k]];
            normals[j]=Unit(Cross(Add(points[j][1],Mul(points[j][0],-1)),Add(points[j][2],Mul(points[j][0],-1))));
        }
        double enter=0,leave=1;
        for(int j=0;j<4;++j)
        {
            double numerator=Dot(normals[j],Add(points[j][0],Mul(a,-1)))+radius,denom=Dot(normals[j],d);
            if(std::abs(denom)<1e-12){if(numerator<0)return false;}
            else{double t=numerator/denom;if(denom<0)enter=std::max(enter,t);else leave=std::min(leave,t);}
            if(leave<enter)return false;
        }
        // Plane inflation is conservative at tetrahedron corners. Preserve
        // the exact rounded-capsule result without constructing temporary
        // Poly/Cell vectors for every walk probe.
        double axisEnter=0,axisLeave=1;
        for(int j=0;j<4;++j)
        {
            double numerator=Dot(normals[j],Add(points[j][0],Mul(a,-1))),denom=Dot(normals[j],d);
            if(std::abs(denom)<1e-12){if(numerator<0){axisEnter=2;break;}}
            else if(denom<0)axisEnter=std::max(axisEnter,numerator/denom);else axisLeave=std::min(axisLeave,numerator/denom);
            if(axisLeave<axisEnter)break;
        }
        if(axisLeave>=axisEnter)return true;
        double limit=radius*radius-1e-12;
        for(int j=0;j<4;++j)if(SegmentTriangleDistance2(a,end,points[j][0],points[j][1],points[j][2])<limit)return true;
        return false;
    }
    inline bool BlocksCapsule(Body const& body,P feet,double radius=.23,double height=1.8)
    {
        P a=Add(feet,P{0,0,radius}),d={0,0,height-2*radius},end=Add(a,d);
        return QueryBlocksCapsule(body.collisionQuery,a,end,radius);
    }
}
