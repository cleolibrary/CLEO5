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

// global constant paths. Initialize before anything else
namespace FS = std::filesystem;

static std::string GetGameDirectory() // already stored in Filepath_Game
{
    std::string path;
    path.resize(MAX_PATH);
    GetModuleFileNameA(NULL, path.data(), path.size()); // game exe absolute path
    path.resize(CLEO::FilepathGetParent(path).length());
    CLEO::FilepathNormalize(path, false);
    return std::move(path);
}

static std::string GetUserDirectory() // already stored in Filepath_User
{
    static const auto GTA_InitUserDirectories =
        (char*(__cdecl*)())0x00744FB0; // SA 1.0 US - CFileMgr::InitUserDirectories

    std::string path = GTA_InitUserDirectories();
    CLEO::FilepathNormalize(path);

    return std::move(path);
}

inline const std::string Filepath_Game   = GetGameDirectory();
inline const std::string Filepath_User   = GetUserDirectory();
inline const std::string Filepath_Cleo   = Filepath_Game + "\\cleo";
inline const std::string Filepath_Config = Filepath_Cleo + "\\.cleo_config.ini";

static std::string GetLogDirectory()
{
    char buffer[MAX_PATH] = {};
    GetPrivateProfileStringA("General", "LogDirectory", "", buffer, sizeof(buffer), Filepath_Config.c_str());

    std::string dir = buffer;

    // strip inline comment
    auto commentPos = dir.find(';');
    if (commentPos != std::string::npos) dir.resize(commentPos);

    // trim leading and trailing whitespace
    auto wsStart = dir.find_first_not_of(" \t");
    if (wsStart == std::string::npos) return Filepath_Game; // all whitespace = default: game root
    dir = dir.substr(wsStart);
    while (!dir.empty() && (dir.back() == ' ' || dir.back() == '\t')) dir.pop_back();

    if (dir.empty()) return Filepath_Game; // default: game root

    // resolve relative paths against game directory
    FS::path fsPath(dir);
    if (!fsPath.is_absolute())
    {
        fsPath = FS::path(Filepath_Game) / fsPath;
    }

    std::string result = fsPath.string();
    CLEO::FilepathNormalize(result, false);
    return result;
}

inline const std::string Filepath_LogDir = GetLogDirectory();
inline const std::string Filepath_Log    = Filepath_LogDir + "\\cleo.log";
