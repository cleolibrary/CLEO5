#pragma once

namespace CLEO
{
    const char cs_ext[] = ".cs";
    const char cs4_ext[] = ".cs4";
    const char cs3_ext[] = ".cs3";

    class CCustomScript : public CRunningScript
    {
        friend class CScriptEngine;
        friend class CCustomOpcodeSystem;
        friend struct ThreadSavingInfo;

        DWORD m_codeSize;
        DWORD m_codeChecksum;

        bool m_saveEnabled;
        bool m_ok;
        eCLEO_Version m_compatVer;
        BYTE m_useTextCommands;
        int m_numDraws;
        int m_numTexts;
        CCustomScript* m_parentScript;
        std::list<CCustomScript*> m_childScripts;
        std::list<RwTexture*> m_scriptTextures;
        std::vector<BYTE> m_scriptDraws;
        std::vector<BYTE> m_scriptTexts;

        bool m_debugMode; // debug opcodes enabled

        std::string m_scriptFileDir;
        std::string m_scriptFileName;
        std::string m_workDir;

    public:
		inline RwTexture* GetScriptTextureById(unsigned int id)
		{
			if (m_scriptTextures.size() > id)
			{
				auto it = m_scriptTextures.begin();
				std::advance(it, id);
				return *it;
			}
			return nullptr;
		}

        inline SCRIPT_VAR * GetVarsPtr() { return LocalVar; }
        inline bool IsOk() const { return m_ok; }
        inline DWORD GetCodeSize() const { return m_codeSize; }
        inline DWORD GetCodeChecksum() const { return m_codeChecksum; }
        inline void EnableSaving(bool en = true) { m_saveEnabled = en; }
        inline void SetCompatibility(eCLEO_Version ver) { m_compatVer = ver; }
        inline eCLEO_Version GetCompatibility() const { return m_compatVer; }

        CCustomScript(const char *szFileName, bool bIsMiss = false, CRunningScript *parent = nullptr, int label = 0);
        CCustomScript(const CCustomScript&) = delete; // no copying
        ~CCustomScript();

        void AddScriptToList(CRunningScript** queuelist);
        void RemoveScriptFromList(CRunningScript** queuelist);

        void Process();
        void Draw(char bBeforeFade);
        void ShutdownThisScript();

        void StoreScriptSpecifics();
        void RestoreScriptSpecifics();
        void StoreScriptTextures();
        void RestoreScriptTextures();
        void StoreScriptDraws();
        void RestoreScriptDraws();

        // debug related utils enabled?
        bool GetDebugMode() const;
        void SetDebugMode(bool enabled);

        // absolute path to directory where script's source file is located
        const char* GetScriptFileDir() const;
        void SetScriptFileDir(const char* directory);

        // filename with type extension of script's source file
        const char* GetScriptFileName() const;
        void SetScriptFileName(const char* filename);

        // absolute path to the script file
        std::string GetScriptFileFullPath() const;

        // current working directory of this script. Can be changed ith 0A99
        const char* GetWorkDir() const;
        void SetWorkDir(const char* directory);

        // create absolute file path
        std::string ResolvePath(const char* path, const char* customWorkDir = nullptr) const;

        // get short info text about script
        std::string GetInfoStr(bool currLineInfo = true) const;
    };

    class CScriptEngine
    {
    public:
        bool gameInProgress = false;

        friend class CCustomScript;
        std::list<CCustomScript *> CustomScripts;
        std::list<CCustomScript *> ScriptsWaitingForDelete;
        std::set<unsigned long> InactiveScriptHashes;
        CCustomScript *CustomMission = nullptr;
        CCustomScript *LastScriptCreated = nullptr;

        CCustomScript* LoadScript(const char* filePath);
        CCustomScript* CreateCustomScript(CRunningScript* fromThread, const char* filePath, int label);

        bool NativeScriptsDebugMode; // debug mode enabled?
        CLEO::eCLEO_Version NativeScriptsVersion; // allows using legacy modes
        std::string MainScriptFileDir;
        std::string MainScriptFileName;
        std::string MainScriptCurWorkDir;

        static SCRIPT_VAR CleoVariables[0x400];

        CScriptEngine() = default;
        CScriptEngine(const CScriptEngine&) = delete; // no copying
        ~CScriptEngine();
        
        void Inject(CCodeInjector&);

        void GameBegin(); // call after new game started
        void GameEnd();

        void LoadCustomScripts();

        // CLEO saves
        void LoadState(int saveSlot);
        void SaveState();

        CRunningScript* FindScriptNamed(const char* threadName, bool standardScripts, bool customScripts, size_t resultIndex = 0); // can be called multiple times to find more scripts named threadName. resultIndex should be incremented until the method returns nullptr
        CRunningScript* FindScriptByFilename(const char* path, size_t resultIndex = 0); // if path is not absolute it will be resolved with cleo directory as root
        bool IsActiveScriptPtr(const CRunningScript*) const; // leads to active script? (regular or custom)
        bool IsValidScriptPtr(const CRunningScript*) const; // leads to any script? (regular or custom)
        void AddCustomScript(CCustomScript*);
        void RemoveScript(CRunningScript*); // native or custom
        void RemoveAllCustomScripts();
        void UnregisterAllScripts();
        void ReregisterAllScripts();

        void DrawScriptStuff(char bBeforeFade);

        inline CCustomScript* GetCustomMission() { return CustomMission; }
        inline size_t WorkingScriptsCount() { return CustomScripts.size(); }

    private:
        void RemoveCustomScript(CCustomScript*);

        static void __cdecl HOOK_DrawScriptText(char beforeFade);
        void(__cdecl* DrawScriptTextBeforeFade_Orig)(char beforeFade) = nullptr;
        void(__cdecl* DrawScriptTextAfterFade_Orig)(char beforeFade) = nullptr;
        static void DrawScriptText_Orig(char beforeFade);
        
        static void __fastcall HOOK_ProcessScript(CLEO::CRunningScript*);
        void(__fastcall* ProcessScript_Orig)(CLEO::CRunningScript*) = nullptr;
    };

    extern char(__thiscall * ScriptOpcodeHandler00)(CRunningScript *, WORD opcode);
    extern void(__thiscall * GetScriptParams)(CRunningScript *, int count);
    extern void(__thiscall * TransmitScriptParams)(CRunningScript *, CRunningScript *);
    extern void(__thiscall * SetScriptParams)(CRunningScript *, int count);
    extern void(__thiscall * SetScriptCondResult)(CRunningScript *, bool);
    extern SCRIPT_VAR * (__thiscall * GetScriptParamPointer1)(CRunningScript *);
    extern SCRIPT_VAR * (__thiscall * GetScriptParamPointer2)(CRunningScript *, int __unused__);

    // reimplemented hook of original game's procedure
    // returns buff or pointer provided by script, nullptr on fail
    // WARNING: Null terminator ommited if not enought space in the buffer!
    const char* __fastcall GetScriptStringParam(CRunningScript* thread, int dummy, char* buff, int buffLen); 

    inline SCRIPT_VAR* GetScriptParamPointer(CRunningScript* thread);

    extern BYTE *scmBlock, *missionBlock;
    extern int MissionIndex;
}

