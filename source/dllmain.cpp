#include "stdafx.h"
#include "CleoBase.h"
#include "CDebug.h"


using namespace CLEO;

class Starter
{
    static Starter dummy;
    Starter()
    {
        auto gv = CleoInstance.VersionManager.GetGameVersion();
        TRACE("Started on game of version: %s",
            (gv == GV_US10) ? "SA 1.0 us" :
            (gv == GV_EU11) ? "SA 1.01 eu" :
            (gv == GV_EU10) ? "SA 1.0 eu" :
            (gv == GV_STEAM) ? "SA 3.0 steam" :
            "<!unknown!>");

        if (gv != GV_US10 && gv != GV_EU11 && gv != GV_EU10 && gv != GV_STEAM)
            TRACE(
                "Unknown game version.\n"
                "The list of all known executables:\n\n"
                "  1) gta_sa.exe, original 1.0 us, 14 405 632 bytes;\n"
                "  2) gta_sa.exe, public no-dvd 1.0 us, 14 383 616 bytes;\n"
                "  3) gta_sa_compact.exe, listener's executable, 5 189 632 bytes;\n"
                "  4) gta_sa.exe, original 1.01 eu, 14 405 632 bytes;\n"
                "  5) gta_sa.exe, public no-dvd 1.01 eu, 15 806 464 bytes;\n"
                "  6) gta_sa.exe, 1C localization, 15 806 464 bytes;\n"
                "  7) gta_sa.exe, original 1.0 eu, unknown size;\n"
                "  8) gta_sa.exe, public no-dvd 1.0eu, 14 386 176 bytes;\n"
                "  9) gta_sa.exe, original 3.0 steam executable, unknown size;"
                " 10) gta_sa.exe, decrypted 3.0 steam executable, 5 697 536 bytes."
            );

        // incompatible game version
        if (gv != GV_US10)
        {
            const auto versionMsg = \
                "Unsupported game version! \n" \
                "Like most of GTA SA mods, CLEO is meant to work with game version 1.0. \n" \
                "Please downgrade your game's executable file to GTA SA 1.0 US, or so called \"Hoodlum\" or \"Compact\" variant.";

            int prevVersion = GetPrivateProfileInt("Internal", "ReportedGameVersion", GV_US10, Filepath_Config.c_str());
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

            char strValue[32];
            _itoa_s(gv, strValue, 10);
            WritePrivateProfileString("Internal", "ReportedGameVersion", strValue, Filepath_Config.c_str());
        }

        CleoInstance.Start(CCleoInstance::InitStage::Initial);
    }

    ~Starter()
    {
        CleoInstance.Stop();
    }
};

Starter Starter::dummy;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
