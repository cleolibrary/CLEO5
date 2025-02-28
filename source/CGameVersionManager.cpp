#include "stdafx.h"
#include "CGameVersionManager.h"

namespace CLEO
{
    memory_pointer MemoryAddresses[MA_TOTAL][GV_TOTAL] =
    {
        // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x0053E981,	memory_und, 0x0053E981, 0x0053EE21, 0x00551174 },		// MA_CALL_UPDATE_GAME_LOGICS,
        { 0x0053BEE0,	memory_und, 0x0053BEE0, 0x0053C380, 0x0054DE60 },		// MA_UPDATE_GAME_LOGICS_FUNCTION,

                                                                                // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x00BA6748,	memory_und, 0x00BA6748, 0x00BA8DC8, 0x00C33100 },		// MA_MENU_MANAGER,
        { 0x0071A700,	memory_und, 0x0071A700, 0x0071AF30, 0x0073BF50 },		// MA_DRAW_TEXT_FUNCTION,
        { 0x00719610,	memory_und, 0x00719610, 0x00719E40, 0x0073ABC0 },		// MA_SET_TEXT_ALIGN_FUNCTION,
        { 0x00719490,	memory_und, 0x00719490, 0x00719CC0, 0x0073AA00 },		// MA_SET_TEXT_FONT_FUNCTION,
        { 0x00719590,	memory_und, memory_und, memory_und, 0x0073AB30 },		// MA_SET_TEXT_EDGE_FUNCTION,
        { 0x005733E0,	memory_und, 0x005733E0, 0x00573950, 0x005885C0 },		// MA_CMENU_SCALE_X_FUNCTION,
        { 0x00573410,	memory_und, 0x00573410, 0x00573980, 0x00588600 },		// MA_CMENU_SCALE_Y_FUNCTION,
        { 0x00719380,	memory_und, 0x00719380, 0x00719BB0, 0x0073A8D0 },		// MA_SET_LETTER_SIZE_FUNCTION,
        { 0x00719430,	memory_und, 0x00719430, 0x00719C60, 0x0073A970 },		// MA_SET_LETTER_COLOR_FUNCTION,
        { 0x00728350,	memory_und, 0x00728350, 0x00728B80, 0x0075D640 },		// MA_CTEXTURE_DRAW_IN_RECT_FUNCTION,
        { 0x0057B9FD,	memory_und, 0x0057B9FD, 0x0057BF71, 0x00591379 },		// MA_CALL_CTEXTURE_DRAW_BG_RECT,

                                                                                // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x00464C00,	memory_und, 0x00464C00, 0x00464C80, 0x0046A490 },		// MA_ADD_SCRIPT_TO_QUEUE_FUNCTION,
        { 0x00464BD0,	memory_und, 0x00464BD0, 0x00464C50, 0x0046A460 },		// MA_REMOVE_SCRIPT_FROM_QUEUE_FUNCTION,
        { 0x00465AA0,	memory_und, 0x00465AA0, 0x00465B20, 0x0046B2C0 },		// MA_STOP_SCRIPT_FUNCTION,
        { 0x00465E60,	memory_und, 0x00465E60, 0x00465EE0, 0x0046B640 },		// MA_SCRIPT_OPCODE_HANDLER0_FUNCTION,
        { 0x00464080,	memory_und, 0x00464080, 0x00464100, 0x00469790 },		// MA_GET_SCRIPT_PARAMS_FUNCTION,
        { 0x00464500,	memory_und, 0x00464500, 0x00464580, 0x00469BF0 },		// MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION,
        { 0x00464370,	memory_und, 0x00464370, 0x004643F0, 0x00469A70 },		// MA_SET_SCRIPT_PARAMS_FUNCTION,
        { 0x004859D0,	memory_und, 0x004859D0, 0x00485A50, 0x0048BF40 },		// MA_SET_SCRIPT_COND_RESULT_FUNCTION,
        { 0x00464700,	memory_und, 0x00464700, 0x00464780, 0x00469DE0 },		// MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION,
        { 0x00463D50,	memory_und, 0x00463D50, 0x00463DD0, 0x00469420 },		// MA_GET_SCRIPT_STRING_PARAM_FUNCTION,
        { 0x00464790,	memory_und, 0x00464790, 0x00464810, 0x00469E80 },		// MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION,
        { 0x00468D50,	memory_und, 0x00468D50, 0x00468DD0, 0x0046E440 },		// MA_INIT_SCM_FUNCTION,
        { 0x005D4C40,	memory_und, 0x005D4C40, 0x005D5420, 0x005F13E0 },		// MA_SAVE_SCM_DATA_FUNCTION,
        { 0x005D4FD0,	memory_und, 0x005D4FD0, 0x005D57B0, 0x005F1770 },		// MA_LOAD_SCM_DATA_FUNCTION,
        { 0x00B7CB84,	memory_und, 0x00B7CB84, 0x00B7F204, 0x00C0F538 },		// MA_GAME_TIMER,

                                                                                // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x00A43C78,	memory_und, 0x00A43C78, 0x00A462F8, 0x00AB8DD0 },		// MA_OPCODE_PARAMS,
        { 0x00A49960,	memory_und, 0x00A49960, 0x00A4BFE0, 0x00ABEA90 },		// MA_SCM_BLOCK,
        { 0x00A48960,	memory_und, 0x00A48960, 0x00A4AFE0, 0x00ABDA90 },		// MA_MISSION_LOCALS,
        { 0x00A444B1,	memory_und, 0x00A444B1, 0x00A46B31, 0x00AB95EA },		// MA_MISSION_LOADED,
        { 0x00A7A6A0,	memory_und, 0x00A7A6A0, 0x00A7CD20, 0x00AEF7D0 },		// MA_MISSION_BLOCK,
        { 0x00A476AC,	memory_und, 0x00A476AC, 0x00A49D2C, 0x00ABC7DC },		// MA_ON_MISSION_FLAG,
        { 0x00A8B42C,	memory_und, 0x00A8B42C, 0x00A8DAAC, 0x00B00558 },		// MA_ACTIVE_THREAD_QUEUE,
        { 0x00A8B428,	memory_und, 0x00A8B428, 0x00A8DAA8, 0x00ABDA8C },		// MA_INACTIVE_THREAD_QUEUE,
        { 0x00A8B430,	memory_und, 0x00A8B430, 0x00A8DAB0, 0x00B00560 },		// MA_STATIC_THREADS,
        { 0x0053BDD7,	memory_und, 0x0053BDD7, memory_und, 0x0054DD49 },		// MA_CALL_INIT_SCM1,
        { 0x005BA340,	memory_und, 0x005BA340, memory_und, 0x005D8EE9 },		// MA_CALL_INIT_SCM2,
        { 0x005D4FD7,	memory_und, 0x005D4FD7, 0x005D57B7, 0x005F1777 },		// MA_CALL_INIT_SCM3,
        { 0x005D14D5,	memory_und, 0x005D14D5, 0x005D157C, 0x005EDBD4 },		// MA_CALL_SAVE_SCM_DATA,
        { 0x005D18F0,	memory_und, 0x005D18F0, 0x005D20D0, 0x005EE017 },		// MA_CALL_LOAD_SCM_DATA,
        { 0x0046A21B,	memory_und,	0x0046A21B, 0x0046AE9B, 0x0046F9A8 },		// MA_CALL_PROCESS_SCRIPT
        { 0x00A94B68,	memory_und, 0x00A94B68,	0x00A971E8, 0x00B09C80 },		// MA_SCRIPT_SPRITE_ARRAY
        { 0x00464980,	memory_und, 0x00464980, 0x00465600, 0x0046A130 },		// MA_DRAW_SCRIPT_SPRITES
        { 0x0058C092,	memory_und, 0x0058C092, 0x0058D462, 0x0059A3F2 },		// MA_CALL_DRAW_SCRIPT_SPRITES
        { 0x00A92D68,	memory_und,	0x00A92D68, 0x00A953E8, 0x00B07E80 },		// MA_SCRIPT_DRAW_ARRAY
        { 0x00A44B5C,	memory_und, 0x00A44B5C,	0x00A471DC, 0x00AB9C8C },		// MA_NUM_SCRIPT_DRAWS
        { 0x00465A6F,	memory_und, 0x00465A6F, 0x00465AEF, 0x0046B291 },		// MA_CODE_JUMP_FOR_TXD_STORE
        { 0x00A44B67,	memory_und, 0x00A44B67, 0x00A471E7, 0x00AB95FA },		// MA_USE_TEXT_COMMANDS
        { 0x00A913E8,	memory_und,	0x00A913E8, 0x00A93A68, 0x00B06500 },		// MA_SCRIPT_TEXT_ARRAY
        { 0x00A44B68,	memory_und, 0x00A44B68,	0x00A471E8, 0x00AB9C98 },		// MA_NUM_SCRIPT_TEXTS
        { 0x0058FCE4,	memory_und, 0x0058FCE4, 0x005904B4, 0x0059E73C },		// MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE
        { 0x0058D552,	memory_und, 0x0058D552, 0x0058DD22, 0x0059BAD4 },		// MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE
        { 0x00748E6B,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_SHUTDOWN TODO: find for other versions
        { 0x0053C758,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_1 TODO: find for other versions
        { 0x00748E04,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_2 TODO: find for other versions
        { 0x00748E3E,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_3 TODO: find for other versions
        { 0x0053EBE4,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_IDLE TODO: find for other versions
        { 0x0053E86C,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_FRONTEND TODO: find for other versions
                                                                                // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x008A6168,	memory_und, 0x008A6168, 0x008A7450, 0x00913C20 },		// MA_OPCODE_HANDLER,
        { 0x00469FEE,	memory_und, 0x00469FEE, 0x0046A06E, 0x0046F75C },		// MA_OPCODE_HANDLER_REF,
        { 0x00B74490,	memory_und, 0x00B74490, 0x00B76B10, 0x00C01038 },		// MA_PED_POOL,
        { 0x00B74494,	memory_und, 0x00B74494, 0x00B76B14, 0x00C0103C },		// MA_VEHICLE_POOL,
        { 0x00B7449C,	memory_und, 0x00B7449C, 0x00B76B18, 0x00C01044 },		// MA_OBJECT_POOL,
        { 0x00569660,	memory_und, 0x00569660, 0x00569B00, 0x00583CB0 },		// MA_FIND_GROUND_Z_FUNCTION,
        { 0x00BA86F0,	memory_und, 0x00BA86F0, 0x00BAAD70, 0x00C36020 },		// MA_RADAR_BLIPS,
        { 0x0056E210,	memory_und, 0x0056E210, 0x0056E6B0, 0x00563900 },		// MA_GET_PLAYER_PED_FUNCTION,
        { 0x0043A0B0,	memory_und, 0x0043A0B0, 0x0043A136, 0x0043D3D0 },		// MA_SPAWN_CAR_FUNCTION,

                                                                                // GV_US10,		GV_US11,		GV_EU10,		GV_EU11,		GV_STEAM
        { 0x007487A8,	memory_und, 0x007487F8, 0x0074907C, 0x0078276D },		// MA_CALL_CREATE_MAIN_WINDOW,
    };

    eGameVersion DetermineGameVersion()
    {
        MemGrantAccess(0x8A6168, sizeof(DWORD));
        if (MemRead<DWORD>(0x8A6168) == 0x8523A0) return GV_EU11;

        MemGrantAccess(0x8A4004, sizeof(DWORD));
        if (MemRead<DWORD>(0x8A4004) == 0x8339CA) return GV_US10;
        if (MemRead<DWORD>(0x8A4004) == 0x833A0A) return GV_EU10;

        MemGrantAccess(0x913000, sizeof(DWORD));
        if (MemRead<DWORD>(0x913000) == 0x8A5B0C) return GV_STEAM;

        return GV_UNK;
    }

    // converts memory address' identifier to actual memory pointer
    memory_pointer CGameVersionManager::TranslateMemoryAddress(eMemoryAddress addrId)
    {
        return MemoryAddresses[addrId][GetGameVersion()];
    }

    extern "C"
    {
        DWORD WINAPI CLEO_GetVersion()
        {
            return CLEO_VERSION;
        }

        LPCSTR WINAPI CLEO_GetVersionStr()
        {
            return CLEO_VERSION_STR;
        }

        eGameVersion WINAPI CLEO_GetGameVersion()
        {
            return DetermineGameVersion();
        }
    }
}
