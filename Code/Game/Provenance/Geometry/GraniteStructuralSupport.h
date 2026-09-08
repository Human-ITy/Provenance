#pragma once
#include "GraniteContactCast.h"
#include <chrono>
#include <deque>
#include <numeric>
#include <queue>
#include <set>

// Structural support is deterministic worker-only number crunching. The lab is
// normally run from a Debug editor, where compiling this header at /Od made the
// time between otherwise smooth strikes grow beyond a second after many cuts.
// Optimize this isolated hot path without changing the surrounding Debug build
// or any fracture/support/conservation rule.
#if defined(_MSC_VER) && defined(_DEBUG)
#pragma optimize("gt", on)
#endif

namespace EE::GraniteContactCast
{
    // Immutable and indexed (no pointers into a movable Cell). Reused while a
    // cell is untouched; a new cut invalidates only that cell's support cache.
    struct SupportCache
    {
        struct Solid {double volume;P moment;bool anchor,fractured;};
        struct Port {int solid,face;P normal,low,high;};
        struct Bond {int a,b;std::vector<P> polygon;};
        struct Edge {int a,b;double area;P moment,normal;};
        std::vector<Solid> solids;std::vector<Port> ports;std::vector<Bond> bonds;std::vector<Edge> edges;
        std::vector<int> crossCellPorts;
    };
}

