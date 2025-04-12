#pragma once

namespace CLEO
{
    const char cs_ext[] = ".cs";
    const char cs4_ext[] = ".cs4";
    const char cs3_ext[] = ".cs3";

    class CCodeInjector;
    class CCustomScript;
        void ShutdownThisScript();

    class CScriptEngine
    {
    public:
        static constexpr size_t Script_Draws_Capacity = 0x80;
        static constexpr size_t Script_Sprites_Capacity = 0x80;
        static constexpr size_t Script_Texts_Capacity = 0x60;

        bool gameInProgress = false;

        friend class CCustomScript;
        std::list<CCustomScript*> CustomScripts;
        std::list<CCustomScript*> ScriptsWaitingForDelete;
        std::set<unsigned long> InactiveScriptHashes;
        CCustomScript* CustomMission = nullptr;
        CCustomScript* LastScriptCreated = nullptr;

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

        void StoreScriptDrawsState();
        void RestoreScriptDrawsState();
        CSprite2d* GetScriptSprite(size_t index);
        static void SetScriptSpritesDefaults();
        static void SetScriptTextsDefaults();

        inline CCustomScript* GetCustomMission() { return CustomMission; }
        inline size_t WorkingScriptsCount() const { return CustomScripts.size(); }

    private:
        static void __cdecl OnDrawScriptText(uchar beforeFade); // hook
        void(__cdecl* DrawScriptTextBeforeFade_Orig)(uchar beforeFade) = nullptr;
        void(__cdecl* DrawScriptTextAfterFade_Orig)(uchar beforeFade) = nullptr;
        
        // stored script draws state
        bool storedDrawsState = false;
        bool storedUseTextCommands = false;
        WORD storedDrawsCount = 0;
        std::array<tScriptRectangle, Script_Draws_Capacity> storedDraws;
        std::array<CSprite2d, Script_Sprites_Capacity> storedSprites;
        WORD storedTextsCount = 0;
        std::array<tScriptText, Script_Texts_Capacity> storedTexts;

        void RemoveCustomScript(CCustomScript*);
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

