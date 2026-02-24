#include "stdafx.h"
#include "CleoBase.h"
#include "Singleton.h"

namespace CLEO
{
    CCleoInstance CleoInstance;

    inline CCleoInstance::~CCleoInstance()
    {
        Stop();
    }

    void __cdecl CCleoInstance::OnGameProcess()
    {
        CleoInstance.CallCallbacks(eCallbackId::GameProcessBefore);
        CleoInstance.OnGameProcess_Orig();
        CleoInstance.CallCallbacks(eCallbackId::GameProcessAfter);
    }

    HWND CCleoInstance::OnCreateMainWnd(HINSTANCE hinst)
    {
        CleoSingletonCheck(); // check once for CLEO.asi duplicates

        auto window = CleoInstance.CreateMainWnd_Orig(hinst); // call original

        // redirect window handling procedure
        *((size_t*)&CleoInstance.MainWndProc_Orig) = GetWindowLongPtr(window, GWLP_WNDPROC); // store original address
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG)OnMainWndProc);

        return window;
    }

    LRESULT __stdcall CCleoInstance::OnMainWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_ACTIVATE:
            CleoInstance.CallCallbacks(eCallbackId::MainWindowFocus, wparam != 0);
            break;

        case WM_KILLFOCUS:
            CleoInstance.CallCallbacks(eCallbackId::MainWindowFocus, false);
            break;
        }

        return CleoInstance.MainWndProc_Orig(wnd, msg, wparam, lparam);
    }

    /*******************************************************
    
        Order of callbacks during game start:

            First Boot → New Game      OnScmInit2
            First Boot → Load Save     OnScmInit2 → OnScmInit1 → OnScmInit3
            Later → New Game           OnScmInit1
            Later → Load Save          OnScmInit1 → OnScmInit3

    ********************************************************/

    void CCleoInstance::OnScmInit1()
    {
        CleoInstance.ScmInit1_Orig();
        if (!FrontEndMenuManager.m_bWantToLoad)
        {
            // if it's a new game, we can call GameBegin right now.
            // Otherwise, defer it to OnScmInit3
            CleoInstance.GameBegin(-1);
        }
    }

    void CCleoInstance::OnScmInit2()
    {
        CleoInstance.ScmInit2_Orig();
        if (!FrontEndMenuManager.m_bWantToLoad)
        {
            // if it's a new game, we can call GameBegin right now.
            // Otherwise, defer it to OnScmInit3
            CleoInstance.GameBegin(-1);
        }
    }

    void CCleoInstance::OnScmInit3()
    {
        CleoInstance.ScmInit3_Orig();
        CleoInstance.GameBegin(FrontEndMenuManager.m_nSelectedSaveGame);
    }

    void __declspec(naked) CCleoInstance::OnGameShutdown()
    {
        CleoInstance.GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(CleoInstance.GameShutdown_Orig);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart1()
    {
        CleoInstance.GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(CleoInstance.GameRestart1_Orig);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart2()
    {
        CleoInstance.GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(CleoInstance.GameRestart2_Orig);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart3()
    {
        CleoInstance.GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(CleoInstance.GameRestart3_Orig);
        _asm jmp oriFunc
    }

    void __cdecl CCleoInstance::OnDebugDisplayTextBuffer_Idle()
    {
        CleoInstance.GameRestartDebugDisplayTextBuffer_IdleOrig(); // call original
        CleoInstance.CallCallbacks(eCallbackId::DrawingFinished);  // execute registered callbacks
    }

    void __cdecl CCleoInstance::OnDebugDisplayTextBuffer_Frontend()
    {
        CleoInstance.GameRestartDebugDisplayTextBuffer_FrontendOrig(); // call original
        CleoInstance.CallCallbacks(eCallbackId::DrawingFinished);      // execute registered callbacks
    }

    void CCleoInstance::Start(InitStage stage)
    {
        if (stage > InitStage::Done) return; // invalid argument

        auto nextStage = InitStage(m_initStage + 1);
        if (stage != nextStage) return;

        if (stage == InitStage::Initial)
        {
            TRACE("CLEO initialization: Phase 1");

            FS::create_directory(Filepath_Cleo);
            FS::create_directory(Filepath_Cleo + "\\cleo_modules");
            FS::create_directory(Filepath_Cleo + "\\cleo_plugins");
            FS::create_directory(Filepath_Cleo + "\\cleo_saves");

            OpcodeInfoDb.LoadCommands((Filepath_Cleo + "\\.config\\sa.json").c_str());

            CodeInjector.OpenReadWriteAccess(); // must do this earlier to ensure plugins write access on init
            GameMenu.Inject(CodeInjector);
            DmaFix.Inject(CodeInjector);
            OpcodeSystem.Inject(CodeInjector);
            ScriptEngine.Inject(CodeInjector);

            CodeInjector.ReplaceFunction(
                OnCreateMainWnd, VersionManager.TranslateMemoryAddress(MA_CALL_CREATE_MAIN_WINDOW), &CreateMainWnd_Orig
            );

            CodeInjector.ReplaceFunction(
                OnScmInit1, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM1), &ScmInit1_Orig
            );
            CodeInjector.ReplaceFunction(
                OnScmInit2, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM2), &ScmInit2_Orig
            );
            CodeInjector.ReplaceFunction(
                OnScmInit3, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM3), &ScmInit3_Orig
            );

            CodeInjector.ReplaceFunction(
                OnGameShutdown, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_SHUTDOWN), &GameShutdown_Orig
            );

            CodeInjector.ReplaceFunction(
                OnGameRestart1, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_1), &GameRestart1_Orig
            );
            CodeInjector.ReplaceFunction(
                OnGameRestart2, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_2), &GameRestart2_Orig
            );
            CodeInjector.ReplaceFunction(
                OnGameRestart3, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_3), &GameRestart3_Orig
            );

            OpcodeSystem.Init();
            PluginSystem.LoadPlugins();
        }

        // delayed until menu background drawing
        if (stage == InitStage::OnDraw)
        {
            TRACE("CLEO initialization: Phase 2");

            const_cast<std::string&>(Filepath_User) =
                GetUserDirectory(); // force update now, as it could be modifed by PortableGTA.asi

            ScriptEngine.InjectLate(CodeInjector);

            CodeInjector.InjectFunction(GetScriptStringParam, gaddrof(::CRunningScript::ReadTextLabelFromScript));
            CodeInjector.ReplaceFunction(
                OnDebugDisplayTextBuffer_Idle,
                VersionManager.TranslateMemoryAddress(MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_IDLE),
                &GameRestartDebugDisplayTextBuffer_IdleOrig
            );
            CodeInjector.ReplaceFunction(
                OnDebugDisplayTextBuffer_Frontend,
                VersionManager.TranslateMemoryAddress(MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_FRONTEND),
                &GameRestartDebugDisplayTextBuffer_FrontendOrig
            );
            CodeInjector.ReplaceFunction(
                OnGameProcess, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_PROCESS), &OnGameProcess_Orig
            );

            PluginSystem.LogLoadedPlugins();
        }

        m_initStage = stage;
    }

    void CCleoInstance::Stop()
    {
        GameEnd();
        PluginSystem.UnloadPlugins();
        m_initStage = InitStage::None;
    }

    void CCleoInstance::GameBegin(int saveSlot)
    {
        if (m_bGameInProgress) return;
        m_bGameInProgress = true;

        CleoInstance.saveSlot = saveSlot;
        TRACE("Starting new game session, save slot: %d", saveSlot);

        // late initialization if not done yet
        CleoInstance.Start(CCleoInstance::InitStage::OnDraw);

        // execute registered callbacks
        CleoInstance.CallCallbacks(eCallbackId::GameBegin, saveSlot);
    }

    void CCleoInstance::GameEnd()
    {
        if (!m_bGameInProgress) return;
        m_bGameInProgress = false;

        TRACE("Ending current game session");
        CleoInstance.CallCallbacks(eCallbackId::GameEnd); // execute registered callbacks
        ScriptEngine.GameEnd();
        OpcodeSystem.GameEnd();

        saveSlot = -1;
    }

    void CCleoInstance::AddCallback(eCallbackId id, void* func)
    {
        m_callbacks[id].insert(func);
    }

    void CCleoInstance::RemoveCallback(eCallbackId id, void* func)
    {
        m_callbacks[id].erase(func);
    }

    const std::set<void*>& CCleoInstance::GetCallbacks(eCallbackId id)
    {
        return m_callbacks[id];
    }

    void CCleoInstance::CallCallbacks(eCallbackId id)
    {
        for (void* func : GetCallbacks(id))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    void CCleoInstance::CallCallbacks(eCallbackId id, DWORD arg)
    {
        for (void* func : GetCallbacks(id))
        {
            typedef void WINAPI callback(DWORD);
            ((callback*)func)(arg);
        }
    }

    void WINAPI CLEO_ResolvePath(CLEO::CRunningScript* thread, char* inOutPath, DWORD pathMaxLen)
    {
        if (thread == nullptr || inOutPath == nullptr || pathMaxLen < 2)
        {
            return; // invalid param
        }

        auto resolved = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(inOutPath);

        if (resolved.length() >= pathMaxLen) resolved.resize(pathMaxLen - 1); // and terminator character

        std::memcpy(inOutPath, resolved.c_str(), resolved.length() + 1); // with terminator
    }

    void WINAPI CLEO_StringListFree(StringList list)
    {
        if (list.count > 0 && list.strings != nullptr)
        {
            for (DWORD i = 0; i < list.count; i++)
            {
                free(list.strings[i]);
            }

            free(list.strings);
        }
    }

    StringList WINAPI
    CLEO_ListDirectory(CLEO::CRunningScript* thread, const char* searchPath, BOOL listDirs, BOOL listFiles)
    {
        if (searchPath == nullptr) return {}; // invalid param

        if (!listDirs && !listFiles) return {}; // nothing to list, done

        // make absolute
        auto fsSearchPath = FS::path(searchPath);
        if (!fsSearchPath.is_absolute())
        {
            if (thread != nullptr)
                fsSearchPath = ((CCustomScript*)thread)->GetWorkDir() / fsSearchPath;
            else
                fsSearchPath = Filepath_Game / fsSearchPath;
        }

        WIN32_FIND_DATA wfd = {0};
        HANDLE hSearch      = FindFirstFile(fsSearchPath.string().c_str(), &wfd);
        if (hSearch == INVALID_HANDLE_VALUE) return {}; // nothing found

        std::set<std::string> found;
        do
        {
            if (!listDirs && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue; // skip directories

            if (!listFiles && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue; // skip files

            auto path = FS::path(wfd.cFileName);
            if (!path.is_absolute()) // keep absolute in case somebody hooked the APIs to return so
                path = fsSearchPath.parent_path() / path;

            found.insert(path.string());
        } while (FindNextFile(hSearch, &wfd));

        FindClose(hSearch);

        return CreateStringList(found);
    }
} // namespace CLEO
