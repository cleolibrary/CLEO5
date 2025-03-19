#pragma once
#pragma warning(disable:4733)
#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#undef UNICODE

#define WIN32_LEAN_AND_MEAN
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

#include <game_sa/CPools.h>
#include <game_sa/CMenuManager.h>
#include <game_sa/CText.h>
#include <game/CRGBA.h>
#include <game_sa/cHandlingDataMgr.h>
#include <game_sa/CPlayerPed.h>
#include <game_sa/CRadar.h>
#include <game_sa/CTheScripts.h>

#include "..\cleo_sdk\CLEO.h"
#include "..\cleo_sdk\CLEO_Utils.h"

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

#define NUM_SCAN_ENTITIES 16

using RGBA = CRGBA;
using RwV3D = RwV3d;
struct RwRect2D;
using CHandling = cHandlingDataMgr;
using CMarker = tRadarTrace;

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

inline CEntity* GetWeaponTarget(CPed* pSelf)
{
    return reinterpret_cast<int>(pSelf->m_pTargetedObject) != -1 ? pSelf->m_pTargetedObject : nullptr;
}

inline bool IsAvailable(CPed* pSelf)
{
    return pSelf->m_nPedState != PEDSTATE_DIE && pSelf->m_nPedState != PEDSTATE_DEAD;
}

inline bool	IsWrecked(CVehicle* pSelf)
{
    return pSelf->m_nStatus == STATUS_WRECKED || pSelf->m_nVehicleFlags.bIsDrowning;
}


#define OP_NOP			0x90
#define OP_RET			0xC3
#define OP_CALL			0xE8
#define OP_JMP			0xE9
#define OP_JMPSHORT		0xEB
#include "Mem.h"
