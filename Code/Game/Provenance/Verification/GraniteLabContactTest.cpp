#include "../Geometry/GraniteLabContact.h"
#include <cstdio>
#include <cstdlib>
namespace C=EE::GraniteLabContact;
namespace G=EE::GraniteContactCast;
namespace D=EE::GraniteContactDebris;
int checks=0;void Check(bool v,char const* text){++checks;if(!v){std::printf("FAIL %s\n",text);std::exit(1);}}
int main()
{
    G::Body body;std::vector<D::FallingChip> pieces;G::P origin={G::Width*.28,-.4,.42},ray={0,1,0};
    auto host=C::Trace(body,pieces,origin,ray);Check(host.material==C::Material::Host,"unobstructed visible host");
    auto below=C::Trace(body,pieces,{.02,G::Depth*.5,1.5},{0,0,-1});double edgeSoil=-1;EE::GraniteOutcropGround::Surface(.02,G::Depth*.5,edgeSoil);
    std::printf("Buried edge material %d distance %.4f soil %.4f\n",int(below.material),below.hit.distance,edgeSoil);
    Check(below.material==C::Material::Soil,"soil occludes buried granite perimeter");
    Check(G::Raycast(body,{.02,G::Depth*.5,1.5},{0,0,-1}).hit,"soil occlusion test actually has buried granite behind soil");
    G::Chip chip;chip.center={origin[0],-.1,origin[2]};chip.geometry=G::Box({origin[0]-.04,-.14,origin[2]-.04},{origin[0]+.04,-.06,origin[2]+.04});chip.mesh=G::Triangulate(chip.geometry);chip.volume=G::Volume(chip.geometry);chip.massMg=uint64_t(chip.volume*G::Density*1e6);chip.id=7;
    pieces.push_back(D::Launch(chip,{0,-1,0}));auto front=C::Trace(body,pieces,origin,ray);
    Check(front.material==C::Material::Loose&&front.loose==0&&front.hit.distance<host.hit.distance,"loose piece blocks wall behind it");
    auto assisted=C::Trace(body,pieces,{origin[0]+.05,origin[1],origin[2]},ray);Check(assisted.material==C::Material::Loose&&assisted.loose==0,"half-inch exact-geometry skin selects a narrowly missed ground chip");
    Check(C::Trace(body,pieces,{origin[0]+.06,origin[1],origin[2]},ray).material!=C::Material::Loose,"pick assist does not select a chip outside its half-inch skin");
    pieces[0].orientation={std::sqrt(.5),0,0,std::sqrt(.5)};pieces[0].offset={0,.04,0};
    front=C::Trace(body,pieces,origin,ray);Check(front.material==C::Material::Loose&&front.hit.distance>.25&&front.hit.distance<.4,"transformed debris contact agrees with visible pose");
    pieces[0].offset={0,1.3,0};Check(C::Trace(body,pieces,origin,ray).material==C::Material::Host,"debris behind host cannot steal contact");
    pieces[0].offset={0,.4,0};pieces[0].settled=true;auto beforeBrush=pieces[0].velocity;
    Check(D::Brush(pieces[0],{origin[0],-.55,.0},{origin[0],.15,.0},.23,1.8),"walking body brushes loose granite");
    Check(!pieces[0].settled&&pieces[0].velocity!=beforeBrush,"body contact wakes and moves existing granite piece");
    Check(!D::Brush(pieces[0],{origin[0],.15,.0},{origin[0],.15,.0},.23,1.8),"standing still does not repeatedly wake debris");
    Check(C::Trace(body,pieces,origin,{0,0,0}).material==C::Material::None,"zero direction refused");
    auto revision=body.revision;auto mass=body.removedMg;(void)C::Trace(body,pieces,origin,ray);Check(body.revision==revision&&body.removedMg==mass,"aim query is read only");
    // Narrow internal tetrahedron plane inflation used to report collisions
    // well beyond its actual edge. Verify exact rounded distance primitives.
    Check(std::abs(G::SegmentTriangleDistance2({.9,.9,0},{.9,.9,1},{0,0,0},{1,0,0},{0,1,0})-.32)<1e-10,"triangle edge distance outside inflated corner");
    Check(G::SegmentTriangleDistance2({.2,.2,-1},{.2,.2,1},{0,0,0},{1,0,0},{0,1,0})==0,"segment through triangle");
    Check(std::abs(G::PointTriangleDistance2({.2,.2,.3},{0,0,0},{1,0,0},{0,1,0})-.09)<1e-10,"triangle face distance");
    Check(std::abs(G::SegmentDistance2({0,0,0},{0,0,0},{1,0,0},{1,0,0})-1)<1e-10,"degenerate segment distance");
    G::Body layers(false);layers.low=layers.substrate.low={0,0,0};layers.high=layers.substrate.high={.1,.1,.1};
    layers.substrate.binCount={1,1,1};layers.substrate.bins.resize(1);layers.substrate.bins[0]={0};layers.substrate.pieces.resize(1);
    layers.substrate.pieces[0].low=layers.low;layers.substrate.pieces[0].high=layers.high;
    auto frontSlab=G::Box({0,0,0},{.1,.005,.1}),rearSlab=G::Box({0,.015,0},{.1,.04,.1});
    auto& cell=layers.changed[0];cell.solids={frontSlab,rearSlab};cell.surface=frontSlab;cell.surface.insert(cell.surface.end(),rearSlab.begin(),rearSlab.end());cell.query.Build(cell.surface);
    Check(std::abs(G::ContactThickness(layers,{.05,0,.05},{0,1,0},.1)-.005)<1e-8,"contact depth stops at air gap");
    auto cut=G::PrepareStrike(layers,{.05,-.05,.05},{0,1,0});Check(cut.receipt.removed,"thin front lip can still chip");
    bool onlyFront=true;for(auto const& f:cut.receipt.chip.geometry)for(auto p:f.p)onlyFront=onlyFront&&p[1]<=.00500001;
    Check(onlyFront,"thin contacted surface does not excavate rear slab across air");
    std::printf("Granite lab contact: %d checks, 0 failures\n",checks);
}
