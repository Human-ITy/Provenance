#pragma once
#include "Engine/Imgui/ImguiGizmo.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/Entity.h"
#include "GraniteLabAssetPlacement.h"
#include "GraniteLabGizmoMode.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Runtime-only placement UI. Map originals are never saved/overwritten here.
namespace EE::OutcropLab
{
    // EditorTool::DrawViewportWindow focuses (and raises) the preview after
    // world DebugDraw. Keep lab overlays above that preview at render time,
    // without stealing keyboard focus or swallowing clicks in the world.
    inline void KeepAuthoringOverlaysVisible(ImGuiContext*, ImGuiContextHook*)
    {
        for(auto name:{"##GraniteLabGizmo","Granite Lab | placement"})
            if(auto* window=ImGui::FindWindowByName(name);window&&window->Active)
                ImGui::BringWindowToDisplayFront(window);
    }
    inline void InstallAuthoringOverlayOrder()
    {
        auto* context=ImGui::GetCurrentContext();
        auto owner=ImHashStr("GraniteLabAuthoringOverlayOrder");
        for(auto const& hook:context->Hooks)if(hook.Owner==owner)return;
        ImGuiContextHook hook;hook.Type=ImGuiContextHookType_RenderPre;
        hook.Owner=owner;hook.Callback=KeepAuthoringOverlaysVisible;
        ImGui::AddContextHook(context,&hook);
    }
    struct Authoring
    {
        bool open=false, key=false, apply=false, selectedRock=true, panelHovered=false;
        bool minimized=false;
        Float3 offset=Float3::Zero, committed=Float3::Zero;
        Float3 previous=Float3::Zero;
        bool hasUndo=false;
        EntityID selectedEntity;
        Transform entityBefore=Transform::Identity;
        bool hasEntityUndo=false;
        ImGuiX::Gizmo gizmo;
        AssetPlacement assets;

        void Cancel() { offset=committed; gizmo.Reset(); }

