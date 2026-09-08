#include "GraniteOutcropLab.h"

#if EE_DEVELOPMENT_TOOLS


#include "WorldSystem_Provenance.h"
#include "Game/Provenance/Components/Component_ProvenanceWorldSettings.h"
#include "Game/Provenance/Geometry/GraniteOutcropGround.h"
#include "Game/Provenance/Geometry/GraniteContactDebris.h"
#include "Game/Provenance/Geometry/GraniteMaterialAnchor.h"
#include "Game/Provenance/Geometry/GraniteStructuralSupport.h"
#include "Game/Provenance/Geometry/GraniteLabMovement.h"
#include "Game/Provenance/Geometry/GraniteLabContact.h"
#include "Game/Provenance/Geometry/GrassCover.h"
#include "Game/Provenance/Geometry/PlayableLandscape.h"
#include "Game/Provenance/Geometry/TerrainSeam.h"
#include "Game/Provenance/Geometry/LandscapeGrass.h"
#include "Game/Provenance/Geometry/GraniteLabPlacement.h"
#include "GraniteLabAuthoring.h"
#include "GrassShadowBenchmark.h"
#include "GraniteLabToolPickup.h"
#include "MovementFrameCapture.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"
#include "Base/Input/InputSystem.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/FileSystem/FileSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <future>
#include <map>
#include <utility>
#include <vector>

