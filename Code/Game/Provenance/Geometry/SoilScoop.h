#pragma once
#include "GraniteContactCast.h"
#include "GraniteOutcropGround.h"
#include <set>
#include <chrono>
#include <unordered_map>

// Sparse volumetric soil. Immutable 24 cm chunks share a 2 cm sampling grid.
// Six tetrahedra per cube define both the rendered boundary and its volume.
// Untouched ground stays coarse; worker snapshots share unchanged chunks/BVHs.
namespace EE::SoilScoop
{
    namespace G=GraniteContactCast;namespace S=GraniteOutcropGround;using G::P;
    using Key=std::array<int,3>;
    inline constexpr int N=12,Side=N+1,Group=16,Columns=(S::NX+Group-1)/Group;
    inline constexpr double Step=.02,Span=N*Step,Band=Step*2,Bottom=-.12,Top=2.04;
    inline constexpr double Diameter=.2159,Depth=.07239,BulkDensity=2650*.60;
    inline constexpr double ScoopVolume=3.141592653589793*Depth*(3*Diameter*Diameter*.25+Depth*Depth)/6;
    inline constexpr int CX=46,CY=44,CZ=9;
    inline constexpr int Tets[6][4]={{0,1,3,7},{0,3,2,7},{0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}};
    inline P Low(Key k){return {S::X+k[0]*Span,S::Y+k[1]*Span,Bottom+k[2]*Span};}
    inline P Point(Key k,int x,int y,int z){return {S::X+(k[0]*N+x)*Step,S::Y+(k[1]*N+y)*Step,Bottom+(k[2]*N+z)*Step};}
    inline double Sample(double value){return std::abs(value)<1e-10?0:std::clamp(value,-Band,Band);}
    inline int ID(Key k){return 1000+k[0]+CX*(k[1]+CY*k[2]);}
    inline int Index(int x,int y,int z){return x+Side*(y+Side*z);}
    using VertexKey=std::array<double,6>;
    struct VertexHash{size_t operator()(VertexKey const& key)const{size_t h=0;for(double v:key)h^=std::hash<double>{}(v)+size_t(0x9e3779b9)+(h<<6)+(h>>2);return h;}};
    inline VertexKey VertexID(P p,P n){return {p[0],p[1],p[2],n[0],n[1],n[2]};}
    inline bool Overlap(P a,P b,P c,P d){for(int k=0;k<3;++k)if(a[k]>d[k]+1e-9||b[k]<c[k]-1e-9)return false;return true;}
    inline double BoxDistance2(P p,P lo,P hi){double d=0;for(int k=0;k<3;++k){double v=std::max({lo[k]-p[k],p[k]-hi[k],0.});d+=v*v;}return d;}
    inline void Closest(G::SurfaceQuery const& q,int id,P p,double& distance)
    {
        auto const& n=q.nodes[id];if(BoxDistance2(p,n.lo,n.hi)>distance)return;
        if(n.left>=0)
        {
            auto const& a=q.nodes[n.left];auto const& b=q.nodes[n.right];
            bool leftFirst=BoxDistance2(p,a.lo,a.hi)<BoxDistance2(p,b.lo,b.hi);
            Closest(q,leftFirst?n.left:n.right,p,distance);Closest(q,leftFirst?n.right:n.left,p,distance);return;
        }
        for(int i=n.begin;i<n.end;++i){auto const& t=q.triangles[i];distance=std::min(distance,G::PointTriangleDistance2(p,t.a,t.b,t.c));}
    }
    template<class F> inline void Visit(G::SurfaceQuery const& q,int id,P lo,P hi,F& emit)
    {
        auto const& n=q.nodes[id];if(!Overlap(lo,hi,n.lo,n.hi))return;
        if(n.left>=0){Visit(q,n.left,lo,hi,emit);Visit(q,n.right,lo,hi,emit);return;}
        for(int i=n.begin;i<n.end;++i)emit(q.triangles[i]);
    }
    inline P Gradient(std::array<P,4> const& p,std::array<double,4> const& v)
    {
        P a=G::Add(p[1],G::Mul(p[0],-1)),b=G::Add(p[2],G::Mul(p[0],-1)),c=G::Add(p[3],G::Mul(p[0],-1));
        return G::Mul(G::Add(G::Add(G::Mul(G::Cross(b,c),v[1]-v[0]),G::Mul(G::Cross(c,a),v[2]-v[0])),G::Mul(G::Cross(a,b),v[3]-v[0])),1/G::Dot(a,G::Cross(b,c)));
    }
    inline double TetVolume(std::array<P,4> const& p,std::array<double,4> const& v)
    {
        if(*std::max_element(v.begin(),v.end())<=0)return 0;
        constexpr double whole=Step*Step*Step/6;int count=0,odd=-1;for(int i=0;i<4;++i)if(v[i]>0){++count;odd=i;}
        if(!count)return 0;if(count==4)return whole;
        if(count==1||count==3)
        {
            if(count==3)for(int i=0;i<4;++i)if(v[i]<=0)odd=i;
            double fraction=1;for(int i=0;i<4;++i)if(i!=odd)fraction*=v[odd]/(v[odd]-v[i]);
            return whole*(count==1?fraction:1-fraction);
        }
        G::Poly poly;for(int face=0;face<4;++face)
        {
            std::vector<P> f;for(int j=0;j<4;++j)if(j!=face)f.push_back(p[j]);
            if(G::Dot(G::Cross(G::Add(f[1],G::Mul(f[0],-1)),G::Add(f[2],G::Mul(f[0],-1))),G::Add(p[face],G::Mul(f[0],-1)))>0)std::swap(f[1],f[2]);
            poly.push_back({std::move(f),0,0});
        }
        P grad=Gradient(p,v);double length=std::sqrt(G::Dot(grad,grad));P normal=G::Mul(grad,-1/length);
        return G::Volume(G::Clip(poly,{normal,G::Dot(normal,p[0])+v[0]/length}));
    }
    inline void TetSurface(std::array<P,4> const& p,std::array<double,4> const& v,G::Poly& faces)
    {
        // Zero is the material boundary, not an interior sample.  Treating it
        // as solid omitted a whole face when an untouched terrain surface lay
        // exactly on an edited chunk plane, producing rectangular corner and
        // top-edge holes.
        std::vector<P> cut;for(int a=0;a<4;++a)for(int b=a+1;b<4;++b)if((v[a]>0)!=(v[b]>0))
        {
            P point=G::Add(p[a],G::Mul(G::Add(p[b],G::Mul(p[a],-1)),v[a]/(v[a]-v[b])));bool duplicate=false;
            for(P old:cut)if(G::Dot(G::Add(old,G::Mul(point,-1)),G::Add(old,G::Mul(point,-1)))<1e-20)duplicate=true;
            if(!duplicate)cut.push_back(point);
        }
        if(cut.size()<3)return;P normal=G::Mul(G::Unit(Gradient(p,v)),-1),center={};for(P point:cut)center=G::Add(center,point);center=G::Mul(center,1./cut.size());
        P u=G::Unit(G::Cross(normal,std::abs(normal[2])<.8?P{0,0,1}:P{0,1,0})),w=G::Cross(normal,u);
        std::sort(cut.begin(),cut.end(),[&](P a,P b){a=G::Add(a,G::Mul(center,-1));b=G::Add(b,G::Mul(center,-1));return std::atan2(G::Dot(a,w),G::Dot(a,u))<std::atan2(G::Dot(b,w),G::Dot(b,u));});
        faces.push_back({std::move(cut),0,0});
    }
    struct Chunk
    {
        Key key;std::array<double,Side*Side*Side> values{};double volume=0;
        std::array<int,Side*Side*Side> labels{};int components=0;
        void Label()
        {
            std::array<int,Side*Side*Side> parent;for(int i=0;i<int(parent.size());++i)parent[i]=i;
            auto root=[&](int i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;};
            // These are exactly the seven increasing edges of our six-tet grid.
            for(int z=0;z<Side;++z)for(int y=0;y<Side;++y)for(int x=0;x<Side;++x)
            {
                int a=Index(x,y,z);if(values[a]<=0)continue;
                for(int d=1;d<8;++d){int u=x+(d&1),v=y+((d>>1)&1),w=z+((d>>2)&1);if(u>=Side||v>=Side||w>=Side)continue;int b=Index(u,v,w);if(values[b]>0)parent[root(b)]=root(a);}
            }
            components=0;std::array<int,Side*Side*Side> ids;ids.fill(-1);labels.fill(-1);
            for(int i=0;i<int(parent.size());++i)if(values[i]>0){int r=root(i);if(ids[r]<0)ids[r]=components++;labels[i]=ids[r];}
        }
        template<class F> void EachTet(F emit)const
        {
            for(int z=0;z<N;++z)for(int y=0;y<N;++y)for(int x=0;x<N;++x)
            {
                std::array<P,8> points;std::array<double,8> field;
                for(int i=0;i<8;++i){int a=x+(i&1),b=y+((i>>1)&1),c=z+((i>>2)&1);points[i]=Point(key,a,b,c);field[i]=values[Index(a,b,c)];}
                for(auto const& tet:Tets){std::array<P,4> p;std::array<double,4> v;for(int i=0;i<4;++i){p[i]=points[tet[i]];v[i]=field[tet[i]];}emit(p,v);}
            }
        }
        double Volume()const {double total=0;EachTet([&](auto const& p,auto const& v){total+=TetVolume(p,v);});return total;}
        G::SurfaceQuery Mesh()const
        {
            bool boundary=key[0]==0||key[0]==CX-1||key[1]==0||key[1]==CY-1||key[2]==0||key[2]==CZ-1;
            G::Poly faces;EachTet([&](auto const& p,auto const& v)
            {
                if(*std::max_element(v.begin(),v.end())<=0)return;
                TetSurface(p,v,faces);
                if(!boundary)return;
                // The field ends at the lab boundary. Zero-valued boundary
                // samples have no outside tetrahedron to generate their cap.
                // Explicitly clip the domain faces, with outward winding.
                for(int axis=0;axis<3;++axis)for(int side=0;side<2;++side)
                {
                    int limit=axis==0?CX:axis==1?CY:CZ;if(key[axis]!=(side?limit-1:0))continue;
                    double edge=axis==0?(side?S::X+S::NX*S::Step:S::X):axis==1?(side?S::Y+S::NY*S::Step:S::Y):(side?Top:Bottom);
                    for(int omit=0;omit<4;++omit)
                    {
                        std::array<int,3> ids{};int count=0;for(int i=0;i<4;++i)if(i!=omit&&std::abs(p[i][axis]-edge)<1e-9)ids[count++]=i;
                        if(count!=3)continue;std::vector<P> polygon;
                        for(int i=0;i<3;++i){int a=ids[i],b=ids[(i+1)%3];if(v[a]>=0)polygon.push_back(p[a]);if((v[a]>=0)!=(v[b]>=0))polygon.push_back(G::Add(p[a],G::Mul(G::Add(p[b],G::Mul(p[a],-1)),v[a]/(v[a]-v[b]))));}
                        std::vector<P> unique;for(P point:polygon){bool duplicate=false;for(P old:unique){P delta=G::Add(point,G::Mul(old,-1));if(G::Dot(delta,delta)<1e-20)duplicate=true;}if(!duplicate)unique.push_back(point);}polygon=std::move(unique);
                        if(polygon.size()<3)continue;
                        P normal=G::Cross(G::Add(polygon[1],G::Mul(polygon[0],-1)),G::Add(polygon[2],G::Mul(polygon[0],-1)));
                        if(G::Dot(normal,normal)<1e-20)continue;
                        if(normal[axis]*(side?1:-1)<0)std::reverse(polygon.begin(),polygon.end());
                        faces.push_back({std::move(polygon),0,0});
                    }
                }
            });faces.erase(std::remove_if(faces.begin(),faces.end(),[](auto const& f){P n=G::Normal(f);return !G::Finite(n)||G::Dot(n,n)<.9;}),faces.end());G::SurfaceQuery query;query.Build(faces);return query;
        }
    };
    struct Body;
    struct RenderSurface
    {
        struct Vertex{P position,normal;};
        std::vector<Vertex> vertices;std::vector<uint32_t> indices;
    };
    struct LooseSoil
    {
        std::shared_ptr<Body const> matter;P offset{};double velocity=0,volume=0;
        P lo={1e30,1e30,1e30},hi={-1e30,-1e30,-1e30};std::vector<P> feet;bool settled=false;double brushCooldown=0;uint32_t supportRevision=0,rockRevision=0,meshRevision=0;
    };
    struct Receipt {double volume=0;uint64_t massMg=0;size_t cells=0;bool blocked=false,collectedClod=false;double supportMs=0;size_t released=0;};
    struct Body
    {
        std::map<Key,std::shared_ptr<Chunk const>> chunks;
        // Initial connectivity is generated lazily, without publishing meshes
        // or counting the untouched interior as excavated material.
        std::map<Key,std::shared_ptr<Chunk const>> supportCache;
        std::map<int,std::shared_ptr<G::SurfaceQuery const>> queries;
        std::map<int,std::shared_ptr<std::vector<G::Triangle> const>> renderSurfaces;
        std::map<int,std::shared_ptr<RenderSurface const>> indexedSurfaces;
        std::shared_ptr<G::SurfaceQuery const> originalRock;
        std::vector<LooseSoil> loose;bool finiteChunks=false;
        bool hasInitialField=false;
        std::set<int> dirty;uint32_t revision=0,scoops=0;double removedM3=0;uint64_t removedMg=0;
        double RockOutside(P p)const
        {
            if(!originalRock||originalRock->nodes.empty())return 100;
            auto const& root=originalRock->nodes[0];double box=BoxDistance2(p,root.lo,root.hi);if(box>Step*Step*4)return std::sqrt(box);
            // The soil field clamps to Band anyway. Do not traverse distant
            // granite triangles to find an exact distance that will be discarded.
            double distance=Band*Band;Closest(*originalRock,0,p,distance);
            G::Hit hit;double reach=10;originalRock->Trace(0,p,{0,0,1},reach,hit,-1);
            return std::sqrt(distance)*(hit.hit&&hit.normal[2]>0?-1:1);
        }
        double Initial(P p)const
        {
            double x=std::clamp(p[0],S::X,S::X+S::NX*S::Step-1e-9),y=std::clamp(p[1],S::Y,S::Y+S::NY*S::Step-1e-9),height=0;S::Surface(x,y,height);
            return Sample(std::min({height-p[2],p[2]-Bottom,p[0]-S::X,p[1]-S::Y,S::X+S::NX*S::Step-p[0],S::Y+S::NY*S::Step-p[1],RockOutside(p)}));
        }
        static int Patch(Key k){return (k[0]*2)/Group+((k[1]*2)/Group)*Columns;}
        G::Poly PatchFaces(int id)const
        {
            G::Poly faces;int sx=(id%Columns)*Group,sy=(id/Columns)*Group;
            for(int y=sy;y<std::min(sy+Group,S::NY);++y)for(int x=sx;x<std::min(sx+Group,S::NX);++x)
            {
                auto q=S::Quad(x,y);faces.push_back({{q[0],q[1],q[2]},0,0});faces.push_back({{q[0],q[2],q[3]},0,0});
                auto skirt=[&](P a,P b){P c=b,d=a;c[2]=d[2]=Bottom;faces.push_back({{a,b,c,d},0,0});};
                if(y==0)skirt(q[1],q[0]);if(y==S::NY-1)skirt(q[3],q[2]);if(x==0)skirt(q[0],q[3]);if(x==S::NX-1)skirt(q[2],q[1]);
            }
            for(auto const& entry:chunks)if(Patch(entry.first)==id)
            {
                P lo=Low(entry.first),hi=G::Add(lo,{Span,Span,Span});std::vector<G::Plane> planes;
                // Include coplanar exterior skirts in the removed coarse patch.
                // Otherwise ClipSurface preserves them as both inside/outside,
                // covering a valid cavity with the original one-sided wall.
                for(int k=0;k<3;++k){P n={};n[k]=1;planes.push_back({n,hi[k]+1e-8});n[k]=-1;planes.push_back({n,-lo[k]+1e-8});}
                G::Poly next;for(auto const& f:faces)
                {
                    P a={1e30,1e30,1e30},b={-1e30,-1e30,-1e30};for(P p:f.p)for(int k=0;k<3;++k){a[k]=std::min(a[k],p[k]);b[k]=std::max(b[k],p[k]);}
                    if(!Overlap(lo,hi,a,b)){next.push_back(f);continue;}
                    auto outside=G::ClipSurface({f},planes,false);next.insert(next.end(),outside.begin(),outside.end());
                }faces=std::move(next);
            }return faces;
        }
        void Reset(G::Body const* rock=nullptr)
        {
            chunks.clear();supportCache.clear();queries.clear();renderSurfaces.clear();indexedSurfaces.clear();loose.clear();finiteChunks=false;dirty.clear();revision=scoops=0;removedM3=0;removedMg=0;originalRock.reset();hasInitialField=true;
            if(rock){G::Poly faces;for(int id=0;id<int(rock->substrate.pieces.size());++id){auto p=G::OriginalSurface(*rock,id);faces.insert(faces.end(),p.begin(),p.end());}auto q=std::make_shared<G::SurfaceQuery>();q->Build(faces);originalRock=q;}
            for(int y=0;y<S::NY;y+=Group)for(int x=0;x<S::NX;x+=Group){int id=x/Group+(y/Group)*Columns;auto q=std::make_shared<G::SurfaceQuery>();q->Build(PatchFaces(id));queries[id]=q;dirty.insert(id);}
            PrepareRender();
        }
        template<class F> void StaticQueries(P lo,P hi,F visit)const
        {
            // Test only the coarse surface patches and edited volume chunks
            // touched by the query bounds. Walking used to traverse every BVH
            // accumulated by hundreds of scoops, even across the map.
            if(!hasInitialField&&!finiteChunks){for(auto const& e:queries)if(!e.second->nodes.empty())visit(*e.second);return;}
            if(hasInitialField&&!finiteChunks)
            {
                constexpr int rows=(S::NY+Group-1)/Group;
                int x0=std::clamp(int(std::floor((lo[0]-S::X)/(S::Step*Group))),0,Columns-1),x1=std::clamp(int(std::floor((hi[0]-S::X)/(S::Step*Group))),0,Columns-1);
                int y0=std::clamp(int(std::floor((lo[1]-S::Y)/(S::Step*Group))),0,rows-1),y1=std::clamp(int(std::floor((hi[1]-S::Y)/(S::Step*Group))),0,rows-1);
                for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){auto q=queries.find(x+y*Columns);if(q!=queries.end()&&!q->second->nodes.empty())visit(*q->second);}
            }
            P base={S::X,S::Y,Bottom};int counts[3]={CX,CY,CZ},lower[3],upper[3];
            for(int k=0;k<3;++k){lower[k]=std::clamp(int(std::floor((lo[k]-base[k])/Span)),0,counts[k]-1);upper[k]=std::clamp(int(std::floor((hi[k]-base[k])/Span)),0,counts[k]-1);}
            for(int z=lower[2];z<=upper[2];++z)for(int y=lower[1];y<=upper[1];++y)for(int x=lower[0];x<=upper[0];++x)
            {auto q=queries.find(ID({x,y,z}));if(q!=queries.end()&&!q->second->nodes.empty())visit(*q->second);}
        }
        void Trace(P origin,P ray,double& nearest,G::Hit& hit,bool includeLoose=true)const
        {
            double length=std::max(0.,nearest),magnitude=std::sqrt(G::Dot(ray,ray));P end=G::Add(origin,G::Mul(ray,magnitude>1e-12?length/magnitude:0));
            P lo,hi;for(int k=0;k<3;++k){lo[k]=std::min(origin[k],end[k])-.001;hi[k]=std::max(origin[k],end[k])+.001;}
            StaticQueries(lo,hi,[&](auto const& q){q.Trace(0,origin,ray,nearest,hit,-1);});
            if(includeLoose)for(size_t i=0;i<loose.size();++i)if(loose[i].volume>1e-12)
            {G::Hit candidate;loose[i].matter->Trace(G::Add(origin,G::Mul(loose[i].offset,-1)),ray,nearest,candidate,false);if(candidate.hit){hit=candidate;hit.id=-1000-int(i);hit.position=G::Add(hit.position,loose[i].offset);}}
        }
        template<class F> void Triangles(P lo,P hi,F emit,bool includeLoose=true)const
        {
            StaticQueries(lo,hi,[&](auto const& q){Visit(q,0,lo,hi,emit);});
            if(includeLoose)for(auto const& d:loose)if(d.volume>1e-12){auto translated=[&](auto t){t.a=G::Add(t.a,d.offset);t.b=G::Add(t.b,d.offset);t.c=G::Add(t.c,d.offset);emit(t);};P localLo=G::Add(lo,G::Mul(d.offset,-1)),localHi=G::Add(hi,G::Mul(d.offset,-1));d.matter->StaticQueries(localLo,localHi,[&](auto const& q){Visit(q,0,localLo,localHi,translated);});}
        }
        double Field(P p)const
        {
            Key key={int(std::floor((p[0]-S::X)/Span)),int(std::floor((p[1]-S::Y)/Span)),int(std::floor((p[2]-Bottom)/Span))};
            auto found=chunks.find(key);
            if(found==chunks.end()&&finiteChunks)return -Band;
            if(found==chunks.end())return Sample(std::min({S::Height(p[0],p[1])-p[2],p[2]-Bottom,p[0]-S::X,p[1]-S::Y,S::X+S::NX*S::Step-p[0],S::Y+S::NY*S::Step-p[1]}));
            P lo=Low(key);int cell[3];double f[3];for(int k=0;k<3;++k){double v=(p[k]-lo[k])/Step;cell[k]=std::clamp(int(std::floor(v)),0,N-1);f[k]=v-cell[k];}
            // Sorting coordinates selects the same body-diagonal tetrahedron
            // used by meshing; barycentric interpolation is an exact inside test.
            int order[3]={0,1,2};std::sort(order,order+3,[&](int a,int b){return f[a]>f[b];});
            auto sample=[&](){return found->second->values[Index(cell[0],cell[1],cell[2])];};
            double value=(1-f[order[0]])*sample();++cell[order[0]];
            value+=(f[order[0]]-f[order[1]])*sample();++cell[order[1]];
            value+=(f[order[1]]-f[order[2]])*sample();++cell[order[2]];value+=f[order[2]]*sample();return value;
        }
        bool Contains(P p,bool includeLoose=true)const
        {
            if(includeLoose)for(auto const& d:loose)if(d.volume>1e-12&&d.matter->Contains(G::Add(p,G::Mul(d.offset,-1)),false))return true;
            if(finiteChunks)return Field(p)>0;
            if(!hasInitialField){double reach=10;G::Hit hit;P ray=G::Unit(P{.127,.073,1});Trace(p,ray,reach,hit);return hit.hit&&G::Dot(hit.normal,ray)>0;}
            if(p[0]<S::X||p[0]>=S::X+S::NX*S::Step||p[1]<S::Y||p[1]>=S::Y+S::NY*S::Step||p[2]<Bottom||p[2]>=Top)return false;
            Key key={int(std::floor((p[0]-S::X)/Span)),int(std::floor((p[1]-S::Y)/Span)),int(std::floor((p[2]-Bottom)/Span))};
            if(chunks.find(key)==chunks.end()){double h=0;return S::Surface(p[0],p[1],h)&&p[2]<h;}
            return Field(p)>0;
        }
        P ShadingNormal(P p,P fallback)const
        {
            // A vertical lab skirt is a hard boundary, not a smoothed terrain
            // edge. Finite differences straddling coarse/edited chunks gave
            // different normals to entire long wall triangles (dark rectangles).
            if(!finiteChunks)for(int axis=0;axis<2;++axis)for(int side=0;side<2;++side)
            {
                double edge=axis==0?(side?S::X+S::NX*S::Step:S::X):(side?S::Y+S::NY*S::Step:S::Y);
                if(std::abs(p[axis]-edge)<1e-7&&fallback[axis]*(side?1:-1)>.999){P n{};n[axis]=side?1:-1;return n;}
            }
            P gradient{};for(int k=0;k<3;++k){P a=p,b=p;a[k]-=Step*.5;b[k]+=Step*.5;gradient[k]=Field(a)-Field(b);}
            P n=G::Unit(gradient);return G::Dot(n,fallback)>.2?n:fallback;
        }
        void PrepareRender()
        {
            // Prepared on the worker, never in the publication loop. One shared
            // normal per position prevents flat tetrahedral shading and patches.
            std::unordered_map<VertexKey,P,VertexHash> normals;
            for(int id:dirty)
            {
                auto mesh=std::make_shared<std::vector<G::Triangle>>();auto const& tris=queries.at(id)->triangles;mesh->reserve(tris.size());
                auto normal=[&](P p,P face){P boundary{};if(!finiteChunks)for(int axis=0;axis<2;++axis)if(std::abs(face[axis])>.999)boundary=face;auto key=VertexID(p,boundary);auto it=normals.find(key);if(it!=normals.end()&&G::Dot(it->second,face)>.2)return it->second;P n=ShadingNormal(p,face);normals[key]=n;return n;};
                for(auto const& t:tris)mesh->push_back({t.a,t.b,t.c,normal(t.a,t.n),normal(t.b,t.n),normal(t.c,t.n)});
                renderSurfaces[id]=mesh;
                auto indexed=std::make_shared<RenderSurface>();indexed->indices.reserve(mesh->size()*3);
                std::unordered_map<VertexKey,uint32_t,VertexHash> vertices;vertices.reserve(mesh->size());
                for(auto const& t:*mesh)
                {
                    auto add=[&](P p,P n){auto inserted=vertices.emplace(VertexID(p,n),uint32_t(indexed->vertices.size()));if(inserted.second)indexed->vertices.push_back({p,n});indexed->indices.push_back(inserted.first->second);};
                    add(t.a,t.na);add(t.b,t.nb);add(t.c,t.nc);
                }
                indexedSurfaces[id]=indexed;
            }
        }
        bool BlocksCapsule(P feet,double radius,double height,bool includeLoose=true)const
        {
            P a=G::Add(feet,{0,0,radius}),b=G::Add(feet,{0,0,height-radius});bool blocked=false;
            Triangles(G::Add(feet,{-radius,-radius,0}),G::Add(feet,{radius,radius,height}),[&](auto const& t){if(!blocked&&G::SegmentTriangleDistance2(a,b,t.a,t.b,t.c)<radius*radius-1e-10)blocked=true;},includeLoose);
            return blocked||Contains(G::Mul(G::Add(a,b),.5),includeLoose);
        }
        bool SurfaceBelow(double x,double y,double ceiling,double& h,bool includeLoose=true)const
        {G::Hit hit;double reach=std::max(0.,ceiling-Bottom)+.01;Trace({x,y,ceiling+1e-5},{0,0,-1},reach,hit,includeLoose);if(!hit.hit||hit.normal[2]<.01)return false;h=hit.position[2];return true;}
        bool Surface(double x,double y,double& h)const{return SurfaceBelow(x,y,Top+.1,h);}
        bool Edited(double x,double y,double radius)const
        {
            if(revision==0||chunks.empty())return false;
            int x0=std::clamp(int(std::floor((x-radius-S::X)/Span)),0,CX-1),x1=std::clamp(int(std::floor((x+radius-S::X)/Span)),0,CX-1);
            int y0=std::clamp(int(std::floor((y-radius-S::Y)/Span)),0,CY-1),y1=std::clamp(int(std::floor((y+radius-S::Y)/Span)),0,CY-1);
            for(int z=0;z<CZ;++z)for(int iy=y0;iy<=y1;++iy)for(int ix=x0;ix<=x1;++ix)if(chunks.count({ix,iy,z}))return true;return false;
        }
    };
    inline void RefreshLoose(LooseSoil& d)
    {
        d.volume=0;d.lo={1e30,1e30,1e30};d.hi={-1e30,-1e30,-1e30};d.feet.clear();std::map<std::array<double,2>,double> columns;
        for(auto const& e:d.matter->chunks)d.volume+=e.second->volume;
        for(auto const& e:d.matter->queries)
        {
            if(!e.second->nodes.empty()){auto const& root=e.second->nodes[0];for(int k=0;k<3;++k){d.lo[k]=std::min(d.lo[k],root.lo[k]);d.hi[k]=std::max(d.hi[k],root.hi[k]);}}
            for(auto const& t:e.second->triangles)for(P p:{t.a,t.b,t.c})
            {auto key=std::array<double,2>{p[0],p[1]};auto it=columns.find(key);if(it==columns.end())columns[key]=p[2];else it->second=std::min(it->second,p[2]);}
        }
        for(auto const& e:columns)d.feet.push_back({e.first[0],e.first[1],e.second});
        d.settled=false;++d.meshRevision;
    }
    // Local labels are cached with immutable chunks. Only changed chunks run
    // the 3D flood fill; the global pass joins their much smaller boundary graph.
    inline size_t ReleaseUnsupported(Body& soil)
    {
        using Ref=std::pair<Key,int>;std::map<Ref,int> solved;int islandCount=0;
        auto chunkAt=[&](Key key)->std::shared_ptr<Chunk const>
        {
            int counts[3]={CX,CY,CZ};for(int axis=0;axis<3;++axis)if(key[axis]<0||key[axis]>=counts[axis])return {};
            auto edited=soil.chunks.find(key);if(edited!=soil.chunks.end())return edited->second;if(soil.finiteChunks)return {};
            auto cached=soil.supportCache.find(key);if(cached!=soil.supportCache.end())return cached->second;
            auto chunk=std::make_shared<Chunk>();chunk->key=key;
            for(int z=0;z<Side;++z)for(int y=0;y<Side;++y)for(int x=0;x<Side;++x)chunk->values[Index(x,y,z)]=soil.Initial(Point(key,x,y,z));
            chunk->Label();soil.supportCache[key]=chunk;return chunk;
        };
        // Follow real positive-field connections to the floor, including
        // untouched interior chunks. Prefer downward paths, use an explicit
        // stack, and stop as soon as a previously proven anchor is reached.
        constexpr int faces[6][2]={{2,0},{0,0},{0,1},{1,0},{1,1},{2,1}};
        struct Frame{Ref node;int face=0;};
        for(auto const& e:soil.chunks)for(int label=0;label<e.second->components;++label)
        {
            Ref seed={e.first,label};if(solved.count(seed))continue;
            std::set<Ref> visited{seed};std::vector<Frame> stack{{seed,0}};bool anchored=false;
            while(!stack.empty()&&!anchored)
            {
                auto& frame=stack.back();Ref node=frame.node;auto c=chunkAt(node.first);
                if(!soil.finiteChunks&&node.first[2]==0)for(int y=0;y<Side;++y)for(int x=0;x<Side;++x)if(c->labels[Index(x,y,1)]==node.second)anchored=true;
                if(anchored)break;if(frame.face==6){stack.pop_back();continue;}
                int axis=faces[frame.face][0],side=faces[frame.face++][1];Key key=node.first;key[axis]+=side?1:-1;
                std::shared_ptr<Chunk const> other;bool loaded=false;std::set<int> neighbors;
                for(int a=0;a<Side;++a)for(int b=0;b<Side;++b)
                {
                    int p[3]={};p[axis]=side?N:0;p[(axis+1)%3]=a;p[(axis+2)%3]=b;if(c->labels[Index(p[0],p[1],p[2])]!=node.second)continue;
                    if(!loaded){other=chunkAt(key);loaded=true;}if(!other)continue;
                    p[axis]=side?0:N;int r=other->labels[Index(p[0],p[1],p[2])];if(r>=0)neighbors.insert(r);
                }
                for(int r:neighbors)
                {
                    Ref next={key,r};auto known=solved.find(next);if(known!=solved.end()&&known->second<0){anchored=true;break;}
                    if(visited.insert(next).second)stack.push_back({next,0});
                }
            }
            int result=anchored?-1:islandCount++;for(auto const& node:visited)solved[node]=result;
        }
        std::set<int> basePatches;
        for(auto const& e:solved)if(e.second>=0&&!soil.chunks.count(e.first.first))
        {
            // An unedited core enclosed by the excavation is real loose soil,
            // not an anchor. Materialize only cores that actually detached.
            auto c=std::make_shared<Chunk>(*chunkAt(e.first.first));c->volume=c->Volume();soil.chunks[e.first.first]=c;basePatches.insert(Body::Patch(e.first.first));
        }
        std::map<int,std::shared_ptr<Body>> islands;
        for(auto& e:soil.chunks)
        {
            auto const& original=*e.second;std::set<int> detached;
            auto component=[&](int l){auto it=solved.find({e.first,l});return it==solved.end()?-1:it->second;};
            for(int l=0;l<original.components;++l){int r=component(l);if(r>=0)detached.insert(r);}
            if(detached.empty())continue;
            auto retained=std::make_shared<Chunk>(original);
            for(int r:detached)
            {
                auto& island=islands[r];if(!island){island=std::make_shared<Body>();island->finiteChunks=true;}
                auto part=std::make_shared<Chunk>(original);
                for(int i=0;i<int(part->values.size());++i)if(original.labels[i]>=0)
                {if(component(original.labels[i])==r)retained->values[i]=-Band;else part->values[i]=-Band;}
                part->volume=part->Volume();part->Label();island->chunks[e.first]=part;int id=ID(e.first);island->queries[id]=std::make_shared<G::SurfaceQuery>(part->Mesh());island->dirty.insert(id);
            }
            retained->volume=retained->Volume();retained->Label();e.second=retained;int id=ID(e.first);soil.queries[id]=std::make_shared<G::SurfaceQuery>(retained->Mesh());soil.dirty.insert(id);
        }
        for(int id:basePatches){auto q=std::make_shared<G::SurfaceQuery>();q->Build(soil.PatchFaces(id));soil.queries[id]=q;soil.dirty.insert(id);}
        for(auto& e:islands){e.second->PrepareRender();LooseSoil d;d.matter=e.second;RefreshLoose(d);soil.loose.push_back(std::move(d));}
        return islands.size();
    }
    inline bool AdvanceLoose(Body& soil,G::Body const& rock,double dt)
    {
        bool moved=false;dt=std::clamp(dt,0.,.05);int steps=std::max(1,int(std::ceil(dt*120)));double h=dt/steps;
        for(size_t i=0;i<soil.loose.size();++i)
        {
            auto& d=soil.loose[i];d.brushCooldown=std::max(0.,d.brushCooldown-dt);if(d.volume<=1e-12)continue;
            if(d.supportRevision!=soil.revision||d.rockRevision!=rock.revision){d.settled=false;d.supportRevision=soil.revision;d.rockRevision=rock.revision;}
            if(d.settled)continue;
            for(int step=0;step<steps&&!d.settled;++step)
            {
                d.velocity-=9.81*h;double fall=-d.velocity*h,allowed=fall;
                for(P local:d.feet)
                {
                    P p=G::Add(local,d.offset);allowed=std::min(allowed,std::max(0.,p[2]-Bottom-.001));
                    P origin=G::Add(p,{0,0,.0005});double reach=fall+.0015;G::Hit hit;
                    soil.Trace(origin,{0,0,-1},reach,hit,false);
                    for(size_t j=0;j<soil.loose.size();++j)if(i!=j&&soil.loose[j].volume>1e-12)soil.loose[j].matter->Trace(G::Add(origin,G::Mul(soil.loose[j].offset,-1)),{0,0,-1},reach,hit,false);
                    auto stone=G::Raycast(rock,origin,{0,0,-1},reach);if(stone.hit)hit=stone;
                    // Only an upward-facing surface can support a falling
                    // clod.  Treating a cavity roof or side wall as a floor
                    // put detached matter to sleep while visibly airborne.
                    if(hit.hit&&hit.normal[2]>.35)allowed=std::min(allowed,std::max(0.,hit.distance-.0015));
                }
                d.offset[2]-=allowed;moved=moved||allowed>0;
                if(allowed<fall-1e-10){d.velocity=0;d.settled=true;}
            }
        }return moved;
    }
    inline size_t BrushLoose(Body& soil,P from,P to,double radius,double height)
    {
        if(!G::Finite(from)||!G::Finite(to)||radius<=0||height<=radius*2)return 0;
        P a=G::Add(to,{0,0,radius}),b=G::Add(to,{0,0,height-radius}),travel=G::Add(to,G::Mul(from,-1));if(G::Dot(travel,travel)<1e-8)return 0;size_t moved=0;
        for(auto& d:soil.loose)
        {
            if(d.volume<=1e-12||d.brushCooldown>0||!d.matter)continue;
            if(d.lo[0]>d.hi[0])continue;P worldLo=G::Add(d.lo,d.offset),worldHi=G::Add(d.hi,d.offset),center=G::Mul(G::Add(worldLo,worldHi),.5);
            // Large detached islands can have a bounding sphere extending
            // through an intact cave roof.  That woke the island whenever the
            // player walked above it.  First use the exact distance from the
            // vertical capsule axis to its AABB, then confirm against the
            // clod's accelerated surface in local space.
            double boxDistance2=0;
            for(int k=0;k<2;++k){double delta=to[k]<worldLo[k]?worldLo[k]-to[k]:to[k]>worldHi[k]?to[k]-worldHi[k]:0;boxDistance2+=delta*delta;}
            double dz=b[2]<worldLo[2]?worldLo[2]-b[2]:a[2]>worldHi[2]?a[2]-worldHi[2]:0;boxDistance2+=dz*dz;
            if(boxDistance2>radius*radius)continue;
            if(!d.matter->BlocksCapsule(G::Add(to,G::Mul(d.offset,-1)),radius,height,false))continue;
            P away={center[0]-(a[0]+b[0])*.5,center[1]-(a[1]+b[1])*.5,0};
            if(G::Dot(away,away)<1e-8)away={travel[0],travel[1],0};if(G::Dot(away,away)<1e-8)away={1,0,0};away=G::Unit(away);
            double amount=std::clamp(std::hypot(travel[0],travel[1])+.006,.008,.035);
            d.offset=G::Add(d.offset,G::Add(G::Mul(away,amount),P{0,0,.006}));d.velocity=std::max(d.velocity,.12);d.settled=false;d.brushCooldown=.12;++moved;
        }return moved;
    }
    inline Receipt Excavate(Body& soil,G::Body const& rock,G::Hit const& contact,P direction={})
    {
        Receipt receipt;if(!contact.hit||!G::Finite(contact.position)||!G::Finite(direction))return receipt;
        if(contact.id<=-1000)
        {
            size_t index=size_t(-1000-contact.id);if(index>=soil.loose.size())return receipt;
            auto& d=soil.loose[index];auto matter=std::make_shared<Body>(*d.matter);matter->dirty.clear();
            auto local=contact;local.id=-1;local.position=G::Add(local.position,G::Mul(d.offset,-1));
            // Cut only the clod's finite field in its original frame. It cannot
            // invent soil or cut granite, regardless of where the clod landed.
            G::Body empty(false);
            if(d.volume<=ScoopVolume+1e-12)
            {
                double distance=1e30;for(auto const& e:matter->queries)if(!e.second->nodes.empty())Closest(*e.second,0,local.position,distance);
                if(distance>1e-8)return receipt; // An old contact in newly empty space is not a new target.
                // A loose clod that fits in one shovel is collected whole at
                // contact, not shaved into additional nuisance fragments.
                receipt.volume=d.volume;receipt.collectedClod=true;matter=std::make_shared<Body>();matter->finiteChunks=true;
            }
            else receipt=Excavate(*matter,empty,local,direction);
            if(receipt.volume>1e-12)
            {
                ReleaseUnsupported(*matter);P offset=d.offset;double velocity=d.velocity;
                if(!matter->loose.empty())
                {
                    auto pieces=std::move(matter->loose);d.matter=pieces.front().matter;RefreshLoose(d);
                    for(size_t i=1;i<pieces.size();++i){pieces[i].offset=offset;pieces[i].velocity=velocity;soil.loose.push_back(std::move(pieces[i]));}
                }
                else {matter->PrepareRender();d.matter=matter;RefreshLoose(d);}
            }
            if(receipt.volume>1e-12){++soil.revision;++soil.scoops;soil.removedM3+=receipt.volume;uint64_t total=uint64_t(std::llround(soil.removedM3*BulkDensity*1e6));receipt.massMg=total-soil.removedMg;soil.removedMg=total;}
            return receipt;
        }
        P outward=G::Dot(direction,direction)>1e-16?G::Mul(G::Unit(direction),-1):G::Unit(contact.normal);
        if(G::Dot(outward,outward)<.5)return receipt;
        P inward=G::Mul(outward,-1);double depthLimit=Depth;
        auto obstacle=G::Raycast(rock,G::Add(contact.position,G::Mul(inward,.00001)),inward,Depth);
        if(obstacle.hit){depthLimit=std::max(0.,obstacle.distance-.0001);receipt.blocked=true;}
        double radius=(Diameter*Diameter*.25+Depth*Depth)/(2*Depth);P center=G::Add(contact.position,G::Mul(outward,radius-Depth));
        int lower[3],upper[3];P base={S::X,S::Y,Bottom};int counts[3]={CX,CY,CZ};
        for(int k=0;k<3;++k){lower[k]=std::max(0,int(std::floor((center[k]-radius-Band-base[k])/Span)));upper[k]=std::min(counts[k]-1,int(std::floor((center[k]+radius+Band-base[k])/Span)));}
        std::set<int> basePatches;
        for(int z=lower[2];z<=upper[2];++z)for(int y=lower[1];y<=upper[1];++y)for(int x=lower[0];x<=upper[0];++x)
        {
            Key key={x,y,z};auto found=soil.chunks.find(key);bool fresh=found==soil.chunks.end();auto chunk=std::make_shared<Chunk>();chunk->key=key;
            if(fresh&&soil.finiteChunks)continue;
            if(!fresh)*chunk=*found->second;
            else
            {
                for(int c=0;c<Side;++c)for(int b=0;b<Side;++b)for(int a=0;a<Side;++a)chunk->values[Index(a,b,c)]=soil.Initial(Point(key,a,b,c));
                chunk->volume=chunk->Volume();
            }
            double before=chunk->volume;bool changed=false;
            for(int c=0;c<Side;++c)for(int b=0;b<Side;++b)for(int a=0;a<Side;++a)
            {
                P point=Point(key,a,b,c),delta=G::Add(point,G::Mul(center,-1));
                // A spherical scoop overlaps the air in front of contact.
                // Clipping it at an entrance plane left rigid wafer-like teeth
                // between overlapping cuts, especially while digging sideways.
                P relative=G::Add(point,G::Mul(contact.position,-1));
                double outside=std::max(std::sqrt(G::Dot(delta,delta))-radius,G::Dot(inward,relative)-depthLimit);
                double& old=chunk->values[Index(a,b,c)];double next=std::min(old,Sample(outside));
                if(next<old-1e-12){changed=true;old=next;}
            }
            if(!changed)continue;chunk->volume=chunk->Volume();chunk->Label();receipt.volume+=std::max(0.,before-chunk->volume);++receipt.cells;
            soil.chunks[key]=chunk;soil.queries[ID(key)]=std::make_shared<G::SurfaceQuery>(chunk->Mesh());soil.dirty.insert(ID(key));
            if(fresh)basePatches.insert(Body::Patch(key));
        }
        for(int id:basePatches){auto q=std::make_shared<G::SurfaceQuery>();q->Build(soil.PatchFaces(id));soil.queries[id]=q;soil.dirty.insert(id);}
        if(receipt.volume>1e-12&&!soil.finiteChunks){auto supportStart=std::chrono::steady_clock::now();receipt.released=ReleaseUnsupported(soil);receipt.supportMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-supportStart).count();}
        soil.PrepareRender();
        if(receipt.volume>1e-12)
        {
            ++soil.revision;++soil.scoops;soil.removedM3+=receipt.volume;
            uint64_t total=uint64_t(std::llround(soil.removedM3*BulkDensity*1e6));receipt.massMg=total-soil.removedMg;soil.removedMg=total;
        }return receipt;
    }
    struct Transaction {Body terrain;Receipt receipt;double milliseconds=0;uint32_t revision=0;};
    inline Transaction Prepare(Body const& soil,G::Body const& rock,G::Hit contact,P direction={})
    {
        auto start=std::chrono::steady_clock::now();Transaction result;result.revision=soil.revision;result.terrain=soil;result.terrain.dirty.clear();
        result.receipt=Excavate(result.terrain,rock,contact,direction);
        result.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
    }
    inline bool Commit(Body& soil,Transaction&& transaction)
    {if(transaction.revision!=soil.revision)return false;soil=std::move(transaction.terrain);return true;}
}