        template<typename Trace>
        void Draw(EntityWorldUpdateContext const& ctx, Float3 rockPivot, double massKg, bool canApply, Trace&& trace)
        {
            auto* runtime=ctx.GetPersistentMap();
            if(!open){assets.Cancel(runtime);return;}
            auto* viewport=ctx.GetMainViewport();if(!viewport)return;
            InstallAuthoringOverlayOrder();
            auto* manager=ctx.GetSystem<EntityWorldManager>();
            auto* world=manager?manager->GetGameWorld():nullptr;
            auto* map=world?world->GetFirstNonPersistentMap():nullptr;
            Entity* selected=nullptr;
            if(map)for(auto* e:map->GetEntities())if(e->GetID()==selectedEntity&&e->IsSpatialEntity())selected=e;
            if(!selected&&runtime)selected=runtime->FindEntity(selectedEntity);
            if(!selectedRock&&!selected){selectedRock=true;hasEntityUndo=false;}
            auto select=[&](Entity* e){selected=e;selectedRock=false;selectedEntity=e->GetID();entityBefore=e->GetWorldTransform();hasEntityUndo=true;gizmo.Reset();};
            auto top=viewport->GetTopLeftPosition();auto size=viewport->GetDimensions();
            float width=std::min(420.f,std::max(240.f,size.m_x-24.f));
            // Include both the title and button rows at the current UI font/DPI.
            // A fixed 78 px bar clipped Expand with the editor's larger font.
            auto const& style=ImGui::GetStyle();
            float compactHeight=std::max(120.f,2.f*ImGui::GetFrameHeight()+2.f*style.WindowPadding.y+style.ItemSpacing.y);
            float height=minimized?compactHeight:std::min(740.f,std::max(100.f,size.m_y-52.f));
            ImGui::SetNextWindowPos(ImVec2(top.m_x+size.m_x-width-12,top.m_y+36),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
            bool visible=ImGui::Begin("Granite Lab | placement",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoDocking);
            panelHovered=ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
            if(visible&&ImGui::Button(minimized?"Expand tools":"Minimize tools"))minimized=!minimized;
            visible=visible&&!minimized;
            if(visible)
            {
                ImGui::TextWrapped("F2: return to play. RMB + WASD: inspect. Session-only edits; simulation paused. Click an object to select; drag its gizmo to adjust.");
                ImGui::Separator();
                if(ImGui::CollapsingHeader("Add new asset"))
                {
                    ImGui::TextWrapped("Click a catalog item, then click/release on soil or rock. Or drag it out and release. Originals stay in place. Escape cancels.");
                    if(map)for(auto* e:map->GetEntities())if(AssetPlacement::IsCatalogAsset(e))
                    {
                        ImGui::PushID(e);ImGui::BeginDisabled(!AssetPlacement::CanCopy(e));
                        bool clicked=ImGui::Button(e->GetNameID().c_str());
                        bool drag=ImGui::IsItemActive()&&ImGui::IsMouseDragging(ImGuiMouseButton_Left)&&!assets.gesture.pending;
                        if(clicked||drag){Cancel();assets.Begin(runtime,e->GetID(),drag);}
                        ImGui::EndDisabled();ImGui::PopID();
                    }
                    ImGui::TextWrapped("New interactive granite instances and sculpt brushes are not available yet.");
                }
                if(assets.gesture.pending)
                {
                    ImGui::TextWrapped("PLACEMENT PREVIEW: release over a valid surface to keep it. Red/hidden means no valid placement or still loading.");
                    if(ImGui::Button("Cancel new asset"))assets.Cancel(runtime);
                }
                if(!assets.placed.empty()&&ImGui::Button("Undo last added asset"))
                {assets.UndoLast(runtime,selectedEntity);selected=nullptr;selectedRock=true;hasEntityUndo=false;gizmo.Reset();}
                ImGui::Separator();ImGui::TextUnformatted("Scene selection (originals and additions)");
                if(ImGui::Selectable("Granite outcrop [interactive]",selectedRock)){assets.Cancel(runtime);selectedRock=true;gizmo.Reset();}
                if(map)for(auto* e:map->GetEntities())
                {
                    if(!e->IsSpatialEntity()||std::strncmp(e->GetNameID().c_str(),"NatureTrial_",12)!=0)continue;
                    if(ImGui::Selectable(e->GetNameID().c_str(),!selectedRock&&selectedEntity==e->GetID()))
                    {assets.Cancel(runtime);select(e);}
                }
                if(runtime)for(auto id:assets.placed)if(auto* e=runtime->FindEntity(id))
                {
                    ImGui::PushID(e);
                    if(ImGui::Selectable(e->GetNameID().c_str(),!selectedRock&&selectedEntity==id)){assets.Cancel(runtime);select(e);}
                    ImGui::PopID();
                }
                ImGui::Separator();
                if(selectedRock)
                {
                    ImGui::TextWrapped("Granite mass: %.3f kg (unchanged)",massKg);
                    ImGui::TextWrapped("Whole coherent outcrop, including buried satellite connections. Translation only in this pass.");
                    ImGui::TextUnformatted("East / west (m)");ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##eastwest",&offset.m_x,-2,2,"%.3f");
                    ImGui::TextUnformatted("North / south (m)");ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##northsouth",&offset.m_y,-2,2,"%.3f");
                    ImGui::TextUnformatted("Elevation offset (m)");ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##elevation",&offset.m_z,-.25f,.6f,"%.3f");
                    ImGui::TextWrapped("Preview only. Apply refits pristine soil around the moved rock; the terrain heightfield stays fixed. This starts a new lab fixture, not a gameplay action.");
                    ImGui::BeginDisabled(!canApply);
                    if(ImGui::Button("Apply placement"))apply=true;
                    ImGui::EndDisabled();
                    ImGui::SameLine();if(ImGui::Button("Cancel preview"))Cancel();
                    if(ImGui::Button("Original placement"))offset=Float3::Zero;
                    ImGui::SameLine();ImGui::BeginDisabled(!canApply||!hasUndo);
                    if(ImGui::Button("Undo placement")){offset=previous;apply=true;}
                    ImGui::EndDisabled();
                    if(!canApply)ImGui::TextWrapped("Apply locked: wait for publication, or return to play and press R to reset mined/damaged granite and excavated soil.");
                }
                else if(selected)
                {
                    ImGui::TextWrapped("Decorative asset: no harvesting or conserved mass. Changes last only for this Play session.");
                    auto t=selected->GetWorldTransform();auto p=t.GetTranslation().ToFloat3();float scale=t.GetScale();
                    bool changed=ImGui::DragFloat3("World position (m)",&p.m_x,.01f,-100.f,100.f,"%.3f");
                    changed=ImGui::SliderFloat("Uniform scale",&scale,.1f,3.f,"%.3f")||changed;
                    if(changed&&std::isfinite(p.m_x)&&std::isfinite(p.m_y)&&std::isfinite(p.m_z)&&std::isfinite(scale))
                    {t.SetTranslation(Vector(p));t.SetScale(std::clamp(scale,.1f,3.f));selected->SetWorldTransform(t);}
                    if(ImGui::Button("Move gizmo"))SetIdleGizmoMode(gizmo,ImGuiX::Gizmo::Mode::Translation);
                    ImGui::SameLine();if(ImGui::Button("Rotate"))SetIdleGizmoMode(gizmo,ImGuiX::Gizmo::Mode::Rotation);
                    ImGui::SameLine();if(ImGui::Button("Scale"))SetIdleGizmoMode(gizmo,ImGuiX::Gizmo::Mode::Scale);
                    if(hasEntityUndo&&ImGui::Button("Restore selection's initial pose"))selected->SetWorldTransform(entityBefore);
                }
                ImGui::Separator();ImGui::TextWrapped("Nature additions are decorative, session-only instances. Original and added items remain editable. No saved presets or granite sculpting yet.");
            }
            ImGui::End();
            if(assets.gesture.pending&&ImGui::IsKeyPressed(ImGuiKey_Escape))assets.Cancel(runtime);
            auto mouse=ImGui::GetMousePos();
            bool worldMouse=viewport->ContainsPointScreenSpace(Float2(mouse.x,mouse.y))&&!panelHovered&&!ImGui::IsAnyItemHovered();
            bool cameraInput=ImGui::IsMouseDown(ImGuiMouseButton_Right);
            auto nearPoint=viewport->ScreenSpaceToWorldSpaceNearPlane(Vector(mouse.x,mouse.y,0)).ToFloat3();
            auto farPoint=viewport->ScreenSpaceToWorldSpaceFarPlane(Vector(mouse.x,mouse.y,0)).ToFloat3();
            auto ray=GraniteContactCast::Unit({double(farPoint.m_x-nearPoint.m_x),double(farPoint.m_y-nearPoint.m_y),double(farPoint.m_z-nearPoint.m_z)});
            BuildSurface surface;
            if(worldMouse)surface=trace(nearPoint,Float3(float(ray[0]),float(ray[1]),float(ray[2])));
            bool wasPlacing=assets.gesture.pending;
            if(wasPlacing)
            {
                assets.Preview(map,runtime,surface);
                if(assets.gesture.Update(worldMouse,assets.valid,ImGui::IsMouseClicked(ImGuiMouseButton_Left),ImGui::IsMouseReleased(ImGuiMouseButton_Left),cameraInput))
                {auto id=assets.Commit();if(auto* e=runtime->FindEntity(id))select(e);}
            }
            if(selectedRock)SetIdleGizmoMode(gizmo,ImGuiX::Gizmo::Mode::Translation);
            // A non-intercepting overlay lets the existing gizmo consume its own
            // mouse tests without covering the placement panel or preview toolbar.
            ImGui::SetNextWindowPos(ImVec2(top.m_x,top.m_y),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(size.m_x,size.m_y),ImGuiCond_Always);
            ImGui::Begin("##GraniteLabGizmo",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoDocking);
            bool consumed=gizmo.IsManipulating();
            if(!wasPlacing&&(gizmo.IsManipulating()||(!panelHovered&&!ImGui::IsAnyItemActive())))
            {
                Transform t=selectedRock?Transform::Identity:selected->GetWorldTransform();
                if(selectedRock)t.SetTranslation(Vector(rockPivot+offset-committed));
                auto result=gizmo.Draw(t.GetTranslation(),t.GetRotation(),*viewport);
                if(result.IsManipulating())
                {
                    consumed=true;
                    result.ApplyResult(t);
                    if(selectedRock)offset=t.GetTranslation().ToFloat3()-rockPivot+committed;
                    else {t.SetScale(std::clamp(t.GetScale(),.1f,3.f));selected->SetWorldTransform(t);}
                }
            }
            // Gizmo handles get first refusal. Decorative selection uses bounds,
            // capped by the nearest solid surface so hidden props aren't picked.
            if(!wasPlacing&&!consumed&&worldMouse&&!cameraInput&&ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                Entity* picked=nullptr;double limit=surface.hit?surface.distance+.015:150;
                auto test=[&](Entity* e)
                {
                    if(!e||!e->IsSpatialEntity()||!e->GetRootSpatialComponent()->IsInitialized())return;
                    auto bounds=e->GetRootSpatialComponentWorldBounds().GetAABB();auto lo=bounds.GetMin().ToFloat3(),hi=bounds.GetMax().ToFloat3();
                    double t=GraniteBuildPlacement::BoxDistance({nearPoint.m_x,nearPoint.m_y,nearPoint.m_z},ray,{lo.m_x,lo.m_y,lo.m_z},{hi.m_x,hi.m_y,hi.m_z},limit);
                    if(t<=limit){limit=t;picked=e;}
                };
                if(map)for(auto* e:map->GetEntities())if(AssetPlacement::IsCatalogAsset(e))test(e);
                if(runtime)for(auto id:assets.placed)test(runtime->FindEntity(id));
                if(picked)select(picked);
                else if(surface.rock){selectedRock=true;gizmo.Reset();}
            }
            if(wasPlacing&&worldMouse)
                ImGui::GetWindowDrawList()->AddCircle(ImVec2(mouse.x,mouse.y),12,assets.valid?IM_COL32(80,230,120,255):IM_COL32(255,90,70,255),24,2);
            ImGui::End();
            if(!std::isfinite(offset.m_x)||!std::isfinite(offset.m_y)||!std::isfinite(offset.m_z))Cancel();
            offset.m_x=std::clamp(offset.m_x,-2.f,2.f);offset.m_y=std::clamp(offset.m_y,-2.f,2.f);offset.m_z=std::clamp(offset.m_z,-.25f,.6f);
        }
    };
}
