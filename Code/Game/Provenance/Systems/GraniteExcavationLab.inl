// Presentation/input adapter only. The cell damage and removal law lives in
// Geometry/GraniteExcavation; this file is included in the debug world TU.
namespace ExcavationLab
{
    using Clock = std::chrono::steady_clock;
    static double Milliseconds(Clock::time_point start)
    { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
    struct Timing
    {
        double last=0, peak=0;
        void Record(Clock::time_point start) { last=Milliseconds(start); peak=std::max(peak,last); }
    };
    struct State
    {
        ProvenanceWorldSystem* owner = nullptr;
        Render::RenderWorldSystem* renderer = nullptr;
        Render::ProceduralMeshID mesh = 0;
        GraniteExcavation::Specimen specimen;
        GraniteExcavation::Receipt last;
        std::vector<GraniteExcavation::Chip> chips;
        bool active = true, dirty = true;
        bool f = false, r = false, g = false, t = false;
        int tool = 1;
        uint32_t attempts = 0, contacts = 0, removals = 0;
        uint64_t recoveredMg = 0;
        bool conserved = true;
        std::vector<size_t> damagedCells;
        GraniteExcavation::Contact aim;
        GraniteExcavation::Strike previousAim;
        bool aimDirty=true, aimCached=false, traceOK=true;
        Timing aimTime, applyTime, ledgerTime, buildTime, registerTime, unregisterTime, tickTime;
        uint64_t frame=0;
        Clock::time_point heartbeat;
    };
    static State state;
    static constexpr float X = -1.4f, Y = 27.0f, Z = 0.12f;

    // Action-stage breadcrumbs are flushed before/after calls that might block.
    // One-second heartbeats distinguish a stall inside the lab from a later
    // engine/GPU wait. This is evidence collection, not a watchdog or hang fix.
    static void Trace(char const* stage)
    {
        auto path=FileSystem::GetCurrentProcessPath()+"GraniteExcavationTrace.log";
        FILE* file=nullptr;
        if (fopen_s(&file,path.c_str(),"a")!=0 || !file) { state.traceOK=false; return; }
        std::fprintf(file,"frame=%llu attempt=%u stage=%s chips=%llu dirty=%d aimMs=%.3f applyMs=%.3f buildMs=%.3f registerMs=%.3f unregisterMs=%.3f\n",
            (unsigned long long)state.frame,state.attempts,stage,(unsigned long long)state.chips.size(),int(state.dirty),
            state.aimTime.last,state.applyTime.last,state.buildTime.last,state.registerTime.last,state.unregisterTime.last);
        if(std::strcmp(stage,"apply.end")==0)
            std::fprintf(file,"response contact=%d cell=%d affected=%u transferred=%.17g removedMg=%llu outcome=%u\n",
                int(state.last.contact.hit),state.last.contact.cell,state.last.affectedCells,state.last.transferredJ,
                (unsigned long long)state.last.removedMassMg,unsigned(state.last.outcome));
        std::fflush(file); std::fclose(file);
    }
    static void TraceRequest(GraniteExcavation::Strike const& r)
    {
        auto path=FileSystem::GetCurrentProcessPath()+"GraniteExcavationTrace.log";
        FILE* file=nullptr;
        if(fopen_s(&file,path.c_str(),"a")!=0 || !file) { state.traceOK=false; return; }
        std::fprintf(file,"request attempt=%u origin=%.17g,%.17g,%.17g direction=%.17g,%.17g,%.17g radius=%.17g energy=%.17g grade=%u reach=%.17g\n",
            state.attempts,r.rayOrigin[0],r.rayOrigin[1],r.rayOrigin[2],r.direction[0],r.direction[1],r.direction[2],
            r.radiusM,r.energyJ,r.implementGrade,r.reachM);
        std::fflush(file); std::fclose(file);
    }

    static Float3 World(GraniteExcavation::Point const& p)
    { return Float3(X+float(p[0]),Y+float(p[1]),Z+float(p[2])); }

    static void Release(ProvenanceWorldSystem* owner)
    {
        if (state.owner != owner) return;
        Trace("release.begin");
        if (state.mesh && state.renderer) state.renderer->UnregisterProceduralMesh(state.mesh);
        Trace("release.end");
        state = State();
    }

    static GraniteExcavation::Strike Request(Viewport const& viewport)
    {
        GraniteExcavation::Strike request;
        Float3 const origin = viewport.GetViewPosition().ToFloat3();
        Float3 const direction = viewport.GetViewForwardDirection().ToFloat3();
        request.rayOrigin = {double(origin.m_x)-X,double(origin.m_y)-Y,double(origin.m_z)-Z};
        request.direction = {direction.m_x,direction.m_y,direction.m_z};
        request.reachM = 2.0;
        request.radiusM = state.tool == 1 ? 0.015 : 0.004;
        request.energyJ = state.tool == 1 ? 4.0 : 0.4;
        request.implementGrade = state.tool == 2 ? 0 : 2;
        return request;
    }

    static char const* OutcomeName(GraniteExcavation::Outcome outcome)
    {
        using O = GraniteExcavation::Outcome;
        switch(outcome)
        {
            case O::InvalidRequest: return "INVALID REQUEST";
            case O::InvalidState: return "INVALID STATE";
            case O::Miss: return "MISS - aim at cube within 2 m; stay outside it";
            case O::InsufficientGrade: return "CONTACT - grade too low; zero removal";
            case O::ZeroTransfer: return "CONTACT - no damage transfer";
            case O::LocalDamage: return "LOCAL DAMAGE - hit here again to deepen";
            case O::RemovedMatter: return "REMOVED MATTER - cavity surface updated";
        }
        return "UNKNOWN";
    }

    static void Rebuild(Render::RenderWorldSystem& renderer, Render::Material const* material)
    {
        if (!state.dirty || !material) return;
        Trace("mesh.build.begin");
        auto buildStart=Clock::now();
        TVector<Render::ProceduralMeshVertex> vertices;
        TVector<uint32_t> indices;
        // Deliberately independent of the old builder's area threshold: its
        // rejection threshold discards valid 1 cm cell faces.
        auto append = [&](Float3 const& a,Float3 const& b,Float3 const& c)
        {
            Float3 n((b.m_y-a.m_y)*(c.m_z-a.m_z)-(b.m_z-a.m_z)*(c.m_y-a.m_y),
                (b.m_z-a.m_z)*(c.m_x-a.m_x)-(b.m_x-a.m_x)*(c.m_z-a.m_z),
                (b.m_x-a.m_x)*(c.m_y-a.m_y)-(b.m_y-a.m_y)*(c.m_x-a.m_x));
            float length=std::sqrt(n.m_x*n.m_x+n.m_y*n.m_y+n.m_z*n.m_z);
            if (length <= 1e-12f) return;
            n.m_x/=length; n.m_y/=length; n.m_z/=length;
            uint32_t base=uint32_t(vertices.size());
            vertices.emplace_back(Render::ProceduralMeshVertex{a,n});
            vertices.emplace_back(Render::ProceduralMeshVertex{b,n});
            vertices.emplace_back(Render::ProceduralMeshVertex{c,n});
            indices.emplace_back(base); indices.emplace_back(base+1); indices.emplace_back(base+2);
        };
        for (auto const& triangle : GraniteExcavation::BuildBoundary(state.specimen))
            append(World(triangle.a),World(triangle.b),World(triangle.c));

        // Show at most 128 actual recovered chips in a tray beside the cube.
        // Every chip remains in the ledger even when the display sample fills.
        size_t shown = std::min(size_t(128),state.chips.size());
        for (size_t i=0;i<shown;++i)
        {
            Float3 low(X+0.45f+float(i%16)*0.014f,Y+float(i/16)*0.014f,Z);
            Float3 v[8];
            for(int k=0;k<8;++k) v[k]=Float3(low.m_x+((k&1)?0.01f:0),low.m_y+((k&2)?0.01f:0),low.m_z+((k&4)?0.01f:0));
            int const faces[12][3]={{0,2,3},{0,3,1},{4,5,7},{4,7,6},{0,1,5},{0,5,4},
                {2,6,7},{2,7,3},{0,4,6},{0,6,2},{1,3,7},{1,7,5}};
            for(auto const& face:faces) append(v[face[0]],v[face[1]],v[face[2]]);
        }
        state.buildTime.Record(buildStart);
        Trace("mesh.build.end");
        Trace("mesh.register.begin");
        auto registerStart=Clock::now();
        auto replacement = vertices.empty() ? Render::ProceduralMeshID(0) : renderer.RegisterProceduralMesh(
            vertices,indices,material,Transform::Identity,
            TBitFlags<Render::ViewLayer>(Render::ViewLayer::ShadowMap,Render::ViewLayer::ForwardShading));
        state.registerTime.Record(registerStart);
        Trace("mesh.register.end");
        if (!vertices.empty() && replacement==0) return; // Keep last valid mesh; retry.
        Trace("mesh.unregister.begin");
        auto unregisterStart=Clock::now();
        if (state.mesh) renderer.UnregisterProceduralMesh(state.mesh);
        state.unregisterTime.Record(unregisterStart);
        Trace("mesh.unregister.end");
        state.mesh=replacement;
        state.dirty=false;
    }

    static bool Tick(ProvenanceWorldSystem* owner, EntityWorldUpdateContext const& ctx,
        DebugDrawContext& draw, Render::RenderWorldSystem* renderer, Render::Material const* material)
    {
        if (state.owner!=owner)
        {
            // Engine permits one game world. Shutdown releases before its renderer dies.
            state = State(); state.owner=owner; state.renderer=renderer;
            Trace("session.begin.v2");
        }
        state.frame=ctx.GetFrameID();
        bool heartbeat=Clock::now()-state.heartbeat>std::chrono::seconds(1);
        if(heartbeat) { state.heartbeat=Clock::now(); Trace("tick.begin"); }
        struct TickTimer
        {
            Clock::time_point start=Clock::now(); bool heartbeat;
            ~TickTimer() { state.tickTime.Record(start); if(heartbeat) Trace("tick.end"); }
        } tickTimer{Clock::now(),heartbeat};
        auto const* viewport=ctx.GetMainViewport();
        auto const* input=ctx.GetSystem<Input::InputSystem>();
        if (!viewport) return state.active;
        bool switched=false;
        bool pressF=false,pressR=false,pressT=false;
        if (input)
        {
            auto const* keyboard=input->GetKeyboardMouse();
            bool f=keyboard->IsHeldDown(Input::InputID::Keyboard_F);
            bool r=keyboard->IsHeldDown(Input::InputID::Keyboard_R);
            bool g=keyboard->IsHeldDown(Input::InputID::Keyboard_G);
            bool t=keyboard->IsHeldDown(Input::InputID::Keyboard_T);
            pressF=f&&!state.f; pressR=r&&!state.r; pressT=t&&!state.t;
            switched=g&&!state.g;
            state.f=f; state.r=r; state.g=g; state.t=t;
            if(switched) state.active=!state.active;
        }
        if (state.active && !switched)
        {
            if (pressT) state.tool=(state.tool+1)%3;
            if (pressR)
            {
                state.specimen=GraniteExcavation::Specimen(); state.last={}; state.chips.clear();
                state.attempts=state.contacts=state.removals=0; state.recoveredMg=0;
                state.conserved=true; state.dirty=true;
                state.damagedCells.clear(); state.aimDirty=true;
                Trace("reset");
            }
            else if (pressF)
            {
                ++state.attempts;
                auto request=Request(*viewport);
                TraceRequest(request);
                Trace("apply.begin");
                auto applyStart=Clock::now();
                state.last=GraniteExcavation::Apply(state.specimen,request);
                state.applyTime.Record(applyStart);
                state.aimDirty=true;
                Trace("apply.end");
                if (state.last.contact.hit) ++state.contacts;
                if (!state.last.chips.empty())
                {
                    ++state.removals;
                    state.chips.insert(state.chips.end(),state.last.chips.begin(),state.last.chips.end());
                    state.dirty=true;
                }
                Trace("ledger.begin");
                auto ledgerStart=Clock::now();
                uint64_t occupied=0, recovered=0;
                std::vector<bool> seen(state.specimen.cells.size(),false);
                state.conserved=true;
                for(auto const& cell:state.specimen.cells) if(cell.occupied) ++occupied;
                for(auto const& chip:state.chips)
                {
                    size_t id=chip.sourceCell;
                    if(id>=seen.size() || seen[id] || state.specimen.cells[id].occupied || chip.massMg!=2700)
                        state.conserved=false;
                    else seen[id]=true;
                    recovered+=chip.massMg;
                }
                state.recoveredMg=recovered;
                state.conserved=state.conserved && occupied*2700+recovered==uint64_t(32*32*32)*2700;
                state.damagedCells.clear();
                for(size_t id=0;id<state.specimen.cells.size();++id)
                    if(state.specimen.cells[id].occupied && state.specimen.cells[id].damageJ>0) state.damagedCells.push_back(id);
                state.ledgerTime.Record(ledgerStart);
                Trace("ledger.end");
            }
        }
        if(renderer) { state.renderer=renderer; Rebuild(*renderer,material); }
        draw.DrawText3D(Float3(X+0.16f,Y+0.16f,Z+0.45f),
            state.active ? "LOCAL EXCAVATION - 32 cm Granite - F strike" : "LOCAL EXCAVATION - press G to select",Colors::Cyan);
        if(!state.active)
        {
            draw.DrawText2D(Float2(930,45),"G: switch to local excavation (cube beside this lab)",Colors::Cyan);
            return false;
        }
        draw.DrawText3D(Float3(-3.0f,27.0f,1.0f),"BRIDGE REGRESSION - G to select",Colors::White);

        auto request=Request(*viewport);
        state.aimCached=!state.aimDirty && request.rayOrigin==state.previousAim.rayOrigin && request.direction==state.previousAim.direction;
        auto aimStart=Clock::now();
        if(!state.aimCached)
        {
            auto distant=request; distant.reachM=10;
            state.aim=GraniteExcavation::Raycast(state.specimen,distant);
            state.previousAim=request; state.aimDirty=false;
        }
        state.aimTime.Record(aimStart);
        auto beyond=state.aim;
        auto contact=beyond;
        if(contact.distanceM>request.reachM) contact={};
        auto color = !contact.hit ? Colors::White : (state.tool==2 ? Colors::Yellow : Colors::Green);
        if(contact.hit)
        {
            // Surface-tangent guide, not a claim of a sub-cell fracture boundary.
            int axis=0;
            for(int a=1;a<3;++a) if(std::abs(contact.normal[a])>std::abs(contact.normal[axis])) axis=a;
            int u=(axis+1)%3,v=(axis+2)%3;
            for(int segment=0;segment<32;++segment)
            {
                auto a=contact.position,b=a;
                a[axis]+=contact.normal[axis]*0.001; b[axis]=a[axis];
                double t0=double(segment)*6.283185307179586/32,t1=double(segment+1)*6.283185307179586/32;
                a[u]+=request.radiusM*std::cos(t0); a[v]+=request.radiusM*std::sin(t0);
                b[u]+=request.radiusM*std::cos(t1); b[v]+=request.radiusM*std::sin(t1);
                draw.DrawLine(World(a),World(b),color,2,DebugDrawLayer::World);
            }
        }
        // Persistent local damage marks until those cells actually leave.
        for(size_t id : state.damagedCells)
        {
            auto const& cell=state.specimen.cells[id];
            if(!cell.occupied || cell.damageJ<=0) continue;
            GraniteExcavation::Point p={double(id%32)*0.01,double((id/32)%32)*0.01,double(id/1024)*0.01};
            for(int axis=0;axis<3;++axis) for(int corner=0;corner<4;++corner)
            {
                auto a=p,b=p; int u=(axis+1)%3,v=(axis+2)%3;
                a[u]+= (corner&1)?0.01:0; a[v]+=(corner&2)?0.01:0;
                b=a; b[axis]+=0.01;
                draw.DrawLine(World(a),World(b),Colors::Yellow,1,DebugDrawLayer::World);
            }
        }
        Float2 dimensions=viewport->GetDimensions();
        draw.DrawText2D(Float2(dimensions.m_x*0.5f,dimensions.m_y*0.5f),"[ + ]",color,DebugFont::Normal,DebugTextAlign::MiddleCenter);
        float px=dimensions.m_x*0.5f,py=65;
        auto line=[&](char const* text, Color c=Colors::White)
        { draw.DrawText2D(Float2(px,py),text,c); py+=17; };
        char buffer[256];
        line("LOCAL CUMULATIVE EXCAVATION v2 - separate specimen",Colors::Cyan);
        line("RMB aim | F strike | R reset | T tool | G bridge lab");
        std::snprintf(buffer,sizeof(buffer),"Tool: %s | radius %.1f mm | %.1f J | F %s",
            state.tool==0?"POINT":state.tool==1?"CHISEL":"HAND (insufficient grade)",
            request.radiusM*1000,request.energyJ,state.f?"DOWN":"UP"); line(buffer);
        if(contact.hit)
            std::snprintf(buffer,sizeof(buffer),"HIT %.3f m | cell %d | local damage %.3f / 1.000 J",
                contact.distanceM,contact.cell,state.specimen.cells[contact.cell].damageJ);
        else if(beyond.hit) std::snprintf(buffer,sizeof(buffer),"OUT OF REACH %.2f m | approach within 2 m",beyond.distanceM);
        else std::snprintf(buffer,sizeof(buffer),"NO HIT - aim at 32 cm cube, not head | stay outside");
        line(buffer,color);
        std::snprintf(buffer,sizeof(buffer),"Attempts %u | contacts %u | removing strikes %u",state.attempts,state.contacts,state.removals); line(buffer);
        line(state.attempts ? OutcomeName(state.last.outcome) : "READY - repeated hits deepen the contacted location");
        std::snprintf(buffer,sizeof(buffer),"Last: %.3f g / %.2f cm3 | affected cells %u",
            double(state.last.removedMassMg)/1000,state.last.removedVolumeM3*1e6,state.last.affectedCells); line(buffer);
        std::snprintf(buffer,sizeof(buffer),"Recovered %.3f g | chips %llu | matter ledger %s",double(state.recoveredMg)/1000,
            (unsigned long long)state.chips.size(),state.conserved?"PASS":"FAIL"); line(buffer,state.conserved?Colors::Cyan:Colors::Red);
        line("Chip tray shows first 128; all recovered matter stays counted.");
        line("1 cm cells | calibrated test tools | no support/detachment yet");
        std::snprintf(buffer,sizeof(buffer),"CPU ms: aim %.2f (%s) | prev lab tick %.2f | peak %.2f",
            state.aimTime.last,state.aimCached?"cached":"query",state.tickTime.last,state.tickTime.peak); line(buffer);
        std::snprintf(buffer,sizeof(buffer),"Last action ms: apply %.2f | ledger %.2f | mesh build %.2f",
            state.applyTime.last,state.ledgerTime.last,state.buildTime.last); line(buffer);
        std::snprintf(buffer,sizeof(buffer),"Mesh register %.2f / remove %.2f ms | trace %s",
            state.registerTime.last,state.unregisterTime.last,state.traceOK?"ON":"FILE ERROR"); line(buffer);
        if(state.dirty || !material) line("MATERIAL MESH NOT READY - presentation pending",Colors::Red);
        return true;
    }
}
