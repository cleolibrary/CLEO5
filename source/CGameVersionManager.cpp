#include "stdafx.h"
#include "CGameVersionManager.h"
#include "CScriptEngine.h"

namespace CLEO
{
    memory_pointer MemoryAddresses[MA_TOTAL][GV_TOTAL] =
    {
        // GV_US10,	    GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
        { 0x0053E981,	memory_und, 0x0053E981, 0x0053EE21, 0x00551174 },		// MA_CALL_UPDATE_GAME_LOGICS,

        // GV_US10,	    GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
        { 0x0057B9FD,	memory_und, 0x0057B9FD, 0x0057BF71, 0x00591379 },		// MA_CALL_CTEXTURE_DRAW_BG_RECT,

        // GV_US10,	    GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
        { 0x00465E60,	memory_und, 0x00465E60, 0x00465EE0, 0x0046B640 },		// MA_SCRIPT_OPCODE_HANDLER0_FUNCTION,
        { 0x00464080,	memory_und, 0x00464080, 0x00464100, 0x00469790 },		// MA_GET_SCRIPT_PARAMS_FUNCTION,
        { 0x00464500,	memory_und, 0x00464500, 0x00464580, 0x00469BF0 },		// MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION,
        { 0x00464370,	memory_und, 0x00464370, 0x004643F0, 0x00469A70 },		// MA_SET_SCRIPT_PARAMS_FUNCTION,
        { 0x00464700,	memory_und, 0x00464700, 0x00464780, 0x00469DE0 },		// MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION,
        { 0x00463D50,	memory_und, 0x00463D50, 0x00463DD0, 0x00469420 },		// MA_GET_SCRIPT_STRING_PARAM_FUNCTION,
        { 0x00464790,	memory_und, 0x00464790, 0x00464810, 0x00469E80 },		// MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION,

        // GV_US10,		GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
        { 0x00A43C78,	memory_und, 0x00A43C78, 0x00A462F8, 0x00AB8DD0 },		// MA_OPCODE_PARAMS,
        { 0x0044CA44,	memory_und, memory_und, memory_und, memory_und },		// MA_SCM_BLOCK_REF,
        { 0x00A48960,	memory_und, 0x00A48960, 0x00A4AFE0, 0x00ABDA90 },		// MA_MISSION_LOCALS,
        { 0x004899D7,	memory_und, memory_und, memory_und, memory_und },		// MA_MISSION_BLOCK_REF,
        { 0x00A8B42C,	memory_und, 0x00A8B42C, 0x00A8DAAC, 0x00B00558 },		// MA_ACTIVE_THREAD_QUEUE,
        { 0x00A8B428,	memory_und, 0x00A8B428, 0x00A8DAA8, 0x00ABDA8C },		// MA_INACTIVE_THREAD_QUEUE,
        { 0x00A8B430,	memory_und, 0x00A8B430, 0x00A8DAB0, 0x00B00560 },		// MA_STATIC_THREADS,
        { 0x0053BDD7,	memory_und, 0x0053BDD7, memory_und, 0x0054DD49 },		// MA_CALL_INIT_SCM1,
        { 0x005BA340,	memory_und, 0x005BA340, memory_und, 0x005D8EE9 },		// MA_CALL_INIT_SCM2,
        { 0x005D4FD7,	memory_und, 0x005D4FD7, 0x005D57B7, 0x005F1777 },		// MA_CALL_INIT_SCM3,
        { 0x005D14D5,	memory_und, 0x005D14D5, 0x005D157C, 0x005EDBD4 },		// MA_CALL_SAVE_SCM_DATA,
        { 0x005D18F0,	memory_und, 0x005D18F0, 0x005D20D0, 0x005EE017 },		// MA_CALL_LOAD_SCM_DATA,
        { 0x0046A21B,	memory_und,	0x0046A21B, 0x0046AE9B, 0x0046F9A8 },		// MA_CALL_PROCESS_SCRIPT
        { 0x0058FCE4,	memory_und, 0x0058FCE4, 0x005904B4, 0x0059E73C },		// MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE
        { 0x0058D552,	memory_und, 0x0058D552, 0x0058DD22, 0x0059BAD4 },		// MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE
        { 0x00748E6B,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_SHUTDOWN TODO: find for other versions
        { 0x0053C758,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_1 TODO: find for other versions
        { 0x00748E04,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_2 TODO: find for other versions
        { 0x00748E3E,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_GAME_RESTART_3 TODO: find for other versions
        { 0x0053EBE4,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_IDLE TODO: find for other versions
        { 0x0053E86C,	memory_und, memory_und, memory_und, memory_und },		// MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_FRONTEND TODO: find for other versions

        // GV_US10,	    GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
        { 0x008A6168,	memory_und, 0x008A6168, 0x008A7450, 0x00913C20 },		// MA_OPCODE_HANDLER,
        { 0x00469FEE,	memory_und, 0x00469FEE, 0x0046A06E, 0x0046F75C },		// MA_OPCODE_HANDLER_REF,

        // GV_US10,	    GV_US11,	GV_EU10,	GV_EU11,	GV_STEAM
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
    memory_pointer CGameVersionManager::TranslateMemoryAddress(eMemoryAddress addrId) const
    {
        return MemoryAddresses[addrId][GetGameVersion()];
    }
}
