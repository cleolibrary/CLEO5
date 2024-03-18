#pragma once
#include "CCodeInjector.h"
#include "CDebug.h"
#include <direct.h>
#include <set>

namespace CLEO
{
    typedef OpcodeResult(__stdcall * CustomOpcodeHandler)(CRunningScript*);

    extern const char* (__cdecl* GetUserDirectory)();
    extern void(__cdecl* ChangeToUserDir)();
    extern void(__cdecl* ChangeToProgramDir)(const char*);

    class CCustomOpcodeSystem : public VInjectible
    {
    public:
        static const size_t MinValidAddress = 0x10000; // used for validation of pointers received from scripts. First 64kb are for sure reserved by Windows.

        static const size_t LastOriginalOpcode = 0x0A4E; // GTA SA
        static const size_t LastCustomOpcode = 0x7FFF;
        static std::set<size_t> ProtectedOpcodes; // these can not be overwritten

        // most recently processed
        static CRunningScript* lastScript;
        static WORD lastOpcode;
        static WORD* lastOpcodePtr;
        static WORD lastCustomOpcode;
        static std::string lastErrorMsg;
        static WORD prevOpcode; // previous
        static BYTE handledParamCount; // read/writen since current opcode handling started
        
        void FinalizeScriptObjects();

        CCustomOpcodeSystem();
        virtual void Inject(CCodeInjector& inj);
        ~CCustomOpcodeSystem()
        {
            TRACE("Last opcode executed: %04X", lastOpcode);
            TRACE("Previous opcode executed: %04X", prevOpcode);
        }

        static bool RegisterOpcode(WORD opcode, CustomOpcodeHandler callback);

        static OpcodeResult CleoReturnGeneric(WORD opcode, CRunningScript* thread, bool returnArgs = false, DWORD returnArgCount = 0, bool strictArgCount = true);

        static OpcodeResult __stdcall opcode_0051(CRunningScript* thread); // GOSUB's return

    private:
        typedef OpcodeResult(__thiscall* _OpcodeHandler)(CRunningScript* thread, WORD opcode);

        static const size_t OriginalOpcodeHandlersCount = (LastOriginalOpcode / 100) + 1; // 100 opcodes peer handler
        static _OpcodeHandler originalOpcodeHandlers[OriginalOpcodeHandlersCount]; // backuped when patching

        static const size_t CustomOpcodeHandlersCount = (LastCustomOpcode / 100) + 1; // 100 opcodes peer handler
        static _OpcodeHandler customOpcodeHandlers[CustomOpcodeHandlersCount]; // original + new opcodes

        static OpcodeResult __fastcall customOpcodeHandler(CRunningScript* thread, int dummy, WORD opcode); // universal CLEO's opcode handler

        static CustomOpcodeHandler customOpcodeProc[LastCustomOpcode + 1]; // procedure for each opcode
    };

    extern void(__thiscall * ProcessScript)(CRunningScript*);

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
