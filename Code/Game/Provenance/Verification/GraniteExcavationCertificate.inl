// Included inside the authority verifier's anonymous namespace, after CheckGate.
namespace Excavation = GraniteExcavation;

static bool ExcavationSameBits(double a, double b)
{
    uint64_t left=0, right=0;
    std::memcpy(&left,&a,sizeof(left));
    std::memcpy(&right,&b,sizeof(right));
    return left == right;
}

static bool ExcavationSamePoint(Excavation::Point const& a, Excavation::Point const& b)
{
    for (int i=0;i<3;++i) if (!ExcavationSameBits(a[i],b[i])) return false;
    return true;
}

static bool ExcavationSameState(Excavation::Specimen const& a, Excavation::Specimen const& b)
{
    if (a.cells.size()!=b.cells.size()) return false;
    for (size_t i=0;i<a.cells.size();++i)
        if (a.cells[i].occupied!=b.cells[i].occupied || !ExcavationSameBits(a.cells[i].damageJ,b.cells[i].damageJ)) return false;
    return true;
}

static bool ExcavationSameReceipt(Excavation::Receipt const& a, Excavation::Receipt const& b)
{
    if (a.requestValid!=b.requestValid || a.outcome!=b.outcome || a.affectedCells!=b.affectedCells ||
        a.removedMassMg!=b.removedMassMg || a.chips.size()!=b.chips.size() ||
        a.contact.hit!=b.contact.hit || a.contact.cell!=b.contact.cell ||
        !ExcavationSamePoint(a.contact.position,b.contact.position) ||
        !ExcavationSamePoint(a.contact.normal,b.contact.normal)) return false;
    double const left[] = {a.contact.distanceM,a.transferredJ,a.damageDeltaJ,a.fractureWorkJ,a.dissipatedJ,a.removedVolumeM3};
    double const right[] = {b.contact.distanceM,b.transferredJ,b.damageDeltaJ,b.fractureWorkJ,b.dissipatedJ,b.removedVolumeM3};
    for (int i=0;i<7;++i) if (!ExcavationSameBits(left[i],right[i])) return false;
    for (size_t i=0;i<a.chips.size();++i)
        if (a.chips[i].sourceCell!=b.chips[i].sourceCell || a.chips[i].massMg!=b.chips[i].massMg ||
            !ExcavationSamePoint(a.chips[i].minimum,b.chips[i].minimum)) return false;
    return true;
}

static bool ExcavationClosedBoundary(Excavation::Specimen const& specimen)
{
    std::map<std::pair<int,int>, std::pair<int,int>> edges;
    auto vertexID=[](Excavation::Point const& p)
    { return int(std::lround(p[0]*100))+33*int(std::lround(p[1]*100))+1089*int(std::lround(p[2]*100)); };
    for (auto const& t : Excavation::BuildBoundary(specimen))
    {
        int const ids[]={vertexID(t.a),vertexID(t.b),vertexID(t.c)};
        for (int e=0;e<3;++e)
        {
            int const a=ids[e],b=ids[(e+1)%3];
            if (a==b) return false;
            auto& edge=edges[{std::min(a,b),std::max(a,b)}];
            ++edge.first; edge.second += a<b ? 1 : -1;
        }
    }
    for (auto const& entry : edges)
        if (entry.second.first!=2 || entry.second.second!=0) return false;
    return true;
}

static double ExcavationMeshVolume(Excavation::Specimen const& specimen)
{
    double sixVolume=0;
    for (auto const& t : Excavation::BuildBoundary(specimen))
    {
        auto const& a=t.a; auto const& b=t.b; auto const& c=t.c;
        sixVolume += a[0]*(b[1]*c[2]-b[2]*c[1]) +
            a[1]*(b[2]*c[0]-b[0]*c[2]) + a[2]*(b[0]*c[1]-b[1]*c[0]);
    }
    return sixVolume/6.0;
}

