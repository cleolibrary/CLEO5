#pragma once

#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#undef UNICODE

#include <windows.h>
#include <assert.h>
#include <ctype.h>
#include <psapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <array>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>

#include "..\cleo_sdk\CLEO.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include "CConfigManager.h"
#include "simdjson.h"

#include <plugin.h>
#include <CFont.h>
#include <CGame.h>
#include <CMenuManager.h>
#include <CRGBA.h>
#include <CRunningScript.h>
#include <CSprite2d.h>
#include <CTheScripts.h>
#include <CTimer.h>
#include <DynAddress.h>
#include <GameVersion.h>
#include <Patch.h>
#include <RenderWare.h>
#include <extensions/Screen.h>

// global constant paths – construct-on-first-use to guarantee init order
namespace FS = std::filesystem;

inline const std::string& GetGameDirectory()
{
    static std::string path = [] {
        std::string p;
        p.resize(MAX_PATH);
        GetModuleFileNameA(NULL, p.data(), p.size()); // game exe absolute path
        p.resize(CLEO::FilepathGetParent(p).length());
        CLEO::FilepathNormalize(p);
        return p;
    }();
    return path;
}

inline const std::string& GetUserDirectory()
{
    static std::string path = [] {
        // SA 1.0 US - CFileMgr::InitUserDirectories
        static const auto GTA_InitUserDirectories = (char*(__cdecl*)())0x00744FB0;

        std::string p = GTA_InitUserDirectories();
        CLEO::FilepathNormalize(p);
        return p;
    }();
    return path;
}

// Force re-read of user directory (may be modified by PortableGTA.asi etc.)
inline void UpdateUserDirectoryPath()
{
    // SA 1.0 US - CFileMgr::InitUserDirectories
    static const auto GTA_InitUserDirectories = (char*(__cdecl*)())0x00744FB0;

    std::string p = GTA_InitUserDirectories();
    CLEO::FilepathNormalize(p);
    const_cast<std::string&>(GetUserDirectory()) = std::move(p);
}

inline const std::string& GetCleoDirectory()
{
    static std::string path = GetGameDirectory() + "\\cleo";
    return path;
}

inline const std::string& GetLogDirectory()
{
    static std::string path = [] {
        auto dir = CLEO::CConfigManager::ReadString("General", "LogDirectory", "");

        if (dir.empty()) return GetGameDirectory();

        FS::path fsPath(dir);
        if (!fsPath.is_absolute())
        {
            fsPath = GetGameDirectory() / fsPath;
        }

        std::string result = fsPath.string();
        CLEO::FilepathNormalize(result);
        return result;
    }();
    return path;
}

inline const std::string& GetLogPath()
{
    static std::string path = GetLogDirectory() + "\\cleo.log";
    return path;
}
