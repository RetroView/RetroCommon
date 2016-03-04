#include "logvisor/logvisor.hpp"
#include "CTweaks.hpp"
#include "CResFactory.hpp"
#include "CResLoader.hpp"
#include "GameGlobalObjects.hpp"
#include "DataSpec/DNAMP1/Tweaks/CTweakGame.hpp"
#include "DataSpec/DNAMP1/Tweaks/CTweakPlayer.hpp"
#include "DataSpec/DNAMP1/Tweaks/CTweakPlayerControl.hpp"

namespace urde
{
DataSpec::ITweakGame*   g_tweakGame = nullptr;
DataSpec::ITweakPlayer* g_tweakPlayer = nullptr;
DataSpec::ITweakPlayerControl* g_tweakPlayerControl = nullptr;

namespace MP1
{

logvisor::Module Log("MP1::CTweaks");

static const SObjectTag& IDFromFactory(CResFactory& factory, const char* name)
{
    const SObjectTag* tag = factory.GetResourceIdByName(name);
    if (!tag)
        Log.report(logvisor::Fatal, "Tweak Asset not found when loading... '%s'", name);
    return *tag;
}

void CTweaks::RegisterTweaks()
{
#if 0
    CResFactory& factory = *g_ResFactory;
    CResLoader& loader = factory.GetLoader();
    std::unique_ptr<CInputStream> strm;

    strm.reset(loader.LoadNewResourceSync(IDFromFactory(factory, "Game"), nullptr));
    TOneStatic<DNAMP1::CTweakGame> game(*strm);
    g_tweakGame = game.GetAllocSpace();
    strm.reset(loader.LoadNewResourceSync(IDFromFactory(factory, "Player"), nullptr));
    TOneStatic<DNAMP1::CTweakPlayer> player(*strm);
    g_tweakPlayer = player.GetAllocSpace();

    strm.reset(loader.LoadNewResourceSync(IDFromFactory(factory, "PlayerControls"), nullptr));
    TOneStatic<DNAMP1::CTweakPlayerControl> playerControl(*strm);
    g_tweakPlayerControl = playerControl.GetAllocSpace();
#endif
}

void CTweaks::RegisterResourceTweaks()
{
}

}
}