// Play-only adapter; the same GraniteOutcrop matter owns rendering, strike
// intersection and the conservative, lab-local walking collision constraint.
namespace EE::OutcropLab
{
    namespace G=GraniteContactCast;
    namespace Ground=GraniteOutcropGround;
    namespace Motion=GraniteContactDebris;
    static constexpr float X=-1.4f,Y=35.f,Z=.12f;
    using Debris=Motion::FallingChip;
    struct State
    {
        ProvenanceWorldSystem* owner=nullptr;
        Render::RenderWorldSystem* renderer=nullptr;
        SoilScoop::Body terrain;SoilScoop::Receipt scoop;
        GrassCover::Model grass;Render::Material const* grassMaterial=nullptr;
        double grassMs=0,grassUploadClock=0,grassBladeMs=0,grassWindSeconds=0;
        std::map<int,Render::ProceduralMeshID> grassBladePatches;
        std::map<int,size_t> grassBladeCounts;
        size_t grassBladeClusters=0;
        struct MeadowPatch {Render::ProceduralMeshID mesh=0;size_t tufts=0;bool shadows=false;};
        std::map<std::pair<int,int>,MeadowPatch> meadowPatches;
        size_t meadowTufts=0;double meadowPublishMs=0;bool meadowReady=false;
        bool grassShadows=false,shadowKey=false,benchmarkKey=false;
        GrassShadowBenchmark shadowBenchmark;
        MovementFrameCapture movementCapture;
        bool movementCaptureKey=false;
        std::map<int,Render::ProceduralMeshID> soilPatches;
        std::vector<Render::ProceduralMeshID> landscapeTiles;
        int landscapeNext=0;size_t landscapeTriangles=0;
        double landscapePublishMs=0,landscapeSetupMs=0;
        bool creekViewKey=false,creekView=false;
        GraniteLabMovement::Body creekReturn;
        Float3 creekReturnDirection=Float3::Zero;
        std::vector<Render::ProceduralMeshID> soilClodMeshes;
        std::vector<uint32_t> soilClodVersions;std::vector<G::P> soilClodOffsets;
        std::map<int,Render::ProceduralMeshID> rockPatches;
        std::map<int,std::vector<G::Triangle>> pendingPatches;
        std::vector<Render::ProceduralMeshID> chipMeshes;
        std::vector<bool> chipPoseDirty;
        std::vector<Render::ProceduralMeshID> settledChipMeshes;
        std::vector<size_t> settledChipTriangleCounts;
        std::vector<bool> chipBatchSettled;
        size_t settledChipTriangles=0;
        double chipBatchQuiet=0;
        G::Body body{false}; G::Receipt receipt; std::vector<Debris> debris;
        std::future<G::Transaction> fracture;
        std::future<SoilScoop::Transaction> shovel;
        bool queuedAction=false;G::P queuedOrigin={},queuedDirection={};
        G::Hit capturedContact;
        bool active=true,h=false,f=false,r=false,v=false,inspect=false,walkStarted=false,dirty=true;
        uint32_t attempts=0,contacts=0;
        double scoopMs=0,soilUploadMs=0;size_t soilUploadTriangles=0,soilUploadVertices=0,soilUpdateCount=0;
        double soilMotionMs=0;
        double actionPeakMs=0,movingFramePeakMs=0;int actionTailFrames=0;
        GraniteLabMovement::Body walker{{G::Width*.5,-1.8,Ground::Height(G::Width*.5,-1.8)+GraniteLabMovement::Skin}};
        double meshMs=0,aimMs=0,walkMs=0,physicsMs=0,poseMs=0,brushMs=0;
        double walkPeakMs=0,physicsPeakMs=0,posePeakMs=0,brushPeakMs=0;size_t walkQueries=0,brushWakes=0,rockRevisionWakes=0,lastPatchCount=0;
        bool conserved=true;
        Authoring authoring;
        ToolPickups hammerPickup;
        Float3 rockPreview=Float3::Zero;
    };
    static State state;
    // The worker only reads body. Join before reset/release; commit and every
    // renderer/physics mutation remain on the world update thread.
    static void FinishFracture(){if(state.fracture.valid())state.fracture.get();if(state.shovel.valid())state.shovel.get();}
    static Float3 World(G::P p){return Float3(X+float(p[0]),Y+float(p[1]),Z+float(p[2]));}
    static G::P Local(Float3 p){return {double(p.m_x)-X,double(p.m_y)-Y,double(p.m_z)-Z};}
    struct Mesh
    {
        bool local=false;
        explicit Mesh(bool inLocal=false):local(inLocal){}
        TVector<Render::ProceduralMeshVertex> vertices; TVector<uint32_t> indices;
        void Triangle(G::P a,G::P b,G::P c)
        {
            auto n=G::Unit(G::Cross(G::Add(b,G::Mul(a,-1)),G::Add(c,G::Mul(a,-1))));if(G::Dot(n,n)<.5)return;
            uint32_t start=uint32_t(vertices.size());Float3 normal{float(n[0]),float(n[1]),float(n[2])};
            vertices.emplace_back(Render::ProceduralMeshVertex{World(a),normal});vertices.emplace_back(Render::ProceduralMeshVertex{World(b),normal});vertices.emplace_back(Render::ProceduralMeshVertex{World(c),normal});
            indices.emplace_back(start);indices.emplace_back(start+1);indices.emplace_back(start+2);
        }
        void SurfaceTriangle(G::Triangle const& t)
        {
            uint32_t start=uint32_t(vertices.size());
            auto add=[&](G::P p,G::P n){vertices.emplace_back(Render::ProceduralMeshVertex{local?Float3(float(p[0]),float(p[1]),float(p[2])):World(p),Float3(float(n[0]),float(n[1]),float(n[2]))});};
            add(t.a,t.na);add(t.b,t.nb);add(t.c,t.nc);
            indices.emplace_back(start);indices.emplace_back(start+1);indices.emplace_back(start+2);
        }
        void SoilSurface(SoilScoop::RenderSurface const& surface,bool hideBuriedPerimeter=false)
        {
            uint32_t start=uint32_t(vertices.size());vertices.reserve(vertices.size()+surface.vertices.size());indices.reserve(indices.size()+surface.indices.size());
            for(auto const& v:surface.vertices)
            {
                auto n=hideBuriedPerimeter?TerrainSeam::TopNormal(v.position,v.normal):v.normal;
                vertices.emplace_back(Render::ProceduralMeshVertex{World(v.position),Float3(float(n[0]),float(n[1]),float(n[2]))});
            }
            for(size_t i=0;i<surface.indices.size();i+=3)
            {
                auto a=surface.indices[i],b=surface.indices[i+1],c=surface.indices[i+2];
                if(hideBuriedPerimeter&&TerrainSeam::BuriedPerimeter(surface.vertices[a].position,surface.vertices[b].position,surface.vertices[c].position))continue;
                indices.insert(indices.end(),{start+a,start+b,start+c});
            }
        }
        bool Replace(Render::ProceduralMeshID& id,Render::Material const* material,Transform const& transform=Transform::Identity,TBitFlags<Render::ViewLayer> viewLayers=TBitFlags<Render::ViewLayer>(Render::ViewLayer::ShadowMap,Render::ViewLayer::ForwardShading),bool fastBuild=true,float displacementRadius=0)
        {
            if(!state.renderer||!material)return false;
            auto replacement=vertices.empty()?Render::ProceduralMeshID(0):state.renderer->RegisterProceduralMesh(vertices,indices,material,transform,viewLayers,fastBuild,displacementRadius);
            if(!vertices.empty()&&!replacement)return false;
            if(id)state.renderer->UnregisterProceduralMesh(id);id=replacement;return true;
        }
    };
    static void PublishLandscape()
    {
        if(!state.renderer||!state.grassMaterial)return;
        auto start=std::chrono::steady_clock::now();
        auto const& grid=PlayableLandscape::Terrain();
        if(state.landscapeNext>=grid.TileCount()){state.landscapePublishMs=0;return;}
        // Bounded publication; tiles become independently culled static meshes.
        // No terrain reconstruction or upload after this initial publication.
        int tile=state.landscapeNext;
        int x0=(tile%grid.TileColumns())*PlayableLandscape::TileCells,y0=(tile/grid.TileColumns())*PlayableLandscape::TileCells;
        int x1=std::min(grid.NX(),x0+PlayableLandscape::TileCells),y1=std::min(grid.NY(),y0+PlayableLandscape::TileCells);
        Mesh mesh;
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)
        {
            auto p=grid.Vertex(x,y),n=grid.Normal(x,y);
            // UV.x=-2-mud: landscape identity and sediment mask; UV.y=grass.
            auto const& surface=grid.values[grid.ID(x,y)];
            mesh.vertices.emplace_back(Render::ProceduralMeshVertex{World(p),Float3(float(n[0]),float(n[1]),float(n[2])),Float2(-2.f-float(surface.mud),float(surface.cover))});
        }
        for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)if(grid.Cell(x,y))
        {
            uint32_t a=uint32_t((y-y0)*(x1-x0+1)+x-x0),b=a+1,d=a+uint32_t(x1-x0+1),c=d+1;
            mesh.indices.insert(mesh.indices.end(),{a,b,c,a,c,d});
        }
        if(mesh.indices.empty()){++state.landscapeNext;return;}
        Render::ProceduralMeshID id=0;
        if(mesh.Replace(id,state.grassMaterial))
        {state.landscapeTiles.push_back(id);state.landscapeTriangles+=mesh.indices.size()/3;++state.landscapeNext;}
        state.landscapePublishMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    }
    static void GrassCard(Mesh& mesh,G::P center,double angle,double width,double height,double vigor,int cellX,int cellY,bool meadow=false)
    {
        double nx=std::cos(angle),ny=std::sin(angle),tx=-ny,ty=nx,half=width*.5;
        G::P bottomLeft={center[0]-tx*half,center[1]-ty*half,center[2]+.004};
        G::P bottomRight={center[0]+tx*half,center[1]+ty*half,center[2]+.004};
        auto side=[&](double sign)
        {
            uint32_t start=uint32_t(mesh.vertices.size());
            // Preserve the owning cover cell across the entire card. Ratios to
            // -Z survive normal normalization and keep card pixels from
            // accidentally selecting adjacent grid cells in the material.
            // The fractional X coordinate carries habitat vigor while floor()
            // in the shader still recovers the exact owning cover cell.
            float habitatCode=.08f+.84f*float(vigor);
            float encodedX=.16f+.04f*(float(cellX)+habitatCode)/float(GrassCover::Resolution);
            float encodedY=.35f+.04f*(float(cellY)+.5f)/float(GrassCover::Resolution);
            Float3 marker{encodedX,encodedY,-1.f};
            // Two vertical segments give a soft bend rather than rotating a
            // rigid card. UV.y is exact root-to-tip weight, independent of the
            // changing soil texture; UV.x identifies this presentation payload.
            for(int row=0;row<=2;++row)
            {
                float t=float(row)*.5f;
                int column=0;
                for(auto p:{bottomLeft,bottomRight})
                {
                    p[2]+=height*t;
                    mesh.vertices.emplace_back(Render::ProceduralMeshVertex{World(p),marker,Float2(meadow?2.f:1.f,t),Float2(float(column++),1.f-t)});
                }
            }
            for(uint32_t row=0;row<2;++row)
            {
                uint32_t a=start+row*2,b=a+1,c=a+3,d=a+2;
                if(sign>0)mesh.indices.insert(mesh.indices.end(),{a,b,c,a,c,d});
                else mesh.indices.insert(mesh.indices.end(),{a,c,b,a,d,c});
            }
        };
        side(1);side(-1);
    }
    static void RebuildGrassBlades()
    {
        if(!state.renderer||!state.grassMaterial||!state.grass.pending.empty()||state.grass.bladeDirty.empty())return;
        auto started=std::chrono::steady_clock::now();int budget=state.grassBladePatches.empty()?8:2;
        while(budget-->0&&!state.grass.bladeDirty.empty())
        {
            int patch=*state.grass.bladeDirty.begin();state.grass.bladeDirty.erase(state.grass.bladeDirty.begin());
            int patchX=patch%GrassCover::BladePatchesPerAxis,patchY=patch/GrassCover::BladePatchesPerAxis;
            Mesh mesh;size_t clusters=0;
            for(int y=patchY*GrassCover::BladePatchSize;y<(patchY+1)*GrassCover::BladePatchSize;++y)
            for(int x=patchX*GrassCover::BladePatchSize;x<(patchX+1)*GrassCover::BladePatchSize;++x)
            {
                int id=x+y*GrassCover::Resolution;auto tuft=state.grass.Blade(id);if(!tuft.emitted)continue;
                // Exterior grass now continues across this boundary. Preserve
                // the editable population instead of carving a bald rectangle.
                // Reject only a tuft's two crossed root segments against the
                // seeded, original granite envelope. Blades rooted in soil may
                // still lean across the stone, and excavated cavities do not
                // instantly reveal dormant grass cards.
                if(state.terrain.originalRock&&!GrassCover::BaseClear(*state.terrain.originalRock,tuft))continue;
                GrassCard(mesh,tuft.center,tuft.angle,tuft.width,tuft.height,tuft.vigor,x,y);
                GrassCard(mesh,tuft.center,tuft.angle+1.570796326794897,tuft.width,tuft.height,tuft.vigor,x,y);++clusters;
            }
            size_t old=state.grassBladeCounts[patch];
            auto grassLayers=TBitFlags<Render::ViewLayer>(Render::ViewLayer::ForwardShading);
            if(state.grassShadows)grassLayers.SetFlag(Render::ViewLayer::ShadowMap);
            if(mesh.Replace(state.grassBladePatches[patch],state.grassMaterial,Transform::Identity,grassLayers,true,.06f))
            {state.grassBladeClusters=state.grassBladeClusters-old+clusters;state.grassBladeCounts[patch]=clusters;}
            else state.grass.bladeDirty.insert(patch);
        }
        state.grassBladeMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    }
    static void PublishMeadow()
    {
        namespace M=LandscapeGrass;state.meadowPublishMs=0;state.meadowReady=false;
        if(!state.renderer||!state.grassMaterial)return;
        int cx=int(std::floor(state.walker.feet[0]/M::TileSize)),cy=int(std::floor(state.walker.feet[1]/M::TileSize));
        auto started=std::chrono::steady_clock::now();
        for(auto it=state.meadowPatches.begin();it!=state.meadowPatches.end();)
        {
            if(std::abs(it->first.first-cx)>M::RetainRadius||std::abs(it->first.second-cy)>M::RetainRadius)
            {if(it->second.mesh)state.renderer->UnregisterProceduralMesh(it->second.mesh);state.meadowTufts-=it->second.tufts;it=state.meadowPatches.erase(it);}
            else ++it;
        }
        std::pair<int,int> next;bool found=false;int best=100000;
        for(int y=cy-M::Radius;y<=cy+M::Radius;++y)for(int x=cx-M::Radius;x<=cx+M::Radius;++x)
        {
            if((x+1)*M::TileSize<PlayableLandscape::MinX||x*M::TileSize>PlayableLandscape::MaxX||(y+1)*M::TileSize<PlayableLandscape::MinY||y*M::TileSize>PlayableLandscape::MaxY)continue;
            auto it=state.meadowPatches.find({x,y});
            if(it!=state.meadowPatches.end()&&it->second.shadows==state.grassShadows)continue;
            int distance=(x-cx)*(x-cx)+(y-cy)*(y-cy);
            if(distance<best){best=distance;next={x,y};found=true;}
        }
        // At most one <=1,024-tuft mesh per update; retain a one-tile hysteresis
        // band. No full-floor allocation and no stationary reuploads.
        if(!found){state.meadowReady=true;return;}
        if(state.meadowPatches.find(next)==state.meadowPatches.end()&&state.meadowPatches.size()>=M::MaxPatches)return;
        Mesh mesh;size_t count=0;
        for(int i=0;i<M::Cells*M::Cells;++i)
        {
            auto t=M::Candidate(next.first,next.second,i);if(!t.emitted)continue;
            uint32_t h=GrassCover::BladeHash(uint32_t(next.first)*0x12a3u^uint32_t(next.second)*0x9e3779b9u^uint32_t(i));
            GrassCard(mesh,t.center,t.angle,t.width,t.height,t.vigor,int(h&127u),int((h>>8)&127u),true);
            GrassCard(mesh,t.center,t.angle+1.570796326794897,t.width,t.height,t.vigor,int(h&127u),int((h>>8)&127u),true);++count;
        }
        auto& patch=state.meadowPatches[next];auto layers=TBitFlags<Render::ViewLayer>(Render::ViewLayer::ForwardShading);
        if(state.grassShadows)layers.SetFlag(Render::ViewLayer::ShadowMap);
        if(mesh.Replace(patch.mesh,state.grassMaterial,Transform::Identity,layers,true,.06f))
        {state.meadowTufts=state.meadowTufts-patch.tufts+count;patch.tufts=count;patch.shadows=state.grassShadows;}
        state.meadowPublishMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    }
    static size_t ChipRenderGeometry(Mesh& mesh,Debris const& d,bool posed)
    {
        // Both moving meshes and world-baked sleeping batches keep the same
        // birth coordinates. Only render positions/normals receive the pose.
        auto sourceTriangle=[&](G::Triangle const& source)
        {
            uint32_t start=uint32_t(mesh.vertices.size());
            auto add=[&](G::P p,G::P n)
            {
                auto birth=World(p);
                auto anchor=GraniteMaterialAnchor::Encode({birth.m_x,birth.m_y,birth.m_z},n);
                auto position=posed?Motion::Position(d,p):G::Add(p,G::Mul(d.chip.center,-1));
                auto normal=posed?Motion::Rotate(d.orientation,n):n;
                mesh.vertices.emplace_back(Render::ProceduralMeshVertex{
                    mesh.local?Float3(float(position[0]),float(position[1]),float(position[2])):World(position),
                    Float3(float(normal[0]),float(normal[1]),float(normal[2])),
                    Float2(anchor.uv[0],anchor.uv[1]),Float2(anchor.uv[2],anchor.uv[3]),anchor.normal});
            };
            add(source.a,source.na);add(source.b,source.nb);add(source.c,source.nc);
            mesh.indices.insert(mesh.indices.end(),{start,start+1,start+2});
        };
        auto emit=[&](G::P a,G::P b,G::P c)
        {
            auto n=G::Unit(G::Cross(G::Add(b,G::Mul(a,-1)),G::Add(c,G::Mul(a,-1))));if(G::Dot(n,n)<.5)return size_t(0);
            sourceTriangle({a,b,c,n,n,n});return size_t(1);
        };
        // Preserve full fracture geometry for structural clods.  Pick-scale
        // debris is represented by a bounded icosphere support proxy: its
        // gameplay body remains exact, while the normal map supplies the
        // centimeter-scale surface detail without rasterizing ~900 triangles.
        if(d.chip.volume>0.00025||d.chip.mesh.size()<=80)
        {
            size_t count=0;for(auto t:d.chip.mesh)
            {
                sourceTriangle(t);++count;
            }
            return count;
        }
        static G::P const raw[12]={{-1,1.61803398875,0},{1,1.61803398875,0},{-1,-1.61803398875,0},{1,-1.61803398875,0},{0,-1,1.61803398875},{0,1,1.61803398875},{0,-1,-1.61803398875},{0,1,-1.61803398875},{1.61803398875,0,-1},{1.61803398875,0,1},{-1.61803398875,0,-1},{-1.61803398875,0,1}};
        static int const faces[20][3]={{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},{3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
        G::P directions[12];for(int i=0;i<12;++i)directions[i]=G::Unit(raw[i]);
        auto point=[&](G::P direction)
        {
            double support=0;for(auto const& p:d.vertices)support=std::max(support,G::Dot(G::Add(p,G::Mul(d.chip.center,-1)),direction));
            support=std::clamp(support,d.radius*.45,d.radius);return G::Add(d.chip.center,G::Mul(direction,support));
        };
        size_t count=0;
        for(auto const& face:faces)
        {
            auto a=directions[face[0]],b=directions[face[1]],c=directions[face[2]];
            if(posed){count+=emit(point(a),point(b),point(c));continue;}
            auto ab=G::Unit(G::Add(a,b)),bc=G::Unit(G::Add(b,c)),ca=G::Unit(G::Add(c,a));
            auto pa=point(a),pb=point(b),pc=point(c),pab=point(ab),pbc=point(bc),pca=point(ca);
            count+=emit(pa,pab,pca);count+=emit(pb,pbc,pab);count+=emit(pc,pca,pbc);count+=emit(pab,pbc,pca);
        }
        return count;
    }
    void Release(ProvenanceWorldSystem* owner)
    {
        if(state.owner!=owner)return;
        FinishFracture();
        if(state.renderer)for(auto id:state.landscapeTiles)if(id)state.renderer->UnregisterProceduralMesh(id);
        if(state.renderer)for(auto id:state.soilClodMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);
        if(state.renderer){for(auto const& p:state.soilPatches)if(p.second)state.renderer->UnregisterProceduralMesh(p.second);for(auto const& p:state.rockPatches)if(p.second)state.renderer->UnregisterProceduralMesh(p.second);for(auto const& p:state.grassBladePatches)if(p.second)state.renderer->UnregisterProceduralMesh(p.second);for(auto id:state.chipMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);for(auto id:state.settledChipMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);}
        if(state.renderer)for(auto const& p:state.meadowPatches)if(p.second.mesh)state.renderer->UnregisterProceduralMesh(p.second.mesh);
        if(state.renderer)state.renderer->ReleaseSurfaceCoverMaterial();
        state=State();
    }
    static void Rebuild(Render::Material const* granite,Render::Material const* soil)
    {
        auto start=std::chrono::steady_clock::now();bool changed=false;
        if(state.dirty&&granite)
        {
            for(auto const& p:state.body.renderGroups)state.pendingPatches[p.first]=G::PatchBoundary(state.body,p.first);
            state.dirty=false;
        }
        size_t updated=0;
        if(granite)for(auto it=state.pendingPatches.begin();it!=state.pendingPatches.end();)
        {
            Mesh mesh;mesh.vertices.reserve(it->second.size()*3);mesh.indices.reserve(it->second.size()*3);for(auto const& t:it->second)mesh.SurfaceTriangle(t);
            if(mesh.Replace(state.rockPatches[it->first],granite)){it=state.pendingPatches.erase(it);++updated;changed=true;}else ++it;
        }
        if(updated)state.lastPatchCount=updated;
        auto soilStart=std::chrono::steady_clock::now();size_t soilTriangles=0,soilVertices=0,soilUpdates=0;
        if(soil)for(auto it=state.terrain.dirty.begin();it!=state.terrain.dirty.end();)
        {
            // Reuse the worker-prepared surface; no terrain generation or BVH
            // construction on the main thread when publishing a scoop.
            Mesh mesh;auto const& surface=*state.terrain.indexedSurfaces.at(*it);mesh.SoilSurface(surface,true);
            // Edited marching-tetrahedra surfaces are not spatially ordered.
            // Let meshoptimizer form compact meshlets; scan-built meshlets can
            // straddle the 16-bit cluster-center range at certain dirt cuts.
            if(mesh.Replace(state.soilPatches[*it],state.grassMaterial?state.grassMaterial:soil,Transform::Identity,TBitFlags<Render::ViewLayer>(Render::ViewLayer::ShadowMap,Render::ViewLayer::ForwardShading),false)){soilTriangles+=surface.indices.size()/3;soilVertices+=surface.vertices.size();++soilUpdates;it=state.terrain.dirty.erase(it);changed=true;}else ++it;
        }
        if(soilUpdates){state.soilUploadMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-soilStart).count();state.soilUpdateCount=soilUpdates;state.soilUploadTriangles=soilTriangles;state.soilUploadVertices=soilVertices;}
        state.soilClodMeshes.resize(state.terrain.loose.size(),0);state.soilClodVersions.resize(state.terrain.loose.size(),0);state.soilClodOffsets.resize(state.terrain.loose.size());
        if(soil)for(size_t i=0;i<state.terrain.loose.size();++i)
        {
            auto const& d=state.terrain.loose[i];Transform pose=Transform::Identity;pose.SetTranslation(Vector(float(d.offset[0]),float(d.offset[1]),float(d.offset[2]),0));
            if(state.soilClodVersions[i]!=d.meshRevision)
            {
                Mesh mesh;for(auto const& e:d.matter->indexedSurfaces)mesh.SoilSurface(*e.second);
                if(mesh.Replace(state.soilClodMeshes[i],soil,pose,TBitFlags<Render::ViewLayer>(Render::ViewLayer::ShadowMap,Render::ViewLayer::ForwardShading),false)){state.soilClodVersions[i]=d.meshRevision;state.soilClodOffsets[i]=d.offset;}changed=true;
            }
            else if(state.soilClodMeshes[i]&&state.soilClodOffsets[i]!=d.offset&&state.renderer->UpdateProceduralMeshTransform(state.soilClodMeshes[i],pose))state.soilClodOffsets[i]=d.offset;
        }
        auto poseStart=std::chrono::steady_clock::now();
        // Fixed-size sleeping batches bound the amount of immutable geometry
        // republished when one chip wakes. The previous monolithic batch made
        // one state change rebuild every chip ever dug (29 chips / 2,320
        // triangles measured 147 ms in the live Debug editor).
        constexpr size_t SleepBatchSize=8;
        size_t batchCount=(state.debris.size()+SleepBatchSize-1)/SleepBatchSize;
        state.chipBatchSettled.resize(state.debris.size(),false);
        state.settledChipMeshes.resize(batchCount,0);
        state.settledChipTriangleCounts.resize(batchCount,0);
        if(granite)for(size_t batch=0;batch<batchCount;++batch)
        {
            size_t begin=batch*SleepBatchSize,end=std::min(begin+SleepBatchSize,state.debris.size());bool dirty=false,woke=false;
            for(size_t i=begin;i<end;++i){dirty|=state.chipBatchSettled[i]!=state.debris[i].settled;woke|=state.chipBatchSettled[i]&&!state.debris[i].settled;}
            if(!dirty||(!woke&&state.chipBatchQuiet<.35))continue;
            Mesh mesh;size_t triangles=0;
            for(size_t i=begin;i<end;++i)if(state.debris[i].settled)triangles+=ChipRenderGeometry(mesh,state.debris[i],true);
            if(mesh.Replace(state.settledChipMeshes[batch],granite,Transform::Identity,TBitFlags<Render::ViewLayer>(Render::ViewLayer::ForwardShading)))
            {
                state.settledChipTriangles-=state.settledChipTriangleCounts[batch];state.settledChipTriangles+=triangles;state.settledChipTriangleCounts[batch]=triangles;
                for(size_t i=begin;i<end;++i)
                {
                    state.chipBatchSettled[i]=state.debris[i].settled;
                    if(state.debris[i].settled&&state.chipMeshes[i]){state.renderer->UnregisterProceduralMesh(state.chipMeshes[i]);state.chipMeshes[i]=0;state.chipPoseDirty[i]=false;}
                }
                changed=true;
            }
        }
        for(size_t i=0;i<state.debris.size();++i)
        {
            if(i<state.chipBatchSettled.size()&&state.chipBatchSettled[i])continue;
            if(state.chipMeshes[i]&&!state.chipPoseDirty[i])continue;
            auto const& d=state.debris[i];auto const& q=d.orientation;
            Transform pose(Quaternion(float(q[1]),float(q[2]),float(q[3]),float(q[0])),Vector(World(G::Add(d.chip.center,d.offset))));
            if(!state.chipMeshes[i])
            {
                Mesh mesh(true);ChipRenderGeometry(mesh,d,false);
                if(mesh.Replace(state.chipMeshes[i],granite,pose,TBitFlags<Render::ViewLayer>(Render::ViewLayer::ForwardShading)))state.chipPoseDirty[i]=false;changed=true;
            }
            else if(state.chipPoseDirty[i]&&state.renderer->UpdateProceduralMeshTransform(state.chipMeshes[i],pose))state.chipPoseDirty[i]=false;
        }
        state.poseMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-poseStart).count();
        state.posePeakMs=std::max(state.posePeakMs,state.poseMs);
        if(changed)state.meshMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    }
    static void Walk(EntityWorldUpdateContext const& ctx)
    {
        auto start=std::chrono::steady_clock::now();auto* cameras=ctx.GetWorldSystem<CameraSystem>();auto* camera=cameras?cameras->GetToolsCamera():nullptr;if(!camera)return;
        if(!state.walkStarted)
        {
            cameras->EnableToolsCamera();camera->SetMoveSpeed(1.6f);
            auto eye=G::Add(state.walker.feet,G::P{0,0,state.walker.Eye()});auto direction=G::Unit(G::Add(G::P{G::Width*.5,G::Depth*.48,.58},G::Mul(eye,-1)));
            camera->SetPositionAndLookAtDirection(Vector(World(eye)),Vector(float(direction[0]),float(direction[1]),float(direction[2]),0));state.walkStarted=true;
        }
        if(cameras->IsToolsCameraEnabled())
        {
            auto* keyboard=ctx.GetSystem<Input::InputSystem>()->GetKeyboardMouse();
            auto held=[&](Input::InputID id){return keyboard->IsHeldDown(id);};
            auto right=camera->GetRightVector().ToFloat3();double length=std::hypot(double(right.m_x),double(right.m_y));
            // Horizontal yaw frame: looking up/down never changes walking speed.
            double rx=length>1e-8?right.m_x/length:1,ry=length>1e-8?right.m_y/length:0;
            auto forward=camera->GetWorldTransform().GetForwardVector().ToFloat3();
            double fx=-ry,fy=rx;if(fx*forward.m_x+fy*forward.m_y<0){fx=-fx;fy=-fy;}
            double fb=double(held(Input::InputID::Keyboard_W))-double(held(Input::InputID::Keyboard_S));
            double lr=double(held(Input::InputID::Keyboard_D))-double(held(Input::InputID::Keyboard_A));
            GraniteLabMovement::Input controls{fx*fb+rx*lr,fy*fb+ry*lr,
                held(Input::InputID::Keyboard_LShift)||held(Input::InputID::Keyboard_RShift),held(Input::InputID::Keyboard_C),held(Input::InputID::Keyboard_Space)};
            if(std::abs(fb)+std::abs(lr)>0)state.movingFramePeakMs=std::max(state.movingFramePeakMs,double(ctx.GetDeltaTime())*1000);
            auto oldFeet=state.walker.feet;
            state.walkQueries=0;
            GraniteLabMovement::Advance(state.walker,controls,double(ctx.GetDeltaTime()),
                [&](G::P feet,double height){++state.walkQueries;return G::BlocksCapsule(state.body,feet,GraniteLabMovement::Radius,height)||state.terrain.BlocksCapsule(feet,GraniteLabMovement::Radius,height,false);},
                [](double x,double y){double height;return PlayableLandscape::Terrain().Surface(x,y,height)?height:-double(Z);},
                [&](double x,double y,double ceiling){return GraniteLabContact::StepSurface(state.body,x,y,ceiling,&state.terrain,false);});
            auto brushStart=std::chrono::steady_clock::now();
            SoilScoop::BrushLoose(state.terrain,oldFeet,state.walker.feet,GraniteLabMovement::Radius,state.walker.Height());
            // Settled granite is non-blocking scenery for ordinary walking.
            // Waking even one settled chip forces its immutable sleeping batch
            // to be republished, which is the single frame spike observed when
            // crossing a pile. Explicit F interaction still wakes and moves a
            // selected loose chip; active chips continue simulating normally.
            state.brushWakes=0;
            state.brushMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-brushStart).count();state.brushPeakMs=std::max(state.brushPeakMs,state.brushMs);
            state.grass.Travel(oldFeet,state.walker.feet,state.walker.grounded);
            auto transform=camera->GetWorldTransform();transform.SetTranslation(Vector(World(G::Add(state.walker.feet,G::P{0,0,state.walker.Eye()}))));camera->SetCameraWorldTransform(transform);
        }
        state.walkMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        state.walkPeakMs=std::max(state.walkPeakMs,state.walkMs);
    }
    bool Tick(ProvenanceWorldSystem* owner,EntityWorldUpdateContext const& ctx,ProvenanceWorldSettingsComponent const& settings)
    {
        if(state.owner!=owner)
        {
            FinishFracture();state=State();state.owner=owner;state.body.Reset(settings.GetSeed());state.terrain.Reset(&state.body);state.grass.Reset();if(state.terrain.originalRock)state.grass.ExposeRockMargin(*state.terrain.originalRock);
            // Resolve immutable terrain during setup, before walking metrics.
            // This does not erase its startup cost or claim a faster frame.
            auto setup=std::chrono::steady_clock::now();(void)PlayableLandscape::Terrain();
            state.landscapeSetupMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-setup).count();
        }
        if(state.fracture.valid()||state.shovel.valid()||state.actionTailFrames>0)
        {state.actionPeakMs=std::max(state.actionPeakMs,double(ctx.GetDeltaTime())*1000);if(state.actionTailFrames>0)--state.actionTailFrames;}
        auto* input=ctx.GetSystem<Input::InputSystem>();if(!input)return true;auto* keyboard=input->GetKeyboardMouse();
        bool h=keyboard->IsHeldDown(Input::InputID::Keyboard_H),f=keyboard->IsHeldDown(Input::InputID::Keyboard_F),r=keyboard->IsHeldDown(Input::InputID::Keyboard_R);
        bool v=keyboard->IsHeldDown(Input::InputID::Keyboard_V);if(v&&!state.v)state.inspect=!state.inspect;state.v=v;
        auto setGrassShadows=[&](bool on)
        {
            if(state.grassShadows==on)return;
            state.grassShadows=on;
            state.meadowReady=false;
            // Drop stale exterior shadow-layer meshes; they republish nearest
            // first. Never retain an ON shadow in the hysteresis band after OFF.
            for(auto const& p:state.meadowPatches)if(p.second.mesh&&state.renderer)state.renderer->UnregisterProceduralMesh(p.second.mesh);
            state.meadowPatches.clear();state.meadowTufts=0;
            // Explicit quality changes only; never a per-frame mesh upload.
            for(auto const& patch:state.grassBladePatches)state.grass.bladeDirty.insert(patch.first);
        };
        bool shadowKey=keyboard->IsHeldDown(Input::InputID::Keyboard_F8);
        bool benchmarkKey=keyboard->IsHeldDown(Input::InputID::Keyboard_F9);
        bool movementKey=keyboard->IsHeldDown(Input::InputID::Keyboard_F11);
        if(movementKey&&!state.movementCaptureKey)
        {
            if(state.movementCapture.running)state.movementCapture.Finish();
            else if(!state.shadowBenchmark.Running())state.movementCapture.Start();
        }
        state.movementCaptureKey=movementKey;
        bool shadowClick=false,benchmarkClick=false,creekClick=false;
        if(auto* testViewport=ctx.GetMainViewport();testViewport&&state.active&&!state.authoring.open)
        {
            auto top=testViewport->GetTopLeftPosition();auto size=testViewport->GetDimensions();
            ImGui::SetNextWindowPos(ImVec2(top.m_x+size.m_x-370,top.m_y+36),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(358,0),ImGuiCond_Always);
            if(ImGui::Begin("Grass shadow trial",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoFocusOnAppearing))
            {
                shadowClick=ImGui::Button(state.grassShadows?"Shadows ON (F8)":"Shadows OFF (F8)");
                ImGui::SameLine();benchmarkClick=ImGui::Button(state.shadowBenchmark.Running()?"Cancel test":"Test (F9)");
                creekClick=ImGui::Button(state.creekView?"Return to lab (F10)":"Creek view (F10)");
                ImGui::SameLine();
                if(ImGui::Button(state.movementCapture.running?"Stop capture":"Capture 30s (F11)"))
                {
                    if(state.movementCapture.running)state.movementCapture.Finish();
                    else if(!state.shadowBenchmark.Running())state.movementCapture.Start();
                }
                if(state.shadowBenchmark.Running())ImGui::Text("Leg %d/4: %.1f / 10 s",state.shadowBenchmark.phase+1,state.shadowBenchmark.elapsed);
                else ImGui::TextUnformatted("Keep view still during test.");
            }
            ImGui::End();
        }
        if((benchmarkKey&&!state.benchmarkKey)||benchmarkClick)
        {
            if(state.shadowBenchmark.Running())state.shadowBenchmark.phase=-1;
            else if(!state.movementCapture.running)
            {
                auto resultPath=FileSystem::GetCurrentProcessPath();
                resultPath.Append("GrassShadowBenchmark.csv");
                state.shadowBenchmark.outputPath=resultPath.GetFullPath().c_str();
                state.shadowBenchmark.Start();setGrassShadows(false);
            }
        }
        if((shadowKey&&!state.shadowKey)||shadowClick){state.shadowBenchmark.phase=-1;setGrassShadows(!state.grassShadows);}
        state.shadowKey=shadowKey;state.benchmarkKey=benchmarkKey;
        bool benchmarkReady=state.grassMaterial&&state.meadowReady&&!state.grass.bladeDirty.size()&&state.grass.pending.empty()
            &&state.landscapeNext>=PlayableLandscape::Terrain().TileCount()
            &&!state.fracture.valid()&&!state.shovel.valid()&&!state.authoring.open&&!state.dirty&&state.pendingPatches.empty();
        bool benchmarkInput=false;
        for(auto key:{Input::InputID::Keyboard_W,Input::InputID::Keyboard_A,Input::InputID::Keyboard_S,Input::InputID::Keyboard_D,
            Input::InputID::Keyboard_Space,Input::InputID::Keyboard_C,Input::InputID::Keyboard_F,Input::InputID::Keyboard_R,Input::InputID::Mouse_Right})
            if(keyboard->IsHeldDown(key))benchmarkInput=true;
        if(benchmarkInput&&state.shadowBenchmark.Running())
        {state.shadowBenchmark.phase=-1;state.shadowBenchmark.finished=false;setGrassShadows(false);}
        for(auto const& chip:state.debris)if(!chip.settled)benchmarkReady=false;
        for(auto const& clod:state.terrain.loose)if(!clod.settled)benchmarkReady=false;
        if(state.shadowBenchmark.Advance(double(ctx.GetUnscaledDeltaTime()),benchmarkReady))
            setGrassShadows(state.shadowBenchmark.Running()?GrassShadowBenchmark::Shadows(state.shadowBenchmark.phase):false);
        bool toggleAuthoring=keyboard->IsHeldDown(Input::InputID::Keyboard_F2);
        if(toggleAuthoring&&!state.authoring.key&&state.active)
        {
            // Do not leave authoring while replacement rendering is unpublished.
            bool publishing=state.dirty||!state.pendingPatches.empty()||!state.terrain.dirty.empty()
                ||!state.grass.pending.empty()||!state.grass.bladeDirty.empty();
            if(!state.authoring.open||!publishing)
            {state.authoring.open=!state.authoring.open;state.authoring.Cancel();state.queuedAction=false;}
        }
        state.authoring.key=toggleAuthoring;
        bool pf=f&&!state.f&&!state.authoring.open,pr=r&&!state.r&&!state.authoring.open;
        if(h&&!state.h&&!state.authoring.open){state.active=!state.active;if(state.active)state.walkStarted=false;}
        state.h=h;state.f=f;state.r=r;if(!state.active){state.hammerPickup.inventory.Update(keyboard->IsHeldDown(Input::InputID::Keyboard_E),false,ToolPickup::Kind::None);return false;}
        state.renderer=ctx.GetWorldSystem<Render::RenderWorldSystem>();
        if(pr)
        {
            state.creekView=false;
            FinishFracture();
            if(state.renderer)for(auto id:state.soilClodMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);
            state.soilClodMeshes.clear();state.soilClodVersions.clear();state.soilClodOffsets.clear();
            if(state.renderer)for(auto id:state.chipMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);
            if(state.renderer)for(auto id:state.settledChipMeshes)if(id)state.renderer->UnregisterProceduralMesh(id);
            if(state.renderer)for(auto const& p:state.rockPatches)if(p.second)state.renderer->UnregisterProceduralMesh(p.second);
            state.rockPatches.clear();state.pendingPatches.clear();
            state.chipMeshes.clear();state.chipPoseDirty.clear();state.settledChipMeshes.clear();state.settledChipTriangleCounts.clear();state.chipBatchSettled.clear();state.settledChipTriangles=0;state.chipBatchQuiet=0;
            state.body.Reset(state.body.seed);state.debris.clear();state.receipt={};state.attempts=state.contacts=0;
            state.authoring.committed=state.authoring.offset=Float3::Zero;state.authoring.hasUndo=false;
            if(state.renderer)for(auto const& p:state.soilPatches)if(p.second)state.renderer->UnregisterProceduralMesh(p.second);
            state.soilPatches.clear();state.terrain.Reset(&state.body);state.scoop={};state.scoopMs=0;
            state.grass.Reset();if(state.terrain.originalRock)state.grass.ExposeRockMargin(*state.terrain.originalRock);state.grassUploadClock=1;
            state.actionPeakMs=state.movingFramePeakMs=state.soilUploadMs=0;state.walkPeakMs=state.physicsPeakMs=state.posePeakMs=state.brushPeakMs=0;state.actionTailFrames=0;state.soilUploadTriangles=state.soilUploadVertices=state.soilUpdateCount=0;
            state.dirty=true;state.conserved=true;state.walkStarted=false;state.walker=GraniteLabMovement::Body{{G::Width*.5,-1.8,Ground::Height(G::Width*.5,-1.8)+GraniteLabMovement::Skin}};
        }
        state.grass.settings.wearPerMeter=std::max(0.,double(settings.GetGrassWearPerMeter()));
        state.grass.settings.recoveryDelay=std::max(0.,double(settings.GetGrassRecoveryDelay()));
        state.grass.settings.recoverySeconds=std::max(1.,double(settings.GetGrassRecoverySeconds()));
        state.grass.Advance(state.authoring.open?0:double(ctx.GetDeltaTime())*std::max(0.,double(settings.GetGrassTimeScale())));
        auto* viewport=ctx.GetMainViewport();if(!viewport)return true;
        if(!state.authoring.open)Walk(ctx);
        auto* camera=ctx.GetWorldSystem<CameraSystem>()->GetToolsCamera();
        bool creekKey=keyboard->IsHeldDown(Input::InputID::Keyboard_F10);
        if(((creekKey&&!state.creekViewKey)||creekClick)&&camera&&!state.authoring.open
            &&!state.shadowBenchmark.Running()&&state.landscapeNext>=PlayableLandscape::Terrain().TileCount())
        {
            // Lab inspection bookmark, not travel/teleport gameplay. Toggle
            // back to the captured position; leave all material state intact.
            if(!state.creekView)
            {
                state.creekReturn=state.walker;state.creekReturnDirection=camera->GetWorldTransform().GetForwardVector().ToFloat3();
                double wy=30.,x=PlayableLandscape::CreekX(wy)-X+1.15,y=wy-Y-1.4,height=0;
                if(PlayableLandscape::Terrain().Surface(x,y,height))
                {
                    state.walker=GraniteLabMovement::Body{{x,y,height+GraniteLabMovement::Skin}};
                    auto eye=G::Add(state.walker.feet,G::P{0,0,state.walker.Eye()});
                    double tx=PlayableLandscape::CreekX(wy+1)-X,ty=wy+1-Y,th=0;PlayableLandscape::Terrain().Surface(tx,ty,th);
                    auto look=G::Unit(G::Add(G::P{tx,ty,th+.03},G::Mul(eye,-1)));
                    camera->SetPositionAndLookAtDirection(Vector(World(eye)),Vector(float(look[0]),float(look[1]),float(look[2]),0));state.creekView=true;
                }
            }
            else if(!G::BlocksCapsule(state.body,state.creekReturn.feet,GraniteLabMovement::Radius,state.creekReturn.Height())
                &&!state.terrain.BlocksCapsule(state.creekReturn.feet,GraniteLabMovement::Radius,state.creekReturn.Height(),false))
            {
                state.walker=state.creekReturn;
                camera->SetPositionAndLookAtDirection(Vector(World(G::Add(state.walker.feet,G::P{0,0,state.walker.Eye()}))),Vector(state.creekReturnDirection));state.creekView=false;
            }
        }
        state.creekViewKey=creekKey;
        if(camera)camera->SetUpdateEnabled(!state.authoring.open||(!state.authoring.panelHovered&&!state.authoring.gizmo.IsManipulating()&&keyboard->IsHeldDown(Input::InputID::Mouse_Right)));
        bool placeable=GraniteLabPlacement::Intact(state.body)&&state.terrain.revision==0&&state.debris.empty()
            &&!state.fracture.valid()&&!state.shovel.valid()&&!state.dirty&&state.pendingPatches.empty()&&state.terrain.dirty.empty();
        auto pivot=World(G::Mul(G::Add(state.body.low,state.body.high),.5));
        state.authoring.Draw(ctx,pivot,double(state.body.initialMg)/1e6,placeable,[&](Float3 start,Float3 direction)
        {
            auto origin=Local(start);G::P ray={direction.m_x,direction.m_y,direction.m_z};
            auto shift=state.authoring.offset-state.authoring.committed;G::P delta={shift.m_x,shift.m_y,shift.m_z};
            auto hit=G::Raycast(state.body,G::Add(origin,G::Mul(delta,-1)),ray,150);
            bool rock=hit.hit;if(rock)hit.position=G::Add(hit.position,delta);
            double nearest=hit.hit?hit.distance:150;G::Hit soil;
            state.terrain.Trace(origin,ray,nearest,soil);
            if(soil.hit&&(!hit.hit||soil.distance<hit.distance)){hit=soil;rock=false;}
            if(state.landscapeNext>=PlayableLandscape::Terrain().TileCount())
            {
                auto land=GraniteBuildPlacement::LandscapeRay(origin,ray,hit.hit?hit.distance:150);
                if(land.hit&&(!hit.hit||land.distance<hit.distance)){hit=land;rock=false;}
            }
            return BuildSurface{hit.hit,rock,hit.hit?World(hit.position):Float3::Zero,hit.hit?hit.distance:150};
        });
        if(state.authoring.apply)
        {
            state.authoring.apply=false;
            if(placeable)
            {
                auto d=state.authoring.offset-state.authoring.committed;
                auto candidate=state.body;
                if(GraniteLabPlacement::Translate(candidate,{d.m_x,d.m_y,d.m_z}))
                {
                    // Explicit authoring fixture regeneration, not harvested or
                    // transported soil. The heightfield itself is not translated.
                    SoilScoop::Body candidateSoil;candidateSoil.Reset(&candidate);
                    if(G::BlocksCapsule(candidate,state.walker.feet,GraniteLabMovement::Radius,state.walker.Height())
                        ||candidateSoil.BlocksCapsule(state.walker.feet,GraniteLabMovement::Radius,state.walker.Height(),false))
                    {state.receipt.status="PLACEMENT REFUSED: overlaps player; cancel, move away in play, retry";}
                    else
                    {
                        state.body=std::move(candidate);state.terrain=std::move(candidateSoil);
                        state.authoring.previous=state.authoring.committed;state.authoring.hasUndo=true;
                        state.authoring.committed=state.authoring.offset;state.grass.Reset();
                        if(state.terrain.originalRock)state.grass.ExposeRockMargin(*state.terrain.originalRock);
                        state.grassUploadClock=1;state.dirty=true;
                        state.receipt.status="AUTHORING: placement applied; new pristine soil fixture";
                    }
                }
            }
        }
        auto origin=camera?Local(camera->GetWorldTransform().GetTranslation().ToFloat3()):Local(viewport->GetViewPosition().ToFloat3());
        Float3 forward=camera?camera->GetWorldTransform().GetForwardVector().ToFloat3():viewport->GetViewForwardDirection().ToFloat3();G::P direction={forward.m_x,forward.m_y,forward.m_z};
        bool actionBusy=state.fracture.valid()||state.shovel.valid()||!state.pendingPatches.empty()||!state.terrain.dirty.empty();
        if(pf&&actionBusy){state.queuedAction=true;state.queuedOrigin=origin;state.queuedDirection=direction;}
        bool useQueued=state.queuedAction&&!actionBusy&&!state.authoring.open;
        if((pf||useQueued)&&!actionBusy)
        {
            if(useQueued){origin=state.queuedOrigin;direction=state.queuedDirection;state.queuedAction=false;}
            ++state.attempts;double energy=400;uint32_t grade=2;state.actionPeakMs=0;state.actionTailFrames=12;
            auto contact=GraniteLabContact::Trace(state.body,state.debris,origin,direction,2,&state.terrain);
            state.capturedContact=contact.hit;
            state.scoop={};
            if(contact.material==GraniteLabContact::Material::Host)
                state.fracture=std::async(std::launch::async,[origin,direction,energy,grade](){return GraniteStructuralSupport::PrepareStrike(state.body,origin,direction,energy,grade);});
            else
            {
                state.receipt={};state.receipt.contact=contact.hit;if(contact.hit.hit)++state.contacts;
                if(contact.material==GraniteLabContact::Material::Soil)
                {
                    auto captured=contact.hit;
                    state.shovel=std::async(std::launch::async,[captured,direction](){return SoilScoop::Prepare(state.terrain,state.body,captured,direction);});
                    state.receipt.status="Preparing captured soil scoop...";
                }
                else if(contact.material==GraniteLabContact::Material::Loose&&contact.loose>=0)
                {
                    Motion::Nudge(state.debris[contact.loose],direction,contact.hit.position);
                    state.chipPoseDirty[contact.loose]=true;
                    state.receipt.status="LOOSE GRANITE: tapped aside; existing piece and mass retained";
                }
                else state.receipt.status="MISS";
            }
        }
        if(state.shovel.valid()&&state.shovel.wait_for(std::chrono::seconds(0))==std::future_status::ready)
        {
            auto transaction=state.shovel.get();state.scoop=transaction.receipt;state.scoopMs=transaction.milliseconds;
            bool published=SoilScoop::Commit(state.terrain,std::move(transaction));state.actionTailFrames=12;
            if(published)state.grass.QueueTerrain(state.terrain);
            state.receipt.status=!published?"REFUSED: stale soil transaction":state.scoop.collectedClod?"SOIL CLOD: collected whole in one scoop; no new fragments":state.scoop.volume>0?"SOIL SCOOP: recovered soil accounted separately; granite untouched":"SOIL SCOOP BLOCKED: granite, test floor, or finite patch edge";
        }
        if(state.fracture.valid()&&state.fracture.wait_for(std::chrono::seconds(0))==std::future_status::ready)
        {
            auto transaction=state.fracture.get();auto patches=std::move(transaction.renderUpdates);
            state.actionTailFrames=12;
            state.receipt=G::Commit(state.body,std::move(transaction));if(state.receipt.contact.hit)++state.contacts;
            if(state.receipt.removed)
            {
                state.debris.push_back(Motion::Launch(std::move(state.receipt.chip),state.receipt.contact.normal));
                for(auto& p:patches)state.pendingPatches[p.first]=std::move(p.second);
                state.chipMeshes.push_back(0);state.chipPoseDirty.push_back(true);
                for(auto& chip:state.receipt.detached)
                {
                    // Structural bodies retain their own cast and scale. They
                    // begin falling, not with the pick chip's ejection impulse.
                    auto falling=Motion::Launch(std::move(chip),state.receipt.contact.normal);falling.velocity=G::Mul(state.receipt.contact.normal,.04);falling.spin={};
                    state.debris.push_back(std::move(falling));state.chipMeshes.push_back(0);state.chipPoseDirty.push_back(true);
                }
            }
            uint64_t recovered=0;double volume=0;std::vector<uint8_t> seen(state.body.nextChipID,0);state.conserved=true;
            for(auto const& d:state.debris)
            {
                auto const& chip=d.chip;
                if(chip.id<0||size_t(chip.id)>=seen.size())state.conserved=false;
                else {if(seen[chip.id])state.conserved=false;seen[chip.id]=1;}
                recovered+=chip.massMg;volume+=chip.volume;
            }
            state.conserved=state.conserved&&recovered==state.body.removedMg&&std::abs(volume-state.body.removedM3)<1e-10;
        }
        double dt=std::clamp(double(ctx.GetDeltaTime()),0.,.05);
        auto physicsStart=std::chrono::steady_clock::now();
        // Worker snapshots share immutable clod geometry. Hold their transforms
        // steady until publication so a captured local scoop cannot race motion.
        if(!state.authoring.open&&!state.shovel.valid())SoilScoop::AdvanceLoose(state.terrain,state.body,dt);
        state.soilMotionMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-physicsStart).count();
        state.rockRevisionWakes=0;
        for(size_t i=0;!state.authoring.open&&i<state.debris.size();++i)
        {
            bool wasRockSleep=state.debris[i].settled&&state.debris[i].restingOnRock;
            if(Motion::Advance(state.debris[i],state.body,dt,&state.terrain,state.rockRevisionWakes<1))state.chipPoseDirty[i]=true;
            if(wasRockSleep&&!state.debris[i].settled)++state.rockRevisionWakes;
        }
        bool chipMoving=false;for(auto const& d:state.debris)if(!d.settled){chipMoving=true;break;}
        state.chipBatchQuiet=chipMoving?0:state.chipBatchQuiet+dt;
        state.physicsMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-physicsStart).count();
        state.physicsPeakMs=std::max(state.physicsPeakMs,state.physicsMs);
        auto grassStart=std::chrono::steady_clock::now();state.grass.Refresh(state.terrain);
        bool firstGrass=false;
        if(!state.grassMaterial&&state.renderer&&settings.GetGrassCoverMaterial())
        {
            state.grassMaterial=state.renderer->CreateSurfaceCoverMaterial(settings.GetGrassCoverMaterial());firstGrass=state.grassMaterial!=nullptr;
            if(firstGrass){for(auto const& e:state.terrain.queries)state.terrain.dirty.insert(e.first);state.grass.dirty=true;}
        }
        state.grassUploadClock+=double(ctx.GetDeltaTime());
        // Wind uses ordinary elapsed game time, not accelerated grass regrowth.
        // Integer shader harmonics make this 64-second phase wrap continuous.
        state.grassWindSeconds=std::fmod(state.grassWindSeconds+std::max(0.0,double(ctx.GetDeltaTime())),64.0);
        if(state.grassMaterial)state.renderer->SetSurfaceCoverWindPhase(float(state.grassWindSeconds*(6.283185307179586/64.0)));
        if(state.grassMaterial&&state.grass.dirty&&(firstGrass||state.grassUploadClock>=.1))
        {
            auto pixels=state.grass.Pixels(Z);TVector<Float4> data;data.reserve(pixels.size());for(auto const& p:pixels)data.emplace_back(p[0],p[1],p[2],p[3]);
            state.renderer->SetSurfaceCoverData(GrassCover::Resolution,GrassCover::Resolution,{data.data(),data.size()});state.grass.dirty=false;state.grassUploadClock=0;
        }
        RebuildGrassBlades();
        PublishLandscape();
        PublishMeadow();
        state.grassMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-grassStart).count();
        bool rockRepublished=state.dirty||!state.pendingPatches.empty();
        bool captureBusy=rockRepublished||!state.terrain.dirty.empty()||state.fracture.valid()||state.shovel.valid()||state.actionTailFrames>0||f||r||state.authoring.open||state.shadowBenchmark.Running()||creekKey;
        for(auto const& chip:state.debris)captureBusy=captureBusy||!chip.settled;
        for(auto const& clod:state.terrain.loose)captureBusy=captureBusy||!clod.settled;
        auto publicationStart=std::chrono::steady_clock::now();
        Rebuild(settings.GetGraniteMaterial(),settings.GetSoilMaterial());
        double publicationMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-publicationStart).count()+state.grassMs;
        auto preview=state.authoring.open?state.authoring.offset-state.authoring.committed:Float3::Zero;
        if(state.renderer&&(rockRepublished||preview.m_x!=state.rockPreview.m_x||preview.m_y!=state.rockPreview.m_y||preview.m_z!=state.rockPreview.m_z))
        {
            Transform pose=Transform::Identity;pose.SetTranslation(Vector(preview));bool placed=true;
            for(auto const& p:state.rockPatches)if(p.second&&!state.renderer->UpdateProceduralMeshTransform(p.second,pose))placed=false;
            if(placed)state.rockPreview=preview;
        }
        auto start=std::chrono::steady_clock::now();auto aim=GraniteLabContact::Trace(state.body,state.debris,origin,direction,2,&state.terrain);auto hit=aim.hit;state.aimMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        auto feet=state.walker.feet;auto creek=PlayableLandscape::Creek(feet[0]+X,feet[1]+Y);
        auto region=PlayableLandscape::InCore(feet[0],feet[1])?MovementFrameCapture::Region::Lab:
            std::abs(creek.across)<creek.half+2?MovementFrameCapture::Region::Creek:MovementFrameCapture::Region::Outer;
        bool movementInput=false;
        for(auto key:{Input::InputID::Keyboard_W,Input::InputID::Keyboard_A,Input::InputID::Keyboard_S,Input::InputID::Keyboard_D,Input::InputID::Keyboard_Space,Input::InputID::Keyboard_C,Input::InputID::Mouse_Right})
            movementInput=movementInput||keyboard->IsHeldDown(key);
        bool captureReady=state.landscapeNext>=PlayableLandscape::Terrain().TileCount()&&state.grassMaterial&&!state.authoring.open;
        state.movementCapture.Observe(double(ctx.GetUnscaledDeltaTime())*1000,{movementInput,captureBusy,captureReady,region,state.walkMs,publicationMs});
        if(state.movementCapture.finished&&!state.movementCapture.writeAttempted)
        {
            auto path=FileSystem::GetCurrentProcessPath();path.Append("MovementFrameCapture.csv");
            // Try once; disk errors must not cause repeated writes every frame.
            state.movementCapture.Write(path.GetFullPath().c_str());
        }
        auto draw=ctx.GetDebugDrawContext();auto dim=viewport->GetDimensions();draw.DrawText2D(Float2(dim.m_x*.5f,dim.m_y*.5f),"[ + ]",aim.material==GraniteLabContact::Material::Host||aim.material==GraniteLabContact::Material::Soil?Colors::Green:hit.hit?Colors::Yellow:Colors::White,DebugFont::Normal,DebugTextAlign::MiddleCenter);
        state.hammerPickup.Update(ctx,World(origin),forward,keyboard->IsHeldDown(Input::InputID::Keyboard_E),
            !state.authoring.open&&!ImGui::GetIO().WantTextInput&&!ImGui::IsAnyItemActive()&&!state.shadowBenchmark.Running(),[&](double reach)
        {
            double obstruction=hit.hit?hit.distance:ToolPickup::Reach+1;
            auto land=GraniteBuildPlacement::LandscapeRay(origin,direction,reach);
            if(land.hit)obstruction=std::min(obstruction,land.distance);
            return obstruction;
        });
        if(state.hammerPickup.aimed)
        {
            char pickupText[80];std::snprintf(pickupText,sizeof(pickupText),"E - Pick up %s",ToolPickup::Names[int(state.hammerPickup.targetKind)]);
            draw.DrawText2D(Float2(dim.m_x*.5f,dim.m_y*.5f+28),pickupText,Colors::Yellow,DebugFont::Normal,DebugTextAlign::MiddleCenter);
        }
        if(state.hammerPickup.inventory.Count())
        {
            char pickupText[100];std::snprintf(pickupText,sizeof(pickupText),"Collected: %s | Tools: %d/4",ToolPickup::Names[int(state.hammerPickup.inventory.last)],state.hammerPickup.inventory.Count());
            draw.DrawText2D(Float2(dim.m_x*.5f,dim.m_y-44),pickupText,Colors::Cyan,DebugFont::Normal,DebugTextAlign::MiddleCenter);
        }
        float py=92;auto line=[&](char const* text,Color c=Colors::White){draw.DrawText2D(Float2(24.f,py),text,c);py+=18;};char text[256];
        line("GRANITE OUTCROP V5.9 - FRACTURED MASS / BURIED SADDLES",Colors::Cyan);line("F2 placement | WASD walk | Shift run | Space jump | C crouch | RMB look | F harvest | R reset | H old fixtures");
        std::snprintf(text,sizeof(text),"Landscape: %d/%d tiles | %llu tris | publish %.2f ms | F10 creek view/return | outer terrain: walking only",state.landscapeNext,PlayableLandscape::Terrain().TileCount(),(unsigned long long)state.landscapeTriangles,state.landscapePublishMs);line(text,Colors::Cyan);
        std::snprintf(text,sizeof(text),"F11 movement capture: %s | %.1f/30 s | terrain setup %.2f ms (one-time)",state.movementCapture.running?"RECORDING":state.movementCapture.wrote?"CSV SAVED":state.movementCapture.writeAttempted?"CSV WRITE FAILED":"READY",state.movementCapture.seconds,state.landscapeSetupMs);line(text,Colors::Cyan);
        if(state.movementCapture.finished)
        {
            auto const& m=state.movementCapture.moving;auto const& idle=state.movementCapture.idle;
            std::snprintf(text,sizeof(text),"Move/view: n=%llu p95 %.2f p99 %.2f max %.2f ms | idle n=%llu p95 %.2f | actions separate",(unsigned long long)m.count,m.p95,m.p99,m.maximum,(unsigned long long)idle.count,idle.p95);line(text);
        }
        std::snprintf(text,sizeof(text),"Grass: %llu worn | %llu pending | %llu blade tufts / %.2f ms | cover %.2f ms | regrowth %.1fx",(unsigned long long)state.grass.active.size(),(unsigned long long)state.grass.pending.size(),(unsigned long long)state.grassBladeClusters,state.grassBladeMs,state.grassMs,settings.GetGrassTimeScale());line(text);
        std::snprintf(text,sizeof(text),"Meadow: %llu tufts | %llu/%d resident patches | upload %.2f ms | 20 m neighborhood",(unsigned long long)state.meadowTufts,(unsigned long long)state.meadowPatches.size(),LandscapeGrass::MaxPatches,state.meadowPublishMs);line(text);
        std::snprintf(text,sizeof(text),"F8 grass cast shadows: %s | F9 ABBA frame test%s",state.grassShadows?"ON":"OFF",state.shadowBenchmark.Running()?" (F9 cancels; keep camera still)":"");line(text,Colors::Cyan);
        if(state.shadowBenchmark.Running())
        {
            std::snprintf(text,sizeof(text),"Shadow test leg %d/4: %.1f/10 s | %s",state.shadowBenchmark.phase+1,state.shadowBenchmark.elapsed,benchmarkReady?"settle 3 s / sample 7 s":"waiting for idle scene");line(text);
        }
        else if(state.shadowBenchmark.finished)
        {
            auto const& off=state.shadowBenchmark.offResult;auto const& on=state.shadowBenchmark.onResult;
            std::snprintf(text,sizeof(text),"Shadows OFF/ON mean %.2f/%.2f | p95 %.2f/%.2f | p99 %.2f/%.2f ms | %s",off.mean,on.mean,off.p95,on.p95,off.p99,on.p99,state.shadowBenchmark.pass?"WITHIN FRAME BUDGET":"OVER BUDGET");line(text);
            line(state.shadowBenchmark.wrote?"Frame samples: GrassShadowBenchmark.csv | opt-in pending visual/uncapped validation":"CSV not written | opt-in pending visual/uncapped validation");
        }
        std::snprintf(text,sizeof(text),"Eye %.2f m | body %.2f m | rock crest %.2f m | 2 m reach | %s",state.walker.Eye(),state.walker.Height(),state.body.high[2],state.walker.grounded?"GROUNDED":"AIRBORNE");line(text);
        char const* contacted=aim.material==GraniteLabContact::Material::Host?"GRANITE":aim.material==GraniteLabContact::Material::Soil?"SOIL":aim.material==GraniteLabContact::Material::Loose?"LOOSE GRANITE":"none";
        std::snprintf(text,sizeof(text),"AUTO: %s | contact %s %.2f m | attempts %u / contacts %u",aim.material==GraniteLabContact::Material::Soil?"SHOVEL 8.5 x 2.85 inches":"BASIC PICK",contacted,hit.hit?hit.distance:0,state.attempts,state.contacts);line(text);line(state.receipt.status);
        std::snprintf(text,sizeof(text),"Soil: last %.3f L / %.3f kg | recovered %.3f L / %.3f kg | %u scoops | worker %.2f ms",state.scoop.volume*1000,double(state.scoop.massMg)/1e6,state.terrain.removedM3*1000,double(state.terrain.removedMg)/1e6,state.terrain.scoops,state.scoopMs);line(text,Colors::Cyan);
        std::snprintf(text,sizeof(text),"Soil publish: %.2f ms | %llu patches / %llu triangles / %llu vertices | %llu volume chunks",state.soilUploadMs,(unsigned long long)state.soilUpdateCount,(unsigned long long)state.soilUploadTriangles,(unsigned long long)state.soilUploadVertices,(unsigned long long)state.terrain.chunks.size());line(text);
        size_t movingSoil=0;double looseVolume=0;for(auto const& d:state.terrain.loose){if(!d.settled&&d.volume>1e-12)++movingSoil;looseVolume+=d.volume;}
        std::snprintf(text,sizeof(text),"Soil slough: %.3f L retained | %llu moving | support %.2f ms | motion %.2f ms",looseVolume*1000,(unsigned long long)movingSoil,state.scoop.supportMs,state.soilMotionMs);line(text);
        std::snprintf(text,sizeof(text),"Frame %.2f ms | moving peak %.2f ms | last action peak %.2f ms",double(ctx.GetDeltaTime())*1000,state.movingFramePeakMs,state.actionPeakMs);line(text);
        std::snprintf(text,sizeof(text),"Last chip %.1f g / %.1f cm3 | recovered %.1f g | pieces %llu | conservation %s",double(state.receipt.chip.massMg)/1000,state.receipt.chip.volume*1e6,double(state.body.removedMg)/1000,(unsigned long long)state.debris.size(),state.conserved?"PASS":"FAIL");line(text,state.conserved?Colors::Cyan:Colors::Red);
        line("Varied fracture faces + roundovers | inherited exterior | automatic support release (lab calibration)");
        std::snprintf(text,sizeof(text),"Last support: %llu falling bodies | %llu failed links | %llu remnants culled | credited %.3f g",(unsigned long long)state.receipt.detached.size(),(unsigned long long)state.receipt.failedLinks,(unsigned long long)state.receipt.culledRemnants,state.receipt.absorbedM3*G::Density*1000);line(text);
        if(state.fracture.valid()||state.shovel.valid())
        {
            line("Preparing captured contact (yellow marker)... camera controls remain active",Colors::Yellow);
            auto p=state.capturedContact.position;
            for(int axis=0;axis<3;++axis){auto a=p,b=p;a[axis]-=.015;b[axis]+=.015;draw.DrawLine(Vector(World(a)),Vector(World(b)),Colors::Yellow);}
        }
        else if(state.queuedAction)line("Strike input queued; it will start after surface publication",Colors::Yellow);
        std::snprintf(text,sizeof(text),"CPU ms: ray %.2f | walk %.2f (%llu collision probes) | last mesh %.2f",state.aimMs,state.walkMs,(unsigned long long)state.walkQueries,state.meshMs);line(text);
        size_t active=0;for(auto const& d:state.debris)if(!d.settled)++active;
        size_t chipRenderMeshes=0;for(auto id:state.settledChipMeshes)if(id)++chipRenderMeshes;for(auto id:state.chipMeshes)if(id)++chipRenderMeshes;
        std::snprintf(text,sizeof(text),"CPU ms: debris %.2f | poses %.2f | active %llu / %llu | render meshes %llu | sleep tris %llu",state.physicsMs,state.poseMs,(unsigned long long)active,(unsigned long long)state.debris.size(),(unsigned long long)chipRenderMeshes,(unsigned long long)state.settledChipTriangles);line(text);
        std::snprintf(text,sizeof(text),"Traversal peak ms: walk %.2f | chip brush %.2f (%llu wakes) | debris %.2f (%llu rock wakes) | poses %.2f",state.walkPeakMs,state.brushPeakMs,(unsigned long long)state.brushWakes,state.physicsPeakMs,(unsigned long long)state.rockRevisionWakes,state.posePeakMs);line(text);
        std::snprintf(text,sizeof(text),"Last rock update: %llu / %llu local groups",(unsigned long long)state.lastPatchCount,(unsigned long long)state.rockPatches.size());line(text);
        std::snprintf(text,sizeof(text),"Worker ms: prepare %.1f / support %.1f | modified matter + cache %.1f MiB",state.receipt.prepareMs,state.receipt.supportMs,double(state.receipt.modifiedBytes)/1048576);line(text);
        std::snprintf(text,sizeof(text),"Support ms: cache %.1f | graph %.1f | load %.1f | nodes %llu",state.receipt.supportCacheMs,state.receipt.supportGraphMs,state.receipt.supportSolveMs,(unsigned long long)state.receipt.supportNodes);line(text);
        if(state.inspect)
        {
            int loose=aim.loose;
            if(loose>=0)
            {
                auto const& d=state.debris[loose];
                std::snprintf(text,sizeof(text),"V inspect: LOOSE #%d | %s | soil clearance %.1f mm",d.chip.id,!d.settled?"MOVING":d.restingOnRock?"SLEEP on ROCK":"SLEEP on SOIL",-Motion::RequiredLift(d,&state.terrain)*1000);line(text,Colors::Yellow);
            }
            else line(aim.material==GraniteLabContact::Material::Soil?(hit.id<=-1000?"V inspect: FALLEN SOIL | still shovel-able; not yet harvested":"V inspect: SOIL | shovel scoop; stops at granite and test floor"):hit.hit?"V inspect: HOST granite | support evaluated after each removing strike":"V inspect: no surface within 2 m",Colors::Yellow);
        }
        else line("V: inspect a suspected floater (host vs loose chip)");
        if(!settings.GetSoilMaterial()||!settings.GetGraniteMaterial())line("Waiting for granite/soil material resources",Colors::Yellow);
        return true;
    }
}

#endif // EE_DEVELOPMENT_TOOLS
