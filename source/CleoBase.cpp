#include "stdafx.h"
#include "CleoBase.h"
#include "Singleton.h"

namespace CLEO
{
    CCleoInstance CleoInstance;
    CCleoInstance& GetInstance() { return CleoInstance; }

    inline CCleoInstance::~CCleoInstance()
    {
        Stop();
    }

    void __cdecl CCleoInstance::OnUpdateGameLogics()
    {
        CleoInstance.CallCallbacks(eCallbackId::GameProcess); // execute registered callbacks

        CleoInstance.UpdateGameLogics(); // call original function

        CleoInstance.m_gameState = GameSessionState::InProgress;
    }

    HWND CCleoInstance::OnCreateMainWnd(HINSTANCE hinst)
    {
        CleoSingletonCheck(); // check once for CLEO.asi duplicates

        auto& base = GetInstance();
        auto window = base.CreateMainWnd_Orig(hinst); // call original

        // redirect window handling procedure
        *((size_t*)&base.MainWndProc_Orig) = GetWindowLongPtr(window, GWLP_WNDPROC); // store original address
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG)OnMainWndProc);

        return window;
    }

    LRESULT __stdcall CCleoInstance::OnMainWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        auto& base = GetInstance();

        switch (msg)
        {
            case WM_ACTIVATE:
                base.CallCallbacks(eCallbackId::MainWindowFocus, wparam != 0);
                break;

            case WM_KILLFOCUS:
                base.CallCallbacks(eCallbackId::MainWindowFocus, false);
                break;
        }

        return base.MainWndProc_Orig(wnd, msg, wparam, lparam);
    }

    void CCleoInstance::OnScmInit1()
    {
        auto& base = GetInstance();
        base.ScmInit1_Orig(); // call original
        base.GameBegin();
    }

    void CCleoInstance::OnScmInit2() // load save
    {
        auto& base = GetInstance();
        base.ScmInit2_Orig(); // call original
        base.GameBegin();
    }

    void CCleoInstance::OnScmInit3()
    {
        auto& base = GetInstance();
        base.ScmInit3_Orig(); // call original
        base.GameBegin();
    }

    void __declspec(naked) CCleoInstance::OnGameShutdown()
    {
        GetInstance().GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GetInstance().GameShutdown);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart1()
    {
        GetInstance().GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GetInstance().GameRestart1);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart2()
    {
        GetInstance().GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GetInstance().GameRestart2);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnGameRestart3()
    {
        GetInstance().GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GetInstance().GameRestart3);
        _asm jmp oriFunc
    }

    void __declspec(naked) CCleoInstance::OnDebugDisplayTextBuffer()
    {
        CleoInstance.CallCallbacks(eCallbackId::DrawingFinished); // execute registered callbacks
        static DWORD oriFunc;
        oriFunc = (DWORD)(GetInstance().DebugDisplayTextBuffer);
        if (oriFunc != (DWORD)nullptr)
            _asm jmp oriFunc
        else
            _asm ret
    }

    void CCleoInstance::Start(InitStage stage)
    {
        if (stage > InitStage::Done) return; // invalid argument

        auto nextStage = InitStage((size_t)m_initStage + 1);
        if (stage != nextStage) return;

        if (stage == InitStage::Initial)
        {
            TRACE("CLEO initialization: Phase 1");

            FS::create_directory(Filepath_Cleo);
            FS::create_directory(Filepath_Cleo + "\\cleo_modules");
            FS::create_directory(Filepath_Cleo + "\\cleo_plugins");
            FS::create_directory(Filepath_Cleo + "\\cleo_saves");

            OpcodeInfoDb.Load((Filepath_Cleo + "\\.config\\sa.json").c_str());

            CodeInjector.OpenReadWriteAccess(); // must do this earlier to ensure plugins write access on init
            GameMenu.Inject(CodeInjector);
            DmaFix.Inject(CodeInjector);
            OpcodeSystem.Inject(CodeInjector);
            ScriptEngine.Inject(CodeInjector);

            CodeInjector.ReplaceFunction(OnCreateMainWnd, VersionManager.TranslateMemoryAddress(MA_CALL_CREATE_MAIN_WINDOW), &CreateMainWnd_Orig);

            CodeInjector.ReplaceFunction(OnUpdateGameLogics, VersionManager.TranslateMemoryAddress(MA_CALL_UPDATE_GAME_LOGICS), &UpdateGameLogics);

            CodeInjector.ReplaceFunction(OnScmInit1, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM1), &ScmInit1_Orig);
            CodeInjector.ReplaceFunction(OnScmInit2, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM2), &ScmInit2_Orig);
            CodeInjector.ReplaceFunction(OnScmInit3, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM3), &ScmInit3_Orig);

            CodeInjector.ReplaceFunction(OnGameShutdown, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_SHUTDOWN), &GameShutdown);

            CodeInjector.ReplaceFunction(OnGameRestart1, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_1), &GameRestart1);
            CodeInjector.ReplaceFunction(OnGameRestart2, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_2), &GameRestart2);
            CodeInjector.ReplaceFunction(OnGameRestart3, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_3), &GameRestart3);

            OpcodeSystem.Init();
            PluginSystem.LoadPlugins();
        }
        
        // delayed until menu background drawing
        if (stage == InitStage::OnDraw)
        {
            TRACE("CLEO initialization: Phase 2");

            CodeInjector.ReplaceJump(OnDebugDisplayTextBuffer, VersionManager.TranslateMemoryAddress(MA_DEBUG_DISPLAY_TEXT_BUFFER), &DebugDisplayTextBuffer);
        }

        m_initStage = stage;
    }

    void CCleoInstance::Stop()
    {
        if (m_initStage >= InitStage::Initial)
        {
            ScriptEngine.GameEnd();
            PluginSystem.UnloadPlugins();
        }

        m_initStage = InitStage::None;
    }

    void CCleoInstance::GameBegin()
    {
        if (m_gameState != GameSessionState::None) return;
        m_gameState = GameSessionState::Initial;

        saveSlot = MenuManager->m_bWantToLoad ? MenuManager->m_nSelectedSaveGame : -1;

        TRACE(""); // separator
        TRACE("Starting new game, save slot: %d", saveSlot);

        // execute registered callbacks
        GetInstance().CallCallbacks(eCallbackId::GameBegin, saveSlot);
    }

    void CCleoInstance::GameEnd()
    {
        if (m_gameState == GameSessionState::None) return;
        m_gameState = GameSessionState::None;

        TRACE(""); // separator
        TRACE("Ending current game");
        GetInstance().CallCallbacks(eCallbackId::GameEnd); // execute registered callbacks
        ScriptEngine.GameEnd();
        OpcodeSystem.FinalizeScriptObjects();

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
        for (void* func : GetInstance().GetCallbacks(id))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    void CCleoInstance::CallCallbacks(eCallbackId id, DWORD arg)
    {
        for (void* func : GetInstance().GetCallbacks(id))
        {
            typedef void WINAPI callback(DWORD);
            ((callback*)func)(arg);
        }
    }

    void WINAPI CLEO_RegisterCallback(eCallbackId id, void* func)
    {
        GetInstance().AddCallback(id, func);
    }

    void WINAPI CLEO_UnregisterCallback(eCallbackId id, void* func)
    {
        GetInstance().RemoveCallback(id, func);
    }

    DWORD WINAPI CLEO_GetInternalAudioStream(CLEO::CRunningScript* unused, DWORD audioStreamPtr)
    {
        return *(DWORD*)(audioStreamPtr + 0x4); // CAudioStream->streamInternal
    }

    void WINAPI CLEO_ResolvePath(CLEO::CRunningScript* thread, char* inOutPath, DWORD pathMaxLen)
    {
        if (thread == nullptr || inOutPath == nullptr || pathMaxLen < 2)
        {
            return; // invalid param
        }

        auto resolved = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(inOutPath);

        if (resolved.length() >= pathMaxLen)
            resolved.resize(pathMaxLen - 1); // and terminator character

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

    StringList WINAPI CLEO_ListDirectory(CLEO::CRunningScript* thread, const char* searchPath, BOOL listDirs, BOOL listFiles)
    {
        if (searchPath == nullptr)
            return {}; // invalid param

        if (!listDirs && !listFiles)
            return {}; // nothing to list, done

        // make absolute
        auto fsSearchPath = FS::path(searchPath);
        if (!fsSearchPath.is_absolute())
        {
            if (thread != nullptr)
                fsSearchPath = ((CCustomScript*)thread)->GetWorkDir() / fsSearchPath;
            else
                fsSearchPath = Filepath_Game / fsSearchPath;
        }

        WIN32_FIND_DATA wfd = { 0 };
        HANDLE hSearch = FindFirstFile(fsSearchPath.string().c_str(), &wfd);
        if (hSearch == INVALID_HANDLE_VALUE)
            return {}; // nothing found

        std::set<std::string> found;
        do
        {
            if (!listDirs && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
                continue; // skip directories

            if (!listFiles && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue; // skip files

            auto path = FS::path(wfd.cFileName);
            if (!path.is_absolute()) // keep absolute in case somebody hooked the APIs to return so
                path = fsSearchPath.parent_path() / path;

            found.insert(path.string());
        } while (FindNextFile(hSearch, &wfd));

        FindClose(hSearch);

        return CreateStringList(found);
    }

    LPCSTR WINAPI CLEO_GetGameDirectory()
    {
        return Filepath_Game.c_str();
    }

    LPCSTR WINAPI CLEO_GetUserDirectory()
    {
        return Filepath_User.c_str();
    }
}

