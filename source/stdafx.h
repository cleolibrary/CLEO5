#pragma once

#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#undef UNICODE

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <assert.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <direct.h>
#include <list>
#include <vector>
#include <set>
#include <cstdint>

#include "..\cleo_sdk\CLEO.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include <plugin.h>

// global constant paths. Initialize before anything else
namespace FS = std::filesystem;

static std::string GetGameDirectory() // already stored in Filepath_Game
{
    std::string path;
    path.resize(MAX_PATH);
    GetModuleFileNameA(NULL, path.data(), path.size()); // game exe absolute path
    path.resize(CLEO::FilepathGetParent(path).length());
    CLEO::FilepathNormalize(path);
    return std::move(path);
}

static std::string GetUserDirectory() // already stored in Filepath_User
{
    static const auto GTA_InitUserDirectories = (char* (__cdecl*)())0x00744FB0; // SA 1.0 US - CFileMgr::InitUserDirectories

    std::string path = GTA_InitUserDirectories();
    CLEO::FilepathNormalize(path);

    return std::move(path);
}

inline const std::string Filepath_Game = GetGameDirectory();
inline const std::string Filepath_User = GetUserDirectory();
inline const std::string Filepath_Cleo = Filepath_Game + "\\cleo";
inline const std::string Filepath_Config = Filepath_Cleo + "\\.cleo_config.ini";
inline const std::string Filepath_Log = Filepath_Game + "\\cleo.log";

class CTexture
{
    RwTexture *texture;
};

// stolen from GTASA
class CTextDrawer
{
public:
    float		m_fScaleX;
    float		m_fScaleY;
    CRGBA		m_Colour;
    BYTE			m_bJustify;
    BYTE			m_bCenter;
    BYTE			m_bBackground;
    BYTE			m_bUnk1;
    float		m_fLineHeight;
    float		m_fLineWidth;
    CRGBA		m_BackgroundColour;
    BYTE			m_bProportional;
    CRGBA		m_EffectColour;
    BYTE			m_ucShadow;
    BYTE			m_ucOutline;
    BYTE			m_bDrawBeforeFade;
    BYTE			m_bAlignRight;
    int			m_nFont;
    float		m_fPosX;
    float		m_fPosY;
    char			m_szGXT[8];
    int			m_nParam1;
    int			m_nParam2;
};

VALIDATE_SIZE(CTextDrawer, 0x44);

