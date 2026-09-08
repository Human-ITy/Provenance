#pragma once
#include "Game/Provenance/Geometry/ToolPickup.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include <cstring>

namespace EE::OutcropLab
{
    struct ToolPickups
    {
        ToolPickup::Inventory inventory;
        bool aimed=false;
        ToolPickup::Kind targetKind=ToolPickup::Kind::None;
        template<typename Occlusion>
        void Update(EntityWorldUpdateContext const& ctx,Float3 origin,Float3 forward,bool down,bool enabled,Occlusion&& occlusion)
        {
            aimed=false;targetKind=ToolPickup::Kind::None;
            auto* manager=ctx.GetSystem<EntityWorldManager>();
            auto* world=manager?manager->GetGameWorld():nullptr;
            auto* map=world?world->GetFirstNonPersistentMap():nullptr;
            Render::StaticMeshComponent* target=nullptr;double nearest=ToolPickup::Reach+1;
            if(ctx.IsGameWorld()&&map&&enabled)
            {
                for(auto* e:map->GetEntities())
                {
                    if(!e->IsSpatialEntity())continue;
                    auto kind=ToolPickup::Kind::None;
                    for(int i=0;i<4;++i)if(std::strcmp(e->GetNameID().c_str(),ToolPickup::EntityNames[i])==0){kind=ToolPickup::Kind(i);break;}
                    if(!ToolPickup::Valid(kind))continue;
                    auto* mesh=TryCast<Render::StaticMeshComponent>(e->GetRootSpatialComponent());
                    if(!mesh||!mesh->IsInitialized()||!mesh->IsMeshLoaded())continue;
                    // The item belongs to this Play session, including delayed resource loads.
                    if(inventory.Owns(kind)){if(mesh->IsVisible())mesh->SetVisible(false);continue;}
                    if(!mesh->IsVisible())continue;
                    auto const& t=e->GetWorldTransform();
                    auto o=t.InverseTransformPoint(Vector(origin)).ToFloat3();
                    auto d=t.InverseTransformVector(Vector(forward)).ToFloat3();
                    double hit=ToolPickup::Hit(kind,{o.m_x,o.m_y,o.m_z},{d.m_x,d.m_y,d.m_z});
                    if(hit<nearest){nearest=hit;target=mesh;targetKind=kind;}
                }
                if(target)aimed=ToolPickup::Visible(nearest,occlusion(nearest));
            }
            if(inventory.Update(down,enabled&&ctx.IsGameWorld(),aimed?targetKind:ToolPickup::Kind::None)&&target)
            {target->SetVisible(false);aimed=false;}
        }
    };
}
