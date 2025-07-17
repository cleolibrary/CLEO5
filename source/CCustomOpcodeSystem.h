#pragma once
#include "CCodeInjector.h"
#include "CDebug.h"
#include "ScriptDelegate.h"

namespace CLEO
{
    void ThreadJump(CRunningScript* thread, int off);

    class CCustomOpcodeSystem
    {
    public:
        static const size_t MinValidAddress = 0x10000; // used for validation of pointers received from scripts. First 64kb are for sure reserved by Windows.

        static const size_t LastOriginalOpcode = 0x0A4E; // GTA SA
        static const size_t LastCustomOpcode = 0x7FFF;

        // most recently processed
        static CRunningScript* lastScript;
        static WORD lastOpcode;
        static WORD* lastOpcodePtr;
        static WORD lastCustomOpcode;
        static std::string lastErrorMsg;
        static WORD prevOpcode; // previous
        static BYTE handledParamCount; // read/writen since current opcode handling started

        ScriptDeleteDelegate scriptDeleteDelegate;

        void FinalizeScriptObjects();

        CCustomOpcodeSystem() = default;
        CCustomOpcodeSystem(const CCustomOpcodeSystem&) = delete; // no copying
        void Inject(CCodeInjector& inj);
        void Init();
        ~CCustomOpcodeSystem();

        static bool RegisterOpcode(WORD opcode, CustomOpcodeHandler callback);

        static OpcodeResult CleoReturnGeneric(WORD opcode, CRunningScript* thread, bool returnArgs = false, DWORD returnArgCount = 0, bool strictArgCount = true);

        // new/customized opcodes
        static OpcodeResult __stdcall opcode_004E(CRunningScript* thread); // terminate_this_script
        static OpcodeResult __stdcall opcode_0051(CRunningScript* thread); // GOSUB return
        static OpcodeResult __stdcall opcode_0417(CRunningScript* thread); // load_and_launch_mission_internal

        static OpcodeResult __stdcall opcode_0A92(CRunningScript* thread); // stream_custom_script
        static OpcodeResult __stdcall opcode_0A93(CRunningScript* thread); // terminate_this_custom_script
        static OpcodeResult __stdcall opcode_0A94(CRunningScript* thread); // load_and_launch_custom_mission
        static OpcodeResult __stdcall opcode_0A95(CRunningScript* thread); // save_this_custom_script
        static OpcodeResult __stdcall opcode_0AA0(CRunningScript* thread); // gosub_if_false
        static OpcodeResult __stdcall opcode_0AA1(CRunningScript* thread); // return_if_false
        static OpcodeResult __stdcall opcode_0AA9(CRunningScript* thread); // is_game_version_original
        static OpcodeResult __stdcall opcode_0AB1(CRunningScript* thread); // cleo_call
        static OpcodeResult __stdcall opcode_0AB2(CRunningScript* thread); // cleo_return
        static OpcodeResult __stdcall opcode_0AB3(CRunningScript* thread); // set_cleo_shared_var
        static OpcodeResult __stdcall opcode_0AB4(CRunningScript* thread); // get_cleo_shared_var

        static OpcodeResult __stdcall opcode_0DD5(CRunningScript* thread); // get_platform

        static OpcodeResult __stdcall opcode_2000(CRunningScript* thread); // get_cleo_arg_count
        // 2001 free slot
        static OpcodeResult __stdcall opcode_2002(CRunningScript* thread); // cleo_return_with
        static OpcodeResult __stdcall opcode_2003(CRunningScript* thread); // cleo_return_fail

    private:
        bool initialized = false;

        typedef OpcodeResult(__thiscall* OpcodeHandler)(CRunningScript* thread, WORD opcode);

        static const size_t OriginalOpcodeHandlersCount = (LastOriginalOpcode / 100) + 1; // 100 opcodes peer handler
        static OpcodeHandler originalOpcodeHandlers[OriginalOpcodeHandlersCount]; // backuped when patching

        static const size_t CustomOpcodeHandlersCount = (LastCustomOpcode / 100) + 1; // 100 opcodes peer handler
        static OpcodeHandler customOpcodeHandlers[CustomOpcodeHandlersCount]; // original + new opcodes

        static OpcodeResult __fastcall customOpcodeHandler(CRunningScript* thread, int dummy, WORD opcode); // universal CLEO's opcode handler

        static CustomOpcodeHandler customOpcodeProc[LastCustomOpcode + 1]; // procedure for each opcode
    };

    // Read null-terminated string into the buffer
    // returns pointer to string or nullptr on fail
    // WARNING: returned pointer may differ from buff and contain string longer than buffSize (ptr to original data source)
    const char* ReadStringParam(CRunningScript* thread, char* buff, int buffSize);

    StringParamBufferInfo GetStringParamWriteBuffer(CRunningScript* thread); // consumes the param
    int ReadFormattedString(CRunningScript* thread, char* buf, DWORD bufSize, const char* format);

    bool WriteStringParam(CRunningScript* thread, const char* str);
    bool WriteStringParam(const StringParamBufferInfo& target, const char* str);

    void SkipUnusedVarArgs(CRunningScript* thread); // for var-args opcodes
    DWORD GetVarArgCount(CRunningScript* thread); // for var-args opcodes

    inline CRunningScript& operator>>(CRunningScript& thread, DWORD& uval);
    inline CRunningScript& operator<<(CRunningScript& thread, DWORD uval);
    inline CRunningScript& operator>>(CRunningScript& thread, int& nval);
    inline CRunningScript& operator<<(CRunningScript& thread, int nval);
    inline CRunningScript& operator>>(CRunningScript& thread, float& fval);
    inline CRunningScript& operator<<(CRunningScript& thread, float fval);
}
