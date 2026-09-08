#include "../Geometry/GraniteStructuralSupport.h"
#include <meshoptimizer.h>
#include <chrono>
#include <cstdio>
#include <numeric>
namespace G=EE::GraniteContactCast;
namespace S=EE::GraniteStructuralSupport;
int main()
{
    G::Body body;size_t culled=0,detached=0;double credited=0,fallingM3=0;for(int i=0;i<96;++i){uint32_t h=G::Hash(i);auto receipt=G::Commit(body,S::PrepareStrike(body,{2.35+.27*double(h&255)/255,-.7,.95+.24*double((h>>8)&255)/255},{0,1,0}));culled+=receipt.culledRemnants;credited+=receipt.absorbedM3;detached+=receipt.detached.size();for(auto const& chip:receipt.detached)fallingM3+=chip.volume;}
    std::vector<G::Triangle> triangles;size_t totalTriangles=0;
    for(auto const& group:body.renderGroups){auto mesh=G::PatchBoundary(body,group.first);totalTriangles+=mesh.size();if(mesh.size()>triangles.size())triangles=std::move(mesh);}
    std::printf("render groups %zu; total triangles %zu; largest group %zu; remnants culled %zu (%.1f cm3); detached %zu (%.1f cm3); total removed %.1f cm3\n",body.renderGroups.size(),totalTriangles,triangles.size(),culled,credited*1e6,detached,fallingM3*1e6,body.removedM3*1e6);
    size_t maxFaces=0,maxSolids=0,maxCellTriangles=0;int maxCell=-1;
    for(auto const& changed:body.changed)
    {
        auto cellTriangles=G::Triangulate(changed.second.surface,&body).size();
        if(cellTriangles>maxCellTriangles){maxCellTriangles=cellTriangles;maxFaces=changed.second.surface.size();maxSolids=changed.second.solids.size();maxCell=changed.first;}
    }
    std::printf("changed cells %zu; largest cell %d: %zu triangles, %zu faces, %zu solids\n",body.changed.size(),maxCell,maxCellTriangles,maxFaces,maxSolids);
    std::vector<std::array<float,6>> vertices;for(auto const& t:triangles){auto add=[&](G::P p,G::P n){vertices.push_back({float(p[0]),float(p[1]),float(p[2]),float(n[0]),float(n[1]),float(n[2])});};add(t.a,t.na);add(t.b,t.nb);add(t.c,t.nc);}
    std::vector<unsigned> indices(vertices.size());std::iota(indices.begin(),indices.end(),0);
    std::vector<unsigned> remap(vertices.size()),remappedIndices(indices.size());
    size_t remappedVertexCount=meshopt_generateVertexRemap(remap.data(),indices.data(),indices.size(),vertices.data(),vertices.size(),sizeof(vertices[0]));
    std::vector<std::array<float,6>> remappedVertices(remappedVertexCount);
    meshopt_remapIndexBuffer(remappedIndices.data(),indices.data(),indices.size(),remap.data());
    meshopt_remapVertexBuffer(remappedVertices.data(),vertices.data(),vertices.size(),sizeof(vertices[0]),remap.data());
    vertices.swap(remappedVertices);indices.swap(remappedIndices);
    std::printf("vertex remap: %zu -> %zu vertices\n",remap.size(),vertices.size());
    auto optimizeStart=std::chrono::steady_clock::now();
    meshopt_optimizeVertexCache(indices.data(),indices.data(),indices.size(),vertices.size());
    auto cacheMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-optimizeStart).count();
    optimizeStart=std::chrono::steady_clock::now();
    meshopt_optimizeVertexFetch(vertices.data(),indices.data(),indices.size(),vertices.data(),vertices.size(),sizeof(vertices[0]));
    auto fetchMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-optimizeStart).count();
    std::printf("optimization: cache %.3f ms; fetch %.3f ms\n",cacheMs,fetchMs);
    size_t bound=meshopt_buildMeshletsBound(indices.size(),64,64);std::vector<meshopt_Meshlet> meshlets(bound);std::vector<unsigned> mv(bound*64);std::vector<unsigned char> mt(bound*64*3);
    int failures=0;
    for(bool fast:{false,true})
    {
        double ms=0;size_t count=0;for(int i=0;i<8;++i){auto start=std::chrono::steady_clock::now();count=fast?meshopt_buildMeshletsScan(meshlets.data(),mv.data(),mt.data(),indices.data(),indices.size(),vertices.size(),64,64):meshopt_buildMeshletsSpatial(meshlets.data(),mv.data(),mt.data(),indices.data(),indices.size(),vertices[0].data(),vertices.size(),sizeof(vertices[0]),64,64,64,1);ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();}
        std::vector<std::array<unsigned,3>> decoded,expected;for(size_t i=0;i<indices.size();i+=3)expected.push_back({indices[i],indices[i+1],indices[i+2]});
        for(size_t i=0;i<count;++i){auto const& m=meshlets[i];if(m.vertex_count>64||m.triangle_count>64)++failures;for(size_t t=0;t<m.triangle_count;++t){std::array<unsigned,3> tri;for(int k=0;k<3;++k)tri[k]=mv[m.vertex_offset+mt[m.triangle_offset+t*3+k]];decoded.push_back(tri);}}
        std::sort(decoded.begin(),decoded.end());std::sort(expected.begin(),expected.end());if(decoded!=expected)++failures;
        std::printf("%s: %.3f ms; %zu triangles %zu clusters; exact index/attribute identity %s\n",fast?"linear":"spatial",ms/8,triangles.size(),count,decoded==expected?"PASS":"FAIL");
    }
    std::printf("cluster builder: %d failures\n",failures);return failures?1:0;
}
