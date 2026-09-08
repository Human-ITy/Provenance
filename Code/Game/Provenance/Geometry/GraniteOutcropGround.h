#pragma once
#include "GraniteOutcropV2.h"
namespace EE::GraniteOutcropGround
{
    using namespace GraniteOutcrop;
    inline constexpr double Step=.12,X=-4,Y=-4;
    inline constexpr int NX=92,NY=88;
    inline double Smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
    inline double SettledFan(double x,double y,double ax,double ay,double bx,double by,double baseWidth)
    {
        // A tapered alluvial wedge: broad at the washed-out mouth, narrow
        // where it climbs into a granite/satellite crease. Feather both ends
        // so the deposit remains a continuous, walkable heightfield.
        double vx=bx-ax,vy=by-ay,length2=vx*vx+vy*vy,length=std::sqrt(length2);
        double px=x-ax,py=y-ay,t=(px*vx+py*vy)/length2;
        double across=std::abs(px*vy-py*vx)/length;
        double width=.020+baseWidth*(1-std::clamp(t,0.,1.));
        double along=Smooth(t/.16)*Smooth((1-t)/.20);
        double side=1-Smooth((across-width*.72)/(width*.28+.012));
        return along*side;
    }
    inline double Height(double x,double y)
    {
        double cx=GraniteOutcropV2::Width*.5,cy=GraniteOutcropV2::Depth*.5;
        double dx=(x-cx)/(GraniteOutcropV2::Width*.72),dy=(y-cy)/(GraniteOutcropV2::Depth*.95);
        double angle=std::atan2(dy,dx);
        double edgeWobble=.045*std::sin(angle*3.0+1.1)+.025*std::sin(angle*7.0-.8);
        double radius=std::sqrt(dx*dx+dy*dy)+edgeWobble;
        double skirt=1-Smooth((radius-.54)/.52);
        double fieldFade=Smooth((x-X)/1.8)*Smooth((X+NX*Step-x)/1.8)*Smooth((y-Y)/1.8)*Smooth((Y+NY*Step-y)/1.8);
        // Broad, fixed-seed relief gives this small lab roughly three feet of
        // deterministic elevation range without turning it into a steep test
        // course. Fade it out through the outcrop's immediate soil mantle so
        // the seeded granite/soil seam and its predetermined mass stay fixed.
        double reliefMask=Smooth((radius-.72)/.90);
        double rolling=.459+.365*std::sin(x*.45+.31)*std::sin(y*.38-1.07)
            +.162*std::sin(x*.28+y*.31+.73)+.081*std::sin(x*.93-y*.52-1.21);
        // Washed mineral soil accumulates vertically against the outcrop. The
        // bank follows its perimeter with large-scale variation rather than a
        // constant ring, and three broader deposits bury the shallow granite
        // saddles leading to the satellite crowns. This is still one heightfield.
        double u=x/GraniteOutcropV2::Width,v=y/GraniteOutcropV2::Depth;
        bool inside=u>=0&&u<=1&&v>=0&&v<=1;
        double outsideX=std::max({-x,x-GraniteOutcropV2::Width,0.});
        double outsideY=std::max({-y,y-GraniteOutcropV2::Depth,0.});
        double edgeMeters=inside?std::min({x,GraniteOutcropV2::Width-x,y,GraniteOutcropV2::Depth-y}):std::hypot(outsideX,outsideY);
        double perimeter=std::exp(-std::pow(edgeMeters/.22,2.));
        double wash=.50+.27*std::sin(x*1.73-y*.91+.4)+.23*std::sin(x*.63+y*2.11-1.2);
        wash=std::clamp(wash,0.,1.);
        // Most of the perimeter receives only a thin wash. Squared patchiness
        // creates isolated higher banks instead of a uniform raised collar.
        double settledBank=skirt*(.008+.120*perimeter*(.10+.90*wash*wash));
        // Broad mantles bury the connecting granite roofs without turning
        // every saddle into a raised berm. One small triangular wash fan
        // climbs into the front-left joint; the rest stays flush with grade.
        double saddleBank=.165*GraniteOutcropV2::Gaussian(u,v,.20,.32,.125,.20)
            +.095*GraniteOutcropV2::Gaussian(u,v,.81,.35,.120,.18)
            +.140*GraniteOutcropV2::Gaussian(u,v,.73,.74,.185,.16)
            +.035*SettledFan(x,y,.42,.64,1.02,.79,.075);
        return .11+fieldFade*(.145*skirt+settledBank+saddleBank+reliefMask*rolling+.008*std::sin(x*1.7+.4)*std::sin(y*1.3-.2)+.004*std::sin(x*4.1+y*2.3));
    }
    inline std::array<P,4> Quad(int x,int y)
    {
        double px=X+x*Step,py=Y+y*Step;
        return {P{px,py,Height(px,py)},P{px+Step,py,Height(px+Step,py)},P{px+Step,py+Step,Height(px+Step,py+Step)},P{px,py+Step,Height(px,py+Step)}};
    }
    inline bool Surface(double x,double y,double& height)
    {
        double gx=(x-X)/Step,gy=(y-Y)/Step;int ix=int(std::floor(gx)),iy=int(std::floor(gy));
        if(ix<0||iy<0||ix>=NX||iy>=NY)return false;
        auto q=Quad(ix,iy);double u=gx-ix,v=gy-iy;
        height=u>=v?q[0][2]+(q[1][2]-q[0][2])*u+(q[2][2]-q[1][2])*v:q[0][2]+(q[2][2]-q[3][2])*u+(q[3][2]-q[0][2])*v;
        return true;
    }
    inline double Side(P a,P b,P p){return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]);}
    inline std::vector<P> ClipXY(std::vector<P> const& polygon,P a,P b)
    {
        std::vector<P> out;if(polygon.empty())return out;
        for(size_t i=0;i<polygon.size();++i)
        {
            P p=polygon[i],q=polygon[(i+1)%polygon.size()];double dp=Side(a,b,p),dq=Side(a,b,q);
            if(dp>=-1e-12)out.push_back(p);
            if((dp<0&&dq>0)||(dp>0&&dq<0))out.push_back(Add(p,Mul(Add(q,Mul(p,-1)),dp/(dp-dq))));
        }
        return out;
    }
    inline double PlaneHeight(P a,P b,P c,P point)
    {
        P n=Cross(Add(b,Mul(a,-1)),Add(c,Mul(a,-1)));
        return a[2]-(n[0]*(point[0]-a[0])+n[1]*(point[1]-a[1]))/n[2];
    }
    // Exact translational support: clip each fragment-face footprint against
    // each overlapped soil triangle. The difference of their planes is linear,
    // so its maximum occurs at one of the clipped polygon's vertices.
    inline double RequiredLift(Poly const& geometry,P offset)
    {
        double lift=-1e30;
        for(auto const& face:geometry)for(size_t k=1;k+1<face.p.size();++k)
        {
            P a=Add(face.p[0],offset),b=Add(face.p[k],offset),c=Add(face.p[k+1],offset);
            if(std::abs(Side(a,b,c))<1e-14)continue;
            int lx=std::max(0,int(std::floor((std::min({a[0],b[0],c[0]})-X)/Step))),ux=std::min(NX-1,int(std::floor((std::max({a[0],b[0],c[0]})-X)/Step)));
            int ly=std::max(0,int(std::floor((std::min({a[1],b[1],c[1]})-Y)/Step))),uy=std::min(NY-1,int(std::floor((std::max({a[1],b[1],c[1]})-Y)/Step)));
            for(int y=ly;y<=uy;++y)for(int x=lx;x<=ux;++x)
            {
                auto q=Quad(x,y);
                for(int half=0;half<2;++half)
                {
                    P s=q[0],t=q[half?2:1],u=q[half?3:2];std::vector<P> polygon={a,b,c};
                    polygon=ClipXY(polygon,s,t);polygon=ClipXY(polygon,t,u);polygon=ClipXY(polygon,u,s);
                    for(P p:polygon)lift=std::max(lift,PlaneHeight(s,t,u,p)-p[2]);
                }
            }
        }
        return lift;
    }
    struct FallingChip
    {
        GraniteOutcropV2::Chip chip;P offset={},velocity={};bool settled=false;
    };
    inline FallingChip Launch(GraniteOutcropV2::Chip const& chip,P normal)
    {
        FallingChip d;d.chip=chip;d.velocity=Add(Mul(normal,.9),P{0,0,.4});
        return d;
    }
    inline bool Advance(FallingChip& d,GraniteOutcropV2::Body const& body,double dt)
    {
        if(d.settled)return false;dt=std::clamp(dt,0.,.05);
        d.velocity[2]-=9.81*dt;P step=Mul(d.velocity,dt);bool supported=false;
        // Bounded center sweeps preserve tangential/outward motion against rock.
        // Full-footprint soil support is solved below. This remains translational
        // lab debris, not a rigid-body/stacking physics solver.
        for(int pass=0;pass<3;++pass)
        {
            double length=std::sqrt(Dot(step,step));if(length<1e-8)break;
            GraniteOutcropV2::Hit nearest;nearest.distance=length+1;
            for(P p:{d.chip.center})
            {
                auto hit=GraniteOutcropV2::Raycast(body,Add(p,d.offset),step,length);
                if(hit.hit&&Dot(step,hit.normal)<-1e-10&&hit.distance<nearest.distance)nearest=hit;
            }
            if(!nearest.hit){d.offset=Add(d.offset,step);break;}
            double fraction=std::clamp((nearest.distance-.0005)/length,0.,1.);d.offset=Add(d.offset,Mul(step,fraction));
            step=Mul(step,1-fraction);step=Add(step,Mul(nearest.normal,-std::min(0.,Dot(step,nearest.normal))));
            d.velocity=Add(d.velocity,Mul(nearest.normal,-std::min(0.,Dot(d.velocity,nearest.normal))));
            d.velocity=Mul(d.velocity,.65);step=Mul(step,.65);
            supported=supported||nearest.normal[2]>.4;
        }
        double lift=RequiredLift(d.chip.geometry,d.offset);
        if(lift>=-.002){d.offset[2]+=lift+.002;d.velocity={};d.settled=true;}
        else if(supported&&Dot(d.velocity,d.velocity)<.0004)d.settled=true;
        return true;
    }
}
