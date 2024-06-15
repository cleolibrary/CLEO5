#include "stdafx.h"
#include "CleoBase.h"
#include "Singleton.h"

namespace CLEO
{
    CCleoInstance CleoInstance;
    CCleoInstance& GetInstance() { return CleoInstance; }

    inline CCleoInstance::CCleoInstance()
    {
        m_bStarted = false;
    }

    inline CCleoInstance::~CCleoInstance()
    {
        Stop();
    }

    void __declspec(naked) CCleoInstance::OnUpdateGameLogics()
    {
        CleoInstance.CallCallbacks(eCallbackId::GameProcess); // execute registered callbacks

        static DWORD dwFunc;
        dwFunc = (DWORD)(CleoInstance.UpdateGameLogics);
        _asm jmp dwFunc
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

    void CCleoInstance::Start()
    {
        if (m_bStarted) return; // already started
        m_bStarted = true;

        FS::create_directory(FS::path(Filepath_Root).append("cleo"));
        FS::create_directory(FS::path(Filepath_Root).append("cleo\\cleo_modules"));
        FS::create_directory(FS::path(Filepath_Root).append("cleo\\cleo_plugins"));
        FS::create_directory(FS::path(Filepath_Root).append("cleo\\cleo_saves"));

        OpcodeInfoDb.Load(FS::path(Filepath_Root).append("cleo\\.config\\sa.json").generic_string().c_str());

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

        CodeInjector.ReplaceFunction(OnDrawingFinished, 0x00734640); // nullsub_63 - originally something like renderDebugStuff?

        OpcodeSystem.Init();
        PluginSystem.LoadPlugins();
    }

    void CCleoInstance::Stop()
    {
        if (!m_bStarted) return;
        m_bStarted = false;

        ScriptEngine.GameEnd();
    }

    void CCleoInstance::GameBegin()
    {
        if (m_bGameInProgress) return;
        m_bGameInProgress = true;

        saveSlot = MenuManager->m_bWantToLoad ? MenuManager->m_nSelectedSaveGame : -1;

        TRACE("Starting new game, save slot: %d", saveSlot);

        // execute registered callbacks
        GetInstance().CallCallbacks(eCallbackId::GameBegin, saveSlot);
    }

    void CCleoInstance::GameEnd()
    {
        if (!m_bGameInProgress) return;
        m_bGameInProgress = false;

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

    void __cdecl CCleoInstance::OnDrawingFinished()
    {
        GetInstance().CallCallbacks(eCallbackId::DrawingFinished); // execute registered callbacks
    }

    DWORD WINAPI CLEO_GetInternalAudioStream(CLEO::CRunningScript* thread, DWORD stream) // arg CAudioStream *
    {
        return stream; // CAudioStream::streamInternal offset is 0
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
        StringList result;
        result.count = 0;
        result.strings = nullptr;

        if (searchPath == nullptr)
        {
            return result; // invalid param
        }

        if (!listDirs && !listFiles)
        {
            return result; // nothing to list, done
        }

        auto fsSearchPath = FS::path(searchPath);
        if (!fsSearchPath.is_absolute())
        {
            auto workDir = (thread != nullptr) ?
                ((CCustomScript*)thread)->GetWorkDir() :
                Filepath_Root.c_str();

            fsSearchPath = workDir / fsSearchPath;
        }

        WIN32_FIND_DATA wfd = { 0 };
        HANDLE hSearch = FindFirstFile(searchPath, &wfd);
        if (hSearch == INVALID_HANDLE_VALUE)
        {
            return result;
        }

        std::set<std::string> found;
        do
        {
            if (!listDirs && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                continue; // skip directories
            }

            if (!listFiles && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                continue; // skip files
            }

            auto path = FS::path(wfd.cFileName);
            if (!path.is_absolute()) // keep absolute in case somebody hooked the APIs to return so
                path = fsSearchPath.parent_path() / path;

            found.insert(path.string());
        } while (FindNextFile(hSearch, &wfd));

        // create results list
        result.strings = (char**)malloc(found.size() * sizeof(DWORD)); // array of pointers

        for (auto& path : found)
        {
            char* str = (char*)malloc(path.length() + 1);
            strcpy(str, path.c_str());

            result.strings[result.count] = str;
            result.count++;
        }

        return result;
    }
}

