#include "CLEO.h"
#include "CLEO_Utils.h"
#include "plugin.h"
#include "CTheScripts.h"
#include <set>

using namespace CLEO;
using namespace plugin;

class MemoryOperations 
{
public:
    static std::set<void*> m_allocations;
    static std::set<HMODULE> m_libraries;

    MemoryOperations()
    {
        auto cleoVer = CLEO_GetVersion();
        if (cleoVer < CLEO_VERSION)
        {
            auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
            MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
            return;
        }

        //register opcodes
        CLEO_RegisterOpcode(0x0A8C, opcode_0A8C); // write_memory
        CLEO_RegisterOpcode(0x0A8D, opcode_0A8D); // read_memory

        CLEO_RegisterOpcode(0x0A96, opcode_0A96); // get_ped_pointer
        CLEO_RegisterOpcode(0x0A97, opcode_0A97); // get_vehicle_pointer
        CLEO_RegisterOpcode(0x0A98, opcode_0A98); // get_object_pointer

        CLEO_RegisterOpcode(0x0A9F, opcode_0A9F); // get_object_pointer

        CLEO_RegisterOpcode(0x0AA2, opcode_0AA2); // load_dynamic_library
        CLEO_RegisterOpcode(0x0AA3, opcode_0AA3); // free_library
        CLEO_RegisterOpcode(0x0AA4, opcode_0AA4); // get_dynamic_library_procedure
        CLEO_RegisterOpcode(0x0AA5, opcode_0AA5); // call_function
        CLEO_RegisterOpcode(0x0AA6, opcode_0AA6); // call_method
        CLEO_RegisterOpcode(0x0AA7, opcode_0AA7); // call_function_return
        CLEO_RegisterOpcode(0x0AA8, opcode_0AA8); // call_method_return

        CLEO_RegisterOpcode(0x0AAA, opcode_0AAA); // get_script_struct_named

        CLEO_RegisterOpcode(0x0AC6, opcode_0AC6); // get_label_pointer
        CLEO_RegisterOpcode(0x0AC7, opcode_0AC7); // get_var_pointer
        CLEO_RegisterOpcode(0x0AC8, opcode_0AC8); // allocate_memory
        CLEO_RegisterOpcode(0x0AC9, opcode_0AC9); // free_memory

        CLEO_RegisterOpcode(0x0AE9, opcode_0AE9); // pop_float
        CLEO_RegisterOpcode(0x0AEA, opcode_0AEA); // get_ped_ref
        CLEO_RegisterOpcode(0x0AEB, opcode_0AEB); // get_vehicle_ref
        CLEO_RegisterOpcode(0x0AEC, opcode_0AEC); // get_object_ref

        CLEO_RegisterOpcode(0x2400, opcode_2400); // copy_memory
        CLEO_RegisterOpcode(0x2401, opcode_2401); // read_memory_with_offset
        CLEO_RegisterOpcode(0x2402, opcode_2402); // write_memory_with_offset
        CLEO_RegisterOpcode(0x2403, opcode_2403); // forget_memory
        CLEO_RegisterOpcode(0x2404, opcode_2404); // get_custom_script_just_created
        CLEO_RegisterOpcode(0x2405, opcode_2405); // is_script_running

        // register event callbacks
        CLEO_RegisterCallback(eCallbackId::ScriptsFinalize, OnFinalizeScriptObjects);
    }

    static void __stdcall OnFinalizeScriptObjects()
    {
        TRACE("Cleaning up %d allocated memory blocks...", m_allocations.size());
        for (auto p : m_allocations) free(p);
        m_allocations.clear();

        TRACE("Cleaning up %d loaded libraries...", m_libraries.size());
        std::for_each(m_libraries.begin(), m_libraries.end(), FreeLibrary);
        m_libraries.clear();
    }

