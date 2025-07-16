#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"

#include <random>

using namespace CLEO;
using namespace plugin;
using namespace std;

class Math
{
public:
    random_device randomDevice;
    mt19937 randomGenerator;

    Math() :
        randomDevice(),
        randomGenerator(randomDevice())
    {
        if (!PluginCheckCleoVersion()) return;

        // register opcodes
        CLEO_RegisterOpcode(0x0A8E, opcode_0A8E); // x = a + b (int)
        CLEO_RegisterOpcode(0x0A8F, opcode_0A8F); // x = a - b (int)
        CLEO_RegisterOpcode(0x0A90, opcode_0A90); // x = a * b (int)
        CLEO_RegisterOpcode(0x0A91, opcode_0A91); // x = a / b (int)

        CLEO_RegisterOpcode(0x0AEE, opcode_0AEE); // pow
        CLEO_RegisterOpcode(0x0AEF, opcode_0AEF); // log

        CLEO_RegisterOpcode(0x0B10, Script_IntOp_AND);
        CLEO_RegisterOpcode(0x0B11, Script_IntOp_OR);
        CLEO_RegisterOpcode(0x0B12, Script_IntOp_XOR);
        CLEO_RegisterOpcode(0x0B13, Script_IntOp_NOT);
        CLEO_RegisterOpcode(0x0B14, Script_IntOp_MOD);
        CLEO_RegisterOpcode(0x0B15, Script_IntOp_SHR);
        CLEO_RegisterOpcode(0x0B16, Script_IntOp_SHL);
        CLEO_RegisterOpcode(0x0B17, Scr_IntOp_AND);
        CLEO_RegisterOpcode(0x0B18, Scr_IntOp_OR);
        CLEO_RegisterOpcode(0x0B19, Scr_IntOp_XOR);
        CLEO_RegisterOpcode(0x0B1A, Scr_IntOp_NOT);
        CLEO_RegisterOpcode(0x0B1B, Scr_IntOp_MOD);
        CLEO_RegisterOpcode(0x0B1C, Scr_IntOp_SHR);
        CLEO_RegisterOpcode(0x0B1D, Scr_IntOp_SHL);
        CLEO_RegisterOpcode(0x0B1E, Sign_Extend);

        CLEO_RegisterOpcode(0x2700, opcode_2700); // is_bit_set
        CLEO_RegisterOpcode(0x2701, opcode_2701); // set_bit
        CLEO_RegisterOpcode(0x2702, opcode_2702); // clear_bit
        CLEO_RegisterOpcode(0x2703, opcode_2703); // toggle_bit
        CLEO_RegisterOpcode(0x2704, opcode_2704); // is_truthy
        CLEO_RegisterOpcode(0x2705, opcode_2705); // pick_random_int
        CLEO_RegisterOpcode(0x2706, opcode_2706); // pick_random_float
        CLEO_RegisterOpcode(0x2707, opcode_2707); // pick_random_text
        CLEO_RegisterOpcode(0x2708, opcode_2708); // random_chance
    }

    //0A8E=3,%3d% = %1d% + %2d% ; int
    static OpcodeResult __stdcall opcode_0A8E(CRunningScript* thread)
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a + b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    //0A8F=3,%3d% = %1d% - %2d% ; int
    static OpcodeResult __stdcall opcode_0A8F(CRunningScript* thread)
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a - b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    //0A90=3,%3d% = %1d% * %2d% ; int
    static OpcodeResult __stdcall opcode_0A90(CRunningScript* thread)
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a * b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    //0A91=3,%3d% = %1d% / %2d% ; int
    static OpcodeResult __stdcall opcode_0A91(CRunningScript* thread)
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a / b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    //0AEE=3,%3d% = %1d% exp %2d% // all floats
    static OpcodeResult __stdcall opcode_0AEE(CRunningScript* thread)
    {
        auto base = OPCODE_READ_PARAM_FLOAT();
        auto exponent = OPCODE_READ_PARAM_FLOAT();

        auto result = (float)pow(base, exponent);

        OPCODE_WRITE_PARAM_FLOAT(result);
        return OR_CONTINUE;
    }

