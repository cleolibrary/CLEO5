#include "stdafx.h"
#include "CleoBase.h"
#include "CDebug.h"
#include "CConfigManager.h"

using namespace CLEO;

class Starter
{
    static Starter dummy;
    Starter()
    {
        auto gv = CleoInstance.VersionManager.GetGameVersion();
        TRACE(
            "Started on game of version: %s", (gv == GV_US10)    ? "SA 1.0 us"
                                              : (gv == GV_EU11)  ? "SA 1.01 eu"
                                              : (gv == GV_EU10)  ? "SA 1.0 eu"
                                              : (gv == GV_STEAM) ? "SA 3.0 steam"
                                                                 : "<!unknown!>"
        );

        // incompatible game version
        if (gv != GV_US10)
        {
            const auto versionMsg = "Unsupported game version! \n"
                                    "Like most GTA SA mods, CLEO works only with GTA San Andreas 1.0 US. \n"
                                    "Please downgrade your game's executable to GTA SA 1.0 US, or so called "
                                    "\"Hoodlum\" or \"Compact\" variant.";

            int prevVersion = CConfigManager::ReadInt("Internal", "ReportedGameVersion", GV_US10);
            if (gv != prevVersion) // we not nagged user yet
            {
                SHOW_ERROR(versionMsg);
            }
            else
            {
                TRACE("");
                TRACE(versionMsg);
                TRACE("");
            }

            CConfigManager::WriteInt("Internal", "ReportedGameVersion", gv);
        }

        CleoInstance.Start(CCleoInstance::InitStage::Initial);
    }

    ~Starter() { CleoInstance.Stop(); }
};

Starter Starter::dummy;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
