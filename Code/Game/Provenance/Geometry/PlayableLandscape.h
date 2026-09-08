#pragma once
#include "GraniteOutcropGround.h"
#include <vector>

// Authored editor floor: world [-50,50] XY, top Z=0 (floor.fbx).
// Coordinates below are outcrop-local. This immutable landscape is NOT the
// excavatable SoilScoop body and does not manufacture conserved soil parcels.
namespace EE::PlayableLandscape
{
    using P=GraniteOutcrop::P;
    namespace Core=GraniteOutcropGround;
    inline constexpr double OriginX=-1.4,OriginY=35,OriginZ=.12;
    inline constexpr double MinX=-50-OriginX,MaxX=50-OriginX;
    inline constexpr double MinY=-50-OriginY,MaxY=50-OriginY;
    inline constexpr double Spacing=.50,RowSpacing=.125,CreekSpacing=.05;
    inline constexpr double CreekMinX=-20.5-OriginX,CreekMaxX=-7.5-OriginX;
    inline constexpr int TileCells=40;
    inline bool InCore(double x,double y)
    {return x>=Core::X&&x<=Core::X+Core::NX*Core::Step&&y>=Core::Y&&y<=Core::Y+Core::NY*Core::Step;}
    inline double CoreDistance(double x,double y)
    {return std::hypot(std::max({Core::X-x,x-(Core::X+Core::NX*Core::Step),0.}),std::max({Core::Y-y,y-(Core::Y+Core::NY*Core::Step),0.}));}
    inline double CreekX(double worldY)
    {return -14+4*std::sin(worldY*.07)+1.5*std::sin(worldY*.19);}
    inline double CreekSlope(double worldY)
    {return .28*std::cos(worldY*.07)+.285*std::cos(worldY*.19);}
    struct CreekSample {double across,width,r,depth,half;};
    inline CreekSample Creek(double wx,double wy)
    {
        double across=(wx-CreekX(wy))/std::sqrt(1+std::pow(CreekSlope(wy),2));
        double width=.3+.9*(.5+.5*std::sin(wy*.16+.8));
        double asym=.22*std::sin(wy*.11);
        double half=width*.5*(across<0?1-asym:1+asym);
        // Narrow reaches stay shallow (about 2-3 inches). Wider pockets may
        // deepen, but still have independent variation; this is an art bound.
        double depth=std::min(.06+.20*(.5+.5*std::sin(wy*.12-.7)),.075+.20*(width-.3));
        return {across,width,std::abs(across)/half,depth,half};
    }
    struct Recipe {double height,cover,mud;};
    // The same removal profile controls geometry and sediment eligibility.
    // Excludes the broad floodplain lowering: that is not the creek bed.
    inline double ChannelRemoval(CreekSample const& creek,double wx,double wy)
    {
        double shelf=.24+.08*std::sin(wy*.29+(creek.across<0?1.3:-.8));
        double channel=creek.depth*(1-Core::Smooth((creek.r-shelf)/(1-shelf)));
        double ripple=.009*std::sin(wy*2.3+wx*3.7)*(1-Core::Smooth(creek.r));
        return channel-ripple;
    }
    inline Recipe Evaluate(double x,double y)
    {
        double wx=x+OriginX,wy=y+OriginY;
        double fade=Core::Smooth(CoreDistance(x,y)/4.0);
        double broad=.85+.32*std::sin(wx*.085+wy*.021)+.27*std::cos(wy*.09-wx*.025)
            +.11*std::sin(wx*.23+wy*.13);
        // Signed transverse coordinate approximates local perpendicular distance
        // to the low-curvature meander. Width means FULL bank-to-bank width.
        auto creek=Creek(wx,wy);
        double across=creek.across,width=creek.width,half=creek.half,r=creek.r;
        // Broad irregular bottom, softened shoulders, asymmetric banks; no
        // false overhang in a single-valued heightfield. Depth is an art control,
        // not a claimed erosion simulation or a fixed function of width.
        double removal=ChannelRemoval(creek,wx,wy);
        double flood=.045*(1-Core::Smooth((std::abs(across)-half)/(1.0+width)));
        double height=.11+fade*(broad-removal-flood);
        // Soil openings and creek margins remain bare for incoming textures.
        double openings=.5+.28*std::sin(wx*.31+wy*.17)+.22*std::cos(wy*.24-wx*.19);
        double cover=Core::Smooth((openings-.32)/.35)*Core::Smooth((std::abs(across)-half-.12)/.65);
        cover=1-fade+fade*cover; // matches intact grass at the protected seam
        // Initial art-directed sediment, NOT a hydrology/water-table simulation.
        // Deposits occupy the bottom/lower inner banks, never the shoulders.
        // Depth below the uncut channel surface gates every deposit; distance
        // or bare-ground coverage alone cannot qualify surrounding ground.
        double pockets=Core::Smooth((.5+.5*std::sin(wy*.37+std::sin(wy*.11))-.40)/.42);
        double side=Core::Smooth((across*std::sin(wy*.11)+.10)/.22);
        double settled=Core::Smooth((removal/creek.depth-.45)/.35);
        double bank=Core::Smooth((r-.25)/.25)*side;
        double bed=(1-Core::Smooth((r-.38)/.62))*pockets;
        double mottling=.78+.22*std::sin(wx*4.3+wy*2.1)*std::sin(wx*1.7-wy*3.2);
        double mud=fade*(1-cover)*settled*std::clamp(std::max(bank*.85,bed)*mottling,0.,1.);
        return {height,cover,mud};
    }
    struct Grid
    {
        std::vector<double> xs,ys;
        std::vector<Recipe> values;
        static std::vector<double> Axis(double lo,double hi,double a,double b,bool refine=false)
        {
            std::vector<double> result;
            int count=int(std::ceil((hi-lo)/(refine?Spacing:RowSpacing)));
            for(int i=0;i<=count;++i){double p=lo+(hi-lo)*i/count;if(!refine||p<CreekMinX||p>CreekMaxX)result.push_back(p);}
            if(refine){int fine=int(std::ceil((CreekMaxX-CreekMinX)/CreekSpacing));for(int i=0;i<=fine;++i)result.push_back(CreekMinX+(CreekMaxX-CreekMinX)*i/fine);}
            result.push_back(a);result.push_back(b);std::sort(result.begin(),result.end());
            result.erase(std::unique(result.begin(),result.end(),[](double x,double y){return std::abs(x-y)<1e-9;}),result.end());
            return result;
        }
        Grid():xs(Axis(MinX,MaxX,Core::X,Core::X+Core::NX*Core::Step,true)),ys(Axis(MinY,MaxY,Core::Y,Core::Y+Core::NY*Core::Step))
        {
            values.reserve(xs.size()*ys.size());
            for(double y:ys)for(double x:xs)values.push_back(Evaluate(x,y));
            // A positive vertex weight influences its incident triangles.
            // Keep that entire footprint inside the carve, so interpolation
            // cannot smear sediment onto an uncut bank even in narrow reaches.
            for(int y=0;y<=NY();++y)for(int x=0;x<=NX();++x)
            {
                auto& value=values[ID(x,y)];if(value.mud==0)continue;
                bool inside=true;
                for(int j=std::max(0,y-1);j<=std::min(NY(),y+1);++j)
                    for(int i=std::max(0,x-1);i<=std::min(NX(),x+1);++i)
                        inside=inside&&Creek(xs[i]+OriginX,ys[j]+OriginY).r<.98;
                if(!inside)value.mud=0;
            }
        }
        int NX()const{return int(xs.size())-1;} int NY()const{return int(ys.size())-1;}
        size_t ID(int x,int y)const{return size_t(y)*xs.size()+size_t(x);}
        bool Cell(int x,int y)const{return !InCore((xs[x]+xs[x+1])*.5,(ys[y]+ys[y+1])*.5);}
        P Vertex(int x,int y)const{return {xs[x],ys[y],values[ID(x,y)].height};}
        P Normal(int x,int y)const
        {
            int a=std::max(0,x-1),b=std::min(NX(),x+1),c=std::max(0,y-1),d=std::min(NY(),y+1);
            double dx=(values[ID(b,y)].height-values[ID(a,y)].height)/(xs[b]-xs[a]);
            double dy=(values[ID(x,d)].height-values[ID(x,c)].height)/(ys[d]-ys[c]);
            double length=std::sqrt(1+dx*dx+dy*dy);return {-dx/length,-dy/length,1/length};
        }
        bool Surface(double x,double y,double& height)const
        {
            if(!std::isfinite(x)||!std::isfinite(y)||x<MinX||x>MaxX||y<MinY||y>MaxY||InCore(x,y))return false;
            int ix=std::min(NX()-1,int(std::upper_bound(xs.begin(),xs.end(),x)-xs.begin())-1);
            int iy=std::min(NY()-1,int(std::upper_bound(ys.begin(),ys.end(),y)-ys.begin())-1);
            double u=(x-xs[ix])/(xs[ix+1]-xs[ix]),v=(y-ys[iy])/(ys[iy+1]-ys[iy]);
            double a=values[ID(ix,iy)].height,b=values[ID(ix+1,iy)].height,c=values[ID(ix+1,iy+1)].height,d=values[ID(ix,iy+1)].height;
            height=u>=v?a+(b-a)*u+(c-b)*v:a+(c-d)*u+(d-a)*v;
            return true; // exactly the rendered diagonal, not analytic height
        }
        int TileColumns()const{return (NX()+TileCells-1)/TileCells;}
        int TileCount()const{return TileColumns()*((NY()+TileCells-1)/TileCells);}
    };
    inline Grid const& Terrain(){static Grid const grid;return grid;}
}
