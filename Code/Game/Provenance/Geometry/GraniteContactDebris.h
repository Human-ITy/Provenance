#pragma once
#include "GraniteContactCast.h"
#include "GraniteOutcropGround.h"
#include "SoilScoop.h"
namespace EE::GraniteContactDebris
{
    using namespace GraniteOutcrop;
    using GraniteOutcrop::Hash;
    using Q=std::array<double,4>;
    inline Q Multiply(Q a,Q b){return {a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]};}
    inline P Rotate(Q q,P p){P u={q[1],q[2],q[3]};return Add(p,Add(Mul(Cross(u,p),2*q[0]),Mul(Cross(u,Cross(u,p)),2)));}
    struct FallingChip
    {
        GraniteContactCast::Chip chip;P offset={},velocity={},spin={};Q orientation={1,0,0,0};
        GraniteContactCast::SurfaceQuery collisionQuery;
        std::vector<P> vertices;std::vector<std::vector<uint32_t>> supportFaces;
        double radius=0,clock=0,restTime=0,rockRestTime=0,brushCooldown=0;
        double watchdogClock=0,trappedTime=0;P watchdogPosition={};bool watchdogContact=false,watchdogRock=false;
        P rockSupportPoint={},rockSupportNormal={};
        bool settled=false,restingOnRock=false;uint32_t supportRevision=0,checkedRockRevision=0,soilRevision=0;
    };
    inline P Position(FallingChip const& d,P p){return Add(Add(d.chip.center,d.offset),Rotate(d.orientation,Add(p,Mul(d.chip.center,-1))));}
    inline bool Nudge(FallingChip& d,P direction,P contact)
    {
        if(!GraniteContactCast::Finite(direction)||!GraniteContactCast::Finite(contact)||Dot(direction,direction)<1e-12)return false;
        P ray=Unit(direction),side=Unit(Cross(ray,P{0,0,1}));
        if(Dot(side,side)<.5)side={1,0,0};
        if(d.chip.id&1)side=Mul(side,-1);
        // A bounded tool tap moves the existing body; no re-fracture, duplicate
        // recovery or geometry allocation. Larger collapsed bodies move less.
        double kg=std::max(.02,double(d.chip.massMg)*1e-6),speed=std::clamp(.45/std::sqrt(kg),.12,2.2);
        d.velocity=Add(Mul(d.velocity,.3),Mul(Add(Add(Mul(ray,.45),side),P{0,0,.7}),speed));
        d.spin=Add(Mul(Cross(Add(contact,Mul(Add(d.chip.center,d.offset),-1)),d.velocity),4/std::max(.002,d.radius*d.radius)),P{1,2,1});
        double spinLength=std::sqrt(Dot(d.spin,d.spin));if(spinLength>12)d.spin=Mul(d.spin,12/spinLength);
        d.settled=d.restingOnRock=false;d.restTime=d.rockRestTime=d.watchdogClock=d.trappedTime=0;d.watchdogPosition=Add(d.chip.center,d.offset);d.watchdogContact=d.watchdogRock=false;return true;
    }
    inline bool Brush(FallingChip& d,P from,P to,double bodyRadius,double bodyHeight)
    {
        if(d.brushCooldown>0||!Finite(from)||!Finite(to)||bodyRadius<=0||bodyHeight<bodyRadius*2)return false;
        P travel=Add(to,Mul(from,-1));if(Dot(travel,travel)<1e-8)return false;
        P a=Add(to,{0,0,bodyRadius}),b=Add(to,{0,0,bodyHeight-bodyRadius}),center=Add(d.chip.center,d.offset);
        if(GraniteContactCast::SegmentDistance2(a,b,center,center)>std::pow(bodyRadius+d.radius,2))return false;
        // Radius is only a broad phase. A thin shard can have a meter-wide
        // bounding sphere while occupying almost none of that space; treating
        // the sphere as contact woke chips through walls and from the hill
        // above the excavation, repeatedly rebuilding the settled render batch.
        constexpr double WakeSkin=5e-6;bool touches=false;double wakeRadius=bodyRadius+WakeSkin,limit=wakeRadius*wakeRadius;
        // Transform the capsule into the chip's immutable local frame, then
        // query its BVH. Walking used to transform and test every fracture
        // triangle of every nearby sleeping chip before deciding to wake it.
        Q inverse=d.orientation;for(int k=1;k<4;++k)inverse[k]=-inverse[k];
        auto local=[&](P p){return Add(d.chip.center,Rotate(inverse,Add(p,Mul(center,-1))));};
        P localA=local(a),localB=local(b);
        if(!d.collisionQuery.nodes.empty())d.collisionQuery.VisitCapsule(0,localA,localB,limit,[&](GraniteContactCast::SurfaceQuery::Tri const& triangle)
        {
            if(GraniteContactCast::SegmentTriangleDistance2(localA,localB,triangle.a,triangle.b,triangle.c)<=limit)touches=true;
            return !touches;
        });
        if(!touches)return false;
        P away={center[0]-(a[0]+b[0])*.5,center[1]-(a[1]+b[1])*.5,0};
        if(Dot(away,away)<1e-8)away={travel[0],travel[1],0};if(Dot(away,away)<1e-8)away={1,0,0};away=Unit(away);
        double speed=std::clamp(std::hypot(travel[0],travel[1])*18+.18,.18,.75);
        d.velocity=Add(Mul(d.velocity,.65),Add(Mul(away,speed),P{0,0,.12}));d.spin=Add(d.spin,P{1.1,-.8,.6});
        d.settled=d.restingOnRock=false;d.restTime=d.rockRestTime=d.watchdogClock=d.trappedTime=0;d.watchdogPosition=Add(d.chip.center,d.offset);d.watchdogContact=d.watchdogRock=false;d.brushCooldown=.12;return true;
    }
    inline Poly Geometry(FallingChip const& d)
    {Poly p=d.chip.geometry;for(auto& f:p)for(auto& v:f.p)v=Position(d,v);return p;}
    inline double CurvedGroundLift(FallingChip const& d,SoilScoop::Body const* terrain=nullptr)
    {
        namespace S=GraniteOutcropGround;
        struct SoilTriangle{P a,b,c;double gx,gy,h;};
        static auto const soil=[]()
        {
            std::vector<SoilTriangle> out;out.reserve(S::NX*S::NY*2);
            for(int y=0;y<S::NY;++y)for(int x=0;x<S::NX;++x)
            {
                auto q=S::Quad(x,y);for(int half=0;half<2;++half)
                {
                    P a=q[0],b=q[half?2:1],c=q[half?3:2],n=Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1)));
                    double gx=-n[0]/n[2],gy=-n[1]/n[2];out.push_back({a,b,c,gx,gy,a[2]-gx*a[0]-gy*a[1]});
                }
            }return out;
        }();
        std::vector<P> vertices;vertices.reserve(d.vertices.size());for(P p:d.vertices)vertices.push_back(Position(d,p));
        std::vector<P> polygon,scratch;polygon.reserve(64);scratch.reserve(64);double lift=-1e30;
        auto clip=[&](P a,P b)
        {
            scratch.clear();if(polygon.empty())return;
            for(size_t i=0;i<polygon.size();++i)
            {
                P p=polygon[i],q=polygon[(i+1)%polygon.size()];double dp=S::Side(a,b,p),dq=S::Side(a,b,q);
                if(dp>=-1e-12)scratch.push_back(p);
                if((dp<0&&dq>0)||(dp>0&&dq<0))scratch.push_back(Add(p,Mul(Add(q,Mul(p,-1)),dp/(dp-dq))));
            }polygon.swap(scratch);
        };
        // Faces are convex clipping results. Clip the whole face rather than
        // repeatedly clipping its triangle fan. The same linear maximum applies.
        for(auto const& face:d.supportFaces)
        {
            P lo={1e30,1e30,1e30},hi={-1e30,-1e30,-1e30};double area=0;
            for(size_t i=0;i<face.size();++i){P p=vertices[face[i]];for(int k=0;k<2;++k){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}if(i>0&&i+1<face.size())area+=S::Side(vertices[face[0]],p,vertices[face[i+1]]);}
            if(std::abs(area)<1e-14)continue;
            int lx=std::max(0,int(std::floor((lo[0]-S::X)/S::Step))),ux=std::min(S::NX-1,int(std::floor((hi[0]-S::X)/S::Step)));
            int ly=std::max(0,int(std::floor((lo[1]-S::Y)/S::Step))),uy=std::min(S::NY-1,int(std::floor((hi[1]-S::Y)/S::Step)));
            auto test=[&](P a,P b,P c)
            {
                polygon.clear();for(auto id:face)polygon.push_back(vertices[id]);clip(a,b);clip(b,c);clip(c,a);
                for(P p:polygon)lift=std::max(lift,S::PlaneHeight(a,b,c,p)-p[2]);
            };
            if(terrain)
            {
                P center=Add(d.chip.center,d.offset);
                terrain->Triangles({lo[0],lo[1],SoilScoop::Bottom},{hi[0],hi[1],center[2]},[&](auto const& t)
                {
                    if(t.n[2]<=.01)return;
                    // A tunnel ceiling must not act as a floor above a chip.
                    polygon.clear();for(auto id:face)polygon.push_back(vertices[id]);clip(t.a,t.b);clip(t.b,t.c);clip(t.c,t.a);
                    for(P p:polygon){double ground=S::PlaneHeight(t.a,t.b,t.c,p);if(ground<=center[2]+.001)lift=std::max(lift,ground-p[2]);}
                });
            }
            else for(int y=ly;y<=uy;++y)for(int x=lx;x<=ux;++x)
            {
                for(int half=0;half<2;++half){auto const& t=soil[(y*S::NX+x)*2+half];test(t.a,t.b,t.c);}
            }
        }return lift;
    }
    inline double RequiredLift(FallingChip const& d,SoilScoop::Body const* terrain=nullptr)
    {
        namespace S=GraniteOutcropGround;P center=Add(d.chip.center,d.offset);
        if(terrain&&terrain->Edited(center[0],center[1],d.radius))return CurvedGroundLift(d,terrain);
        int lx=int(std::floor((center[0]-d.radius-S::X)/S::Step)),ux=int(std::floor((center[0]+d.radius-S::X)/S::Step));
        int ly=int(std::floor((center[1]-d.radius-S::Y)/S::Step)),uy=int(std::floor((center[1]+d.radius-S::Y)/S::Step));
        if(lx>=0&&ly>=0&&ux<S::NX&&uy<S::NY)
        {
            auto q=S::Quad(lx,ly);P n=Unit(Cross(Add(q[1],Mul(q[0],-1)),Add(q[3],Mul(q[0],-1))));bool planar=true;
            for(int y=ly;y<=uy+1&&planar;++y)for(int x=lx;x<=ux+1;++x)
            {
                P p={S::X+x*S::Step,S::Y+y*S::Step,0};p[2]=S::Height(p[0],p[1]);
                if(std::abs(Dot(n,Add(p,Mul(q[0],-1))))>1e-10){planar=false;break;}
            }
            if(planar)
            {
                // A plane's maximum separation from a polygon is attained at
                // a vertex. Exact support, without copying/clipping all faces.
                double lift=-1e30;for(P v:d.vertices){P p=Position(d,v);lift=std::max(lift,-Dot(n,Add(p,Mul(q[0],-1)))/n[2]);}return lift;
            }
        }
        return CurvedGroundLift(d);
    }
    inline FallingChip Launch(GraniteContactCast::Chip chip,P normal)
    {
        FallingChip d;d.chip=std::move(chip);d.collisionQuery.Build(d.chip.geometry);auto const& matter=d.chip;uint32_t seed=Hash(uint32_t(matter.id)^uint32_t(std::llround(matter.center[0]*10000))^uint32_t(std::llround(matter.center[2]*10000)));
        auto random=[&](){return GraniteContactCast::Random(seed);};
        P tangent=Unit(Cross(normal,P{0,0,1}));
        d.velocity=Add(Add(Mul(normal,.75+.5*random()),Mul(tangent,(random()-.5)*.55)),P{0,0,.05+.18*random()});
        d.spin={3+random()*5,-4+random()*8,-4+random()*8};
        std::map<P,uint32_t> unique;
        d.watchdogPosition=matter.center;
        for(auto const& f:matter.geometry)
        {
            std::vector<uint32_t> face;
            for(P p:f.p){auto found=unique.find(p);if(found==unique.end()){uint32_t id=uint32_t(d.vertices.size());unique.emplace(p,id);d.vertices.push_back(p);face.push_back(id);auto v=Add(p,Mul(matter.center,-1));d.radius=std::max(d.radius,std::sqrt(Dot(v,v)));}else face.push_back(found->second);}
            d.supportFaces.push_back(std::move(face));
        }return d;
    }
    inline bool RockSupportRemains(FallingChip const& d,GraniteContactCast::Body const& body)
    {
        if(Dot(d.rockSupportNormal,d.rockSupportNormal)<.5)return false;
        constexpr double skin=.002,reach=.0045;
        auto hit=GraniteContactCast::Raycast(body,Add(d.rockSupportPoint,Mul(d.rockSupportNormal,skin)),Mul(d.rockSupportNormal,-1),reach);
        return hit.hit&&hit.distance<=skin+.001&&Dot(hit.normal,d.rockSupportNormal)>.45;
    }
    inline bool Advance(FallingChip& d,GraniteContactCast::Body const& body,double dt,SoilScoop::Body const* terrain=nullptr,bool allowRockWake=true)
    {
        d.brushCooldown=std::max(0.,d.brushCooldown-std::max(0.,dt));
        if(terrain&&d.soilRevision!=terrain->revision)
        {
            // Settled piles are deliberately quiet scenery. A nearby shovel
            // scoop used to wake every soil-resting granite chip over any
            // previously edited terrain (33 bodies in one captured frame).
            // Active/new chips still collide with current terrain, and an
            // explicit tool tap remains the authority for waking a settled one.
            d.soilRevision=terrain->revision;
        }
        if(d.settled&&d.restingOnRock&&d.checkedRockRevision!=body.revision)
        {
            uint32_t local=GraniteContactCast::LocalRevision(body,Add(d.chip.center,d.offset),d.radius);
            if(d.supportRevision==local||RockSupportRemains(d,body))
            {d.checkedRockRevision=body.revision;d.supportRevision=local;}
            else if(allowRockWake)
            {
                d.checkedRockRevision=body.revision;d.settled=false;d.restingOnRock=false;
                d.restTime=d.rockRestTime=d.watchdogClock=d.trappedTime=0;
                d.watchdogPosition=Add(d.chip.center,d.offset);d.watchdogContact=d.watchdogRock=false;
            }
        }
        if(d.settled||!std::isfinite(dt)||dt<=0)return false;
        constexpr double stepTime=1./120;d.clock+=std::min(dt,.1);bool moved=false;
        while(d.clock+1e-12>=stepTime&&!d.settled)
        {
            d.clock-=stepTime;moved=true;d.velocity[2]-=9.81*stepTime;P step=Mul(d.velocity,stepTime);bool rockSupport=false,contacted=false;
            for(int pass=0;pass<4;++pass)
            {
                double length=std::sqrt(Dot(step,step));if(length<1e-10)break;
                auto hit=GraniteContactCast::Raycast(body,Add(d.chip.center,d.offset),step,length);
                bool soilHit=false;
                P centerBefore=Add(d.chip.center,d.offset);
                bool editedTerrain=terrain&&terrain->Edited(centerBefore[0],centerBefore[1],d.radius+length);
                if(editedTerrain)
                {
                    double nearest=hit.hit?hit.distance:length+1e-8;GraniteContactCast::Hit soil;
                    terrain->Trace(Add(d.chip.center,d.offset),Mul(step,1/length),nearest,soil);
                    if(soil.hit){hit=soil;soilHit=true;}
                }
                if(hit.hit&&Dot(d.velocity,hit.normal)<0)
                {
                    contacted=true;
                    double fraction=std::clamp(hit.distance/length,0.,1.);d.offset=Add(Add(d.offset,Mul(step,fraction)),Mul(hit.normal,.0005));
                    // Keep the unconsumed tangential motion. Previously it was
                    // discarded at every wall hit, pinning chips in place.
                    step=Mul(step,1-fraction);step=Add(step,Mul(hit.normal,-std::min(0.,Dot(step,hit.normal))));
                    d.velocity=Add(d.velocity,Mul(hit.normal,-1.05*Dot(d.velocity,hit.normal)));d.spin=Mul(d.spin,.85);
                    bool supportedByRock=!soilHit&&hit.normal[2]>.55;rockSupport=rockSupport||supportedByRock;
                    if(supportedByRock){d.rockSupportPoint=hit.position;d.rockSupportNormal=hit.normal;}
                    if(hit.normal[2]>.55){d.velocity=Mul(d.velocity,.8);step=Mul(step,.8);}
                }
                else {d.offset=Add(d.offset,step);break;}
            }
            if(rockSupport&&Dot(d.velocity,d.velocity)<.025)d.rockRestTime+=stepTime;else d.rockRestTime=0;
            if(d.rockRestTime>.2)
            {
                d.settled=d.restingOnRock=true;d.checkedRockRevision=body.revision;
                d.supportRevision=GraniteContactCast::LocalRevision(body,Add(d.chip.center,d.offset),d.radius);
                d.velocity={};d.spin={};continue;
            }
            double speed=std::sqrt(Dot(d.spin,d.spin));if(speed>1e-8){double a=speed*stepTime*.5;auto w=Mul(d.spin,std::sin(a)/speed);d.orientation=Multiply(Q{std::cos(a),w[0],w[1],w[2]},d.orientation);double norm=0;for(double v:d.orientation)norm+=v*v;for(double& v:d.orientation)v/=std::sqrt(norm);}
            // Cheap conservative air test avoids full footprint clipping while
            // above terrain. Near contact use the actual rotated cast geometry.
            auto surface=[&](double x,double y,double& h)
            {
                return terrain&&terrain->Edited(x,y,d.radius)?terrain->SurfaceBelow(x,y,d.chip.center[2]+d.offset[2],h):GraniteOutcropGround::Surface(x,y,h);
            };
            P center=Add(d.chip.center,d.offset);double ground=0;surface(center[0],center[1],ground);
            if(center[2]-d.radius>ground+4*d.radius){d.restTime=0;continue;}
            double lift=RequiredLift(d,terrain);
            if(lift>=-.001)
            {
                contacted=true;
                d.offset[2]+=lift+.001;
                double hx=0,hy=0;surface(center[0]+.005,center[1],hx);surface(center[0],center[1]+.005,hy);
                P n=Unit(P{-(hx-ground)/.005,-(hy-ground)/.005,1});double vn=Dot(d.velocity,n);
                if(vn<0)d.velocity=Add(d.velocity,Mul(n,-1.18*vn));
                P normal=Mul(n,Dot(d.velocity,n));d.velocity=Add(normal,Mul(Add(d.velocity,Mul(normal,-1)),.72));d.spin=Mul(d.spin,.73);
            }
            // Rest detection tolerates the millimetre skin and a tiny rebound;
            // requiring contact every step left low-energy chips awake forever.
            if(lift>=-.003&&Dot(d.velocity,d.velocity)<.025&&Dot(d.spin,d.spin)<.25)d.restTime+=stepTime;else d.restTime=0;
            if(d.restTime>.15){d.offset[2]+=RequiredLift(d,terrain)+.001;d.settled=true;d.velocity={};d.spin={};}
            // A chip trapped between two surfaces can retain enough jitter to
            // miss the ordinary low-speed sleep threshold forever.  Detect a
            // contacted body that makes no net progress over several one-second
            // windows and put that existing body to sleep in place.
            d.watchdogClock+=stepTime;d.watchdogContact=d.watchdogContact||contacted;d.watchdogRock=d.watchdogRock||rockSupport;
            if(d.watchdogClock>=1.)
            {
                P position=Add(d.chip.center,d.offset),delta=Add(position,Mul(d.watchdogPosition,-1));
                if(d.watchdogContact&&Dot(delta,delta)<.01)d.trappedTime+=d.watchdogClock;else d.trappedTime=0;
                d.watchdogPosition=position;d.watchdogClock=0;d.watchdogContact=false;
                if(d.trappedTime>=3.)
                {
                    double finalLift=RequiredLift(d,terrain);if(finalLift>0)d.offset[2]+=finalLift+.001;
                    d.settled=true;d.restingOnRock=d.watchdogRock;d.checkedRockRevision=body.revision;
                    d.supportRevision=d.restingOnRock?GraniteContactCast::LocalRevision(body,Add(d.chip.center,d.offset),d.radius):0;
                    d.velocity={};d.spin={};
                }
                d.watchdogRock=false;
            }
        }return moved;
    }
}