    // opcodes 0AA5 - 0AA8
    static OpcodeResult CallFunctionGeneric(CLEO::CRunningScript* thread, void* func, void* obj, int numArg, int numPop, bool returnArg)
    {
        int nVarArg = CLEO_GetVarArgCount(thread);
        if (numArg + returnArg != nVarArg) // and return argument
        {
            SHOW_ERROR("Declared %d input args, but provided %d in script %s\nScript suspended.", numArg, (int)nVarArg - returnArg, CLEO::ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        constexpr size_t Max_Args = 32;
        if (nVarArg > Max_Args)
        {
            SHOW_ERROR("Provided more (%d) than supported (%d) arguments in script %s\nScript suspended.", nVarArg, Max_Args, CLEO::ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        static SCRIPT_VAR arguments[Max_Args] = { 0 };
        SCRIPT_VAR* arguments_end = arguments + numArg;

        constexpr size_t Max_Text_Params = 5;
        static char textParams[Max_Text_Params][MAX_STR_LEN];
        size_t currTextParam = 0;

        numPop *= 4; // bytes peer argument

        // retrieve parameters
        auto scriptParams = CLEO_GetOpcodeParamsArray();
        for (size_t i = 0; i < (size_t)numArg; i++)
        {
            auto& param = arguments[i];

            auto paramType = thread->PeekDataType();
            if (IsImmString(paramType) || IsVarString(paramType))
            {
                if (currTextParam >= Max_Text_Params)
                {
                    SHOW_ERROR("Provided more (%d) than supported (%d) string arguments in script %s\nScript suspended.", currTextParam + 1, Max_Text_Params, CLEO::ScriptInfoStr(thread).c_str());
                    return thread->Suspend();
                }

                param.pcParam = OPCODE_READ_PARAM_STRING_BUFF(textParams[currTextParam], MAX_STR_LEN);
                currTextParam++;
            }
            else if (IsImmInteger(paramType) || IsImmFloat(paramType) || IsVariable(paramType))
            {
                CLEO_RetrieveOpcodeParams(thread, 1);
                param = scriptParams[0];
            }
            else
            {
                SHOW_ERROR("Invalid param type (%s) in script %s \nScript suspended.", ToKindStr(paramType), CLEO::ScriptInfoStr(thread).c_str());
                return thread->Suspend();
            }
        }

        DWORD result;
        _asm
        {
            // transfer args to stack
            lea ecx, arguments
            call_func_loop :
            cmp ecx, arguments_end
                jae call_func_loop_end
                push[ecx]
                add ecx, 0x4
                jmp call_func_loop
                call_func_loop_end :

            // call function
            mov ecx, obj
                xor eax, eax
                call func
                mov result, eax // get result
                add esp, numPop // cleanup stack
        }

        if (returnArg)
        {
            auto paramType = thread->PeekDataType();

            if (IsVarString(paramType))
            {
                OPCODE_WRITE_PARAM_STRING((char*)result);
            }
            else
            {
                OPCODE_WRITE_PARAM_UINT(result);
            }
        }

        CLEO_SkipUnusedVarArgs(thread);
        return OR_CONTINUE;
    }

    //0A8C=4,write_memory %1d% size %2d% value %3d% virtual_protect %4d%
    static OpcodeResult __stdcall opcode_0A8C(CLEO::CRunningScript* thread)
    {
        // collect params
        auto address = OPCODE_READ_PARAM_PTR();
        auto size = OPCODE_READ_PARAM_INT();

        // value param
        const void* source;
        auto paramType = thread->PeekDataType();
        bool sourceText = false;
        if (IsVariable(paramType))
        {
            source = CLEO_GetPointerToScriptVariable(thread);
        }
        else if (IsImmString(paramType) || IsVarString(paramType))
        {
            static char buffer[MAX_STR_LEN];

            if (size > MAX_STR_LEN)
            {
                SHOW_ERROR("Size argument (%d) greater than supported (%d) in script %s\nScript suspended.", size, MAX_STR_LEN, ScriptInfoStr(thread).c_str());
                return thread->Suspend();
            }

            ZeroMemory(buffer, size); // padd with zeros if size > length
            source = CLEO_ReadStringOpcodeParam(thread, buffer, sizeof(buffer));
            sourceText = true;
        }
        else
        {
            static SCRIPT_VAR value;

            CLEO_RetrieveOpcodeParams(thread, 1);
            value = CLEO_GetOpcodeParamsArray()[0];
            source = &value;
        }

        auto virtualProtect = OPCODE_READ_PARAM_BOOL();

        // validate params
        if ((size_t)address <= MinValidAddress)
        {
            SHOW_ERROR("Invalid '0x%X' pointer param in script %s\nScript suspended.", address, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        // perform
        if (size == 0) return OR_CONTINUE; // done

        if (virtualProtect)
        {
            DWORD oldProtect;
            VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        }

        if (!sourceText)
        {
            // that's how it worked since ever...
            if (size == 2 || size == 4)
                memcpy(address, source, size);
            else
                memset(address, *((int*)source), size);
        }
        else
        {
            memcpy(address, source, size);
        }

        return OR_CONTINUE;
    }

    //0A8D=4,read_memory %1d% size %2d% virtual_protect %3d% store_to %4d%
    static OpcodeResult __stdcall opcode_0A8D(CLEO::CRunningScript* thread)
    {
        // collect params
        auto address = OPCODE_READ_PARAM_PTR();
        auto size = OPCODE_READ_PARAM_INT();
        auto virtualProtect = OPCODE_READ_PARAM_BOOL();

        // validate params
        if ((size_t)address <= MinValidAddress)
        {
            SHOW_ERROR("Invalid '0x%X' pointer param of in script %s\nScript suspended.", address, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        if (size < 0 || size > sizeof(SCRIPT_VAR))
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        // perform
        DWORD value = 0;
        if (size > 0)
        {
            if (virtualProtect)
            {
                DWORD oldProtect;
                VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
            }

            memcpy(&value, address, size);
        }

        OPCODE_WRITE_PARAM_UINT(value);
        return OR_CONTINUE;
    }

    //0A96=2,get_ped_pointer %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0A96(CLEO::CRunningScript* thread)
    {
        // collect params
        auto handle = OPCODE_READ_PARAM_UINT();

        // validate params
        auto index = handle >> 8; // lowest byte holds flags
        if (index >= (DWORD)CPools::ms_pPedPool->m_nSize)
        {
            OPCODE_WRITE_PARAM_PTR(nullptr);
            return OR_CONTINUE;
        }

        auto ptr = CPools::GetPed(handle);

        OPCODE_WRITE_PARAM_PTR(ptr);
        return OR_CONTINUE;
    }

    //0A97=2,get_vehicle_pointer %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0A97(CLEO::CRunningScript* thread)
    {
        // collect params
        auto handle = OPCODE_READ_PARAM_UINT();

        // validate params
        auto index = handle >> 8; // lowest byte holds flags
        if (index >= (DWORD)CPools::ms_pVehiclePool->m_nSize)
        {
            OPCODE_WRITE_PARAM_PTR(nullptr);
            return OR_CONTINUE;
        }

        auto ptr = CPools::GetVehicle(handle);

        OPCODE_WRITE_PARAM_PTR(ptr);
        return OR_CONTINUE;
    }

    //0A98=2,get_object_pointer %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0A98(CLEO::CRunningScript* thread)
    {
        // collect params
        auto handle = OPCODE_READ_PARAM_UINT();

        // validate params
        auto index = handle >> 8; // lowest byte holds flags
        if (index >= (DWORD)CPools::ms_pObjectPool->m_nSize)
        {
            OPCODE_WRITE_PARAM_PTR(nullptr);
            return OR_CONTINUE;
        }

        auto ptr = CPools::GetObject(handle);

        OPCODE_WRITE_PARAM_PTR(ptr);
        return OR_CONTINUE;
    }

    //0A9F=1, get_this_script_struct store_to %1d%
    static OpcodeResult __stdcall opcode_0A9F(CLEO::CRunningScript* thread)
    {
        OPCODE_WRITE_PARAM_PTR(thread);
        return OR_CONTINUE;
    }

    //0AA2=2, load_dynamic_library %1s% store_to %2d% // IF and SET
    static OpcodeResult __stdcall opcode_0AA2(CLEO::CRunningScript* thread)
    {
        auto str = OPCODE_READ_PARAM_FILEPATH();

        auto ptr = LoadLibrary(str);
        if (ptr != nullptr)
        {
            m_libraries.insert(ptr);
        }

        OPCODE_WRITE_PARAM_PTR(ptr);
        OPCODE_CONDITION_RESULT(ptr != nullptr);
        return OR_CONTINUE;
    }

    //0AA3=1,free_library %1h%
    static OpcodeResult __stdcall opcode_0AA3(CLEO::CRunningScript* thread)
    {
        auto ptr = (HMODULE)OPCODE_READ_PARAM_PTR();

        // validate
        if (m_libraries.find(ptr) == m_libraries.end())
        {
            LOG_WARNING(thread, "Invalid '0x%X' pointer param to unknown or already freed library in script %s", ptr, ScriptInfoStr(thread).c_str());
            return OR_CONTINUE;
        }

        FreeLibrary(ptr);
        m_libraries.erase(ptr);
        return OR_CONTINUE;
    }

    //0AA4=3, get_proc_address %1d% library %2d% result %3d% // IF and SET
    static OpcodeResult __stdcall opcode_0AA4(CLEO::CRunningScript* thread)
    {
        auto name = OPCODE_READ_PARAM_STRING();
        auto ptr = (HMODULE)OPCODE_READ_PARAM_PTR();

        // validate
        if (m_libraries.find(ptr) == m_libraries.end())
        {
            LOG_WARNING(thread, "Invalid '0x%X' pointer param to unknown or freed library in script %s", ptr, ScriptInfoStr(thread).c_str());
            OPCODE_WRITE_PARAM_PTR(nullptr);
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        auto funcPtr = (void*)GetProcAddress(ptr, name);

        OPCODE_WRITE_PARAM_PTR(funcPtr);
        OPCODE_CONDITION_RESULT(funcPtr != nullptr);
        return OR_CONTINUE;
    }

    //0AA5=-1,call %1d% num_params %2h% pop %3h%
    static OpcodeResult __stdcall opcode_0AA5(CLEO::CRunningScript* thread)
    {
        auto func = OPCODE_READ_PARAM_PTR();
        auto numArgs = OPCODE_READ_PARAM_INT();
        auto numPop = OPCODE_READ_PARAM_INT();

        return CallFunctionGeneric(thread, func, nullptr, numArgs, numPop, false);
    }

    //0AA6=-1,call_method %1d% struct %2d% num_params %3h% pop %4h%
    static OpcodeResult __stdcall opcode_0AA6(CLEO::CRunningScript* thread)
    {
        auto func = OPCODE_READ_PARAM_PTR();
        auto obj = OPCODE_READ_PARAM_PTR();
        auto numArgs = OPCODE_READ_PARAM_INT();
        auto numPop = OPCODE_READ_PARAM_INT();

        return CallFunctionGeneric(thread, func, obj, numArgs, numPop, false);
    }

    //0AA7=-1,call_function_return %1d% num_params %2h% pop %3h%
    static OpcodeResult __stdcall opcode_0AA7(CLEO::CRunningScript* thread)
    {
        auto func = OPCODE_READ_PARAM_PTR();
        auto numArgs = OPCODE_READ_PARAM_INT();
        auto numPop = OPCODE_READ_PARAM_INT();

        return CallFunctionGeneric(thread, func, nullptr, numArgs, numPop, true);
    }

    //0AA8=-1,call_method_return %1d% struct %2d% num_params %3h% pop %4h%
    static OpcodeResult __stdcall opcode_0AA8(CLEO::CRunningScript* thread)
    {
        auto func = OPCODE_READ_PARAM_PTR();
        auto obj = OPCODE_READ_PARAM_PTR();
        auto numArgs = OPCODE_READ_PARAM_INT();
        auto numPop = OPCODE_READ_PARAM_INT();

        return CallFunctionGeneric(thread, func, obj, numArgs, numPop, true);
    }

    //0AAA=2,  get_script_struct_named %1d% pointer %2d% // IF and SET
    static OpcodeResult __stdcall opcode_0AAA(CLEO::CRunningScript *thread)
    {
        auto name = OPCODE_READ_PARAM_STRING();

        auto ptr = CLEO_GetScriptByName(name, true, true, 0);

        OPCODE_WRITE_PARAM_PTR(ptr);
        OPCODE_CONDITION_RESULT(ptr != nullptr);
        return OR_CONTINUE;
    }

    //0AC6=2,get_label_pointer %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AC6(CLEO::CRunningScript* thread)
    {
        auto label = OPCODE_READ_PARAM_INT();

        // perform
        void* ptr = nullptr;
        if (label < 0)
            ptr = thread->GetBasePointer() - label;
        else
            ptr = CTheScripts::ScriptSpace + label;

        OPCODE_WRITE_PARAM_PTR(ptr);
        return OR_CONTINUE;
    }

    //0AC7=2,get_var_pointer %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AC7(CLEO::CRunningScript* thread)
    {
        auto resultType = thread->PeekDataType();
        if (!IsVariable(resultType) && IsVarString(resultType))
        {
            SHOW_ERROR("Input argument #%d expected to be variable, got constant in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        auto ptr = CLEO_GetPointerToScriptVariable(thread);

        OPCODE_WRITE_PARAM_PTR(ptr);
        return OR_CONTINUE;
    }

    //0AC8=2,  allocate_memory size %1d% store_to %2d%
    static OpcodeResult WINAPI opcode_0AC8(CLEO::CRunningScript* thread)
    {
        // collect params
        int size = OPCODE_READ_PARAM_INT();

        // validate params
        if (size <= 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        // perform
        void* mem = calloc(size, 1);
        if (mem)
        {
            DWORD oldProtect;
            VirtualProtect(mem, size, PAGE_EXECUTE_READWRITE, &oldProtect);

            m_allocations.insert(mem);
        }
        else
            LOG_WARNING(thread, "Failed to allocate %d bytes of memory in script %s", size, ScriptInfoStr(thread).c_str());

        OPCODE_WRITE_PARAM_PTR(mem);
        OPCODE_CONDITION_RESULT(mem != nullptr);
        return OR_CONTINUE;
    }

    //0AC9=1,free_memory %1d%
    static OpcodeResult __stdcall opcode_0AC9(CLEO::CRunningScript* thread)
    {
        // collect params
        auto address = OPCODE_READ_PARAM_PTR();

        // validate params
        if (m_allocations.find(address) == m_allocations.end())
        {
            LOG_WARNING(thread, "Invalid '0x%X' pointer param to unknown or already freed memory in script %s", address, ScriptInfoStr(thread).c_str());
            return OR_CONTINUE;
        }

        free(address);
        m_allocations.erase(address);
        return OR_CONTINUE; // done
    }

    //0AE9=1,pop_float store_to %1d%
    static OpcodeResult __stdcall opcode_0AE9(CLEO::CRunningScript* thread)
    {
        float result;
        _asm fstp result

        OPCODE_WRITE_PARAM_FLOAT(result);
        return OR_CONTINUE;
    }

    //0AEA=2,get_ped_ref %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AEA(CLEO::CRunningScript* thread)
    {
        // collect params
        auto ptr = (CPed*)OPCODE_READ_PARAM_PTR();

        int handle = -1;
        if (!CPools::ms_pPedPool->IsObjectValid(ptr))
        {
            OPCODE_WRITE_PARAM_INT(-1); // invalid handle
            return OR_CONTINUE;
        }

        handle = CPools::GetPedRef(ptr);

        OPCODE_WRITE_PARAM_INT(handle);
        return OR_CONTINUE;
    }

    //0AEB=2,get_vehicle_ref %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AEB(CLEO::CRunningScript* thread)
    {
        auto ptr = (CVehicle*)OPCODE_READ_PARAM_PTR();

        int handle = -1;
        if (!CPools::ms_pVehiclePool->IsObjectValid(ptr))
        {
            OPCODE_WRITE_PARAM_INT(-1); // invalid handle
            return OR_CONTINUE;
        }

        handle = CPools::GetVehicleRef(ptr);

        OPCODE_WRITE_PARAM_INT(handle);
        return OR_CONTINUE;
    }

    //0AEC=2,get_object_ref %1d% store_to %2d%
    static OpcodeResult __stdcall opcode_0AEC(CLEO::CRunningScript* thread)
    {
        auto ptr = (CObject*)OPCODE_READ_PARAM_PTR();

        int handle = -1;
        if (!CPools::ms_pObjectPool->IsObjectValid(ptr))
        {
            OPCODE_WRITE_PARAM_INT(-1); // invalid handle
            return OR_CONTINUE;
        }

        handle = CPools::GetObjectRef(ptr);

        OPCODE_WRITE_PARAM_INT(handle);
        return OR_CONTINUE;
    }

    //2400=3,copy_memory %1d% to %2d% size %3d%
    static OpcodeResult __stdcall opcode_2400(CLEO::CRunningScript* thread)
    {
        auto src = (BYTE*)OPCODE_READ_PARAM_PTR();
        auto trg = (BYTE*)OPCODE_READ_PARAM_PTR();
        auto size = OPCODE_READ_PARAM_INT();

        if (size == 0)
        {
            return OR_CONTINUE; // done
        }
        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        // memory blocks can not overlap
        auto first = min(src, trg);
        auto second = max(src, trg);
        if ((first + size) > second)
        {
            SHOW_ERROR("Invalid overlapping memory blocks in script %s\nScript suspended.", ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        memcpy((void*)trg, (void*)src, size);
        return OR_CONTINUE;
    }

    //2401=4,read_memory_with_offset %1d% offset %2d% size %3d% store_to %4d%
    static OpcodeResult __stdcall opcode_2401(CLEO::CRunningScript* thread)
    {
        auto ptr = (BYTE*)OPCODE_READ_PARAM_PTR();
        auto offset = OPCODE_READ_PARAM_INT();
        auto size = OPCODE_READ_PARAM_INT();

        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        auto resultType = thread->PeekDataType();
        if (IsVariable(resultType))
        {
            if (size == 0)
            {
                OPCODE_WRITE_PARAM_INT(0);
                return OR_CONTINUE; // done
            }

            DWORD result = 0;
            if (size > sizeof(result))
            {
                LOG_WARNING(thread, "Size '%d' argument out of supported range (%d) in script %s", size, sizeof(result), ScriptInfoStr(thread).c_str());
                size = sizeof(result);
            }
            if (size > 0) memcpy(&result, (void*)(ptr + offset), size);

            OPCODE_WRITE_PARAM_INT(result);
            return OR_CONTINUE;
        }
        else if (IsVarString(resultType))
        {
            std::string str(std::string_view((char*)ptr + offset, size)); // null terminated
            OPCODE_WRITE_PARAM_STRING(str.c_str());
            return OR_CONTINUE;
        }

        SHOW_ERROR("Invalid type (%s) of the result argument in script %s \nScript suspended.", ToKindStr(resultType), ScriptInfoStr(thread).c_str());
        return thread->Suspend();
    }

    //2402=4,write_memory_with_offset %1d% offset %2d% size %3d% value %4d%
    static OpcodeResult __stdcall opcode_2402(CLEO::CRunningScript* thread)
    {
        auto ptr = (BYTE*)OPCODE_READ_PARAM_PTR();
        auto offset = OPCODE_READ_PARAM_INT();
        auto size = OPCODE_READ_PARAM_INT();

        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        if (size == 0)
        {
            CLEO_SkipOpcodeParams(thread, 1); // value not used
            return OR_CONTINUE; // done
        }

        auto valueType = thread->PeekDataType();
        if (IsImmInteger(valueType) || IsImmFloat(valueType) || IsVariable(valueType))
        {
            if (size > sizeof(DWORD))
            {
                LOG_WARNING(thread, "Size '%d' argument out of supported range (%d) in script %s", size, sizeof(DWORD), ScriptInfoStr(thread).c_str());
                size = sizeof(DWORD);
            }

            auto value = OPCODE_READ_PARAM_INT();
            memcpy(ptr + offset, &value, size);

            return OR_CONTINUE;
        }
        else if (IsImmString(valueType) || IsVarString(valueType))
        {
            auto str = OPCODE_READ_PARAM_STRING();
            auto len = (int)strlen(str);

            memcpy(ptr + offset, str, min(size, len));
            if (size > len) ZeroMemory(ptr + offset + len, size - len); // fill rest with zeros

            return OR_CONTINUE;
        }

        SHOW_ERROR("Invalid type (%s) of the input argument #%d in script %s \nScript suspended.", CLEO_GetParamsHandledCount(), ToKindStr(valueType), ScriptInfoStr(thread).c_str());
        return thread->Suspend();
    }

    //2403=1,forget_memory %1d%
    static OpcodeResult __stdcall opcode_2403(CLEO::CRunningScript* thread)
    {
        // collect params
        auto address = OPCODE_READ_PARAM_PTR();

        // validate params
        if (m_allocations.find(address) == m_allocations.end())
        {
            LOG_WARNING(thread, "Invalid '0x%X' pointer param to unknown or already freed memory in script %s", address, ScriptInfoStr(thread).c_str());
            return OR_CONTINUE;
        }

        m_allocations.erase(address);
        return OR_CONTINUE; // done
    }

    //2404=1, get_custom_script_just_created %1d%
    static OpcodeResult __stdcall opcode_2404(CLEO::CScriptThread* thread)
    {
        auto address = CLEO_GetLastCreatedCustomScript();

        OPCODE_WRITE_PARAM_PTR(address);
        OPCODE_CONDITION_RESULT(address != nullptr);
        return OR_CONTINUE;
    }

    //2405=1, is_script_running %1d%
    static OpcodeResult __stdcall opcode_2405(CLEO::CScriptThread* thread)
    {
        auto address = (CLEO::CScriptThread*)OPCODE_READ_PARAM_INT();

        auto name = CLEO_GetScriptFilename(address);

        OPCODE_CONDITION_RESULT(name != nullptr);
        return OR_CONTINUE;
    }
} Memory;

std::set<void*> MemoryOperations::m_allocations;
std::set<HMODULE> MemoryOperations::m_libraries;
