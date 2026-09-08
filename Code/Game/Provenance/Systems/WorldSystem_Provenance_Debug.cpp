#include "WorldSystem_Provenance.h"
#include "GraniteOutcropLab.h"
#include "Game/Provenance/Components/Component_ProvenanceWorldSettings.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void ProvenanceWorldSystem::DebugDraw(
        EntityWorldUpdateContext const& ctx )
    {
        if ( m_pSettings ==
             nullptr )
        {
            return;
        }

        uint32_t const seed =
            m_pSettings->GetSeed();

        if ( ctx.IsGameWorld() && OutcropLab::Tick(this,ctx,*m_pSettings) )
        {
            return;
        }

        DebugDrawWorkbench( ctx, seed );
    }

    void ProvenanceWorldSystem::ReleaseGraniteToolStrikeLabState()
    {
        OutcropLab::Release(this);
        ReleaseDebugWorkbenchState();
    }
}
#endif
