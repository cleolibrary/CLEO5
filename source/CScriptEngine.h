#pragma once
#include "CCustomScript.h"

namespace CLEO
{
    const char cs_ext[] = ".cs";
    const char cs4_ext[] = ".cs4";
    const char cs3_ext[] = ".cs3";

    class CScriptEngine
    {
    public:
        bool gameInProgress = false;

        BYTE* scmBlock = nullptr;
        BYTE* missionBlock = nullptr;
        int missionIndex = -1;

        friend class CCustomScript;
        std::list<CCustomScript*> CustomScripts;
        std::list<CCustomScript*> ScriptsWaitingForDelete;
        std::set<unsigned long> InactiveScriptHashes;
        CCustomScript *CustomMission = nullptr;
        CCustomScript *LastScriptCreated = nullptr;

        CCustomScript* LoadScript(const char* filePath);
        CCustomScript* CreateCustomScript(Script* fromScript, const char* filePath, int label);

        bool NativeScriptsDebugMode; // debug mode enabled?
        eCLEO_Version NativeScriptsVersion; // allows using legacy modes
        std::string MainScriptFileDir;
        std::string MainScriptFileName;
        std::string MainScriptCurWorkDir;

        // most recently processed
        static Script* lastScript;
        static WORD lastOpcode;
        static BYTE* lastOpcodePtr; // start of the command
        static std::string lastErrorMsg;
        static WORD prevOpcode; // previous
        static BYTE handledParamCount; // read/writen since current opcode handling started

        static SCRIPT_VAR CleoVariables[0x400];

        CScriptEngine() = default;
        CScriptEngine(const CScriptEngine&) = delete; // no copying
        ~CScriptEngine();
        
        void Inject(CCodeInjector&); // Phase 1
        void InjectLate(CCodeInjector&); // Phase 2

        void GameBegin(); // call after new game started
        void GameEnd();

        void LoadCustomScripts();

        // CLEO saves
        void LoadState(int saveSlot);
        void SaveState();

        Script* FindScriptNamed(const char* scriptName, bool standardScripts, bool customScripts, size_t resultIndex = 0); // can be called multiple times to find more scripts named scriptName. resultIndex should be incremented until the method returns nullptr
        Script* FindScriptByFilename(const char* path, size_t resultIndex = 0); // if path is not absolute it will be resolved with cleo directory as root
        bool IsActiveScriptPtr(const Script* script) const; // leads to active script? (regular or custom)
        bool IsValidScriptPtr(const Script* script) const; // leads to any script? (regular or custom)
        void AddCustomScript(CCustomScript* script);
        void RemoveScript(Script* script); // native or custom
        void RemoveAllCustomScripts();

        // remove/re-add to active scripts queue
        void UnregisterAllCustomScripts();
        void ReregisterAllCustomScripts();

        inline CCustomScript* GetCustomMission() { return CustomMission; }
        inline size_t WorkingScriptsCount() { return CustomScripts.size(); }

        // params into/from opcodeParams array
        static void ReadScriptParams(Script* script, BYTE count);
        static void WriteScriptParams(Script* script, BYTE count);

        // read string params
        static SCRIPT_VAR* GetScriptParamPointer(Script* script); // peek
        static StringParamBufferInfo ReadStringParamWriteBuffer(Script* script); // get buffer info to write later
        static const char* ReadStringParam(Script* script, char* buff, int buffSize);
        static int ReadFormattedString(Script* script, char* buf, DWORD bufSize, const char* format);

        // write string params
        static bool WriteStringParam(Script* script, const char* str);
        static bool WriteStringParam(const StringParamBufferInfo& target, const char* str);

        // variable arguments
        static DWORD GetVarArgCount(Script* script);
        static void SkipUnusedVarArgs(Script* script);

        static void ThreadJump(Script* script, int offset);

        static void DrawScriptText_Orig(char beforeFade);

    private:
        void RemoveCustomScript(CCustomScript*);

        static void __cdecl HOOK_DrawScriptText(char beforeFade);
        void(__cdecl* DrawScriptTextBeforeFade_Orig)(char beforeFade) = nullptr;
        void(__cdecl* DrawScriptTextAfterFade_Orig)(char beforeFade) = nullptr;

        static void HOOK_LoadScmData(void);
        static void HOOK_SaveScmData(void);
        
        // returns buff or pointer provided by script, nullptr on fail
        // WARNING: Null terminator ommited if not enought space in the target buffer!
        static const char* __fastcall HOOK_GetScriptStringParam(Script* script, int dummy, char* buff, int buffLen);

        static void __fastcall HOOK_ProcessScript(Script*);
        void(__fastcall* ProcessScript_Orig)(Script*) = nullptr;
    };
}

