#include "../Geometry/ToolPickup.h"
#include <cassert>
#include <cstdio>
int main()
{
    namespace P=EE::ToolPickup;
    double hit=P::HammerHit({0,-1,.57},{0,1,0});assert(hit>.9&&hit<1);
    assert(!std::isfinite(P::HammerHit({.13,-1,.25},{0,1,0}))); // empty space beside shaft
    assert(!std::isfinite(P::HammerHit({0,-3,.57},{0,1,0}))); // out of reach
    assert(!std::isfinite(P::HammerHit({0,-1,.57},{0,0,0})));
    assert(P::Visible(hit,2));assert(!P::Visible(hit,.5));
    assert(P::HammerHit({0,-.5,.57},{0,.5,0})> .8); // scaled entity preserves world t
    using K=P::Kind;
    P::Inventory i;assert(!i.Update(true,false,K::Hammer));assert(!i.Owns(K::Hammer));
    assert(!i.Update(true,true,K::Hammer)); // focus transfer cannot reuse held E
    i.Update(false,true,K::Hammer);assert(i.Update(true,true,K::Hammer));assert(i.Owns(K::Hammer));
    assert(!i.Update(true,true,K::Hammer));i.Update(false,true,K::Hammer);assert(!i.Update(true,true,K::Hammer));
    for(auto kind:{K::Shovel,K::Axe,K::Pickaxe})
    {assert(!i.Update(true,true,kind));i.Update(false,true,kind);assert(i.Update(true,true,kind));assert(i.Owns(kind));}
    assert(i.Count()==4);assert(i.last==K::Pickaxe);
    P::Inventory fresh;assert(fresh.Count()==0);assert(!fresh.Update(true,true,K::None));
    fresh.Update(false,true,K::Hammer);assert(fresh.Update(true,true,K::Hammer));
    assert(std::isfinite(P::Hit(K::Shovel,{0,-1,.15},{0,1,0})));
    assert(!std::isfinite(P::Hit(K::Shovel,{0,-1,1.09},{0,1,0}))); // D handle is open
    assert(std::isfinite(P::Hit(K::Shovel,{.094,-1,1.10},{0,1,0})));
    assert(std::isfinite(P::Hit(K::Axe,{-.19,-1,.72},{0,1,0})));
    assert(!std::isfinite(P::Hit(K::Axe,{-.17,-1,.4},{0,1,0})));
    assert(std::isfinite(P::Hit(K::Pickaxe,{-.23,-1,.87},{0,1,0})));
    assert(!std::isfinite(P::Hit(K::Pickaxe,{-.23,-1,.70},{0,1,0})));
    assert(std::isfinite(P::Hit(K::Pickaxe,{-1,0,.90},{1,0,0}))); // edge-on head
    for(auto kind:{K::Hammer,K::Shovel,K::Axe,K::Pickaxe})
    {assert(!std::isfinite(P::Hit(kind,{0,-3,.2},{0,1,0})));assert(!std::isfinite(P::Hit(kind,{0,0,.2},{0,0,0})));}
    assert(P::Capsule({0,-1,.5},{0,1,0},{0,0,0},{0,0,1},.03)>.96);
    assert(P::Capsule({0,0,2},{0,0,-1},{0,0,0},{0,0,1},.03)>.96);
    std::puts("PASS: all four tools, hollow grip, shaped head/edge rays, reach, occlusion, scaled rays, focus latch, independent ownership, duplicate prevention and fresh session.");
}
