#pragma once

namespace EE::OutcropLab
{
    // Engine Gizmo::SetMode asserts before its same-mode early return.
    // Never reconfigure an active drag, including a request for its current mode.
    template<typename Gizmo>
    void SetIdleGizmoMode(Gizmo& gizmo, typename Gizmo::Mode mode)
    {
        if(!gizmo.IsManipulating() && gizmo.GetMode()!=mode)gizmo.SetMode(mode);
    }
}
