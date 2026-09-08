#include "../Geometry/GraniteLabContact.h"
#include <cstdio>
#include <cstdlib>
#include <future>
namespace S=EE::SoilScoop;namespace G=EE::GraniteContactCast;namespace C=EE::GraniteLabContact;namespace D=EE::GraniteContactDebris;namespace M=EE::GraniteLabMovement;
int checks=0;void Check(bool v,char const* message){++checks;if(!v){std::printf("FAIL %s\n",message);std::exit(1);}}
G::Hit Trace(S::Body const& soil,G::P origin,G::P direction,double reach=3)
{G::Hit hit;soil.Trace(origin,G::Unit(direction),reach,hit);return hit;}
int main()
{
    auto start=std::chrono::steady_clock::now();G::Body rock;S::Body soil;soil.Reset(&rock);soil.dirty.clear();
    // Reproduce mining the exterior lab boundary: generated volume chunks
    // must retain their uncut wall, with outward normals, not backface holes.
    for(int axis=0;axis<2;++axis)for(int side=0;side<2;++side)
    {
        S::Body edge;edge.Reset();G::P p={4.8,3,0},ray={};
        p[axis]=axis==0?(side?S::S::X+S::S::NX*S::S::Step:S::S::X):(side?S::S::Y+S::S::NY*S::S::Step:S::S::Y);
        double sign=side?1.:-1.;p[axis]+=sign*.2;ray[axis]=-sign;
        auto hit=Trace(edge,p,ray);Check(hit.hit&&hit.normal[axis]*sign>.95,"all exterior soil sides face outward before digging");
        auto cut=S::Excavate(edge,rock,hit,ray);Check(cut.volume>0,"shovel removes soil from every exterior side");
        auto deeper=Trace(edge,p,ray);std::printf("edge %d/%d hit %.6f next %.6f valid %d normal %.2f %.2f %.2f volume %.6f\n",axis,side,hit.distance,deeper.distance,deeper.hit,deeper.normal[0],deeper.normal[1],deeper.normal[2],cut.volume);Check(deeper.hit&&deeper.distance>hit.distance+.025,"exterior cut reaches recessed soil, not stale outer wall");
        G::P witness=p;witness[axis==0?1:0]+=.15;auto wall=Trace(edge,witness,ray);
        Check(wall.hit&&wall.normal[axis]*sign>.95&&std::abs(wall.distance-.2)<1e-6,"uncut edited-chunk boundary cap remains visible from outside");
        Check(!edge.Contains(G::Add(hit.position,G::Mul(ray,.025))),"excavated entrance is air for collision");
        double boundary=axis==0?(side?S::S::X+S::S::NX*S::S::Step:S::S::X):(side?S::S::Y+S::S::NY*S::S::Step:S::S::Y);
        for(auto const& e:edge.renderSurfaces)for(auto const& t:*e.second)
        {
            if(std::abs(t.a[axis]-boundary)>1e-7||std::abs(t.b[axis]-boundary)>1e-7||std::abs(t.c[axis]-boundary)>1e-7)continue;
            auto cross=G::Cross(G::Add(t.b,G::Mul(t.a,-1)),G::Add(t.c,G::Mul(t.a,-1)));auto faceNormal=G::Unit(cross);
            if(G::Dot(cross,cross)<1e-16)continue;
            Check(faceNormal[axis]*sign>.999999,"coarse and edited wall faces remain outward: no full-patch dark bands");
        }
    }
    // A detached sphere straddling a sparse chunk seam must be ONE clod, with
    // exact retained volume, collision, gravity and repeatable shovel contact.
    S::Body slough;G::Body noRock(false);G::P clodCenter=S::Point({25,25,3},12,6,6);double clodVolume=0;
    for(int k=25;k<=26;++k)
    {
        auto chunk=std::make_shared<S::Chunk>();chunk->key={k,25,3};
        for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x){auto delta=G::Add(S::Point(chunk->key,x,y,z),G::Mul(clodCenter,-1));chunk->values[S::Index(x,y,z)]=S::Sample(.085-std::sqrt(G::Dot(delta,delta)));}
        chunk->volume=chunk->Volume();chunk->Label();clodVolume+=chunk->volume;slough.chunks[chunk->key]=chunk;slough.queries[S::ID(chunk->key)]=std::make_shared<G::SurfaceQuery>(chunk->Mesh());
    }
    auto clodFloor=std::make_shared<G::SurfaceQuery>();clodFloor->Build(G::Box({1.5,1.5,.1},{3,3,.25}));slough.queries[0]=clodFloor;
    S::Body tethered=slough;
    // The old terrain bank rose to this airborne clod, so a short horizontal
    // strand reached untouched soil.  The V5 field is intentionally low and
    // level around the outcrop.  Build the same support case explicitly: a
    // pick-thin, multi-chunk root runs from the clod to the immutable floor.
    for(int z=0;z<3;++z)for(int x=25;x<=26;++x)
    {
        S::Key key={x,25,z};auto c=std::make_shared<S::Chunk>();c->key=key;
        c->values.fill(-S::Band);tethered.chunks[key]=c;
    }
    for(auto& e:tethered.chunks)
    {
        auto c=std::make_shared<S::Chunk>(*e.second);
        for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x)
        {auto p=S::Point(c->key,x,y,z);double strand=std::min(clodCenter[2]-p[2],.025-std::hypot(p[0]-clodCenter[0],p[1]-clodCenter[1]));c->values[S::Index(x,y,z)]=std::max(c->values[S::Index(x,y,z)],S::Sample(strand));}
        c->volume=c->Volume();c->Label();e.second=c;
    }
    Check(S::ReleaseUnsupported(tethered)==0,"soil connected by a multi-chunk root is not falsely released");
    Check(!G::Raycast(noRock,{0,0,1},{0,0,-1}).hit,"empty granite fixture is safe for downward soil queries");
    S::Body core;G::P coreCenter=S::Point({25,25,3},6,6,6);double coreVolume=S::Span*S::Span*S::Span;
    for(int k=24;k<=26;++k)for(int j=24;j<=26;++j)for(int h=2;h<=4;++h)
    {
        if(k==25&&j==25&&h==3)continue;auto c=std::make_shared<S::Chunk>();c->key={k,j,h};
        for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x){auto delta=G::Add(S::Point(c->key,x,y,z),G::Mul(coreCenter,-1));c->values[S::Index(x,y,z)]=S::Sample(.27-std::sqrt(G::Dot(delta,delta)));}
        c->volume=c->Volume();c->Label();coreVolume+=c->volume;core.chunks[c->key]=c;
    }
    // Supply the enclosed, deliberately unedited chunk through the same lazy
    // support cache used by real terrain.  Its contents are synthetic because
    // the new low field no longer reaches this high laboratory fixture.
    {
        auto c=std::make_shared<S::Chunk>();c->key={25,25,3};
        for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x)
        {auto delta=G::Add(S::Point(c->key,x,y,z),G::Mul(coreCenter,-1));c->values[S::Index(x,y,z)]=S::Sample(.27-std::sqrt(G::Dot(delta,delta)));}
        c->volume=c->Volume();c->Label();core.supportCache[c->key]=c;
    }
    Check(!core.chunks.count({25,25,3})&&S::ReleaseUnsupported(core)==1,"large isolated clod releases even with an untouched interior chunk");
    Check(core.loose[0].matter->chunks.count({25,25,3})==1&&std::abs(core.loose[0].volume-coreVolume)<1e-9,"untouched interior transfers to falling clod with exact volume");
    Check(core.removedM3==0&&core.removedMg==0,"interior materialization is not a harvested scoop");
    Check(S::ReleaseUnsupported(slough)==1,"cross-chunk detached soil becomes one clod");
    Check(slough.removedM3==0&&slough.removedMg==0&&std::abs(slough.loose[0].volume-clodVolume)<1e-10,"slough retains exact volume without harvesting it");
    double hostVolume=0;for(auto const& e:slough.chunks)hostVolume+=e.second->volume;Check(hostVolume<1e-12,"detached soil no longer belongs to static terrain");
    auto fallStart=std::chrono::steady_clock::now();double peakFall=0;
    for(int i=0;i<120&&!slough.loose[0].settled;++i){auto tick=std::chrono::steady_clock::now();S::AdvanceLoose(slough,noRock,1./60);peakFall=std::max(peakFall,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tick).count());}
    Check(slough.loose[0].settled&&slough.loose[0].offset[2]<-.3,"clod falls and settles on actual hole floor");
    S::Body undermined=slough;undermined.queries.erase(0);++undermined.revision;
    for(int i=0;i<120;++i)S::AdvanceLoose(undermined,noRock,1./60);
    Check(undermined.loose[0].settled&&undermined.loose[0].offset[2]<slough.loose[0].offset[2]-.3,"settled soil wakes and falls again when its support is dug away");
    auto settledCenter=G::Add(clodCenter,slough.loose[0].offset);
    Check(slough.Contains(settledCenter)&&slough.BlocksCapsule(G::Add(settledCenter,{0,0,-.04}),.03,.08),"fallen clod retains containment and collision");
    S::Body passThrough;passThrough.loose.push_back(slough.loose[0]);G::P brushFeet=G::Add(settledCenter,{-.1,0,-.23});
    Check(passThrough.BlocksCapsule(brushFeet,.23,1.15)&&!passThrough.BlocksCapsule(brushFeet,.23,1.15,false),"loose soil is targetable but optional for player blocking");
    auto oldOffset=passThrough.loose[0].offset;Check(S::BrushLoose(passThrough,G::Add(brushFeet,{-.2,0,0}),brushFeet,.23,1.15)==1,"walking body brushes loose soil");
    Check(passThrough.loose[0].offset!=oldOffset&&!passThrough.loose[0].settled,"body contact moves and wakes the retained clod");
    Check(S::BrushLoose(passThrough,brushFeet,brushFeet,.23,1.15)==0,"standing still does not keep waking loose soil");
    auto target=Trace(slough,G::Add(settledCenter,{0,0,.5}),{0,0,-1});Check(target.hit&&target.id==-1000,"fallen soil owns nearest shovel contact");
    auto looseCut=S::Excavate(slough,noRock,target,{0,0,-1});Check(looseCut.volume>1e-5&&slough.loose[0].volume<clodVolume,"fallen clod can be shoveled again");
    Check(std::abs(slough.loose[0].volume+slough.removedM3-clodVolume)<1e-10,"re-shoveling conserves retained plus harvested soil");
    auto repeat=S::Excavate(slough,noRock,target,{0,0,-1});Check(repeat.volume<1e-12,"repeated captured loose-soil scoop cannot double count");
    while(slough.loose[0].volume>S::ScoopVolume)
    {auto h=Trace(slough,G::Add(settledCenter,{0,0,.5}),{0,0,-1});Check(h.id==-1000,"remaining large clod stays targetable");auto cut=S::Excavate(slough,noRock,h,{0,0,-1});Check(cut.volume>1e-8,"large clod accepts another normal scoop");}
    double lastClod=slough.loose[0].volume;size_t beforeAbsorption=slough.loose.size();Check(lastClod>0,"fixture leaves a sub-scoop clod");
    auto smallTarget=Trace(slough,G::Add(settledCenter,{0,0,.5}),{0,0,-1});auto absorbed=S::Excavate(slough,noRock,smallTarget,{0,0,-1});
    Check(std::abs(absorbed.volume-lastClod)<1e-10&&slough.loose[0].volume==0,"targeting a sub-scoop loose clod collects it whole");
    double otherClods=0;for(auto const& d:slough.loose)otherClods+=d.volume;
    Check(std::abs(slough.removedM3+otherClods-clodVolume)<1e-10&&slough.loose.size()==beforeAbsorption,"small clod absorption conserves mass and creates no fragments");
    std::printf("slough %.3f L, gravity peak %.3f ms, fixture %.2f ms\n",clodVolume*1000,peakFall,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-fallStart).count());
    auto first=Trace(soil,{.02,-1.9,1},{0,0,-1});Check(first.hit,"flat soil contact");
    auto r=S::Excavate(soil,rock,first,{0,0,-1});double ideal=3.141592653589793*S::Depth*(3*S::Diameter*S::Diameter*.25+S::Depth*S::Depth)/6;
    std::printf("first scoop %.6f L, ideal %.6f L, chunks %zu\n",r.volume*1000,ideal*1000,soil.chunks.size());
    Check(std::abs(r.volume-ideal)<ideal*.12,"spherical-cap volume within 12 percent grid tolerance");
    auto below=Trace(soil,{.02,-1.9,1},{0,0,-1});Check(below.hit&&below.position[2]<first.position[2]-.06,"scoop has expected depth below the seeded surface");
    auto duplicate=S::Excavate(soil,rock,first,{0,0,-1});Check(duplicate.volume<1e-10,"same captured cut cannot double count");
    Check(soil.removedMg==r.massMg&&rock.removedMg==0,"separate soil recovery ledger and untouched granite");
    for(int i=0;i<5;++i)S::Excavate(soil,rock,Trace(soil,{.02,-1.9,1},{0,0,-1}),{0,0,-1});
    below=Trace(soil,{.02,-1.9,1},{0,0,-1});Check(!below.hit||below.position[2]>=S::Bottom-1e-8,"finite test floor cannot be excavated");

    // Drive a narrow bore into an explicit laboratory soil bank, following the
    // reticle instead of the surface normal.  V5's production heightfield is a
    // low field around the outcrop; tunnel coverage must not reshape that field.
    S::Body bank;bank.finiteChunks=true;
    auto bankField=[](G::P p){return S::Sample(std::min({p[0]-4.5,5.1-p[0],p[1]+1.25,.45-p[1],p[2],1.25-p[2]}));};
    for(int z=0;z<=6;++z)for(int y=10;y<=19;++y)for(int x=34;x<=38;++x)
    {
        auto c=std::make_shared<S::Chunk>();c->key={x,y,z};
        for(int iz=0;iz<S::Side;++iz)for(int iy=0;iy<S::Side;++iy)for(int ix=0;ix<S::Side;++ix)c->values[S::Index(ix,iy,iz)]=bankField(S::Point(c->key,ix,iy,iz));
        c->volume=c->Volume();c->Label();bank.chunks[c->key]=c;bank.queries[S::ID(c->key)]=std::make_shared<G::SurfaceQuery>(c->Mesh());
    }
    G::P origin={4.8,-1.5,.7};auto entrance=Trace(bank,origin,{0,1,0});Check(entrance.hit,"bank entrance contact");
    double entranceY=entrance.position[1],last=entranceY,total=0,maxMs=0;size_t maxTriangles=0;
    for(int i=0;i<22;++i)
    {
        auto hit=Trace(bank,origin,{0,1,0});Check(hit.hit,"horizontal tunnel face remains targetable");
        bank.dirty.clear();auto begin=std::chrono::steady_clock::now();auto scoop=S::Excavate(bank,rock,hit,{0,1,0});
        maxMs=std::max(maxMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
        Check(scoop.volume>1e-7,"horizontal strike removes material");total+=scoop.volume;
        size_t triangles=0;for(int id:bank.dirty)triangles+=bank.queries.at(id)->triangles.size();maxTriangles=std::max(maxTriangles,triangles);
        auto next=Trace(bank,origin,{0,1,0});Check(next.hit&&next.position[1]>last+.025,"tunnel advances horizontally rather than making an open trench");
        last=next.position[1];origin={4.8,last-.04,.7};
    }
    std::printf("bore advanced %.3f m, recovered %.3f L, worst prepare %.2f ms, chunks %zu\n",last-entranceY,total*1000,maxMs,bank.chunks.size());
    std::printf("maximum surface publication %zu triangles\n",maxTriangles);
    Check(maxTriangles<12000,"bounded local mesh publication during tunnel sequence");
    Check(last-entranceY>1,"horizontal excavation advances beyond one meter");
    G::P interior={4.8,last-.2,.7};auto roof=Trace(bank,interior,{0,0,1}),floor=Trace(bank,interior,{0,0,-1});
    Check(roof.hit&&roof.normal[2]<-.2&&roof.position[2]>.72,"roof survives above excavated tunnel");
    Check(floor.hit&&floor.normal[2]>.2&&floor.position[2]<.68,"tunnel has a separate floor");
    Check(!bank.Contains(interior)&&bank.Contains({4.8,last-.2,1.1}),"air cavity below retained solid soil");
    Check(!bank.BlocksCapsule({4.8,last-.2,.67},.025,.06),"small probe fits bore");
    Check(bank.BlocksCapsule({4.8,last-.2,.65},M::Radius,M::CrouchingHeight),"player cannot pass through an opening smaller than body");
    double h=0;Check(bank.SurfaceBelow(interior[0],interior[1],.7,h)&&h<.68,"support selects tunnel floor not roof");
    auto upward=S::Excavate(bank,rock,roof,{0,0,1});Check(upward.volume>0,"upward ceiling scoops work");
    auto raised=Trace(bank,interior,{0,0,1});Check(raised.hit&&raised.position[2]>roof.position[2]+.025,"upward scoop raises ceiling locally");
    G::Chip chip;chip.center=interior;chip.geometry=G::Box(G::Add(interior,{-.008,-.008,-.008}),G::Add(interior,{.008,.008,.008}));chip.mesh=G::Triangulate(chip.geometry);
    auto debris=D::Launch(chip,{0,0,1});debris.velocity=debris.spin={};
    for(int i=0;i<600&&!debris.settled;++i)D::Advance(debris,rock,1./60,&bank);
    Check(debris.settled&&D::Position(debris,chip.center)[2]<.7,"granite debris settles inside tunnel rather than on roof");
    Check(std::abs(D::RequiredLift(debris,&bank)+.001)<.003,"debris has contact skin on tunnel floor");
    auto chipMass=debris.chip.massMg;auto chipVolume=debris.chip.volume;auto vertices=debris.vertices.size();
    Check(D::Nudge(debris,{0,1,0},D::Position(debris,chip.center)),"loose chip accepts tool tap");
    Check(!debris.settled&&G::Dot(debris.velocity,debris.velocity)>.01&&debris.velocity[2]>0,"tap wakes and lofts loose chip");
    Check(debris.chip.massMg==chipMass&&debris.chip.volume==chipVolume&&debris.vertices.size()==vertices,"tap preserves mass and existing mesh");
    Check(!D::Nudge(debris,{},chip.center),"invalid tap direction refused");

    // Adjacent sparse chunks must share every boundary field sample, even if a
    // cut only affected an empty neighbor's narrow-band values.
    for(auto const& entry:soil.chunks)for(int axis=0;axis<3;++axis)
    {
        auto key=entry.first;++key[axis];auto it=soil.chunks.find(key);if(it==soil.chunks.end())continue;
        for(int a=0;a<S::Side;++a)for(int b=0;b<S::Side;++b)
        {
            int p[3]={},q[3]={};p[axis]=S::N;q[axis]=0;p[(axis+1)%3]=q[(axis+1)%3]=a;p[(axis+2)%3]=q[(axis+2)%3]=b;
            Check(std::abs(entry.second->values[S::Index(p[0],p[1],p[2])]-it->second->values[S::Index(q[0],q[1],q[2])])<1e-9,"neighboring volume chunks share field values");
        }
    }
    double integrated=0;for(auto const& e:soil.chunks)
    {
        S::Chunk original;original.key=e.first;
        for(int z=0;z<S::Side;++z)for(int y=0;y<S::Side;++y)for(int x=0;x<S::Side;++x)original.values[S::Index(x,y,z)]=soil.Initial(S::Point(e.first,x,y,z));
        integrated+=original.Volume()-e.second->volume;
        auto const& query=*soil.queries.at(S::ID(e.first));for(auto const& tri:query.triangles)Check(G::Finite(tri.n)&&G::Dot(tri.n,tri.n)>.9,"finite unit surface normals");
    }
    size_t rawVertices=0,sharedVertices=0;for(auto const& e:soil.renderSurfaces)
    {
        auto const& indexed=*soil.indexedSurfaces.at(e.first);rawVertices+=e.second->size()*3;sharedVertices+=indexed.vertices.size();
        Check(indexed.indices.size()==e.second->size()*3,"indexed publication retains every triangle");
        size_t i=0;for(auto const& tri:*e.second)
        {
            for(auto const& expected:std::array<S::RenderSurface::Vertex,3>{{{tri.a,tri.na},{tri.b,tri.nb},{tri.c,tri.nc}}})
            {auto id=indexed.indices[i++];Check(id<indexed.vertices.size(),"valid shared vertex index");auto const& v=indexed.vertices[id];Check(v.position==expected.position&&v.normal==expected.normal,"indexed publication preserves exact positions and normals");}
        }
    }
    Check(sharedVertices<rawVertices*.7,"worker deduplication reduces main-thread vertex publication by at least 30 percent");
    std::printf("soil vertex publication %zu versus %zu unshared (%.1f%% less)\n",sharedVertices,rawVertices,100.*(1.-double(sharedVertices)/rawVertices));
    Check(std::abs(integrated-soil.removedM3)<1e-8,"tetrahedral volume difference equals recovery ledger");
    // Granite exclusion is fixed at material initialization, so chipping rock
    // cannot later reveal newly invented soil where the rock used to be.
    Check(soil.RockOutside({G::Width*.28,G::Depth*.51,.5})<0,"original granite interior excluded from soil field");
    std::vector<D::FallingChip> empty;auto originalMass=rock.removedMg;bool exposed=false;
    auto rockBegin=std::chrono::steady_clock::now();for(int i=0;i<8;++i)
    {
        auto contact=C::Trace(rock,empty,{1.4,1.25,2.5},{0,0,-1},3,&soil);
        if(contact.material==C::Material::Host){exposed=true;break;}
        Check(contact.material==C::Material::Soil,"soil owns contact until granite is exposed");
        S::Excavate(soil,rock,contact.hit,{0,0,-1});
    }
    Check(exposed&&rock.removedMg==originalMass,"exposed granite takes contact away from shovel without changing rock mass");
    std::printf("rock exposure sequence %.2f ms\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-rockBegin).count());

    // Normal approach still moves with full soil collision enabled.
    S::Body ground;ground.Reset(&rock);double approachGround=C::StepSurface(rock,1.45,-1.8,2,&ground);M::Body approach{{1.45,-1.8,approachGround+M::Skin}};
    auto walkBegin=std::chrono::steady_clock::now();for(int i=0;i<180;++i)M::Advance(approach,{0,1},1./60,[&](G::P p,double height){return G::BlocksCapsule(rock,p,M::Radius,height)||ground.BlocksCapsule(p,M::Radius,height);},[](double,double){return S::Bottom;},[&](double x,double y,double ceiling){return C::StepSurface(rock,x,y,ceiling,&ground);});
    std::printf("full soil + rock walking average %.3f ms/frame\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-walkBegin).count()/180);
    Check(approach.feet[1]>-.5&&approach.feet[2]<1.1,"player reaches and traverses the walkable outcrop apron without sticking");
    // A heavily excavated map can own hundreds of sparse query objects. A
    // local player probe must visit only overlapping chunks, not all of them.
    S::Body crowded;crowded.Reset();for(int n=0;n<200;++n)
    {
        S::Key k={n%S::CX,(n/S::CX)%S::CY,(n/(S::CX*S::CY))%S::CZ};auto lo=S::Low(k),hi=G::Add(lo,{S::Span,S::Span,S::Span});
        auto q=std::make_shared<G::SurfaceQuery>();q->Build(G::Box(G::Add(lo,{.02,.02,.02}),G::Add(hi,{-.02,-.02,-.02})));crowded.queries[S::ID(k)]=q;
    }
    int localQueries=0;crowded.StaticQueries({7,5,0},{7.5,5.5,1.8},[&](auto const&){++localQueries;});
    Check(crowded.queries.size()>200&&localQueries<20,"local collision culls distant accumulated excavation queries");

    // Walk a genuine triangle-collision ramp in both directions; the camera's
    // feet must follow continuously rather than hopping by the capsule radius.
    S::Body ramp;auto rampQuery=std::make_shared<G::SurfaceQuery>();
    rampQuery->Build({{{{0,0,0},{4,0,.8},{4,2,.8},{0,2,0}},0,0}});ramp.queries[0]=rampQuery;
    M::Body onRamp{{.4,1,.4*.2+M::Radius*(std::sqrt(1.04)-1)+M::Skin}};double maxUp=0,maxDown=0;
    auto rampMove=[&](double direction)
    {
        double old=onRamp.feet[2];M::Advance(onRamp,{direction,0},1./60,[&](G::P p,double height){return ramp.BlocksCapsule(p,M::Radius,height);},[](double,double){return S::Bottom;},[&](double x,double y,double ceiling){double h=-1e30;ramp.SurfaceBelow(x,y,ceiling,h);return h;});
        return std::abs(onRamp.feet[2]-old);
    };
    for(int i=0;i<60;++i)maxUp=std::max(maxUp,rampMove(1));
    Check(onRamp.feet[0]>1.9&&maxUp<.015,"smooth capsule ramp ascent has no stair-sized hops");
    for(int i=0;i<60;++i)maxDown=std::max(maxDown,rampMove(-1));
    Check(onRamp.feet[0]<.5&&maxDown<.015&&onRamp.grounded,"descending slope follows ground without repeated falls");
    std::printf("ramp max frame rise %.4f m / descent %.4f m\n",maxUp,maxDown);

    // Full-size walkable corridor fixture: geometry collision includes ceilings.
    S::Body corridor;auto query=std::make_shared<G::SurfaceQuery>();G::Poly shell;
    auto box=[&](G::P lo,G::P hi){auto p=G::Box(lo,hi);shell.insert(shell.end(),p.begin(),p.end());};
    box({4,-.5,0},{5,2,.2});box({3.8,-.5,.2},{4,2,1.6});box({5,-.5,.2},{5.2,2,1.6});box({3.8,-.5,1.6},{5.2,2,1.8});query->Build(shell);corridor.queries[0]=query;
    M::Body walker{{4.5,-.2,.203}};walker.crouched=true;
    auto move=[&](M::Input input){M::Advance(walker,input,1./60,[&](G::P p,double height){return corridor.BlocksCapsule(p,M::Radius,height);},[](double,double){return S::Bottom;},[&](double x,double y,double ceiling){double ground=S::Bottom;corridor.SurfaceBelow(x,y,ceiling,ground);return ground;});};
    for(int i=0;i<90;++i)move({0,1,false,false,false});
    Check(walker.feet[1]>.85&&walker.feet[2]<.22,"crouched player walks inside corridor without snapping to roof");
    move({0,0,false,true,false});Check(walker.crouched,"cannot stand through soil ceiling");
    for(int i=0;i<40;++i)move({0,0,false,false,i==0});Check(walker.feet[2]+walker.Height()<1.61,"jump respects soil ceiling");

    auto captured=Trace(soil,{1,-2.4,1},{0,0,-1});auto oldMass=soil.removedMg;
    auto worker=std::async(std::launch::async,[&](){return S::Prepare(soil,rock,captured,{0,0,-1});});
    for(int i=0;i<100;++i)Check(Trace(soil,{1,-2.4,1},{0,0,-1}).position[2]==captured.position[2],"live surface unchanged during worker preparation");
    auto tx=worker.get();Check(soil.removedMg==oldMass&&tx.receipt.volume>0,"worker does not mutate live state");
    auto shared=soil.chunks.begin();Check(tx.terrain.chunks.at(shared->first)==shared->second,"untouched chunks shared instead of copied");
    Check(S::Commit(soil,std::move(tx)),"matching revision publication accepted");Check(Trace(soil,{1,-2.4,1},{0,0,-1}).position[2]<captured.position[2]-.06,"publication exposes prepared scoop");
    S::Transaction stale;stale.revision=soil.revision-1;auto conservedMass=soil.removedMg;Check(!S::Commit(soil,std::move(stale))&&soil.removedMg==conservedMass,"stale publication cannot overwrite live terrain");
    soil.Reset(&rock);Check(soil.chunks.empty()&&soil.removedMg==0&&soil.scoops==0,"reset clears edits and accounting");
    std::printf("PASS %d checks; %.2f ms total\n",checks,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
}
