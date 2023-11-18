#pragma once

#include "CCodeInjector.h"
#include "CGameVersionManager.h"
#include "CDebug.h"
#include "CDmaFix.h"
#include "CGameMenu.h"
#include "CModuleSystem.h"
#include "CPluginSystem.h"
#include "CScriptEngine.h"
#include "CCustomOpcodeSystem.h"
#include "CTextManager.h"
#include "CSoundSystem.h"
#include "FileEnumerator.h"
#include "crc32.h"

namespace CLEO
{
    class CCleoInstance
    {
        bool m_bStarted;
        bool m_bGameInProgress;
        std::map<eCallbackId, std::set<void*>> m_callbacks;

    public:
        CDmaFix					DmaFix;
        CGameMenu				GameMenu;
        CCodeInjector			CodeInjector;
        CGameVersionManager		VersionManager;
        CScriptEngine			ScriptEngine;
        CTextManager			TextManager;
        CCustomOpcodeSystem		OpcodeSystem;
        CModuleSystem			ModuleSystem;
        CSoundSystem			SoundSystem;
        CPluginSystem			PluginSystem;
        //CLegacy				Legacy;

        HWND MainWnd = NULL;
        int saveSlot = -1; // -1 if not loaded from save

        CCleoInstance();
        virtual ~CCleoInstance();

        void Start();
        void Stop();

        void GameBegin();
        void GameEnd();

        bool IsStarted() const { return m_bStarted; }

        void AddCallback(eCallbackId id, void* func);
        const std::set<void*>& GetCallbacks(eCallbackId id);

        static void __cdecl OnDrawingFinished();

        void(__cdecl * UpdateGameLogics)() = nullptr;
        static void __cdecl OnUpdateGameLogics();

        // calls to CTheScripts::Init
        void(__cdecl* ScmInit1_Orig)() = nullptr;
        void(__cdecl* ScmInit2_Orig)() = nullptr;
        void(__cdecl* ScmInit3_Orig)() = nullptr;
        static void OnScmInit1();
        static void OnScmInit2();
        static void OnScmInit3();

        // call for Game::Shutdown
        void(__cdecl* GameShutdown)() = nullptr;
        static void __cdecl OnGameShutdown();

        // calls for Game::ShutDownForRestart
        void(__cdecl* GameRestart1)() = nullptr;
        void(__cdecl* GameRestart2)() = nullptr;
        void(__cdecl* GameRestart3)() = nullptr;
        static void __cdecl OnGameRestart1();
        static void __cdecl OnGameRestart2();
        static void __cdecl OnGameRestart3();
    };

    CCleoInstance& GetInstance();
}