// Lab support certificate, not an engineering granite solver. Connectivity is
// exact positive-area contact between retained convex volumes. Strength uses a
// gravity-load network and equivalent circular section (explicit calibration).
namespace EE::GraniteStructuralSupport
{
    namespace G=GraniteContactCast;
    using namespace GraniteOutcrop;
    struct Calibration
    {
        double tensionPa=3e6,shearPa=8e6,compressionPa=80e6;
        // A 4,096-shape sample of the 400 J pick cutter bottoms out at
        // 48.39 cm3. Keep a conservative margin: anything smaller is not a
        // valid pick chip and may be removed as mass-only fracture debris.
        double minimumPickChipM3=45e-6;
        // Ten square centimeters remains below the sampled pick's roughly
        // 44 mm minimum span. Contacts below this are strand-scale.
        double minimumPickContactM2=1e-3;
        double contactAreaTolerance=1e-12;
    };
    struct DSU
    {
        std::vector<int> p,rank;
        explicit DSU(size_t n):p(n),rank(n,0){std::iota(p.begin(),p.end(),0);}
        int Find(int a){while(p[a]!=a){p[a]=p[p[a]];a=p[a];}return a;}
        void Join(int a,int b){a=Find(a);b=Find(b);if(a==b)return;if(rank[a]<rank[b])std::swap(a,b);p[b]=a;if(rank[a]==rank[b])++rank[a];}
    };
    inline std::pair<double,P> Measure(Poly const& poly)
    {
        double v=0;P moment={},ref={};if(!poly.empty()&&!poly[0].p.empty())ref=poly[0].p[0];
        for(auto const& f:poly)for(size_t i=1;i+1<f.p.size();++i){P a=Add(f.p[0],Mul(ref,-1)),b=Add(f.p[i],Mul(ref,-1)),c=Add(f.p[i+1],Mul(ref,-1));double dv=Dot(a,Cross(b,c))/6;v+=dv;moment=Add(moment,Mul(Add(Add(a,b),c),dv*.25));}
        return {v,Add(moment,Mul(ref,v))};
    }
    inline std::pair<double,P> AreaMoment(std::vector<P> const& polygon)
    {
        double area=0;P moment={};for(size_t i=1;i+1<polygon.size();++i){P cross=Cross(Add(polygon[i],Mul(polygon[0],-1)),Add(polygon[i+1],Mul(polygon[0],-1)));double a=.5*std::sqrt(Dot(cross,cross));area+=a;moment=Add(moment,Mul(Add(Add(polygon[0],polygon[i]),polygon[i+1]),a/3));}return {area,moment};
    }
    inline std::vector<Plane> EdgePlanes(std::vector<P> const& p,P n)
    {std::vector<Plane> planes;for(size_t i=0;i<p.size();++i){P out=Unit(Cross(Add(p[(i+1)%p.size()],Mul(p[i],-1)),n));planes.push_back({out,Dot(out,p[i])});}return planes;}
    inline double AnchorElevation(G::Body const& body)
    {
        // Support belongs to the generated body's buried basal plane, not to
        // world Z=0.  Natural outcrops may be translated below the surrounding
        // heightfield; using zero here made the entire V5 body rootless and
        // allowed the first pick strike to release it as one enormous chip.
        return body.substrate.vertices.empty()?0.:body.substrate.low[2]+.001;
    }
    // Coplanar fracture fragments can share one identity but be far apart.
    // Sweep their bounds instead of comparing every pair in that bucket.
    template<class Ports,class Visit>
    inline void OverlappingPairs(std::vector<int> ids,Ports const& ports,Visit visit)
    {
        P low={1e30,1e30,1e30},high={-1e30,-1e30,-1e30};
        for(int id:ids)for(int k=0;k<3;++k){low[k]=std::min(low[k],ports[id].low[k]);high[k]=std::max(high[k],ports[id].high[k]);}
        int axis=0;for(int k=1;k<3;++k)if(high[k]-low[k]>high[axis]-low[axis])axis=k;
        std::sort(ids.begin(),ids.end(),[&](int a,int b){return ports[a].low[axis]==ports[b].low[axis]?a<b:ports[a].low[axis]<ports[b].low[axis];});
        for(size_t i=0;i<ids.size();++i)for(size_t j=i+1;j<ids.size();++j)
        {
            auto const& a=ports[ids[i]];auto const& b=ports[ids[j]];if(b.low[axis]>a.high[axis]+1e-10)break;
            bool overlap=true;for(int k=0;k<3;++k)if(a.low[k]>b.high[k]+1e-10||b.low[k]>a.high[k]+1e-10)overlap=false;
            if(overlap)visit(std::min(ids[i],ids[j]),std::max(ids[i],ids[j]));
        }
    }
    inline std::shared_ptr<G::SupportCache const> Cache(G::Cell const& cell,G::Body const& body,Calibration const& config={})
    {
        auto cache=std::make_shared<G::SupportCache>();std::unordered_map<int,std::vector<int>> buckets;int si=0;size_t originalCount=body.substrate.pieces.size();double anchorZ=AnchorElevation(body);
        for(auto const& poly:cell.solids)
        {
            auto measure=Measure(poly);cache->solids.push_back({measure.first,measure.second,false,false});int fi=0;
            for(auto const& face:poly)
            {
                int index=fi++;if(face.p.size()<3||face.sign<=0||AreaMoment(face.p).first<=config.contactAreaTolerance)continue;
                G::SupportCache::Port port{si,index,G::Normal(face),{1e30,1e30,1e30},{-1e30,-1e30,-1e30}};bool bottom=true;
                for(P p:face.p){bottom=bottom&&std::abs(p[2]-anchorZ)<1e-8;for(int k=0;k<3;++k){port.low[k]=std::min(port.low[k],p[k]);port.high[k]=std::max(port.high[k],p[k]);}}
                if(bottom)cache->solids.back().anchor=true;if(size_t(face.sign)>originalCount*16)cache->solids.back().fractured=true;
                int portID=int(cache->ports.size());
                if(size_t(face.sign)<=originalCount*16)
                {
                    int owner=(face.sign-1)/16,facetIndex=(face.sign-1)%16;auto const& piece=body.substrate.pieces[owner];
                    if(facetIndex<int(piece.faces.size())&&piece.faces[facetIndex].neighbor>=0&&piece.faces[facetIndex].neighbor!=owner)cache->crossCellPorts.push_back(portID);
                }
                else
                {
                    // One cutter plane can cross several neighboring original
                    // parcels. Its fragments retain the same fracture token,
                    // so they are also legitimate cross-cell support ports.
                    // The graph's same-cell guard still excludes internal
                    // fragment pairs when this compact cache is expanded.
                    cache->crossCellPorts.push_back(portID);
                }
                buckets[face.sign].push_back(portID);cache->ports.push_back(port);
            }++si;
        }
        for(auto const& entry:buckets)OverlappingPairs(entry.second,cache->ports,[&](int a,int b)
        {
            auto const& pa=cache->ports[a];auto const& pb=cache->ports[b];if(pa.solid==pb.solid||Dot(pa.normal,pb.normal)>-.999999)return;
            auto polygon=cell.solids[pa.solid][pa.face].p;auto const& other=cell.solids[pb.solid][pb.face];for(auto plane:EdgePlanes(other.p,pb.normal)){polygon=ClipPolygon(polygon,plane);if(polygon.size()<3)break;}
            if(AreaMoment(polygon).first>config.contactAreaTolerance)cache->bonds.push_back({a,b,std::move(polygon)});
        });
        std::map<std::pair<int,int>,int> edges;
        for(auto const& bond:cache->bonds){auto const& a=cache->ports[bond.a];auto const& b=cache->ports[bond.b];int na=std::min(a.solid,b.solid),nb=std::max(a.solid,b.solid);auto inserted=edges.emplace(std::make_pair(na,nb),int(cache->edges.size()));if(inserted.second)cache->edges.push_back({na,nb,0,{},{}});auto& edge=cache->edges[inserted.first->second];auto measure=AreaMoment(bond.polygon);edge.area+=measure.first;edge.moment=Add(edge.moment,measure.second);edge.normal=Add(edge.normal,Mul(a.normal,measure.first*(a.solid==na?1:-1)));}
        return cache;
    }
    struct Node {double volume=0;P moment={};bool anchor=false;int cell=-1,solid=-1;bool fractured=false;};
    struct Edge {int a=0,b=0;double area=0;P moment={},normal={};bool failed=false;};
    struct Port {int node=0,cell=0;Face const* face=nullptr;P n={},low={},high={};std::vector<int> portals;};
    struct Portal {int a=0,b=0;std::vector<P> polygon;};
    struct Graph
    {
        std::vector<Node> nodes;std::vector<Edge> edges;std::vector<Port> ports;std::vector<Portal> portals;
        std::vector<int> originalNode;std::deque<Face> originalFaces;
        std::map<int,G::Cell const*> cells;
        std::map<int,std::vector<int>> cellNodes;
    };
    inline Graph Build(G::Body const& body,G::Transaction const& tx,Calibration const& config={},bool fullBoundary=true)
    {
        Graph graph;for(auto const& entry:body.changed)graph.cells[entry.first]=&entry.second;for(auto const& entry:tx.updates)graph.cells[entry.first]=&entry.second;
        auto const& base=body.substrate;size_t count=base.pieces.size();DSU dsu(count);std::vector<bool> changed(count,false);double anchorZ=AnchorElevation(body);
        for(auto const& entry:graph.cells)changed[entry.first]=true;
        // Each shared face is present on both pieces. Joining from the lower
        // piece ID retains identical connectivity without repeating the same
        // union in reverse during every strike.
        for(int id=0;id<int(count);++id)if(!changed[id])for(auto const& f:base.pieces[id].faces)if(f.neighbor>id&&!changed[f.neighbor])dsu.Join(id,f.neighbor);
        graph.originalNode.assign(count,-1);std::vector<int> rootNode(count,-1);
        for(int id=0;id<int(count);++id)if(!changed[id])
        {
            int root=dsu.Find(id);if(rootNode[root]<0){rootNode[root]=int(graph.nodes.size());graph.nodes.emplace_back();}int n=graph.originalNode[id]=rootNode[root];auto& node=graph.nodes[n];
            for(auto const& tet:base.pieces[id].tets){P a=base.vertices[tet[0]],b=base.vertices[tet[1]],c=base.vertices[tet[2]],d=base.vertices[tet[3]];double v=std::abs(Dot(Add(b,Mul(a,-1)),Cross(Add(c,Mul(a,-1)),Add(d,Mul(a,-1)))))/6;node.volume+=v;node.moment=Add(node.moment,Mul(Add(Add(a,b),Add(c,d)),v*.25));}
            for(auto const& f:base.pieces[id].faces)if(f.neighbor<0){bool bottom=true;for(int v:f.v)bottom=bottom&&std::abs(base.vertices[v][2]-anchorZ)<1e-8;if(bottom)node.anchor=true;}
        }
        std::unordered_map<int,std::vector<int>> buckets;
        auto addPort=[&](int node,int cell,Face const& face)
        {
            if(face.p.size()<3||face.sign<=0)return;auto area=AreaMoment(face.p);if(area.first<=config.contactAreaTolerance)return;
            Port port;port.node=node;port.cell=cell;port.face=&face;port.n=G::Normal(face);port.low={1e30,1e30,1e30};port.high={-1e30,-1e30,-1e30};
            for(P p:face.p)for(int k=0;k<3;++k){port.low[k]=std::min(port.low[k],p[k]);port.high[k]=std::max(port.high[k],p[k]);}
            buckets[face.sign].push_back(int(graph.ports.size()));graph.ports.push_back(std::move(port));
        };
        std::vector<std::pair<int,G::SupportCache const*>> cachedBonds;std::vector<std::shared_ptr<G::SupportCache const>> temporaryCaches;
        for(auto const& entry:graph.cells)
        {
            auto cache=entry.second->support;if(!cache){cache=Cache(*entry.second,body,config);temporaryCaches.push_back(cache);}int firstNode=int(graph.nodes.size()),firstPort=int(graph.ports.size());
            int index=0;auto& cellNodes=graph.cellNodes[entry.first];cellNodes.reserve(cache->solids.size());for(auto const& solid:cache->solids){int n=int(graph.nodes.size());cellNodes.push_back(n);graph.nodes.push_back({solid.volume,solid.moment,solid.anchor,entry.first,index++,solid.fractured});}
            auto addCachedPort=[&](G::SupportCache::Port const& p){auto const& face=entry.second->solids[p.solid][p.face];buckets[face.sign].push_back(int(graph.ports.size()));graph.ports.push_back({firstNode+p.solid,entry.first,&face,p.normal,p.low,p.high,{}});};
            if(fullBoundary)for(auto const& p:cache->ports)addCachedPort(p);else for(int p:cache->crossCellPorts)addCachedPort(cache->ports[p]);
            if(fullBoundary)cachedBonds.emplace_back(firstPort,cache.get());
            else for(auto const& edge:cache->edges)graph.edges.push_back({firstNode+edge.a,firstNode+edge.b,edge.area,edge.moment,edge.normal,false});
        }
        // Only interfaces next to changed cells need expansion. The uncut
        // interior is compacted into connected regions, never a voxel proxy.
        for(int id=0;id<int(count);++id)if(!changed[id])
        {
            int fi=0;for(auto const& f:base.pieces[id].faces)
            {
                if(f.neighbor>=0&&changed[f.neighbor]){graph.originalFaces.push_back({{base.vertices[f.v[0]],base.vertices[f.v[1]],base.vertices[f.v[2]]},G::Hidden,G::SupportToken(body,id,fi)});addPort(graph.originalNode[id],id,graph.originalFaces.back());}++fi;
            }
        }
        std::unordered_map<uint64_t,int> edgeIDs;
        auto bond=[&](int a,int b,std::vector<P> polygon)
        {
            auto& pa=graph.ports[a];auto& pb=graph.ports[b];auto area=AreaMoment(polygon);int portal=int(graph.portals.size());pa.portals.push_back(portal);pb.portals.push_back(portal);graph.portals.push_back({a,b,std::move(polygon)});
            int na=std::min(pa.node,pb.node),nb=std::max(pa.node,pb.node);uint64_t key=(uint64_t(na)<<32)|uint32_t(nb);auto inserted=edgeIDs.emplace(key,int(graph.edges.size()));if(inserted.second)graph.edges.push_back({na,nb});
            auto& edge=graph.edges[inserted.first->second];edge.area+=area.first;edge.moment=Add(edge.moment,area.second);edge.normal=Add(edge.normal,Mul(pa.n,area.first*(pa.node==na?1:-1)));
        };
        for(auto const& cache:cachedBonds)for(auto const& b:cache.second->bonds)bond(cache.first+b.a,cache.first+b.b,b.polygon);
        for(auto const& bucket:buckets)
        {
            OverlappingPairs(bucket.second,graph.ports,[&](int a,int b)
            {
                auto& pa=graph.ports[a];auto& pb=graph.ports[b];if(pa.cell==pb.cell||pa.node==pb.node||Dot(pa.n,pb.n)>-.999999)return;
                auto polygon=pa.face->p;for(auto plane:EdgePlanes(pb.face->p,pb.n)){polygon=ClipPolygon(polygon,plane);if(polygon.size()<3)break;}
                auto area=AreaMoment(polygon);if(area.first<=config.contactAreaTolerance)return;
                bond(a,b,std::move(polygon));
            });
        }
        return graph;
    }
    // Demand/capacity ratio. A short thin neck fails under bending even if its
    // area could carry the same weight in pure compression. No body health bar.
    inline double Utilization(double area,P normal,P contact,P center,double massKg,Calibration const& c={})
    {
        if(area<=c.contactAreaTolerance)return 1e30;P force={0,0,-massKg*9.81};normal=Unit(normal);double axial=Dot(force,normal);P shear=Add(force,Mul(normal,-axial));P torque=Cross(Add(center,Mul(contact,-1)),force);
        P bending=Add(torque,Mul(normal,-Dot(torque,normal)));double section=area*std::sqrt(area)/(4*std::sqrt(3.141592653589793));
        return std::abs(axial)/(area*(axial>0?c.compressionPa:c.tensionPa))+std::sqrt(Dot(shear,shear))/(area*c.shearPa)+std::sqrt(Dot(bending,bending))/(section*c.tensionPa);
    }
    inline std::vector<int> Solve(Graph& graph,Calibration const& config={})
    {
        // Flat adjacency avoids one allocation per solid node on every strike.
        size_t n=graph.nodes.size();std::vector<size_t> offsets(n+1,0);
        for(auto const& e:graph.edges){++offsets[e.a+1];++offsets[e.b+1];}
        for(size_t i=1;i<=n;++i)offsets[i]+=offsets[i-1];
        auto cursor=offsets;std::vector<int> edgeIDs(graph.edges.size()*2+1);
        for(int i=0;i<int(graph.edges.size());++i){auto const& e=graph.edges[i];edgeIDs[cursor[e.a]++]=i;edgeIDs[cursor[e.b]++]=i;}
        struct Range {int const* first;int const* last;int const* begin()const{return first;}int const* end()const{return last;}};
        auto adjacency=[&](int a){return Range{edgeIDs.data()+offsets[a],edgeIDs.data()+offsets[a+1]};};
        std::vector<int> distance(n),order;order.reserve(n);std::vector<double> mass(n);std::vector<P> moment(n);
        // Gravity is shared over all rootward interfaces, weighted by area.
        // Failure redistributes load. This is a bounded lab network model,
        // not an elastic continuum or a calibrated geological fracture law.
        for(int iteration=0;iteration<16;++iteration)
        {
            std::fill(distance.begin(),distance.end(),-1);order.clear();for(int i=0;i<int(n);++i)if(graph.nodes[i].anchor){distance[i]=0;order.push_back(i);}
            for(size_t next=0;next<order.size();++next){int a=order[next];for(int id:adjacency(a)){auto const& e=graph.edges[id];int b=e.a==a?e.b:e.a;if(!e.failed&&distance[b]<0){distance[b]=distance[a]+1;order.push_back(b);}}}
            for(int i:order){mass[i]=graph.nodes[i].volume*Density;moment[i]=Mul(graph.nodes[i].moment,Density);}
            bool failed=false;
            for(auto it=order.rbegin();it!=order.rend();++it)
            {
                int a=*it;if(graph.nodes[a].anchor)continue;double area=0;for(int id:adjacency(a)){auto const& e=graph.edges[id];int b=e.a==a?e.b:e.a;if(!e.failed&&distance[b]>=0&&distance[b]<distance[a])area+=e.area;}
                if(area<=0)continue;
                for(int id:adjacency(a)){auto& e=graph.edges[id];int b=e.a==a?e.b:e.a;if(e.failed||distance[b]<0||distance[b]>=distance[a])continue;double fraction=e.area/area;P center=mass[a]>0?Mul(moment[a],1/mass[a]):P{};
                    if(Utilization(e.area,Mul(e.normal,a==e.a?1:-1),Mul(e.moment,1/e.area),center,mass[a]*fraction,config)>1){e.failed=true;failed=true;}
                    else{mass[b]+=mass[a]*fraction;moment[b]=Add(moment[b],Mul(moment[a],fraction));}}
            }
            if(!failed)break;
        }
        DSU groups(n);for(auto const& e:graph.edges)if(!e.failed)groups.Join(e.a,e.b);std::vector<bool> rooted(n,false);for(int i=0;i<int(n);++i)if(graph.nodes[i].anchor)rooted[groups.Find(i)]=true;
        std::vector<int> owner(n);for(int i=0;i<int(n);++i)owner[i]=rooted[groups.Find(i)]?0:groups.Find(i)+1;return owner;
    }
    // Collapse full-area contacts first, leaving a graph of strand-scale
    // contacts. Peeling only sub-pick leaf branches removes wisps and floaters
    // while preserving every path that connects two full-sized granite bodies.
    inline size_t AssignSubPickRemnants(Graph const& graph,std::vector<int>& owners,Calibration const& config={})
    {
        size_t n=graph.nodes.size();DSU blocks(n);for(auto const& edge:graph.edges)if(!edge.failed&&!owners[edge.a]&&!owners[edge.b]&&edge.area>=config.minimumPickContactM2)blocks.Join(edge.a,edge.b);
        std::map<int,int> blockIDs;std::vector<int> nodeBlock(n,-1);for(int i=0;i<int(n);++i)if(!owners[i]){auto inserted=blockIDs.emplace(blocks.Find(i),int(blockIDs.size()));nodeBlock[i]=inserted.first->second;}
        size_t blockCount=blockIDs.size();std::vector<double> volume(blockCount,0);std::vector<bool> anchored(blockCount,false),fractured(blockCount,false);std::vector<std::vector<int>> adjacency(blockCount);
        for(size_t i=0;i<n;++i)if(nodeBlock[i]>=0){int block=nodeBlock[i];volume[block]+=graph.nodes[i].volume;anchored[block]=anchored[block]||graph.nodes[i].anchor;fractured[block]=fractured[block]||graph.nodes[i].fractured;}
        std::set<std::pair<int,int>> links;for(auto const& edge:graph.edges)if(!edge.failed&&!owners[edge.a]&&!owners[edge.b]){int a=nodeBlock[edge.a],b=nodeBlock[edge.b];if(a!=b)links.emplace(std::min(a,b),std::max(a,b));}
        for(auto const& link:links){adjacency[link.first].push_back(link.second);adjacency[link.second].push_back(link.first);}
        std::vector<int> degree(blockCount);std::queue<int> leaves;for(size_t i=0;i<blockCount;++i){degree[i]=int(adjacency[i].size());if(!anchored[i]&&fractured[i]&&volume[i]<config.minimumPickChipM3&&degree[i]<=1)leaves.push(int(i));}
        DSU remnantBlocks(blockCount);std::vector<double> remnantVolume=volume;std::vector<bool> cull(blockCount,false);size_t culledBlocks=0;
        while(!leaves.empty())
        {
            int block=leaves.front();leaves.pop();if(cull[block]||anchored[block]||!fractured[block]||degree[block]>1)continue;double total=volume[block];std::vector<int> roots;
            for(int other:adjacency[block])if(cull[other]){int root=remnantBlocks.Find(other);if(std::find(roots.begin(),roots.end(),root)==roots.end()){roots.push_back(root);total+=remnantVolume[root];}}
            if(total>=config.minimumPickChipM3)continue;cull[block]=true;remnantVolume[block]=volume[block];++culledBlocks;
            for(int root:roots){double a=remnantVolume[remnantBlocks.Find(block)],b=remnantVolume[remnantBlocks.Find(root)];remnantBlocks.Join(block,root);remnantVolume[remnantBlocks.Find(block)]=a+b;}
            for(int other:adjacency[block])if(!cull[other]){--degree[other];if(!anchored[other]&&fractured[other]&&volume[other]<config.minimumPickChipM3&&degree[other]<=1)leaves.push(other);}
        }
        if(!culledBlocks)return 0;std::vector<bool> remove(n,false);size_t removedCount=0;for(size_t i=0;i<n;++i)if(nodeBlock[i]>=0&&cull[nodeBlock[i]]){remove[i]=true;++removedCount;}
        if(!removedCount)return 0;
        DSU remnants(n);for(auto const& edge:graph.edges)if(!edge.failed&&remove[edge.a]&&remove[edge.b])remnants.Join(edge.a,edge.b);
        int nextOwner=1;for(int owner:owners)nextOwner=std::max(nextOwner,owner+1);std::map<int,int> labels;
        for(size_t i=0;i<n;++i)if(remove[i]){int root=remnants.Find(int(i));auto label=labels.emplace(root,nextOwner);if(label.second)++nextOwner;owners[i]=label.first->second;}
        return removedCount;
    }
    inline Face VisibleFace(G::Body const& body,Face face)
    {
        face.axis=G::Fresh;if(face.sign>0&&size_t((face.sign-1)/16)<body.substrate.pieces.size())
        {auto const& piece=body.substrate.pieces[(face.sign-1)/16];int index=(face.sign-1)%16;if(index<int(piece.faces.size())&&piece.faces[index].neighbor<0)face.axis=G::Weathered;}
        return face;
    }
    inline Poly PortBoundary(G::Body const& body,Graph const& graph,int id,std::vector<int> const& owner)
    {
        auto const& port=graph.ports[id];Poly surface={VisibleFace(body,*port.face)};
        for(int p:port.portals){auto const& portal=graph.portals[p];int other=portal.a==id?portal.b:portal.a;if(owner[port.node]!=owner[graph.ports[other].node])continue;
            P normal=G::Normal(Face{portal.polygon});surface=G::ClipSurface(surface,EdgePlanes(portal.polygon,normal),false);if(surface.empty())break;}
        return surface;
    }
    // The compact solve already computed every cross-cell portal. A release
    // needs internal boundary ports/bonds too, but not a second connectivity
    // collapse, mass measurement or cross-cell polygon intersection pass.
    // Restore the same port/portal order as Build(..., true). Edges retain the
    // solved compact representation; this expanded graph is for boundary use.
    inline void CompleteBoundary(G::Body const& body,Graph& graph,Calibration const& config={})
    {
        std::vector<Port> ports;
        std::vector<Portal> portals;
        std::unordered_map<Face const*,int> portIDs;
        for(auto const& entry:graph.cells)
        {
            auto cache=entry.second->support;if(!cache)cache=Cache(*entry.second,body,config);
            int firstPort=int(ports.size());auto const& nodes=graph.cellNodes.at(entry.first);
            for(auto const& p:cache->ports)
            {
                auto const* face=&entry.second->solids[p.solid][p.face];
                portIDs.emplace(face,int(ports.size()));
                ports.push_back({nodes[p.solid],entry.first,face,p.normal,p.low,p.high,{}});
            }
            for(auto const& bond:cache->bonds)portals.push_back({firstPort+bond.a,firstPort+bond.b,bond.polygon});
        }
        // Unchanged-side original ports follow changed-cell ports in Build.
        for(auto const& port:graph.ports)if(!graph.cells.count(port.cell))
        {
            portIDs.emplace(port.face,int(ports.size()));ports.push_back(port);ports.back().portals.clear();
        }
        std::unordered_map<uint64_t,std::vector<P>> external;
        for(auto& portal:graph.portals)
        {
            int a=portIDs.at(graph.ports[portal.a].face),b=portIDs.at(graph.ports[portal.b].face);
            external.emplace((uint64_t(a)<<32)|uint32_t(b),std::move(portal.polygon));
        }
        // Full ports introduce extra identity buckets. Reproduce their visit
        // order too: unordered bucket order can otherwise change per-face
        // clipping order, despite representing the same contact areas.
        std::unordered_map<int,std::vector<int>> buckets;
        for(int i=0;i<int(ports.size());++i)buckets[ports[i].face->sign].push_back(i);
        for(auto const& bucket:buckets)OverlappingPairs(bucket.second,ports,[&](int a,int b)
        {
            auto found=external.find((uint64_t(a)<<32)|uint32_t(b));
            if(found!=external.end())portals.push_back({a,b,std::move(found->second)});
        });
        for(int i=0;i<int(portals.size());++i){ports[portals[i].a].portals.push_back(i);ports[portals[i].b].portals.push_back(i);}
        graph.ports=std::move(ports);graph.portals=std::move(portals);
    }
    // Reuse remains opt-in while the extended repeat-cut certificate is under
    // investigation. Keep ordinary callers on the existing production path.
    inline void Apply(G::Body const& body,G::Transaction& tx,Calibration const& config={},bool referenceBoundaryRebuild=true)
    {
        if(!tx.receipt.removed)return;auto start=std::chrono::steady_clock::now();auto& receipt=tx.receipt;
        for(auto& entry:tx.updates)if(!entry.second.support)entry.second.support=Cache(entry.second,body,config);
        auto cacheEnd=std::chrono::steady_clock::now();receipt.supportCacheMs=std::chrono::duration<double,std::milli>(cacheEnd-start).count();
        auto graph=Build(body,tx,config,false);auto graphEnd=std::chrono::steady_clock::now();receipt.supportGraphMs=std::chrono::duration<double,std::milli>(graphEnd-cacheEnd).count();
        auto owners=Solve(graph,config);AssignSubPickRemnants(graph,owners,config);receipt.supportSolveMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-graphEnd).count();receipt.supportNodes=graph.nodes.size();
        // The common path needs only cached bond sums and cross-cell ports.
        // Expand internal boundary polygons only when a body actually releases.
        if(std::any_of(owners.begin(),owners.end(),[](int owner){return owner!=0;}))
        {
            for(auto const& e:graph.edges)if(e.failed&&owners[e.a]!=owners[e.b])++receipt.failedLinks;
            if(referenceBoundaryRebuild)graph=Build(body,tx,config,true);
            else CompleteBoundary(body,graph,config);
        }
        std::map<int,G::Chip> releases;std::map<int,P> moments;std::map<int,G::Cell> replacements;
        for(size_t n=0;n<owners.size();++n)if(owners[n]){auto& chip=releases[owners[n]];chip.volume+=graph.nodes[n].volume;moments[owners[n]]=Add(moments[owners[n]],graph.nodes[n].moment);}
        if(releases.empty()){receipt.supportMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return;}
        // Rebuild only cells whose ownership changed or whose shared interface
        // was exposed by a release. Unchanged surfaces remain byte-for-byte.
        for(auto const& entry:graph.cellNodes)for(int n:entry.second)if(owners[n]){replacements[entry.first];break;}
        for(auto const& portal:graph.portals){auto const& a=graph.ports[portal.a];auto const& b=graph.ports[portal.b];if(owners[a.node]!=owners[b.node]){replacements[a.cell];replacements[b.cell];}}
        for(int id=0;id<int(graph.originalNode.size());++id){int n=graph.originalNode[id];if(n>=0&&owners[n]){replacements[id];auto exterior=G::OriginalSurface(body,id);auto& geometry=releases[owners[n]].geometry;geometry.insert(geometry.end(),exterior.begin(),exterior.end());}}
        for(auto& entry:replacements)
        {
            auto found=graph.cells.find(entry.first);
            if(found!=graph.cells.end()){auto nodes=graph.cellNodes.find(entry.first);if(nodes!=graph.cellNodes.end())for(int n:nodes->second)if(!owners[n])entry.second.solids.push_back(found->second->solids[graph.nodes[n].solid]);}
            else if(!owners[graph.originalNode[entry.first]])entry.second=G::OriginalCell(body,entry.first);
        }
        for(int p=0;p<int(graph.ports.size());++p)
        {
            auto const& port=graph.ports[p];int owner=owners[port.node];if(!owner&&!replacements.count(port.cell))continue;
            auto surface=PortBoundary(body,graph,p,owners);auto& target=owner?releases[owner].geometry:replacements[port.cell].surface;target.insert(target.end(),surface.begin(),surface.end());
        }
        // Validate closed bodies before altering the transaction. Credits are
        // explicit non-rendered fines; their volume is not disguised as mesh.
        bool valid=true;for(auto& entry:releases){auto& chip=entry.second;chip.center=Mul(moments[entry.first],1/chip.volume);double measured=Volume(chip.geometry);if(std::abs(measured-chip.volume)>std::max(1e-10,chip.volume*2e-5))valid=false;}
        if(!valid){receipt.failedLinks=0;receipt.status="CHIP RELEASED; support release REFUSED: boundary mismatch";receipt.supportMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return;}
        std::vector<G::Chip> fines;
        for(auto& entry:releases){auto& chip=entry.second;
            // Volume is the relevant validity test. A long, hair-thin strand
            // is still below the pick's minimum coherent chip geometry.
            if(chip.volume<config.minimumPickChipM3)fines.push_back(std::move(chip));
            else{chip.mesh=G::Triangulate(chip.geometry,&body);receipt.detached.push_back(std::move(chip));}}
        // The user's last valid struck chip owns only the remnant's accounting;
        // its mesh and collision geometry deliberately remain unchanged.
        for(auto const& fine:fines){receipt.chip.volume+=fine.volume;receipt.chip.absorbedM3+=fine.volume;receipt.absorbedM3+=fine.volume;++receipt.culledRemnants;}
        for(auto& entry:replacements){entry.second.query.Build(entry.second.surface);entry.second.support=Cache(entry.second,body,config);auto it=std::find_if(tx.updates.begin(),tx.updates.end(),[&](auto const& update){return update.first==entry.first;});if(it==tx.updates.end())tx.updates.emplace_back(entry.first,std::move(entry.second));else it->second=std::move(entry.second);}
        double volume=body.removedM3;uint64_t mass=body.removedMg;int id=int(body.nextChipID);auto account=[&](G::Chip& chip){chip.id=id++;volume+=chip.volume;uint64_t total=uint64_t(std::llround(volume*Density*1e6));chip.massMg=total-mass;mass=total;};account(receipt.chip);for(auto& chip:receipt.detached)account(chip);
        tx.renderUpdates.clear();for(auto const& entry:tx.updates)tx.renderUpdates.emplace(G::RenderPatchID(body.substrate.pieces[entry.first]),std::vector<G::Triangle>{});for(auto& patch:tx.renderUpdates)patch.second=G::PatchBoundary(body,patch.first,tx.updates);
        receipt.status="CHIP + SUPPORT RELEASE: sub-pick remnants mass-credited";receipt.supportMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    }
    struct Storage {size_t bytes=0,solids=0,faces=0,vertices=0;};
    inline Storage MeasureStorage(G::Body const& body,G::Transaction const* tx=nullptr)
    {
        Storage s;auto poly=[&](Poly const& p){s.bytes+=p.capacity()*sizeof(Face);s.faces+=p.size();for(auto const& f:p){s.bytes+=f.p.capacity()*sizeof(P);s.vertices+=f.p.size();}};
        std::map<int,G::Cell const*> cells;for(auto const& entry:body.changed)cells[entry.first]=&entry.second;if(tx)for(auto const& entry:tx->updates)cells[entry.first]=&entry.second;
        for(auto const& entry:cells){auto const& cell=*entry.second;s.bytes+=sizeof(G::Cell)+cell.solids.capacity()*sizeof(Poly);s.solids+=cell.solids.size();for(auto const& solid:cell.solids)poly(solid);poly(cell.surface);s.bytes+=cell.query.triangles.capacity()*sizeof(G::SurfaceQuery::Tri)+cell.query.nodes.capacity()*sizeof(G::SurfaceQuery::Node);if(cell.support){auto const& c=*cell.support;s.bytes+=sizeof(c)+c.solids.capacity()*sizeof(G::SupportCache::Solid)+c.ports.capacity()*sizeof(G::SupportCache::Port)+c.bonds.capacity()*sizeof(G::SupportCache::Bond)+c.edges.capacity()*sizeof(G::SupportCache::Edge)+c.crossCellPorts.capacity()*sizeof(int);for(auto const& b:c.bonds)s.bytes+=b.polygon.capacity()*sizeof(P);}}return s;
    }
    inline G::Transaction PrepareStrike(G::Body const& body,P origin,P direction,double energy=400,uint32_t grade=2,bool referenceBoundaryRebuild=true)
    {auto start=std::chrono::steady_clock::now();auto tx=G::PrepareStrike(body,origin,direction,energy,grade);Apply(body,tx,{},referenceBoundaryRebuild);tx.receipt.modifiedBytes=MeasureStorage(body,tx.receipt.removed?&tx:nullptr).bytes;tx.receipt.prepareMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return tx;}
}


#if defined(_MSC_VER) && defined(_DEBUG)
#pragma optimize("", off)
#endif
