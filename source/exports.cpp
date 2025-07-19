#include "stdafx.h"
#include "CleoBase.h"


namespace CLEO
{
extern "C"
{
    DWORD WINAPI CLEO_GetVersion()
    {
        return CLEO_VERSION;
    }

    LPCSTR WINAPI CLEO_GetVersionStr()
    {
        return CLEO_VERSION_STR;
    }

    eGameVersion WINAPI CLEO_GetGameVersion()
    {
        return DetermineGameVersion();
    }

    BOOL WINAPI CLEO_RegisterOpcode(WORD opcode, CustomOpcodeHandler callback)
    {
        return CCustomOpcodeSystem::RegisterOpcode(opcode, callback);
    }

    BOOL WINAPI CLEO_RegisterCommand(const char* commandName, CustomOpcodeHandler callback)
    {
        WORD opcode = CleoInstance.OpcodeInfoDb.GetOpcode(commandName);
        if (opcode == 0xFFFF)
        {
            LOG_WARNING(0, "Failed to register opcode [%s]! Command name not found in the database.", commandName);
            return false;
        }

        return CCustomOpcodeSystem::RegisterOpcode(opcode, callback);
    }

    void WINAPI CLEO_RegisterCallback(eCallbackId id, void* func)
    {
        CleoInstance.AddCallback(id, func);
    }

    void WINAPI CLEO_UnregisterCallback(eCallbackId id, void* func)
    {
        CleoInstance.RemoveCallback(id, func);
    }

    void WINAPI CLEO_AddScriptDeleteDelegate(FuncScriptDeleteDelegateT func)
    {
        CleoInstance.OpcodeSystem.scriptDeleteDelegate += func;
    }

    void WINAPI CLEO_RemoveScriptDeleteDelegate(FuncScriptDeleteDelegateT func)
    {
        CleoInstance.OpcodeSystem.scriptDeleteDelegate -= func;
    }

    BOOL WINAPI CLEO_IsScriptRunning(const Script* script)
    {
        return CleoInstance.ScriptEngine.IsActiveScriptPtr(script);
    }

    void WINAPI CLEO_GetScriptInfoStr(const Script* script, bool currLineInfo, char* buf, DWORD bufSize)
    {
        if (script == nullptr || buf == nullptr || bufSize < 2)
        {
            return; // invalid param
        }

        auto text = reinterpret_cast<const CCustomScript*>(script)->GetInfoStr(currLineInfo);

        if (text.length() >= bufSize)
            text.resize(bufSize - 1); // and terminator character

        std::memcpy(buf, text.c_str(), text.length() + 1); // with terminator
    }

    void WINAPI CLEO_GetScriptParamInfoStr(int idexOffset, char* buf, DWORD bufSize)
    {
        auto curr = idexOffset - 1 + CScriptEngine::handledParamCount;
        auto name = CleoInstance.OpcodeInfoDb.GetArgumentName(CScriptEngine::lastOpcode, curr);

        curr++; // 1-based argument index display

        std::string msg;
        if (name != nullptr) msg = StringPrintf("#%d \"%s\"", curr, name);
        else msg = StringPrintf("#%d", curr);

        strncpy_s(buf, bufSize, msg.c_str(), bufSize);
    }

    DWORD WINAPI CLEO_GetScriptBaseRelativeOffset(const Script* script, const BYTE* codePos)
    {
        auto base = (BYTE*)script->BaseIP;
        if (base == 0) base = CleoInstance.ScriptEngine.scmBlock;

        if (script->IsMission() && !script->IsCustom())
        {
            if (codePos >= CleoInstance.ScriptEngine.missionBlock)
            {
                // we are in mission code buffer
                // native missions are loaded from script file into mission block area
                codePos += CTheScripts::MultiScriptArray[CleoInstance.ScriptEngine.missionIndex]; // start offset of this mission within source script file
            }
            else
            {
                base = CleoInstance.ScriptEngine.scmBlock; // seems that mission uses main scm code
            }
        }

        return codePos - base;
    }

    eCLEO_Version WINAPI CLEO_GetScriptVersion(const Script* script)
    {
        if (script->IsCustom())
            return reinterpret_cast<const CCustomScript*>(script)->GetCompatibility();
        else
            return CleoInstance.ScriptEngine.NativeScriptsVersion;
    }

    void WINAPI CLEO_SetScriptVersion(Script* script, eCLEO_Version version)
    {
        if (script->IsCustom())
            ((CCustomScript*)script)->SetCompatibility(version);
        else
            CleoInstance.ScriptEngine.NativeScriptsVersion = version;
    }

    LPCSTR WINAPI CLEO_GetScriptFilename(const Script* script)
    {
        if (!CleoInstance.ScriptEngine.IsValidScriptPtr(script))
        {
            return nullptr;
        }

        auto cs = (CCustomScript*)script;
        return cs->GetScriptFileName();
    }

    LPCSTR WINAPI CLEO_GetScriptWorkDir(const Script* script)
    {
        auto cs = (CCustomScript*)script;
        return cs->GetWorkDir();
    }

    void WINAPI CLEO_SetScriptWorkDir(Script* script, const char* path)
    {
        auto cs = (CCustomScript*)script;
        cs->SetWorkDir(path);
    }

    void WINAPI CLEO_SetThreadCondResult(Script* script, BOOL result)
    {
        ((CRunningScript*)script)->UpdateCompareFlag(result != FALSE);
    }

    void WINAPI CLEO_ThreadJumpAtLabelPtr(Script* script, int offset)
    {
        CleoInstance.ScriptEngine.ThreadJump(script, offset);
    }

    void WINAPI CLEO_TerminateScript(Script* script)
    {
        CleoInstance.ScriptEngine.RemoveScript(script);
    }

    int WINAPI CLEO_GetOperandType(const Script* script)
    {
        return (int)script->PeekDataType();
    }

    DWORD WINAPI CLEO_GetVarArgCount(Script* script)
    {
        return CScriptEngine::GetVarArgCount(script);
    }

    SCRIPT_VAR* opcodeParams;
    SCRIPT_VAR* missionLocals;
    Script* staticThreads;

    BYTE* WINAPI CLEO_GetScmMainData()
    {
        return CleoInstance.ScriptEngine.scmBlock;
    }

    SCRIPT_VAR* WINAPI CLEO_GetOpcodeParamsArray()
    {
        return CLEO::opcodeParams;
    }

    BYTE WINAPI CLEO_GetParamsHandledCount()
    {
        return CScriptEngine::handledParamCount;
    }

    SCRIPT_VAR* WINAPI CLEO_GetPointerToScriptVariable(Script* script)
    {
        return CScriptEngine::GetScriptParamPointer(script);
    }

    void WINAPI CLEO_RetrieveOpcodeParams(Script* script, int count)
    {
        CScriptEngine::ReadScriptParams(script, count);
    }

    DWORD WINAPI CLEO_GetIntOpcodeParam(Script* script)
    {
        CScriptEngine::ReadScriptParams(script, 1);
        return opcodeParams[0].dwParam;
    }

    float WINAPI CLEO_GetFloatOpcodeParam(Script* script)
    {
        CScriptEngine::ReadScriptParams(script, 1);
        return opcodeParams[0].fParam;
    }

    LPCSTR WINAPI CLEO_ReadStringOpcodeParam(Script* script, char* buff, int buffSize)
    {
        static char internal_buff[MAX_STR_LEN + 1]; // and terminator
        if (!buff)
        {
            buff = internal_buff;
            buffSize = (buffSize > 0) ? std::min<int>(buffSize, sizeof(internal_buff)) : sizeof(internal_buff); // allow user's length limit
        }

        auto result = CScriptEngine::ReadStringParam(script, buff, buffSize);
        return (result != nullptr) ? buff : nullptr;
    }

    LPCSTR WINAPI CLEO_ReadStringPointerOpcodeParam(Script* script, char* buff, int buffSize)
    {
        static char internal_buff[MAX_STR_LEN + 1]; // and terminator
        bool userBuffer = buff != nullptr;
        if (!userBuffer)
        {
            buff = internal_buff;
            buffSize = (buffSize > 0) ? std::min<int>(buffSize, sizeof(internal_buff)) : sizeof(internal_buff); // allow user's length limit
        }

        return CScriptEngine::ReadStringParam(script, buff, buffSize);
    }

    void WINAPI CLEO_ReadStringParamWriteBuffer(Script* script, char** outBuf, int* outBufSize, BOOL* outNeedsTerminator)
    {
        if (script == nullptr ||
            outBuf == nullptr ||
            outBufSize == nullptr ||
            outNeedsTerminator == nullptr)
        {
            LOG_WARNING(script, "Invalid argument of CLEO_ReadStringParamWriteBuffer in script %s", ((CCustomScript*)script)->GetInfoStr().c_str());
            return;
        }

        auto target = CScriptEngine::ReadStringParamWriteBuffer(script);
        *outBuf = target.data;
        *outBufSize = target.size;
        *outNeedsTerminator = target.needTerminator;
    }

    char* WINAPI CLEO_ReadParamsFormatted(Script* script, const char* format, char* buf, int bufSize)
    {
        static char internal_buf[MAX_STR_LEN * 4];
        if (!buf) { buf = internal_buf; bufSize = sizeof(internal_buf); }
        if (!bufSize) bufSize = MAX_STR_LEN;

        if (CScriptEngine::ReadFormattedString(script, buf, bufSize, format) == -1) // error?
        {
            return nullptr; // error
        }

        return buf;
    }

    DWORD WINAPI CLEO_PeekIntOpcodeParam(Script* script)
    {
        // store state
        auto param = CLEO::opcodeParams[0];
        auto ip = script->CurrentIP;
        auto count = CScriptEngine::handledParamCount;

        CScriptEngine::ReadScriptParams(script, 1);
        DWORD result = CLEO::opcodeParams[0].dwParam;

        // restore state
        script->CurrentIP = ip;
        CScriptEngine::handledParamCount = count;
        CLEO::opcodeParams[0] = param;

        return result;
    }

    float WINAPI CLEO_PeekFloatOpcodeParam(Script* script)
    {
        // store state
        auto param = CLEO::opcodeParams[0];
        auto ip = script->CurrentIP;
        auto count = CScriptEngine::handledParamCount;

        CScriptEngine::ReadScriptParams(script, 1);
        float result = CLEO::opcodeParams[0].fParam;

        // restore state
        script->CurrentIP = ip;
        CScriptEngine::handledParamCount = count;
        CLEO::opcodeParams[0] = param;

        return result;
    }

    SCRIPT_VAR* WINAPI CLEO_PeekPointerToScriptVariable(Script* script)
    {
        // store state
        auto ip = script->CurrentIP;
        auto count = CScriptEngine::handledParamCount;

        auto result = CScriptEngine::GetScriptParamPointer(script);

        // restore state
        script->CurrentIP = ip;
        CScriptEngine::handledParamCount = count;

        return result;
    }

    void WINAPI CLEO_SkipOpcodeParams(Script* script, int count)
    {
        if (count < 1) return;

        for (int i = 0; i < count; i++)
        {
            switch (script->ReadDataType())
            {
            case DT_VAR:
            case DT_LVAR:
            case DT_VAR_STRING:
            case DT_LVAR_STRING:
            case DT_VAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL:
                script->IncPtr(2);
                break;
            case DT_VAR_ARRAY:
            case DT_LVAR_ARRAY:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_LVAR_TEXTLABEL_ARRAY:
            case DT_VAR_STRING_ARRAY:
            case DT_LVAR_STRING_ARRAY:
                script->IncPtr(6);
                break;
            case DT_BYTE:
                //case DT_END: // should be only skipped with var args dediacated functions
                script->IncPtr();
                break;
            case DT_WORD:
                script->IncPtr(2);
                break;
            case DT_DWORD:
            case DT_FLOAT:
                script->IncPtr(4);
                break;
            case DT_VARLEN_STRING:
                script->IncPtr((int)1 + *script->GetBytePointer()); // as unsigned! length byte + string data
                break;

            case DT_TEXTLABEL:
                script->IncPtr(8);
                break;
            case DT_STRING:
                script->IncPtr(16);
                break;
            }
        }

        CScriptEngine::handledParamCount += count;
    }

    void WINAPI CLEO_SkipUnusedVarArgs(Script* script)
    {
        CScriptEngine::SkipUnusedVarArgs(script);
    }

    void WINAPI CLEO_RecordOpcodeParams(Script* script, int count)
    {
        CScriptEngine::WriteScriptParams(script, count);
    }

    void WINAPI CLEO_SetIntOpcodeParam(Script* script, DWORD value)
    {
        CLEO::opcodeParams[0].dwParam = value;
        CScriptEngine::WriteScriptParams(script, 1);
    }

    void WINAPI CLEO_SetFloatOpcodeParam(Script* script, float value)
    {
        CLEO::opcodeParams[0].fParam = value;
        CScriptEngine::WriteScriptParams(script, 1);
    }

    void WINAPI CLEO_WriteStringOpcodeParam(Script* script, const char* str)
    {
        if (!CScriptEngine::WriteStringParam(script, str))
            LOG_WARNING(script, "%s in script %s", CScriptEngine::lastErrorMsg.c_str(), ((CCustomScript*)script)->GetInfoStr().c_str());
    }

    BOOL WINAPI CLEO_GetScriptDebugMode(const Script* script)
    {
        return reinterpret_cast<const CCustomScript*>(script)->GetDebugMode();
    }

    void WINAPI CLEO_SetScriptDebugMode(Script* script, BOOL enabled)
    {
        reinterpret_cast<CCustomScript*>(script)->SetDebugMode(enabled);
    }

    Script* WINAPI CLEO_CreateCustomScript(Script* fromScript, const char* filePath, int label)
    {
        return (Script*)CleoInstance.ScriptEngine.CreateCustomScript(fromScript, filePath, label);
    }

    Script* WINAPI CLEO_GetLastCreatedCustomScript()
    {
        return CleoInstance.ScriptEngine.LastScriptCreated;
    }

    Script* WINAPI CLEO_GetScriptByName(const char* scriptName, BOOL standardScripts, BOOL customScripts, DWORD resultIndex)
    {
        return CleoInstance.ScriptEngine.FindScriptNamed(scriptName, standardScripts, customScripts, resultIndex);
    }

    Script* WINAPI CLEO_GetScriptByFilename(const char* path, DWORD resultIndex)
    {
        return CleoInstance.ScriptEngine.FindScriptByFilename(path, resultIndex);
    }

    DWORD WINAPI CLEO_GetScriptTextureById(Script* script, int id)
    {
        HMODULE textPlugin = GetModuleHandleA("SA.Text.cleo");
        if (textPlugin == nullptr)
        {
            return (DWORD)nullptr;
        }

        auto GetScriptTexture = (RwTexture* (__cdecl*)(Script*, DWORD)) GetProcAddress(textPlugin, "GetScriptTexture");
        if (GetScriptTexture == nullptr)
        {
            return (DWORD)nullptr;
        }

        return (DWORD)GetScriptTexture(script, id);
    }

    DWORD WINAPI CLEO_GetInternalAudioStream(Script* unused, DWORD audioStreamPtr)
    {
        return *(DWORD*)(audioStreamPtr + 0x4); // CAudioStream->streamInternal
    }

    // void WINAPI CLEO_StringListFree(StringList list)
    // void WINAPI CLEO_ResolvePath(Script* script, char* inOutPath, DWORD pathMaxLen)
    // StringList WINAPI CLEO_ListDirectory(Script* script, const char* searchPath, BOOL listDirs, BOOL listFiles)
    // defined in CleoBase.h

    LPCSTR WINAPI CLEO_GetGameDirectory()
    {
        return Filepath_Game.c_str();
    }

    LPCSTR WINAPI CLEO_GetUserDirectory()
    {
        return Filepath_User.c_str();
    }

    void WINAPI CLEO_Log(eLogLevel level, const char* msg)
    {
        Debug.Trace(level, msg);
    }
}
}