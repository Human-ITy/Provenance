// Observational sweep around the user-reported smaller crown and ground lip.
// Stops are investigation evidence, not automatically movement failures.
#include "../Geometry/GraniteLabContact.h"
#include <cstdio>
namespace G=EE::GraniteContactCast;
namespace M=EE::GraniteLabMovement;
namespace C=EE::GraniteLabContact;
struct HotspotLab
{
    G::Body rock;EE::SoilScoop::Body soil;
    HotspotLab(){soil.Reset(&rock);}
    bool Blocks(M::P p,double h)const{return G::BlocksCapsule(rock,p,M::Radius,h)||soil.BlocksCapsule(p,M::Radius,h,false);}
    void Advance(M::Body& b,M::Input input)const
    {M::Advance(b,input,1./60,[&](M::P p,double h){return Blocks(p,h);},[](double,double){return -double(.12f);},[&](double x,double y,double z){return C::StepSurface(rock,x,y,z,&soil,false);});}
    M::Body Land(double x,double y,bool crouch)const
    {M::Body b{{x,y,rock.high[2]+.4}};b.crouched=crouch;for(int i=0;i<180;++i)Advance(b,{});return b;}
    void Witness(M::Body const& b,double dx,double dy)const
    {
        double speed=b.crouched?M::CrouchSpeed:M::WalkSpeed;
        for(int axis=0;axis<2;++axis)
        {
            double delta=(axis?dy:dx)*speed/60;if(std::abs(delta)<1e-12)continue;
            auto next=b.feet;next[axis]+=delta;
            auto lead=next;lead[axis]+=(delta>0?1:-1)*(M::Radius+M::Skin);
            double top=C::StepSurface(rock,lead[0],lead[1],b.feet[2]+M::StepHeight,&soil,false);
            top=std::max(top,C::StepSurface(rock,next[0],next[1],b.feet[2]+M::StepHeight,&soil,false));
            for(int side=0;side<2;++side)for(int sign:{-1,1}){auto p=next;p[side]+=sign*M::Radius;top=std::max(top,C::StepSurface(rock,p[0],p[1],b.feet[2]+M::StepHeight,&soil,false));}
            auto hit=G::Raycast(rock,{lead[0],lead[1],rock.high[2]+1},{0,0,-1},5);
            double lift=-1;
            for(int mm=1;mm<=220;++mm){auto up=b.feet;up[2]+=mm*.001;auto end=next;end[2]=up[2];if(Blocks(up,b.Height()))break;if(!Blocks(end,b.Height())){lift=mm*.001;break;}}
            // Evidence only: an eligible tread farther ahead does not prove a
            // capsule can reach it. Do not feed this search into movement.
            double farther=-1,farRise=-999.;
            for(int cm=2;cm<=22;cm+=2){auto p=lead;p[axis]+=(delta>0?1:-1)*cm*.01;double z=C::StepSurface(rock,p[0],p[1],b.feet[2]+M::StepHeight,&soil,false);double rise=z+M::Skin-b.feet[2];if(rise>M::Skin&&rise<=M::StepHeight){farther=cm*.01;farRise=rise;break;}}
            std::printf("  axis=%d nextRock=%d nextSoil=%d offeredRise=%.6f leadRockZ=%.6f leadNormalZ=%.6f endpointClearLift=%.3f fartherTreadOffset=%.2f fartherRise=%.6f\n",axis,G::BlocksCapsule(rock,next,M::Radius,b.Height()),soil.BlocksCapsule(next,M::Radius,b.Height(),false),top<-1e20?-999.:top+M::Skin-b.feet[2],hit.hit?hit.position[2]:-999.,hit.hit?hit.normal[2]:-999.,lift,farther,farRise);
        }
    }
};
int main()
{
    HotspotLab lab;int routes=0,stops=0,invalid=0,overlaps=0;
    std::printf("HOTSPOT seed=%u crest=%.6f dt=1/60; approximate regions, not recovered screenshot coordinates\n",lab.rock.seed,lab.rock.high[2]);
    auto route=[&](char const* kind,double x,double y,double dx,double dy,double length,bool crouch)
    {
        ++routes;double n=std::hypot(dx,dy);dx/=n;dy/=n;
        auto b=lab.Land(x,y,crouch);auto start=b.feet;
        if(!b.grounded||lab.Blocks(b.feet,b.Height())){++invalid;std::printf("INVALID %s crouch=%d start=%.3f,%.3f,%.3f\n",kind,crouch,start[0],start[1],start[2]);return;}
        int frames=int(std::ceil(length/(crouch?M::CrouchSpeed:M::WalkSpeed)*60)),run=0;bool caught=false;
        for(int i=0;i<frames;++i)
        {
            auto old=b.feet;lab.Advance(b,{dx,dy});overlaps+=lab.Blocks(b.feet,b.Height());
            double progress=(b.feet[0]-old[0])*dx+(b.feet[1]-old[1])*dy;
            run=progress<(crouch?M::CrouchSpeed:M::WalkSpeed)/60*.1?run+1:0;
            if(run==12&&!caught){caught=true;++stops;std::printf("STOP %s crouch=%d start=%.3f,%.3f,%.3f dir=%.4f,%.4f feet=%.6f,%.6f,%.6f grounded=%d\n",kind,crouch,start[0],start[1],start[2],dx,dy,b.feet[0],b.feet[1],b.feet[2],b.grounded);lab.Witness(b,dx,dy);}
        }
    };
    for(bool crouch:{false,true})
    {
        for(double x:{2.7,3.1,3.5})for(double y:{.9,1.3,1.7})for(int dir=0;dir<8;++dir)
        {double a=dir*3.14159265358979323846/4;route("upper",x,y,std::cos(a),std::sin(a),.55,crouch);}
        for(double x:{.6,1.,1.4,1.8,2.2,2.6,3.,3.4,3.8})
        {route("front",x,-.6,0,1,1.8,crouch);route("rear",x,G::Depth+.6,0,-1,1.8,crouch);}
    }
    std::printf("HOTSPOT routes=%d sustainedStops=%d invalidStarts=%d overlapFrames=%d; stops require classification\n",routes,stops,invalid,overlaps);
    return invalid||overlaps?1:0;
}
