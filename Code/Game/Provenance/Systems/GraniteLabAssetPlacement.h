#pragma once
#include "Game/Provenance/Geometry/GraniteBuildPlacement.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/Entity.h"
#include <cstring>
#include <vector>

namespace EE::OutcropLab
{
    struct BuildSurface
    {
        bool hit=false,rock=false;
        Float3 position=Float3::Zero;
        double distance=150;
    };
    // Session entities belong to the Play world's persistent map, never to the
    // source map. IDs, rather than retained entity pointers, survive load phases.
    struct AssetPlacement
    {
        GraniteBuildPlacement::Gesture gesture;
        EntityID source,draft;
        std::vector<EntityID> placed;
        bool valid=false;
        static bool IsCatalogAsset(Entity const* e)
        {return e&&e->IsSpatialEntity()&&std::strncmp(e->GetNameID().c_str(),"NatureTrial_",12)==0;}
        static Render::StaticMeshComponent* Mesh(Entity* e)
        {return e&&e->IsSpatialEntity()?TryCast<Render::StaticMeshComponent>(e->GetRootSpatialComponent()):nullptr;}
        static bool CanCopy(Entity* e)
        {auto* m=Mesh(e);return IsCatalogAsset(e)&&e->GetComponents().size()==1&&m&&m->IsInitialized()&&m->IsMeshLoaded();}
        void Cancel(EntityModel::EntityMap* runtime)
        {
            if(runtime&&runtime->FindEntity(draft))runtime->DestroyEntity(draft);
            draft=EntityID();source=EntityID();gesture.Cancel();valid=false;
        }
        void Begin(EntityModel::EntityMap* runtime,EntityID id,bool drag)
        {Cancel(runtime);source=id;gesture.Begin(drag);}
        Entity* Preview(EntityModel::EntityMap* originals,EntityModel::EntityMap* runtime,BuildSurface const& surface)
        {
            valid=surface.hit;
            if(!gesture.pending||!runtime||!originals)return nullptr;
            Entity* e=runtime->FindEntity(draft);
            if(!e&&valid)
            {
                auto* original=originals->FindEntity(source);
                if(!CanCopy(original)){valid=false;return nullptr;}
                auto* src=Mesh(original);
                auto* mesh=EE::New<Render::StaticMeshComponent>(StringID("Lab placed visual"));
                mesh->SetMesh(src->GetMesh()->GetResourceID());
                mesh->SetNonUniformScale(src->GetNonUniformScale());
                for(int i=0;i<src->GetNumSubmeshes();++i)
                    if(auto id=src->GetMaterialOverrideResourceID(i);id.IsValid())mesh->SetMaterialOverride(i,id);
                e=EE::New<Entity>(original->GetNameID());e->AddComponent(mesh);
                auto t=original->GetWorldTransform();t.SetTranslation(Vector(surface.position));e->SetWorldTransform(t);
                runtime->AddEntity(e);draft=e->GetID();
            }
            if(e)
            {
                if(Mesh(e)->IsVisible()!=valid)Mesh(e)->SetVisible(valid);
                if(valid){auto t=e->GetWorldTransform();t.SetTranslation(Vector(surface.position));e->SetWorldTransform(t);}
                // Do not accept a drop before resource initialization completes.
                valid=valid&&Mesh(e)->IsInitialized()&&Mesh(e)->IsMeshLoaded();
            }
            return e;
        }
        EntityID Commit()
        {auto id=draft;placed.push_back(id);draft=EntityID();source=EntityID();valid=false;return id;}
        bool UndoLast(EntityModel::EntityMap* runtime,EntityID& selection)
        {
            if(placed.empty()||!runtime)return false;
            auto id=placed.back();placed.pop_back();
            if(runtime->FindEntity(id))runtime->DestroyEntity(id);
            if(selection==id)selection=EntityID();return true;
        }
    };
}