// Reconstruct the removed set from actual occupancy changes, independently of
// receipt aggregates. Validate every chip's identity, position and density mass.
static bool ExcavationConserved(Excavation::Specimen const& before,
    Excavation::Specimen const& after, Excavation::Receipt const& receipt)
{
    if (before.cells.size()!=after.cells.size()) return false;
    std::vector<bool> emitted(before.cells.size(),false);
    uint64_t massMg=0;
    for (auto const& chip : receipt.chips)
    {
        size_t const id=chip.sourceCell;
        if (id>=emitted.size() || emitted[id] || !before.cells[id].occupied || after.cells[id].occupied) return false;
        emitted[id]=true;
        Excavation::Point const expected = {double(id%32)*0.01,double((id/32)%32)*0.01,double(id/1024)*0.01};
        if (!ExcavationSamePoint(chip.minimum,expected) || chip.massMg!=2700) return false;
        massMg += chip.massMg;
    }
    uint64_t removed=0;
    double previousDamage=0, currentDamage=0;
    for (size_t id=0;id<before.cells.size();++id)
    {
        if (!before.cells[id].occupied && after.cells[id].occupied) return false;
        bool const lost=before.cells[id].occupied && !after.cells[id].occupied;
        if (lost!=emitted[id]) return false;
        if (lost) ++removed;
        previousDamage += before.cells[id].damageJ;
        currentDamage += after.cells[id].damageJ;
    }
    double const measuredLoss=ExcavationMeshVolume(before)-ExcavationMeshVolume(after);
    return ExcavationClosedBoundary(after) && massMg==receipt.removedMassMg && massMg==removed*2700 &&
        std::abs(measuredLoss-double(massMg)/1000000.0/2700.0)<1e-10 &&
        std::abs(measuredLoss-receipt.removedVolumeM3)<1e-10 &&
        std::abs(currentDamage-previousDamage-receipt.damageDeltaJ)<1e-9 &&
        std::abs(receipt.transferredJ-receipt.damageDeltaJ-receipt.fractureWorkJ-receipt.dissipatedJ)<1e-9;
}

