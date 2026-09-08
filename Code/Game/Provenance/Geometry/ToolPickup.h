#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace EE::ToolPickup
{
    using Point=std::array<double,3>;
    inline constexpr double Reach=2.0;
    enum class Kind {Hammer,Shovel,Axe,Pickaxe,None};
    inline constexpr char const* EntityNames[]={"ToolPickup_GuildHammer","ToolPickup_StoneShovel","ToolPickup_StoneAxe","ToolPickup_StonePickaxe"};
    inline constexpr char const* Names[]={"Guild hammer","Stone shovel","Stone axe","Stone pickaxe"};
    inline bool Valid(Kind k){return int(k)>=0&&int(k)<4;}
    inline double Dot(Point a,Point b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
    inline Point Sub(Point a,Point b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
    inline Point Along(Point p,Point d,double t){return {p[0]+d[0]*t,p[1]+d[1]*t,p[2]+d[2]*t};}
    inline double Sphere(Point o,Point d,Point center,double radius)
    {
        auto w=Sub(o,center);double a=Dot(d,d),b=Dot(w,d),c=Dot(w,w)-radius*radius;
        if(c<=0)return 0;if(a<1e-16)return INFINITY;
        double discriminant=b*b-a*c;if(discriminant<0)return INFINITY;
        double t=(-b-std::sqrt(discriminant))/a;return t>=0&&t<=Reach?t:INFINITY;
    }
    inline double Capsule(Point o,Point d,Point a,Point b,double radius)
    {
        auto axis=Sub(b,a),w=Sub(o,a);double length2=Dot(axis,axis);
        if(length2<1e-16)return Sphere(o,d,a,radius);
        double alongD=Dot(d,axis)/length2,alongW=Dot(w,axis)/length2;
        auto dp=Along(d,axis,-alongD),wp=Along(w,axis,-alongW);
        double A=Dot(dp,dp),B=Dot(wp,dp),C=Dot(wp,wp)-radius*radius;
        double result=std::min(Sphere(o,d,a,radius),Sphere(o,d,b,radius));
        if(C<=0&&alongW>=0&&alongW<=1)return 0;
        double disc=B*B-A*C;
        if(A>1e-16&&disc>=0)for(double t:{(-B-std::sqrt(disc))/A,(-B+std::sqrt(disc))/A})
        {double u=alongW+t*alongD;if(t>=0&&t<=Reach&&u>=0&&u<=1)result=std::min(result,t);}
        return result;
    }
    // X/Z polygon extruded across Y. Unlike a large bounding box this leaves
    // empty space beneath the pick point and around the shaped cutting edges.
    template<size_t N> double Prism(Point o,Point d,std::array<std::array<double,2>,N> const& poly,double halfY)
    {
        auto inside=[&](double x,double z){bool yes=false;for(size_t i=0,j=N-1;i<N;j=i++)
        {auto a=poly[i],b=poly[j];if((a[1]>z)!=(b[1]>z)&&x<(b[0]-a[0])*(z-a[1])/(b[1]-a[1])+a[0])yes=!yes;}return yes;};
        if(std::abs(o[1])<=halfY&&inside(o[0],o[2]))return 0;
        double result=INFINITY;
        if(std::abs(d[1])>1e-12)for(double y:{-halfY,halfY})
        {double t=(y-o[1])/d[1];auto p=Along(o,d,t);if(t>=0&&t<=Reach&&inside(p[0],p[2]))result=std::min(result,t);}
        for(size_t i=0;i<N;++i)
        {
            auto a=poly[i],b=poly[(i+1)%N];double ex=b[0]-a[0],ez=b[1]-a[1],den=d[0]*ez-d[2]*ex;
            if(std::abs(den)<1e-12)continue;
            double ax=a[0]-o[0],az=a[1]-o[2],t=(ax*ez-az*ex)/den,u=(ax*d[2]-az*d[0])/den;
            if(t>=0&&t<=Reach&&u>=0&&u<=1&&std::abs(o[1]+t*d[1])<=halfY)result=std::min(result,t);
        }
        return result;
    }
    inline double Box(Point origin,Point direction,Point low,Point high,double limit)
    {
        double enter=0,leave=limit;
        for(int k=0;k<3;++k)
        {
            if(!std::isfinite(origin[k])||!std::isfinite(direction[k]))return std::numeric_limits<double>::infinity();
            if(std::abs(direction[k])<1e-12){if(origin[k]<low[k]||origin[k]>high[k])return std::numeric_limits<double>::infinity();continue;}
            double a=(low[k]-origin[k])/direction[k],b=(high[k]-origin[k])/direction[k];
            if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);
            if(enter>leave)return std::numeric_limits<double>::infinity();
        }
        return enter;
    }
    // Local metre-scale pick proxies follow the head, shaft/grip and socket.
    // A normalized world ray transformed without renormalization keeps t in
    // world metres, including the entity's uniform scale.
    inline double HammerHit(Point origin,Point direction)
    {
        double norm=0;for(double v:direction)norm+=v*v;
        if(!std::isfinite(norm)||norm<1e-16)return std::numeric_limits<double>::infinity();
        double head=Box(origin,direction,{-.195,-.067,.505},{.206,.067,.665},Reach);
        double handle=Box(origin,direction,{-.029,-.029,.020},{.029,.029,.477},Reach);
        double pommel=Box(origin,direction,{-.033,-.033,0},{.033,.033,.047},Reach);
        double collar=Box(origin,direction,{-.036,-.036,.467},{.036,.036,.523},Reach);
        return std::min({head,handle,pommel,collar});
    }
    inline bool Visible(double target,double obstruction)
    {return std::isfinite(target)&&target>=0&&target<=Reach&&target<=obstruction+1e-5;}
    inline double Hit(Kind kind,Point o,Point d)
    {
        if(!Valid(kind)||Dot(d,d)<1e-16)return INFINITY;
        for(int i=0;i<3;++i)if(!std::isfinite(o[i])||!std::isfinite(d[i]))return INFINITY;
        if(kind==Kind::Hammer)return HammerHit(o,d);
        double hit=INFINITY;
        auto capsule=[&](Point a,Point b,double r){hit=std::min(hit,Capsule(o,d,a,b,r));};
        if(kind==Kind::Shovel)
        {
            constexpr std::array<std::array<double,2>,13> outline={{{-.132,.313},{-.137,.287},{-.135,.223},{-.122,.161},{-.087,.092},{-.041,.031},{0,0},{.046,.032},{.089,.092},{.122,.16},{.137,.228},{.134,.290},{.124,.313}}};
            hit=Prism(o,d,outline,.028);capsule({0,0,.15},{0,0,.957},.029);
            for(double s:{-1.,1.}){capsule({s*.012,0,.937},{s*.075,0,1.035},.016);capsule({s*.075,0,1.035},{s*.098,0,1.166},.017);}
            capsule({-.102,0,1.165},{.102,0,1.165},.026);
        }
        else
        {
            double height=kind==Kind::Axe?.83:.98;
            auto center=[&](double z){return kind==Kind::Axe?.026*std::sin(z/.83*3.141592653589793*1.8)-.015:.037*std::sin(z/.98*3.141592653589793*1.75)-.012;};
            for(int i=0;i<12;++i){double a=height*i/12,b=height*(i+1)/12;capsule({center(a),0,a},{center(b),0,b},i==0?.037:.029);}
            auto headOrigin=o;headOrigin[0]-=center(kind==Kind::Axe?.735:.891);
            if(kind==Kind::Axe)
            {
                constexpr std::array<std::array<double,2>,14> outline={{{-.207,.814},{-.220,.782},{-.223,.733},{-.213,.680},{-.191,.612},{-.159,.632},{-.102,.670},{-.035,.684},{.048,.690},{.064,.714},{.064,.755},{.045,.779},{-.037,.775},{-.11,.788}}};
                hit=std::min(hit,Prism(headOrigin,d,outline,.042));
            }
            else
            {
                constexpr std::array<std::array<double,2>,18> outline={{{-.345,.797},{-.303,.841},{-.251,.880},{-.182,.918},{-.09,.94},{-.015,.942},{.080,.93},{.175,.928},{.298,.920},{.301,.875},{.291,.791},{.232,.821},{.176,.850},{.081,.870},{-.023,.865},{-.132,.870},{-.223,.845},{-.290,.819}}};
                hit=std::min(hit,Prism(headOrigin,d,outline,.046));
            }
        }
        return hit;
    }
    struct Inventory
    {
        std::array<bool,4> owned{};bool keyHeld=false;Kind last=Kind::None;
        bool Owns(Kind kind)const{return Valid(kind)&&owned[int(kind)];}
        int Count()const{return int(std::count(owned.begin(),owned.end(),true));}
        bool Update(bool down,bool enabled,Kind candidate)
        {
            bool pressed=down&&!keyHeld;keyHeld=down;
            if(!pressed||!enabled||!Valid(candidate)||Owns(candidate))return false;
            owned[int(candidate)]=true;last=candidate;return true;
        }
    };
}
