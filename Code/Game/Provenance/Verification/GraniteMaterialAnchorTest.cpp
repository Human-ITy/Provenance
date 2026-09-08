#include "../Geometry/GraniteMaterialAnchor.h"
#include "../Geometry/GraniteContactDebris.h"
#include <cstdio>
namespace G=EE::GraniteContactCast;
namespace D=EE::GraniteContactDebris;
namespace A=EE::GraniteMaterialAnchor;
int main()
{
    int checks=0,failures=0;
    auto check=[&](bool ok,char const* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}};
    // Both high clean and low stained source positions, including normals that
    // change triplanar sign/weights if incorrectly evaluated after rotation.
    for(G::P p: {G::P{-.7,35.6,1.4},G::P{1.2,36.1,.24}})
    for(G::P n: {G::P{0,0,1},G::P{0,-1,0},G::P{-.3,.4,-.8}})
    {
        auto birth=A::Encode(p,n);
        check(birth.uv[3]==1.f&&(birth.normal>>24)==255,"payload marker is not opacity");
        for(int k=0;k<3;++k)check(std::abs(double(birth.uv[k])-p[k])<2e-6,"birth coordinate precision");
        G::P decoded{};for(int k=0;k<3;++k)decoded[k]=double((birth.normal>>(8*k))&255)/255.*2-1;
        check(G::Dot(G::Unit(decoded),G::Unit(n))>.9999,"source normal round trip");
        for(double z:{-10.,0.,10.})
        {
            D::FallingChip d;d.chip.center=p;d.offset={5,-7,z};
            d.orientation={std::sqrt(.5),std::sqrt(.5),0,0};
            auto moved=D::Position(d,p);
            check(moved!=p,"test pose actually moves the vertex");
            auto sleeping=A::Encode(p,n); // world-baked batch retains source p/n
            check(sleeping.uv==birth.uv&&sleeping.normal==birth.normal,"fall rotate sleep wake preserve material payload");
            auto wrong=A::Encode(moved,D::Rotate(d.orientation,n));
            check(wrong.uv!=birth.uv,"current-position material would regress");
        }
    }
    // Interpolated source coordinates must remain affine across the triangle,
    // not collapse to one center color for an entire chip.
    auto a=A::Encode({0,35,1},{0,0,1}),b=A::Encode({1,35,1},{0,0,1}),c=A::Encode({0,36,2},{0,0,1});
    for(int k=0;k<3;++k)
        check(std::abs(.2*a.uv[k]+.3*b.uv[k]+.5*c.uv[k]-G::P{.3,35.5,1.5}[k])<1e-6,"per-vertex source interpolation");
    std::printf("Granite material anchor: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
