#pragma once

#include "Base/Esoterica.h"

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class ProvenanceWorldSystem;
    class ProvenanceWorldSettingsComponent;
    class EntityWorldUpdateContext;

    // Development-only playable lab. Its state and implementation are private
    // to GraniteOutcropLab.cpp; no simulation types leak into the debug host.
    namespace OutcropLab
    {
        bool Tick(ProvenanceWorldSystem* owner, EntityWorldUpdateContext const& ctx,
                  ProvenanceWorldSettingsComponent const& settings);
        void Release(ProvenanceWorldSystem* owner);
    }
}
#endif
