#pragma once
#include "CCodeInjector.h"
#include "Mem.h"

namespace CLEO
{
    // returned by 0DD5: get_platform opcode
    enum ePlatform
    {
        PLATFORM_NONE,
        PLATFORM_ANDROID,
        PLATFORM_PSP,
        PLATFORM_IOS,
        PLATFORM_FOS,
        PLATFORM_XBOX,
        PLATFORM_PS2,
        PLATFORM_PS3,
        PLATFORM_MAC,
        PLATFORM_WINDOWS
    };

    // determines the list of memory adresses, that can be translated 
    // considering to game version
    enum eMemoryAddress
    {
        // UpdateGameLogics
        MA_CALL_UPDATE_GAME_LOGICS,
        MA_UPDATE_GAME_LOGICS_FUNCTION,

        // MenuStatusNotifier
        MA_MENU_MANAGER,
        MA_DRAW_TEXT_FUNCTION,
        MA_SET_TEXT_ALIGN_FUNCTION,
        MA_SET_TEXT_FONT_FUNCTION,
        MA_SET_TEXT_EDGE_FUNCTION,
        MA_CMENU_SCALE_X_FUNCTION,
        MA_CMENU_SCALE_Y_FUNCTION,
        MA_SET_LETTER_SIZE_FUNCTION,
        MA_SET_LETTER_COLOR_FUNCTION,
        MA_CTEXTURE_DRAW_IN_RECT_FUNCTION,
        MA_CALL_CTEXTURE_DRAW_BG_RECT,

        // ScriptEngine
        MA_ADD_SCRIPT_TO_QUEUE_FUNCTION,
        MA_REMOVE_SCRIPT_FROM_QUEUE_FUNCTION,
        MA_STOP_SCRIPT_FUNCTION,
        MA_SCRIPT_OPCODE_HANDLER0_FUNCTION,
        MA_GET_SCRIPT_PARAMS_FUNCTION,
        MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION,
        MA_SET_SCRIPT_PARAMS_FUNCTION,
        MA_SET_SCRIPT_COND_RESULT_FUNCTION,
        MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION,
        MA_GET_SCRIPT_STRING_PARAM_FUNCTION,
        MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION,
        MA_INIT_SCM_FUNCTION,
        MA_SAVE_SCM_DATA_FUNCTION,
        MA_LOAD_SCM_DATA_FUNCTION,
        MA_GAME_TIMER,
        MA_OPCODE_PARAMS,
        MA_SCM_BLOCK,
        MA_MISSION_LOCALS,
        MA_MISSION_LOADED,
        MA_MISSION_BLOCK,
        MA_ON_MISSION_FLAG,
        MA_ACTIVE_THREAD_QUEUE,
        MA_INACTIVE_THREAD_QUEUE,
        MA_STATIC_THREADS,
        MA_CALL_INIT_SCM1,
        MA_CALL_INIT_SCM2,
        MA_CALL_INIT_SCM3,
        MA_CALL_SAVE_SCM_DATA,
        MA_CALL_LOAD_SCM_DATA,
        MA_CALL_PROCESS_SCRIPT,
        MA_SCRIPT_SPRITE_ARRAY,
        MA_DRAW_SCRIPT_SPRITES,
        MA_CALL_DRAW_SCRIPT_SPRITES,
        MA_SCRIPT_DRAW_ARRAY,
        MA_NUM_SCRIPT_DRAWS,
        MA_CODE_JUMP_FOR_TXD_STORE,
        MA_USE_TEXT_COMMANDS,
        MA_SCRIPT_TEXT_ARRAY,
        MA_NUM_SCRIPT_TEXTS,
        MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE,
        MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE,
        MA_CALL_GAME_SHUTDOWN,
        MA_CALL_GAME_RESTART_1,
        MA_CALL_GAME_RESTART_2,
        MA_CALL_GAME_RESTART_3,
        MA_DEBUG_DISPLAY_TEXT_BUFFER, // empty function called after everything else is drawn

        // CustomOpcodeSystem
        MA_OPCODE_HANDLER,
        MA_OPCODE_HANDLER_REF,
        MA_PED_POOL,
        MA_VEHICLE_POOL,
        MA_OBJECT_POOL,
        MA_FIND_GROUND_Z_FUNCTION,
        MA_RADAR_BLIPS,
        MA_GET_PLAYER_PED_FUNCTION,
        MA_SPAWN_CAR_FUNCTION,

        MA_CALL_CREATE_MAIN_WINDOW,

        MA_TOTAL,
    };

    eGameVersion DetermineGameVersion();

    class CGameVersionManager
    {
        eGameVersion m_eVersion;

    public:
        CGameVersionManager()
        {
            m_eVersion = DetermineGameVersion();
        }

        ~CGameVersionManager()
        {
        }

        eGameVersion GetGameVersion()
        {
            return m_eVersion;
        }

        memory_pointer TranslateMemoryAddress(eMemoryAddress addrId);
    };
}
