#pragma once
#include "CCodeInjector.h"

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

        // MenuStatusNotifier
        MA_CALL_CTEXTURE_DRAW_BG_RECT,

        // ScriptEngine
        MA_GET_SCRIPT_PARAMS_FUNCTION,
        MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION,
        MA_SET_SCRIPT_PARAMS_FUNCTION,
        MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION,
        MA_GET_SCRIPT_STRING_PARAM_FUNCTION,
        MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION,
        MA_OPCODE_PARAMS,
        MA_SCM_BLOCK_REF,
        MA_MISSION_LOCALS,
        MA_MISSION_BLOCK_REF,
        MA_ACTIVE_THREAD_QUEUE,
        MA_INACTIVE_THREAD_QUEUE,
        MA_STATIC_THREADS,
        MA_CALL_INIT_SCM1,
        MA_CALL_INIT_SCM2,
        MA_CALL_INIT_SCM3,
        MA_CALL_SAVE_SCM_DATA,
        MA_CALL_LOAD_SCM_DATA,
        MA_CALL_PROCESS_SCRIPT,
        MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE,
        MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE,
        MA_CALL_GAME_SHUTDOWN,
        MA_CALL_GAME_RESTART_1,
        MA_CALL_GAME_RESTART_2,
        MA_CALL_GAME_RESTART_3,
        MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_IDLE,
        MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_FRONTEND,

        // CustomOpcodeSystem
        MA_OPCODE_HANDLER,
        MA_OPCODE_HANDLER_REF,

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

        eGameVersion GetGameVersion() const
        {
            return m_eVersion;
        }

        memory_pointer TranslateMemoryAddress(eMemoryAddress addrId) const;
    };
}
