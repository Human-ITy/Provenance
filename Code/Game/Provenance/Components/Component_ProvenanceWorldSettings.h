#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"
#include "Engine/Render/RenderMaterial.h"

namespace EE
{
    class EE_GAME_API ProvenanceWorldSettingsComponent final : public EntityComponent
    {
        EE_ENTITY_COMPONENT( ProvenanceWorldSettingsComponent );

    public:

        ProvenanceWorldSettingsComponent() = default;

        uint32_t GetSeed() const { return m_seed; }

        Render::Material const* GetGraniteMaterial() const
        {
            return m_graniteMaterial.IsLoaded()
                ? m_graniteMaterial.GetPtr()
                : nullptr;
        }

        Render::Material const* GetSoilMaterial() const
        {
            return m_soilMaterial.IsLoaded() ? m_soilMaterial.GetPtr() : nullptr;
        }
        Render::Material const* GetGrassCoverMaterial() const { return m_grassCoverMaterial.IsLoaded()?m_grassCoverMaterial.GetPtr():nullptr; }
        float GetGrassWearPerMeter() const { return m_grassWearPerMeter; }
        float GetGrassRecoveryDelay() const { return m_grassRecoveryDelay; }
        float GetGrassRecoverySeconds() const { return m_grassRecoverySeconds; }
        float GetGrassTimeScale() const { return m_grassTimeScale; }

    private:

        EE_REFLECT( Category = "Provenance" );
        uint32_t m_seed = 12345;

        EE_REFLECT( Category = "Provenance|Rendering" );
        TResourcePtr<Render::Material> m_graniteMaterial = ResourceID
        (
            "data://provenance/materials/granite/granite.material"
        );

        EE_REFLECT( Category = "Provenance|Rendering" );
        TResourcePtr<Render::Material> m_soilMaterial = ResourceID
        (
            "data://provenance/materials/soil/soil.material"
        );
        EE_REFLECT( Category = "Provenance|Rendering" );
        TResourcePtr<Render::Material> m_grassCoverMaterial = ResourceID("data://provenance/materials/grass/grasscover.material");
        EE_REFLECT( Category = "Provenance|Grass" );
        float m_grassWearPerMeter = 0.20f;
        EE_REFLECT( Category = "Provenance|Grass" );
        float m_grassRecoveryDelay = 120.0f;
        EE_REFLECT( Category = "Provenance|Grass" );
        float m_grassRecoverySeconds = 1200.0f;
        EE_REFLECT( Category = "Provenance|Grass" );
        float m_grassTimeScale = 1.0f;
    };
}
