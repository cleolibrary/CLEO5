#pragma once
#include "CCodeInjector.h"
#include "CDebug.h"
#include "ScriptDelegate.h"

namespace CLEO
{
    class CCustomOpcodeSystem
    {
    public:
        static constexpr size_t Opcodes_Table_Size = 100; // opcodes per handler
        static constexpr size_t Last_Original_Opcode = 0x0A4E; // GTA SA
        static constexpr size_t Last_Custom_Opcode = 0x7FFF;

        ScriptDeleteDelegate scriptDeleteDelegate; // script deletion callbacks

        CCustomOpcodeSystem() = default;
        CCustomOpcodeSystem(const CCustomOpcodeSystem&) = delete; // no copying
        void Inject(CCodeInjector& inj);
        void Init();
        ~CCustomOpcodeSystem() = default;

        void GameEnd(); // cleanup current game session stuff

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

        static constexpr size_t Original_Opcode_Handlers_Count = (Last_Original_Opcode / Opcodes_Table_Size) + 1;
        static OpcodeHandler originalOpcodeHandlers[OriginalOpcodeHandlersCount]; // backuped when patching

        static constexpr size_t Custom_Opcode_Handlers_Count = (Last_Custom_Opcode / Opcodes_Table_Size) + 1;
        static OpcodeHandler customOpcodeHandlers[CustomOpcodeHandlersCount]; // original + new opcodes

        static OpcodeResult __fastcall customOpcodeHandler(CRunningScript* thread, int dummy, WORD opcode); // universal CLEO's opcode handler

        static CustomOpcodeHandler customOpcodeProc[Last_Custom_Opcode + 1]; // procedure for each opcode
    };
}
