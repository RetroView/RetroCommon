#ifndef __URDE_CRUMBLEMANAGER_HPP__
#define __URDE_CRUMBLEMANAGER_HPP__

#include "CRumbleGenerator.hpp"

namespace urde
{
class CStateManager;
class CRumbleManager
{
    CRumbleGenerator x0_rumbleGenerator;
public:
    CRumbleManager() = default;
    void Update(float);
    void StopRumble(u16) {}
    void Rumble(CStateManager&, ERumbleFxId, ERumblePriority priority) {}
    void Rumble(CStateManager&, ERumbleFxId, float, ERumblePriority priority) {}
};
}

#endif // __URDE_CRUMBLEMANAGER_HPP__
