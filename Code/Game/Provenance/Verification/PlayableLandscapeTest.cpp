#include "../Geometry/PlayableLandscape.h"
#include "../Geometry/GraniteLabMovement.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>

namespace L=EE::PlayableLandscape;
static size_t checks=0;
static void Check(bool value,char const* label){++checks;if(!value){std::printf("FAIL: %s\n",label);std::exit(1);}}
int main()
{
    auto began=std::chrono::steady_clock::now();auto const& g=L::Terrain();
    double buildMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
    Check(std::abs((L::MaxX-L::MinX)*(L::MaxY-L::MinY)-10000)<1e-8,"whole 100m floor");
    double area=0,minHeight=1e9,maxHeight=-1e9;size_t tris=0,bare=0,green=0,renderedMudVertices=0;
    for(int y=0;y<g.NY();++y)for(int x=0;x<g.NX();++x)
    {
        if(!g.Cell(x,y))continue;
        area+=(g.xs[x+1]-g.xs[x])*(g.ys[y+1]-g.ys[y]);tris+=2;
        for(double u:{.19,.73})for(double v:{.23,.81})
        {
            double px=g.xs[x]+u*(g.xs[x+1]-g.xs[x]),py=g.ys[y]+v*(g.ys[y+1]-g.ys[y]),z=0;
            Check(g.Surface(px,py,z),"no missing exterior cell");
            auto a=g.Vertex(x,y),b=g.Vertex(x+1,y),c=g.Vertex(x+1,y+1),d=g.Vertex(x,y+1);
            double rendered=u>=v?a[2]+u*(b[2]-a[2])+v*(c[2]-b[2]):a[2]+u*(c[2]-d[2])+v*(d[2]-a[2]);
            Check(std::abs(z-rendered)<1e-10,"collision/render triangle equality");
        }
        auto r=g.values[g.ID(x,y)];auto n=g.Normal(x,y);
        Check(std::isfinite(r.height)&&r.height+L::OriginZ>0,"never intersects editor floor");
        Check(r.cover>=0&&r.cover<=1&&n[2]>0,"valid cover/upward normals");
        Check(r.mud>=0&&r.mud<=1&&r.mud<=1-r.cover+1e-9,"mud confined to exposed ground");
        renderedMudVertices+=r.mud>.25;
        minHeight=std::min(minHeight,r.height);maxHeight=std::max(maxHeight,r.height);
        bare+=r.cover<.1;green+=r.cover>.9;
    }
    Check(std::abs(area-(10000-L::Core::NX*L::Core::NY*L::Core::Step*L::Core::Step))<1e-6,"exact protected core hole");
    Check(tris<720000&&g.TileCount()<=272,"bounded mesh budget");
    Check(bare>10000&&green>10000,"dirt openings and grass-covered ground");
    Check(renderedMudVertices>1000,"interpolation guard retains visible bed deposits");
    double h=0;
    double creekError=0,creekErrorSum=0;size_t creekSamples=0,muddySamples=0,unmuddyBedSamples=0;
    began=std::chrono::steady_clock::now();
    for(int iy=0;iy<2000;++iy)for(int ix=0;ix<=40;++ix)
    {
        double wy=-49.9+iy*.0499,x=L::CreekX(wy)-L::OriginX+(ix-20)*.04,y=wy-L::OriginY;
        double z=0;Check(g.Surface(x,y,z),"creek query");
        double error=std::abs(z-L::Evaluate(x,y).height);creekError=std::max(creekError,error);creekErrorSum+=error;++creekSamples;
        auto recipe=L::Evaluate(x,y);muddySamples+=recipe.mud>.5;
        auto cut=L::Creek(x+L::OriginX,wy);
        double removal=L::ChannelRemoval(cut,x+L::OriginX,wy);
        Check(recipe.mud==0||removal>cut.depth*.45,"mud requires actual lower-channel depression");
        if(ix==20)unmuddyBedSamples+=recipe.mud<.05;
    }
    double probeMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
    std::printf("Creek representation max/mean error %.6f/%.6f m | %zu queries+recipe checks %.3f ms | grid initialization %.3f ms\n",creekError,creekErrorSum/creekSamples,creekSamples,probeMs,buildMs);
    Check(creekError<.025,"creek shape error below 2.5 cm");
    Check(muddySamples>1000&&unmuddyBedSamples>200,"selected mud pockets, not an entirely muddy bed");
    for(int i=0;i<200;++i)
    {
        double wy=-49.5+i*.49;
        for(double offset:{-2.,2.})Check(L::Evaluate(L::CreekX(wy)-L::OriginX+offset,wy-L::OriginY).mud==0,"mud excludes surrounding ground");
        auto creek=L::Creek(L::CreekX(wy),wy);
        Check(creek.width>=.3&&creek.width<=1.2&&creek.depth<=.075+.20*(creek.width-.3)+1e-10,"narrow reaches shallow, wider pools bounded");
    }
    // Test the actual barycentric payload sent to the GPU, not just the recipe
    // at isolated vertices. Both outer banks and the floodplain must stay clean.
    double maxOutsideMud=0;
    for(int i=0;i<2000;++i)for(double sign:{-1.,1.})for(double ratio:{1.,1.05,1.15,1.4,1.9})
    {
        double wy=-49.8+i*.0498;
        auto side=L::Creek(L::CreekX(wy)+sign,wy);
        double px=L::CreekX(wy)-L::OriginX+sign*ratio*side.half*std::sqrt(1+std::pow(L::CreekSlope(wy),2));
        double py=wy-L::OriginY;
        Check(L::Evaluate(px,py).mud==0,"analytic mud excludes bank lip and floodplain");
        int ix=int(std::upper_bound(g.xs.begin(),g.xs.end(),px)-g.xs.begin())-1;
        int iy=int(std::upper_bound(g.ys.begin(),g.ys.end(),py)-g.ys.begin())-1;
        double u=(px-g.xs[ix])/(g.xs[ix+1]-g.xs[ix]),v=(py-g.ys[iy])/(g.ys[iy+1]-g.ys[iy]);
        double a=g.values[g.ID(ix,iy)].mud,b=g.values[g.ID(ix+1,iy)].mud,c=g.values[g.ID(ix+1,iy+1)].mud,d=g.values[g.ID(ix,iy+1)].mud;
        double rendered=u>=v?a+(b-a)*u+(c-b)*v:a+(c-d)*u+(d-a)*v;
        maxOutsideMud=std::max(maxOutsideMud,rendered);
        Check(rendered<1e-9,"rendered mud excludes bank lip and floodplain");
    }
    std::printf("Maximum interpolated mud outside carve: %.9f\n",maxOutsideMud);
    Check(!g.Surface(1,1,h),"never covers original granite/soil");
    Check(!g.Surface(L::MinX-1,0,h),"does not extend beyond editor floor");
    Check(g.Surface(L::MaxX,L::MaxY,h),"includes exact far corner");
    for(int i=0;i<=1000;++i)
    {
        double t=i/1000.;
        for(auto p:{L::P{L::Core::X,L::Core::Y+t*L::Core::NY*L::Core::Step,0},L::P{L::Core::X+L::Core::NX*L::Core::Step,L::Core::Y+t*L::Core::NY*L::Core::Step,0},L::P{L::Core::X+t*L::Core::NX*L::Core::Step,L::Core::Y,0},L::P{L::Core::X+t*L::Core::NX*L::Core::Step,L::Core::Y+L::Core::NY*L::Core::Step,0}})
            Check(std::abs(L::Evaluate(p[0],p[1]).height-L::Core::Height(p[0],p[1]))<1e-10,"unchanged seam elevation");
    }
    auto floor=[&](double x,double y){double z;if(g.Surface(x,y,z)||L::Core::Surface(x,y,z))return z;return -L::OriginZ;};
    // Walk across the creek in both directions at several path stations.
    for(double wy:{-35.,-10.,12.,30.})for(double sign:{-1.,1.})
    {
        double y=wy-L::OriginY,x=L::CreekX(wy)-L::OriginX-2*sign;
        EE::GraniteLabMovement::Body walker{{x,y,floor(x,y)+EE::GraniteLabMovement::Skin}};
        for(int i=0;i<180;++i)EE::GraniteLabMovement::Advance(walker,{sign,0,false,false,false},1./60.,[](auto,double){return false;},floor,[&](double px,double py,double ceiling){double z=floor(px,py);return z<=ceiling?z:-1e30;});
        Check(sign*(walker.feet[0]-x)>4.7,"creek traversal without artificial wall");
        Check(walker.feet[2]>=floor(walker.feet[0],walker.feet[1])-1e-8,"no terrain penetration");
    }
    std::printf("PASS %zu checks | %.3f m2 exterior | %zu triangles | %d tiles | local height %.3f..%.3f | bare/green cells %zu/%zu\n",checks,area,tris,g.TileCount(),minHeight,maxHeight,bare,green);
}