    //0AEF=3,%3d% = log %1d% base %2d% // all floats
    static OpcodeResult __stdcall opcode_0AEF(CRunningScript* thread)
    {
        auto argument = OPCODE_READ_PARAM_FLOAT();
        auto base = OPCODE_READ_PARAM_FLOAT();

        auto exponent = log(argument) / log(base);

        OPCODE_WRITE_PARAM_FLOAT(exponent);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_AND(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B10=3,%3d% = %1d% AND %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a & b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_OR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B11=3,%3d% = %1d% OR %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a | b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_XOR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B12=3,%3d% = %1d% XOR %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a ^ b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_NOT(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B13=2,%2d% = NOT %1d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();

        OPCODE_WRITE_PARAM_INT(~a);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_MOD(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B14=3,%3d% = %1d% MOD %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a % b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    static  OpcodeResult __stdcall Script_IntOp_SHR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B15=3,%3d% = %1d% SHR %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a >> b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Script_IntOp_SHL(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B16=3,%3d% = %1d% SHL %2d%
        ****************************************************************/
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a << b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }

    /****************************************************************
    Now do them as real operators...
    *****************************************************************/

    static OpcodeResult __stdcall Scr_IntOp_AND(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B17=2,%1d% &= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand &= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_OR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B18=2,%1d% |= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand |= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_XOR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B19=2,%1d% ^= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand ^= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_NOT(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B1A=1,~%1d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();

        *operand = ~*operand;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_MOD(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B1B=2,%1d% %= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand %= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_SHR(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B1C=2,%1d% >>= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand >>= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Scr_IntOp_SHL(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B1D=2,%1d% <<= %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto value = OPCODE_READ_PARAM_INT();

        *operand <<= value;
        return OR_CONTINUE;
    }

    static OpcodeResult __stdcall Sign_Extend(CRunningScript* thread)
        /****************************************************************
        Opcode Format
        0B1E=2,sign_extend %1d% size %2d%
        ****************************************************************/
    {
        auto operand = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto size = OPCODE_READ_PARAM_INT();

        if (size <= 0 || size > 4)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        size_t offset = size * 8 - 1; // bit offset of top most bit in source value
        bool signBit = *operand & (1 << offset);

        if(signBit)
        {
            *operand |= 0xFFFFFFFF << offset; // set all upper bits
        }
        
        return OR_CONTINUE;
    }

    //2700=2,  is_bit_set value %1d% bit_index %2d%
    static OpcodeResult __stdcall opcode_2700(CRunningScript* thread)
    {
        auto value = OPCODE_READ_PARAM_UINT();
        auto bitIndex = OPCODE_READ_PARAM_INT();

        if (bitIndex < 0 || bitIndex > 31)
        {
            SHOW_ERROR("Invalid '%d' bit index argument in script %s\nScript suspended.", bitIndex, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        bool result = (value >> bitIndex) & 1;

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    //2701=2,set_bit value %1d% bit_index %2d%
    static OpcodeResult __stdcall opcode_2701(CRunningScript* thread)
    {
        auto value = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto bitIndex = OPCODE_READ_PARAM_INT();

        if (bitIndex < 0 || bitIndex > 31)
        {
            SHOW_ERROR("Invalid '%d' bit index argument in script %s\nScript suspended.", bitIndex, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        *value |= 1 << bitIndex;

        return OR_CONTINUE;
    }

    //2702=2,clear_bit value %1d% bit_index %2d%
    static OpcodeResult __stdcall opcode_2702(CRunningScript* thread)
    {
        auto value = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto bitIndex = OPCODE_READ_PARAM_INT();

        if (bitIndex < 0 || bitIndex > 31)
        {
            SHOW_ERROR("Invalid '%d' bit index argument in script %s\nScript suspended.", bitIndex, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        *value &= ~(1 << bitIndex);

        return OR_CONTINUE;
    }

    //2703=3,toggle_bit value %1d% bit_index %2d% state %3d%
    static OpcodeResult __stdcall opcode_2703(CRunningScript* thread)
    {
        auto value = OPCODE_READ_PARAM_OUTPUT_VAR_INT();
        auto bitIndex = OPCODE_READ_PARAM_INT();
        auto state = OPCODE_READ_PARAM_BOOL();

        if (bitIndex < 0 || bitIndex > 31)
        {
            SHOW_ERROR("Invalid '%d' bit index argument in script %s\nScript suspended.", bitIndex, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        DWORD flag = 1 << bitIndex;
        if (state)
            *value |= flag;
        else
            *value &= ~flag;

        return OR_CONTINUE;
    }

    //2704=1,  is_truthy value %1d%
    static OpcodeResult __stdcall opcode_2704(CRunningScript* thread)
    {
        auto paramType = OPCODE_PEEK_PARAM_TYPE();

        if(IsImmString(paramType) || IsVarString(paramType))
        {
            OPCODE_READ_PARAM_STRING_LEN(text, 1); // one character is all we need
            OPCODE_CONDITION_RESULT(text[0] != '\0');
            return OR_CONTINUE;
        }

        auto value = OPCODE_READ_PARAM_ANY32();
        OPCODE_CONDITION_RESULT(value != 0);
        return OR_CONTINUE;
    }

    //2705=-1,pick_random_int values %d% store_to %d%
    static OpcodeResult __stdcall opcode_2705(CRunningScript* thread)
    {
        auto valueCount = CLEO_GetVarArgCount(thread);

        if (valueCount < 2) // value + result
        {
            SHOW_ERROR("Insufficient number of arguments in script %s\nScript suspended.", CLEO::ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }
        valueCount -= 1; // output param

        uniform_int_distribution<size_t> dist(0, valueCount - 1); // 0 to last index
        auto idx = dist(Instance.randomGenerator);

        // read n-th param
        OPCODE_SKIP_PARAMS(idx);
        auto value = OPCODE_READ_PARAM_INT();

        OPCODE_SKIP_PARAMS(valueCount - (idx + 1)); // seek to output param
        OPCODE_WRITE_PARAM_INT(value);

        CLEO_SkipUnusedVarArgs(thread);
        return OR_CONTINUE;
    }

    //2706=-1,pick_random_float values %d% store_to %d%
    static OpcodeResult __stdcall opcode_2706(CRunningScript* thread)
    {
        auto valueCount = CLEO_GetVarArgCount(thread);

        if (valueCount < 2) // value + result
        {
            SHOW_ERROR("Insufficient number of arguments in script %s\nScript suspended.", CLEO::ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }
        valueCount -= 1; // output param

        uniform_int_distribution<size_t> dist(0, valueCount - 1); // 0 to last index
        auto idx = dist(Instance.randomGenerator);

        // read n-th param
        OPCODE_SKIP_PARAMS(idx);
        auto value = OPCODE_READ_PARAM_FLOAT();

        OPCODE_SKIP_PARAMS(valueCount - (idx + 1)); // seek to output param
        OPCODE_WRITE_PARAM_FLOAT(value);

        CLEO_SkipUnusedVarArgs(thread);
        return OR_CONTINUE;
    }

    //2707=-1,pick_random_text values %d% store_to %d%
    static OpcodeResult __stdcall opcode_2707(CRunningScript* thread)
    {
        auto valueCount = CLEO_GetVarArgCount(thread);

        if (valueCount < 2) // value + result
        {
            SHOW_ERROR("Insufficient number of arguments in script %s\nScript suspended.", CLEO::ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }
        valueCount -= 1; // output param

        uniform_int_distribution<size_t> dist(0, valueCount - 1); // 0 to last index
        auto idx = dist(Instance.randomGenerator);

        // read n-th param
        OPCODE_SKIP_PARAMS(idx);
        OPCODE_READ_PARAM_STRING(value);

        OPCODE_SKIP_PARAMS(valueCount - (idx + 1)); // seek to output param
        OPCODE_WRITE_PARAM_STRING(value);

        CLEO_SkipUnusedVarArgs(thread);
        return OR_CONTINUE;
    }

    //2708=1,random_chance %1d%
    static OpcodeResult __stdcall opcode_2708(CRunningScript* thread)
    {
        auto chance = OPCODE_READ_PARAM_FLOAT();

        uniform_real_distribution<float> dist(0.0f, 100.0f); // 0.0 to 99.99(9)
        auto random = dist(Instance.randomGenerator);

        auto result = random < chance;

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }
} Instance;
