#pragma once
#include "SoilScoop.h"

// Surface ecology is independent of the excavatable soil volume and ledger.
// One height-qualified cover layer per XY cell: roofs hide tunnel interiors.
namespace EE::GrassCover
{
    namespace S=SoilScoop;namespace G=GraniteContactCast;
    inline constexpr int Resolution=128;
    inline constexpr int BladePatchSize=16,BladePatchesPerAxis=Resolution/BladePatchSize;
    inline constexpr double Width=S::S::NX*S::S::Step,Length=S::S::NY*S::S::Step;
    inline uint32_t BladeHash(uint32_t value)
    {value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;value^=value>>16;return value;}
    inline double BladeUnit(uint32_t value){return double(value&0xffffu)/65535.;}
    struct Settings {double wearPerMeter=.20,recoveryDelay=120,recoverySeconds=1200,footRadius=.19;};
    struct Cell {float cover=0,height=0,originalHeight=0,eligible=0;double disturbed=0;};
    struct Tuft {G::P center{};double angle=0,width=0,height=0,vigor=0,moisture=0,slope=0;bool emitted=false;};
    inline bool BaseClear(G::SurfaceQuery const& granite,Tuft const& tuft,double clearance=.006)
    {
        if(!tuft.emitted||granite.nodes.empty())return true;
        auto const& root=granite.nodes[0];double half=tuft.width*.5+clearance;
        if(tuft.center[0]+half<root.lo[0]||tuft.center[0]-half>root.hi[0]||
            tuft.center[1]+half<root.lo[1]||tuft.center[1]-half>root.hi[1]||
            tuft.center[2]+clearance<root.lo[2]||tuft.center[2]-clearance>root.hi[2])return true;
        auto clearSegment=[&](double angle)
        {
            double tx=-std::sin(angle),ty=std::cos(angle),cardHalf=tuft.width*.5;
            G::P a={tuft.center[0]-tx*cardHalf,tuft.center[1]-ty*cardHalf,tuft.center[2]+.004};
            G::P b={tuft.center[0]+tx*cardHalf,tuft.center[1]+ty*cardHalf,tuft.center[2]+.004};
            bool blocked=false;granite.VisitCapsule(0,a,b,clearance*clearance,[&](G::SurfaceQuery::Tri const& triangle)
            {if(G::SegmentTriangleDistance2(a,b,triangle.a,triangle.b,triangle.c)<clearance*clearance)blocked=true;return !blocked;});
            return !blocked&&!G::Contains(granite,G::Mul(G::Add(a,b),.5));
        };
        // Only the two crossed root lines are granite-exclusive. The cards
        // above them may lean over or through the outcrop naturally.
        return clearSegment(tuft.angle)&&clearSegment(tuft.angle+1.570796326794897);
    }
    struct Model
    {
        std::vector<Cell> cells;std::set<int> active,pending,bladeDirty;Settings settings;
        double time=0;bool dirty=true;
        G::P Position(int id)const{return {S::S::X+(id%Resolution+.5)*Width/Resolution,S::S::Y+(id/Resolution+.5)*Length/Resolution,0};}
        Tuft Blade(int id)const
        {
            Tuft result;if(id<0||id>=int(cells.size()))return result;auto const& cell=cells[id];uint32_t h=BladeHash(uint32_t(id)+0x9e3779b9u);
            if(!cell.eligible||(h&1u))return result;
            result.center=Position(id);result.center[2]=cell.height;
            double cellWidth=Width/Resolution,cellLength=Length/Resolution;
            result.center[0]+=(BladeUnit(BladeHash(h^0x34b91a7du))-.5)*cellWidth*.90;
            result.center[1]+=(BladeUnit(BladeHash(h^0xc2b2ae35u))-.5)*cellLength*.90;
            // Habitat varies continuously in world space, independently of
            // the coverage lattice. Sheltered/moister ground biases a tuft
            // taller and greener; exposed slopes stay shorter without ever
            // leaving the requested twelve-to-eighteen-inch range.
            double probe=.08,x=result.center[0],y=result.center[1];
            result.slope=std::hypot(S::S::Height(x+probe,y)-S::S::Height(x-probe,y),S::S::Height(x,y+probe)-S::S::Height(x,y-probe))/(probe*2);
            result.moisture=std::clamp(.52+.24*std::sin(x*.57+y*.23+.81)+.14*std::sin(x*.19-y*.61-1.37),0.,1.);
            double shelter=1-std::clamp(result.slope/.75,0.,1.),variation=BladeUnit(BladeHash(h^0x68bc21ebu));
            result.vigor=std::clamp(.40*variation+.38*result.moisture+.22*shelter,0.,1.);
            result.width=.115+.045*BladeUnit(h>>1);result.height=.3048+.1524*result.vigor;
            result.angle=BladeUnit(BladeHash(h^0xa511e9b3u))*6.283185307179586;result.emitted=true;return result;
        }
        void Reset()
        {
            cells.assign(Resolution*Resolution,{});active.clear();pending.clear();bladeDirty.clear();time=0;dirty=true;
            for(int i=0;i<int(cells.size());++i){auto p=Position(i);double h=0;S::S::Surface(p[0],p[1],h);cells[i]={1,float(h),float(h),1,0};}
            for(int i=0;i<BladePatchesPerAxis*BladePatchesPerAxis;++i)bladeDirty.insert(i);
        }
        void ExposeRockMargin(G::SurfaceQuery const& granite)
        {
            if(granite.nodes.empty())return;
            auto const& root=granite.nodes[0];
            for(int i=0;i<int(cells.size());++i)
            {
                auto& cell=cells[i];if(!cell.eligible)continue;auto p=Position(i);p[2]=cell.height;
                constexpr double maximum=.30;double box=S::BoxDistance2(p,root.lo,root.hi);if(box>maximum*maximum)continue;
                double distance2=maximum*maximum;S::Closest(granite,0,p,distance2);double distance=std::sqrt(distance2);
                uint32_t h=BladeHash(uint32_t(i)^0x6d2b79f5u);
                // Patch-scale habitat makes soil fingers approach the stone in
                // some places and retreat in others. Avoid a constant-width
                // procedural moat while preserving exposed mineral pockets.
                double macro=std::clamp(.50+.27*std::sin(p[0]*1.31-p[1]*.83+.4)
                    +.23*std::sin(p[0]*.57+p[1]*1.67-1.2),0.,1.);
                double inner=.008+.038*macro*macro+.008*BladeUnit(h);
                double outer=inner+.025+.180*macro*macro*macro
                    +.020*BladeUnit(BladeHash(h^0x85ebca6bu));
                double t=std::clamp((distance-inner)/std::max(.001,outer-inner),0.,1.);t=t*t*(3-2*t);
                cell.cover=float(std::min<double>(cell.cover,t));
            }
            dirty=true;
        }
        template<class F> void Area(double loX,double loY,double hiX,double hiY,F f)
        {
            int x0=std::max(0,int(std::floor((loX-S::S::X)/Width*Resolution))),x1=std::min(Resolution-1,int(std::floor((hiX-S::S::X)/Width*Resolution)));
            int y0=std::max(0,int(std::floor((loY-S::S::Y)/Length*Resolution))),y1=std::min(Resolution-1,int(std::floor((hiY-S::S::Y)/Length*Resolution)));
            for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)f(x+y*Resolution);
        }
        void QueueTerrain(S::Body const& terrain)
        {
            // Coarse patch IDs are not an excuse to query their entire area:
            // sparse changed volume chunks already bound the affected surface.
            for(auto const& e:terrain.chunks)if(terrain.dirty.count(S::ID(e.first)))
            {auto p=S::Low(e.first);Area(p[0]-.1,p[1]-.1,p[0]+S::Span+.1,p[1]+S::Span+.1,[&](int i){pending.insert(i);});}
        }
        void Refresh(S::Body const& terrain,int budget=48)
        {
            while(budget-->0&&!pending.empty())
            {
                int i=*pending.begin();pending.erase(pending.begin());auto p=Position(i);p[2]=S::Top+.01;double reach=S::Top-S::Bottom+.02;G::Hit hit;
                // Deliberately exclude loose bodies. Only stable host soil gets
                // this cover material; clods use the separate bare-soil material.
                terrain.Trace(p,{0,0,-1},reach,hit,false);
                auto& c=cells[i];float eligible=hit.hit&&hit.normal[2]>.55&&hit.position[2]>S::Bottom+.005?1.f:0.f;
                float height=hit.hit?float(hit.position[2]):float(S::Bottom);
                bool changed=std::abs(height-c.height)>.003f||eligible!=c.eligible;
                if(!changed)continue;
                // Meshing an edited volume chunk replaces a coarse top quad by
                // finer triangles. That small interpolation difference is not
                // excavation and must not stamp a square bare patch into grass.
                bool intactTop=hit.hit&&std::abs(height-c.originalHeight)<float(S::Step*1.25)&&
                    terrain.Field({p[0],p[1],double(c.originalHeight)-S::Step*.4})>0;
                if(intactTop)eligible=c.eligible;c.height=height;c.eligible=eligible;dirty=true;
                bladeDirty.insert((i%Resolution)/BladePatchSize+(i/Resolution)/BladePatchSize*BladePatchesPerAxis);
                if(!intactTop){c.cover=0;c.disturbed=time;if(eligible)active.insert(i);else active.erase(i);}
            }
        }
        void Travel(G::P from,G::P to,bool grounded)
        {
            if(!grounded||!G::Finite(from)||!G::Finite(to))return;
            double dx=to[0]-from[0],dy=to[1]-from[1],distance=std::hypot(dx,dy);
            if(distance<1e-7||distance>1)return; // Standing, falling and teleports are not foot traffic.
            int steps=std::max(1,int(std::ceil(distance/.04)));double portion=distance/steps,r=settings.footRadius;
            for(int s=0;s<steps;++s)
            {
                double t=(s+.5)/steps;auto p=G::Add(from,G::Mul(G::Add(to,G::Mul(from,-1)),t));
                Area(p[0]-r,p[1]-r,p[0]+r,p[1]+r,[&](int i)
                {
                    auto q=Position(i);double radial=std::hypot(q[0]-p[0],q[1]-p[1])/r;auto& c=cells[i];
                    if(radial>=1||!c.eligible||std::abs(p[2]-c.height)>.12)return;
                    float next=float(std::max(0.,c.cover-settings.wearPerMeter*portion*(1-radial*radial)));
                    if(next!=c.cover){c.cover=next;dirty=true;}c.disturbed=time;active.insert(i);
                });
            }
        }
        void Advance(double dt)
        {
            if(!std::isfinite(dt)||dt<=0)return;double previous=time;time+=dt;
            for(auto it=active.begin();it!=active.end();)
            {
                auto& c=cells[*it];double elapsed=std::max(0.,time-std::max(previous,c.disturbed+settings.recoveryDelay));
                float next=c.eligible?float(std::min(1.,c.cover+elapsed/std::max(.001,settings.recoverySeconds))):0;
                if(next!=c.cover){c.cover=next;dirty=true;}
                if(!c.eligible||c.cover>=1)it=active.erase(it);else ++it;
            }
        }
        // RGBA32F: density, world-space surface height, eligibility, reserved.
        std::vector<std::array<float,4>> Pixels(float worldZ)const
        {std::vector<std::array<float,4>> result;result.reserve(cells.size());for(auto const& c:cells)result.push_back({c.cover,c.height+worldZ,c.eligible,0});return result;}
    };
}
