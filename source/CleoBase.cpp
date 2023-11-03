#include "stdafx.h"
#include "CleoBase.h"


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
        //GetInstance().UpdateGameLogics(); // !
        GetInstance().SoundSystem.Update();
        static DWORD dwFunc;
        dwFunc = (DWORD)(GetInstance().UpdateGameLogics);
        _asm jmp dwFunc
    }

    void CCleoInstance::Start()
    {
        if (m_bStarted) return; // already started

        /*if (FS::current_path() != Filepath_Root)
        {
            MessageBox(NULL, "CLEO.asi has to be placed in game's root directory!", "CLEO error", MB_SYSTEMMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
            exit(1); // terminate the game
        }*/

        FS::create_directory(Filepath_Cleo);
        FS::create_directory(FS::path(Filepath_Cleo).append("cleo_modules"));
        FS::create_directory(FS::path(Filepath_Cleo).append("cleo_plugins"));
        FS::create_directory(FS::path(Filepath_Cleo).append("cleo_saves"));
        FS::create_directory(FS::path(Filepath_Cleo).append("cleo_text"));

        CodeInjector.OpenReadWriteAccess(); // must do this earlier to ensure plugins write access on init
        GameMenu.Inject(CodeInjector);
        DmaFix.Inject(CodeInjector);
        UpdateGameLogics = VersionManager.TranslateMemoryAddress(MA_UPDATE_GAME_LOGICS_FUNCTION);
        CodeInjector.ReplaceFunction(&OnUpdateGameLogics, VersionManager.TranslateMemoryAddress(MA_CALL_UPDATE_GAME_LOGICS));
        TextManager.Inject(CodeInjector);
        SoundSystem.Inject(CodeInjector);
        OpcodeSystem.Inject(CodeInjector);
        ScriptEngine.Inject(CodeInjector);

        CodeInjector.ReplaceFunction(OnDrawingFinished, 0x00734640); // nullsub_63 - originally something like renderDebugStuff?

        m_bStarted = true;
        TRACE("CLEO instance started successfully!");
    }

    void CCleoInstance::Stop()
    {
        if (!m_bStarted) return;

        m_bStarted = false;
    }

    void CCleoInstance::AddCallback(eCallbackId id, void* func)
    {
        m_callbacks[id].insert(func);
    }

    const std::set<void*>& CCleoInstance::GetCallbacks(eCallbackId id)
    {
        return m_callbacks[id];
    }

    void WINAPI CLEO_RegisterCallback(eCallbackId id, void* func)
    {
        GetInstance().AddCallback(id, func);
    }

    void __cdecl CCleoInstance::OnDrawingFinished()
    {
        // execute callbacks
        for (void* func : GetInstance().GetCallbacks(eCallbackId::DrawingFinished))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }
}

