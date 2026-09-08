#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <map>

// Lab-local kinematic body. Queries are supplied by the live remaining granite
// and rendered soil; no editor-camera translation or artificial XY fence.
namespace EE::GraniteLabMovement
{
    using P=std::array<double,3>;
    inline constexpr double Radius=.23,StandingHeight=1.8,CrouchingHeight=1.15;
    inline constexpr double WalkSpeed=1.6,RunSpeed=3.4,CrouchSpeed=.8;
    inline constexpr double Skin=.003,StepHeight=.22,Gravity=9.81,JumpSpeed=4.2;
    inline constexpr int MaxFrameSubsteps=2;
    inline constexpr double HorizontalProbeSpacing=Radius*.125;
    struct Body
    {
        P feet={}; double verticalSpeed=0; bool grounded=false,crouched=false,jumpHeld=false,crouchHeld=false;
        double Height() const {return crouched?CrouchingHeight:StandingHeight;}
        double Eye() const {return crouched?1.05:1.65;}
    };
    struct Input {double x=0,y=0;bool run=false,crouch=false,jump=false;};

    // blocks(feet,height): conservative capsule overlap; floor(x,y): rendered
    // soil height or the lab's editor-floor datum outside the finite soil patch.
    template<class Blocks,class Floor,class StepSurface>
    void Advance(Body& b,Input input,double dt,Blocks query,Floor floor,StepSurface stepSurface)
    {
        std::map<std::pair<P,double>,bool> collisionCache;
        auto blocks=[&](P p,double height){auto key=std::make_pair(p,height);auto found=collisionCache.find(key);if(found!=collisionCache.end())return found->second;bool result=query(p,height);collisionCache.emplace(key,result);return result;};
        if(input.crouch&&!b.crouchHeld)
        {
            if(!b.crouched)b.crouched=true;
            else if(!blocks(b.feet,StandingHeight))b.crouched=false;
        }
        b.crouchHeld=input.crouch;
        bool jump=input.jump&&!b.jumpHeld;b.jumpHeld=input.jump;
        dt=std::clamp(dt,0.,.1);if(dt<=0)return;
        double length=std::hypot(input.x,input.y);if(length>1){input.x/=length;input.y/=length;}
        double speed=b.crouched?CrouchSpeed:(input.run?RunSpeed:WalkSpeed);
        // Shorter horizontal samples keep a long frame's leading tread probe
        // from jumping past the legal riser window on an eligible steep slope.
        // Walking at 60 Hz still needs one pass. Longer travel (including
        // sprinting) can use two, never unbounded catch-up. Vertical travel
        // retains its own spatial subdivision below.
        double horizontalDistance=speed*dt;
        int steps=std::clamp(int(std::ceil(horizontalDistance/HorizontalProbeSpacing)),1,MaxFrameSubsteps);double h=dt/steps;
        auto support=[&](){P below=b.feet;below[2]-=Skin*2;return b.feet[2]<=floor(b.feet[0],b.feet[1])+Skin*2||blocks(below,b.Height());};
        b.grounded=b.verticalSpeed<=0&&support();
        if(jump&&b.grounded){b.verticalSpeed=JumpSpeed;b.grounded=false;}
        for(int i=0;i<steps;++i)
        {
            double previousZ=b.feet[2];P horizontalStart=b.feet;
            for(int axis=0;axis<2;++axis)
            {
                double delta=(axis?input.y:input.x)*speed*h;if(std::abs(delta)<1e-12)continue;
                P next=b.feet;next[axis]+=delta;double ground=floor(next[0],next[1]);
                // Follow modest upward soil slopes/steps, never snap down a ledge.
                double rise=ground+Skin-next[2];
                if(rise>0)
                {
                    if(!b.grounded||rise>StepHeight)continue;
                    bool clear=true;int liftSteps=std::max(1,int(std::ceil(rise/.02)));
                    for(int j=1;j<=liftSteps;++j){P probe=b.feet;probe[2]+=rise*j/liftSteps;if(blocks(probe,b.Height())){clear=false;break;}}
                    if(!clear)continue;next[2]+=rise;
                }
                if(!blocks(next,b.Height())){b.feet=next;continue;}
                if(!b.grounded)continue;
                // A capsule meets the riser before its center reaches the tread.
                // Find an upward-facing tread under the leading edge, rather
                // than blindly raising the body against any blocked wall.
                P leading=next;leading[axis]+=(delta>0?1:-1)*(Radius+Skin);
                double top=stepSurface(leading[0],leading[1],b.feet[2]+StepHeight);
                top=std::max(top,stepSurface(next[0],next[1],b.feet[2]+StepHeight));
                for(int side=0;side<2;++side)for(int sign:{-1,1})
                {P sample=next;sample[side]+=sign*Radius;top=std::max(top,stepSurface(sample[0],sample[1],b.feet[2]+StepHeight));}
                double treadRise=top+Skin-b.feet[2];
                // A narrow riser can hide its top just beyond the leading
                // sample. Try a bounded set of forward tread candidates; this
                // only offers a height, never bypasses the capsule lift checks.
                if(treadRise<=Skin)
                {
                    for(double offset:{.04,.08,.12,.16})
                    {
                        P sample=leading;sample[axis]+=(delta>0?1:-1)*offset;
                        double candidate=stepSurface(sample[0],sample[1],b.feet[2]+StepHeight);
                        double candidateRise=candidate+Skin-b.feet[2];
                        if(candidateRise>Skin&&candidateRise<=StepHeight){top=candidate;treadRise=candidateRise;break;}
                    }
                }
                // A rounded foot may need clearance even when its qualifying
                // tread lies below the feet datum. Continuation is travel-
                // bounded, not a full stair lift. Bare fallback floor retains
                // the small skin allowance; local treads use the bound below.
                if(treadRise<=Skin&&treadRise>=-Radius)
                {
                    double lift=std::min(Skin*2,std::abs(delta)*.25);
                    // A qualifying tread above the fallback floor is distinct
                    // from permission supplied by equal-height bare ground.
                    // Bound incline continuation by travel at the lab's 60
                    // degree slope and 5 cm; retain tiny clearance for floor.
                    if(top>ground+Skin*2)lift=std::min(.05,std::abs(delta)*1.7320508075688772+Skin*.1);
                    P up=b.feet;up[2]+=lift;P middle=up;middle[axis]+=delta*.5;
                    P end=next;end[2]=up[2];
                    if(lift>0&&!blocks(up,b.Height())&&!blocks(middle,b.Height())&&!blocks(end,b.Height()))
                    {
                        double low=b.feet[2],high=end[2];
                        for(int k=0;k<4;++k){double mid=(low+high)*.5;P p=end;p[2]=mid;if(blocks(p,b.Height()))low=mid;else high=mid;}
                        end[2]=std::min(b.feet[2]+lift,high+std::min(Skin*.1,lift*.05));
                        middle[2]=end[2];
                        if(!blocks(middle,b.Height())&&!blocks(end,b.Height())){b.feet=end;continue;}
                    }
                }
                // Beyond the bounded continuation above, the terrain directly
                // beneath a capsule is not permission for a full step search.
                // Equal-height floor must not trigger expensive wall lifting.
                if(treadRise<=Skin||treadRise>StepHeight)continue;
                // Rounded capsule feet require some clearance above an inclined
                // tread. This is only a search bound; use the minimum below.
                double lift=std::min(StepHeight,std::max(.006,treadRise+.06));
                bool clear=true;int count=std::max(1,int(std::ceil(lift/.02)));
                for(int j=1;j<=count;++j){P probe=b.feet;probe[2]+=lift*j/count;if(blocks(probe,b.Height())){clear=false;break;}}
                P stepped=next;stepped[2]=b.feet[2]+lift;
                P middle=stepped;middle[axis]-=delta*.5;
                if(clear&&!blocks(middle,b.Height())&&!blocks(stepped,b.Height()))
                {
                    // Find the actual capsule clearance, not the tread height
                    // under its leading edge. The latter hops on smooth ramps.
                    double low=b.feet[2],high=stepped[2];
                    for(int k=0;k<5;++k){double mid=(low+high)*.5;P probe=next;probe[2]=mid;if(blocks(probe,b.Height()))low=mid;else high=mid;}
                    stepped[2]=std::min(b.feet[2]+lift,high+Skin*.1);
                    middle[2]=stepped[2];
                    if(!blocks(middle,b.Height())&&!blocks(stepped,b.Height()))b.feet=stepped;
                }
            }
            // Axis separation can wedge on a diagonal/faceted wall when each
            // requested component points slightly inward, even though their
            // combined motion has a useful tangent. If neither axis advanced,
            // try a small bounded fan around the requested heading and take
            // the closest clear direction. Straight-on impacts remain stopped.
            double horizontalAdvance=std::hypot(b.feet[0]-horizontalStart[0],b.feet[1]-horizontalStart[1]);
            if(b.grounded&&horizontalAdvance<speed*h*.25&&length>1e-8)
            {
                double ux=input.x/length,uy=input.y/length;
                for(double degrees:{15.,-15.,30.,-30.,45.,-45.,60.,-60.})
                {
                    double angle=degrees*3.14159265358979323846/180.,c=std::cos(angle),s=std::sin(angle);
                    P candidate=horizontalStart;candidate[0]+=(ux*c-uy*s)*speed*h;candidate[1]+=(ux*s+uy*c)*speed*h;
                    double rise=floor(candidate[0],candidate[1])+Skin-candidate[2];
                    if(rise>StepHeight)continue;if(rise>0)candidate[2]+=rise;
                    if(!blocks(candidate,b.Height())){b.feet=candidate;break;}
                }
            }
            // Follow a descending slope only over this substep's short travel.
            // Larger drops still enter free fall; jumping never snaps downward.
            if(b.grounded&&b.verticalSpeed<=0&&b.feet[2]<=previousZ+1e-8&&(std::abs(input.x)+std::abs(input.y)>0)&&!support())
            {
                double drop=std::min(.035,speed*h*1.25+Skin*2);P probe=b.feet;probe[2]-=drop;
                double datum=floor(probe[0],probe[1])+Skin;
                if(blocks(probe,b.Height())||probe[2]<=datum)
                {
                    double low=std::max(probe[2],datum-Skin),high=b.feet[2];
                    for(int k=0;k<5;++k){double mid=(low+high)*.5;P test=b.feet;test[2]=mid;if(mid<datum||blocks(test,b.Height()))low=mid;else high=mid;}
                    b.feet[2]=high;
                }
            }
            if(b.verticalSpeed<=0&&support()){b.verticalSpeed=0;b.grounded=true;continue;}
            b.grounded=false;b.verticalSpeed-=Gravity*h;
            double dz=b.verticalSpeed*h;
            // Spatial substeps also bound collision travel after a long fall.
            int verticalSteps=std::max(1,int(std::ceil(std::abs(dz)/.02)));
            for(int j=0;j<verticalSteps;++j)
            {
                P next=b.feet;next[2]+=dz/verticalSteps;double ground=floor(next[0],next[1]);
                if(dz<0&&next[2]<=ground+Skin){next[2]=ground+Skin;if(!blocks(next,b.Height())){b.feet=next;b.verticalSpeed=0;b.grounded=true;break;}}
                if(blocks(next,b.Height()))
                {
                    double low=0,high=1;for(int k=0;k<10;++k){double mid=(low+high)*.5;P test=b.feet;test[2]+=(next[2]-b.feet[2])*mid;if(blocks(test,b.Height()))high=mid;else low=mid;}
                    b.feet[2]+=(next[2]-b.feet[2])*low;b.verticalSpeed=0;b.grounded=dz<0;break;
                }
                b.feet=next;
            }
        }
    }
    template<class Blocks,class Floor>
    void Advance(Body& b,Input input,double dt,Blocks blocks,Floor floor)
    {Advance(b,input,dt,blocks,floor,[&](double x,double y,double){return floor(x,y);});}
}