static void RunGraniteExcavationCertificate(GraniteAuthorityVerificationReport& report)
{
    using namespace Excavation;
    auto check = [&](bool pass, char const* name)
    { ++report.m_numExcavationChecks; CheckGate(report,pass,0,"GraniteLocalExcavationV2",name); };
    Specimen pristine, specimen, replay;
    Strike strike;
    check(std::abs(ExcavationMeshVolume(pristine)-0.32*0.32*0.32)<1e-10,"InitialCoherentVolume");

    Strike weak=strike; weak.implementGrade=0;
    auto weakReceipt=Apply(specimen,weak);
    check(weakReceipt.requestValid && weakReceipt.contact.hit && weakReceipt.transferredJ>0 &&
        weakReceipt.outcome==Outcome::InsufficientGrade && weakReceipt.removedMassMg==0 &&
        ExcavationSameState(pristine,specimen) && ExcavationConserved(pristine,specimen,weakReceipt),"WeakContactZeroRemovalReceipt");
    Strike zero=strike; zero.energyJ=0;
    auto zeroReceipt=Apply(specimen,zero);
    check(zeroReceipt.contact.hit && zeroReceipt.outcome==Outcome::ZeroTransfer &&
        ExcavationSameState(pristine,specimen),"ZeroEnergyContactReceipt");
    Strike invalid=strike; invalid.energyJ=std::numeric_limits<double>::quiet_NaN();
    check(!Apply(specimen,invalid).requestValid && ExcavationSameState(pristine,specimen),"InvalidEnergyNoMutation");
    Strike miss=strike; miss.rayOrigin[0]=1;
    check(Apply(specimen,miss).outcome==Outcome::Miss && ExcavationSameState(pristine,specimen),"MissNoMutation");
    Strike interior=strike; interior.rayOrigin[2]=0.2;
    check(!Raycast(specimen,interior).hit,"InteriorOriginCannotMineHiddenMatter");
    Strike shortReach=strike; shortReach.reachM=0.1;
    check(!Raycast(specimen,shortReach).hit,"ReachLimit");

    Contact const first=Raycast(specimen,strike);
    for (int hit=0;hit<6;++hit)
    {
        Specimen before=specimen;
        auto receipt=Apply(specimen,strike);
        auto repeated=Apply(replay,strike);
        check(ExcavationConserved(before,specimen,receipt),"PerStrikeMeshMatterEnergyConservation");
        check(ExcavationSameState(specimen,replay) && ExcavationSameReceipt(receipt,repeated),"OrderedReplayExactNamedFields");
        check(receipt.affectedCells==1,"SmallImplementLocalFootprint");
        if (hit==0 || hit==1)
            check(receipt.removedMassMg==0 && specimen.cells[first.cell].damageJ>before.cells[first.cell].damageJ,"LocalDamageAccumulatesBeforeRemoval");
        if (hit==2)
        {
            check(receipt.removedMassMg==2700 && !specimen.cells[first.cell].occupied,"ThirdHitRemovesDamagedCell");
            auto dropped=receipt; dropped.chips.clear();
            auto duplicate=receipt; duplicate.chips.push_back(receipt.chips.front());
            auto wrongMass=receipt; wrongMass.chips.front().massMg++;
            auto wrongLocation=receipt; wrongLocation.chips.front().minimum[0]+=0.01;
            check(!ExcavationConserved(before,specimen,dropped),"DroppedChipDetected");
            check(!ExcavationConserved(before,specimen,duplicate),"DuplicateChipDetected");
            check(!ExcavationConserved(before,specimen,wrongMass),"ChipMassCorruptionDetected");
            check(!ExcavationConserved(before,specimen,wrongLocation),"ChipLocationCorruptionDetected");
            report.m_numNegativeControls += 4;
        }
    }
    Contact const deep=Raycast(specimen,strike);
    check(deep.hit && std::abs(first.position[2]-deep.position[2]-0.02)<1e-12,"RepeatedHitsDeepenUpdatedSurface");

    Specimen beforeNeighbor=specimen;
    Strike neighbor=strike; neighbor.rayOrigin[0]+=0.2;
    auto neighboring=Apply(specimen,neighbor);
    bool independent=neighboring.contact.hit && neighboring.contact.cell!=deep.cell && neighboring.removedMassMg==0;
    for (size_t id=0;id<specimen.cells.size();++id)
        if (int(id)!=neighboring.contact.cell && (specimen.cells[id].occupied!=beforeNeighbor.cells[id].occupied ||
            !ExcavationSameBits(specimen.cells[id].damageJ,beforeNeighbor.cells[id].damageJ))) independent=false;
    check(independent && ExcavationConserved(beforeNeighbor,specimen,neighboring),"Neighbor20cmDoesNotAdvanceFirstSite");

    for (int axis=0;axis<3;++axis)
    for (int sign : {-1,1})
    {
        Specimen side;
        Strike sideStrike; sideStrike.rayOrigin={0.155,0.155,0.155}; sideStrike.direction={0,0,0};
        sideStrike.rayOrigin[axis]=sign<0 ? -0.2 : 0.6;
        sideStrike.direction[axis]=double(-sign); sideStrike.energyJ=1.1;
        auto contact=Apply(side,sideStrike);
        check(contact.contact.hit && contact.removedMassMg==2700 && ExcavationConserved(pristine,side,contact),"AllSixFacesAcceptLocalExcavation");
    }
    Specimen wide;
    // Recessed contact near a voxel edge is visible through a narrow opening,
    // while a parallel ray through that voxel's CENTER is blocked by the lip.
    // V1 incorrectly threw away the authoritative contact in this situation.
    for (int axis=0;axis<3;++axis)
    for (int side : {-1,1})
    for (int tilt : {-1,1})
    {
        Specimen recess;
        int const stride[3]={1,32,1024};
        int const target=16+16*32+16*1024;
        for (int depth=0;depth<32;++depth)
            if ((side>0 && depth>16) || (side<0 && depth<16))
            { auto& c=recess.cells[target+(depth-16)*stride[axis]]; c.occupied=false; }
        Specimen before=recess;
        int const u=(axis+1)%3;
        Strike edge;
        edge.rayOrigin={0.165,0.165,0.165};
        edge.direction={0,0,0};
        edge.direction[axis]=-double(side);
        edge.direction[u]=double(tilt)*0.05;
        double const face=side>0 ? 0.17 : 0.16;
        edge.rayOrigin[axis]=side>0 ? 0.6 : -0.3;
        double const travel=std::abs(edge.rayOrigin[axis]-face);
        edge.rayOrigin[u]=(tilt>0 ? 0.1695 : 0.1605)-travel*edge.direction[u];
        edge.radiusM=0.0001;
        edge.energyJ=0.4;
        auto contact=Raycast(recess,edge);
        auto receipt=Apply(recess,edge);
        check(contact.hit && contact.cell==target,"RecessedEdgeActualContact");
        check(receipt.affectedCells==1 && recess.cells[target].damageJ>0,
            "RecessedEdgeMustDamageContactedMatter");
        check(ExcavationConserved(before,recess,receipt),"RecessedEdgeConservation");
    }
    Strike wideStrike=strike; wideStrike.radiusM=0.025; wideStrike.energyJ=0.4;
    auto wideReceipt=Apply(wide,wideStrike);
    check(wideReceipt.affectedCells>1 && wideReceipt.removedMassMg==0 && ExcavationConserved(pristine,wide,wideReceipt),"ImplementFootprintDistributesFiniteEnergy");
    // Play-lab chisel calibration: several hits create a visible multi-cell
    // cavity rather than relying exclusively on the single-cell point test.
    Specimen chisel;
    Strike chiselStrike=strike; chiselStrike.radiusM=0.015; chiselStrike.energyJ=4.0;
    uint64_t chiselMass=0;
    for (int hit=0;hit<6;++hit)
    {
        Specimen before=chisel;
        auto receipt=Apply(chisel,chiselStrike);
        chiselMass+=receipt.removedMassMg;
        check(receipt.contact.hit && ExcavationConserved(before,chisel,receipt),"PlayChiselLocalMeshMatterConservation");
    }
    check(chiselMass>2700 && Raycast(chisel,chiselStrike).position[2]<0.32,"PlayChiselRepeatedVisibleCavity");
    Specimen hard;
    MaterialResponse resistance; resistance.removalWorkJPerM3=2000000;
    Strike stronger=strike; stronger.energyJ=1.1;
    auto hardReceipt=Apply(hard,stronger,resistance);
    check(hardReceipt.contact.hit && hardReceipt.removedMassMg==0 && hard.cells[first.cell].damageJ==1.1,"MaterialResistanceControlsRemoval");
    Specimen oblique;
    Strike angled=strike; angled.direction={0.2,0,-1};
    auto angledReceipt=Apply(oblique,angled);
    check(angledReceipt.contact.hit && angledReceipt.transferredJ>0 && angledReceipt.transferredJ<angled.energyJ &&
        ExcavationConserved(pristine,oblique,angledReceipt),"IncidenceControlsTransferredEnergy");

    // Exhaust a complete column; further rays must miss, not harvest old cells.
    Specimen column;
    std::vector<bool> harvested(Side*Side*Side,false);
    uint64_t recovered=0;
    bool unique=true;
    for (int layer=0;layer<Side;++layer)
    {
        auto removed=Apply(column,stronger);
        if (!removed.contact.hit || removed.chips.size()!=1) unique=false;
        for (auto const& chip : removed.chips)
        { if (harvested[chip.sourceCell]) unique=false; harvested[chip.sourceCell]=true; recovered+=chip.massMg; }
    }
    Specimen exhausted=column;
    auto finalHit=Apply(column,stronger);
    check(unique && recovered==uint64_t(Side)*2700 && finalHit.outcome==Outcome::Miss &&
        ExcavationSameState(exhausted,column),"NoReharvestAfterColumnExhausted");
    check(std::abs(ExcavationMeshVolume(pristine)-ExcavationMeshVolume(column)-double(recovered)/1e6/2700)<1e-10,
        "TerminalRemovedMeshMatchesActualChips");
}
