// some utilities usefull when creating CLEO plugins
// requires adding "CPools.cpp" from GTA Plugin SDK to the project

#pragma once
#include "CLEO.h"
#include "CFileMgr.h" // from GTA Plugin SDK
#include "CPools.h" // from GTA Plugin SDK
#include "shellapi.h" // game window minimize/maximize support
#include <string>
#include <wtypes.h>

namespace CLEO
{
    // this plugin's config file
    static std::string GetConfigFilename()
    {
        std::string configFile = CFileMgr::ms_rootDirName;
        if (!configFile.empty() && configFile.back() != '\\') configFile.push_back('\\');

        configFile += "cleo\\cleo_plugins\\" TARGET_NAME ".ini";

        return configFile;
    }

    static std::string StringPrintf(const char* format, ...)
    {
        va_list args;

        va_start(args, format);
        auto len = std::vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);

        std::string result(len, '\0');

        va_start(args, format);
        std::vsnprintf(result.data(), result.length(), format, args);
        va_end(args);

        return result;
    }

    static std::string ScriptInfoStr(CLEO::CRunningScript* thread)
    {
        std::string info(1024, '\0');
        CLEO_GetScriptInfoStr(thread, true, info.data(), info.length());
        return std::move(info);
    }

    static const char* TraceVArg(CLEO::eLogLevel level, const char* format, va_list args)
    {
        static char szBuf[1024];
        vsprintf(szBuf, format, args); // put params into format
        CLEO_Log(level, szBuf);
        return szBuf;
    }

    static void Trace(CLEO::eLogLevel level, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        TraceVArg(level, format, args);
        va_end(args);
    }

    static void Trace(const CLEO::CRunningScript* thread, CLEO::eLogLevel level, const char* format, ...)
    {
        if (thread != nullptr && CLEO_GetScriptVersion(thread) < CLEO::eCLEO_Version::CLEO_VER_5)
        {
            return; // do not log this in older versions
        }

        va_list args;
        va_start(args, format);
        TraceVArg(level, format, args);
        va_end(args);
    }

    static void ShowError(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        auto msg = TraceVArg(CLEO::eLogLevel::Error, format, args);
        va_end(args);

        QUERY_USER_NOTIFICATION_STATE pquns;
        SHQueryUserNotificationState(&pquns);
        bool fullscreen = (pquns == QUNS_BUSY) || (pquns == QUNS_RUNNING_D3D_FULL_SCREEN) || (pquns == QUNS_PRESENTATION_MODE);

        if (fullscreen)
        {
            PostMessage(NULL, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            ShowWindow(NULL, SW_MINIMIZE);
        }

        MessageBox(NULL, msg, "CLEO error", MB_SYSTEMMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);

        if (fullscreen)
        {
            PostMessage(NULL, WM_SYSCOMMAND, SC_RESTORE, 0);
            ShowWindow(NULL, SW_RESTORE);
        }
    }

    #define TRACE(format,...) {CLEO::Trace(CLEO::eLogLevel::Default, format, __VA_ARGS__);}
    #define LOG_WARNING(script, format, ...) {CLEO::Trace(script, CLEO::eLogLevel::Error, format, __VA_ARGS__);}
    #define SHOW_ERROR(a,...) {CLEO::ShowError(a, __VA_ARGS__);}

    const size_t MinValidAddress = 0x10000; // used for validation of pointers received from scripts. First 64kb are for sure reserved by Windows.
    #define OPCODE_VALIDATE_POINTER(x) if((size_t)x <= MinValidAddress) { SHOW_ERROR("Invalid '0x%X' pointer argument in script %s \nScript suspended.", x, ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_CONDITION_RESULT(value) CLEO_SetThreadCondResult(thread, value);

    // opcode param handling utils internal
    static SCRIPT_VAR* _paramsArray = nullptr;
    static eDataType _lastParamType = eDataType::DT_END;
    static eArrayDataType _lastParamArrayType = eArrayDataType::ADT_NONE;

    static SCRIPT_VAR& _getParamSimple(CRunningScript* thread)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        CLEO_RetrieveOpcodeParams(thread, 1);
        if (_paramsArray == nullptr) _paramsArray = CLEO_GetParamsCollectiveArray();
        return _paramsArray[0];
    }

    static SCRIPT_VAR* _getParamVariable(CRunningScript* thread)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        return CLEO_GetPointerToScriptVariable(thread);
    }

    static void _setParamSimplePtr(CRunningScript* thread, void* valuePtr)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        if (_paramsArray == nullptr) _paramsArray = CLEO_GetParamsCollectiveArray();
        _paramsArray[0].pParam = valuePtr;
        CLEO_RecordOpcodeParams(thread, 1);
    }

    template<typename T> static void _setParamSimple(CRunningScript* thread, T value)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        if (_paramsArray == nullptr) _paramsArray = CLEO_GetParamsCollectiveArray();
        _paramsArray[0].dwParam = 0;
        memcpy(&_paramsArray[0], &value, sizeof(T));
        CLEO_RecordOpcodeParams(thread, 1);
    }

    static inline bool _paramWasInt(bool output = false)
    {
        if (_lastParamArrayType != eArrayDataType::ADT_NONE) return _lastParamArrayType == eArrayDataType::ADT_INT;
        if (IsVariable(_lastParamType)) return true;
        if (!output && IsImmInteger(_lastParamType)) return true;
        return false;
    }

    static inline bool _paramWasFloat(bool output = false)
    {
        if (_lastParamArrayType != eArrayDataType::ADT_NONE) return _lastParamArrayType == eArrayDataType::ADT_FLOAT;
        if (IsVariable(_lastParamType)) return true;
        if (!output && IsImmFloat(_lastParamType)) return true;
        return false;
    }

    static inline bool _paramWasString(bool output = false)
    {
        if (_lastParamArrayType != eArrayDataType::ADT_NONE)
        {
            return _lastParamArrayType == eArrayDataType::ADT_STRING ||
                _lastParamArrayType == eArrayDataType::ADT_TEXTLABEL ||
                _lastParamArrayType == eArrayDataType::ADT_INT; // pointer to output buffer
        }

        if (IsVarString(_lastParamType)) return true;
        if (!output && IsImmString(_lastParamType)) return true;

        // pointer to output buffer
        if (IsVariable(_lastParamType)) return true; 

        return false;
    }

    static inline bool _paramWasVariable()
    {
        return IsVariable(_lastParamType);
    }

    static char* _getParamText(CRunningScript* thread, char* buffer = nullptr, DWORD bufferSize = 0)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        if (!_paramWasString())
        {
            SHOW_ERROR("Input argument #%d expected to be string, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), ToKindStr(_lastParamType, _lastParamArrayType), ScriptInfoStr(thread).c_str());
            thread->Suspend();
            _lastParamType = DT_INVALID; // mark error
            return nullptr;
        }

        auto str = CLEO_ReadStringOpcodeParam(thread, buffer, bufferSize);
        if (str == nullptr) // other error?
        {
            SHOW_ERROR("Invalid input argument #%d in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str());
            thread->Suspend();
            _lastParamType = DT_INVALID; // mark error
            return nullptr;
        }

        return str;
    }

    static char* _getParamFilepath(CRunningScript* thread)
    {
        auto str = _getParamText(thread);
        if (str == nullptr) return nullptr;

        CLEO_ResolvePath(thread, str, MAX_STR_LEN); // uses generic readStringParam's buffer
        return str;
    }

    static bool _setParamText(CRunningScript* thread, const char* str)
    {
        _lastParamType = thread->PeekDataType();
        _lastParamArrayType = IsArray(_lastParamType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;

        if (str != nullptr && (size_t)str <= MinValidAddress)
        {
            SHOW_ERROR("Invalid '0x%X' source pointer of output string argument #%d in script %s \nScript suspended.", str, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str());
            thread->Suspend();
            return false;
        }

        if (!_paramWasString(true))
        {
            SHOW_ERROR("Output argument #%d expected to be variable string, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), ToKindStr(_lastParamType, _lastParamArrayType), ScriptInfoStr(thread).c_str());
            thread->Suspend();
            return false;
        }

        if (IsVariable(_lastParamType)) // pointer to buffer
        {
            auto ptr = CLEO_PeekIntOpcodeParam(thread);

            if ((size_t)ptr <= MinValidAddress)
            {
                SHOW_ERROR("Invalid '0x%X' pointer of output string argument #%d in script %s \nScript suspended.", ptr, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str());
                thread->Suspend();
                return false;
            }
        }
        
        char* buff = nullptr;
        int size = 0;
        DWORD needTerminator = false;
        CLEO_ReadStringParamWriteBuffer(thread, &buff, &size, &needTerminator);

        if (buff == nullptr) // all error types already handled, but check just in case
        {
            SHOW_ERROR("Invalid output argument #%d in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str());
            thread->Suspend();
            return false;
        }

        if (size == 0)
        {
            return true; // done
        }

        bool addTerminator = needTerminator;
        size_t buffLen = size - addTerminator;
        size_t length = str == nullptr ? 0 : strlen(str);

        if (buffLen > length) addTerminator = true; // there is space left for terminator

        length = min(length, buffLen);
        if (length > 0) std::memcpy(buff, str, length);
        if (addTerminator) buff[length] = '\0';
        return true; // done
    }

    static bool _isObjectHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pObjectPool->m_nSize)
            return false; // index out of bounds

        if (CPools::ms_pObjectPool->m_byteMap[index].IntValue() != flags)
            return false; // flags mismatch

        return true;
    }

    static bool _isPedHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pPedPool->m_nSize)
            return false; // index out of bounds

        if (CPools::ms_pPedPool->m_byteMap[index].IntValue() != flags)
            return false; // flags mismatch

        return true;
    }

    static bool _isVehicleHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pVehiclePool->m_nSize)
            return false; // index out of bounds

        if (CPools::ms_pVehiclePool->m_byteMap[index].IntValue() != flags)
            return false; // flags mismatch

        return true;
    }

    #define OPCODE_SKIP_PARAMS(_count) CLEO_SkipOpcodeParams(thread, _count)

    // macros for reading opcode input params. Performs type validation, throws error and suspends script if user provided invalid argument type
    // TOD: add range checks for limited size types?

    #define OPCODE_READ_PARAM_BOOL() _getParamSimple(thread).bParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_INT8() _getParamSimple(thread).cParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_UINT8() _getParamSimple(thread).ucParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_INT16() _getParamSimple(thread).wParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_UINT16() _getParamSimple(thread).usParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_INT() _getParamSimple(thread).nParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_UINT() _getParamSimple(thread).dwParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_FLOAT() _getParamSimple(thread).fParam; \
        if (!_paramWasFloat()) { SHOW_ERROR("Input argument #%d expected to be float, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_STRING() _getParamText(thread); if(!_paramWasString()) { return OpcodeResult::OR_INTERRUPT; }

    #define OPCODE_READ_PARAM_STRING_BUFF(_buffer, _bufferSize) _getParamText(thread, _buffer, _bufferSize); if(!_paramWasString()) { return OpcodeResult::OR_INTERRUPT; }

    #define OPCODE_READ_PARAM_FILEPATH() _getParamFilepath(thread); if(!_paramWasString()) { return OpcodeResult::OR_INTERRUPT; }

    #define OPCODE_READ_PARAM_PTR() _getParamSimple(thread).pParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        else if (_paramsArray[0].dwParam <= MinValidAddress) { SHOW_ERROR("Invalid pointer '0x%X' input argument #%d in script %s \nScript suspended.", _paramsArray[0].dwParam, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_OBJECT_HANDLE() _getParamSimple(thread).dwParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        else if (_isObjectHandleValid(_paramsArray[0].dwParam)) { SHOW_ERROR("Invalid object handle '0x%X' input argument #%d in script %s \nScript suspended.", _paramsArray[0].dwParam, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_PED_HANDLE() _getParamSimple(thread).dwParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        else if (_isPedHandleValid(_paramsArray[0].dwParam)) { SHOW_ERROR("Invalid character handle '0x%X' input argument #%d in script %s \nScript suspended.", _paramsArray[0].dwParam, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_VEHICLE_HANDLE() _getParamSimple(thread).dwParam; \
        if (!_paramWasInt()) { SHOW_ERROR("Input argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        else if (_isVehicleHandleValid(_paramsArray[0].dwParam)) { SHOW_ERROR("Invalid vehicle handle '0x%X' input argument #%d in script %s \nScript suspended.", _paramsArray[0].dwParam, CLEO_GetParamsHandledCount(), ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_OUTPUT_VAR() _getParamVariable(thread); \
        if (!_paramWasVariable()) { SHOW_ERROR("Output argument #%d expected to be variable, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_OUTPUT_VAR_INT() _getParamVariable(thread); \
        if (!_paramWasVariable()) { SHOW_ERROR("Output argument #%d expected to be variable, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_READ_PARAM_OUTPUT_VAR_FLOAT() _getParamVariable(thread); \
        if (!_paramWasVariable()) { SHOW_ERROR("Output argument #%d expected to be variable, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); } \
        if (!_paramWasFloat(true)) { SHOW_ERROR("Output argument #%d expected to be float, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    // macros for writing opcode output params. Performs type validation, throws error and suspends script if user provided invalid argument type

    #define OPCODE_WRITE_PARAM_BOOL(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_INT8(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_UINT8(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_INT16(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_UINT16(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_INT(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_UINT(value) _setParamSimple(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_FLOAT(value) _setParamSimple(thread, value); \
        if (!_paramWasFloat(true)) { SHOW_ERROR("Output argument #%d expected to be float, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }

    #define OPCODE_WRITE_PARAM_STRING(value) if(!_setParamText(thread, value)) { return OpcodeResult::OR_INTERRUPT; }

    #define OPCODE_WRITE_PARAM_PTR(value) _setParamSimplePtr(thread, value); \
        if (!_paramWasInt(true)) { SHOW_ERROR("Output argument #%d expected to be integer, got %s in script %s\nScript suspended.", CLEO_GetParamsHandledCount(), CLEO::ToKindStr(_lastParamType, _lastParamArrayType), CLEO::ScriptInfoStr(thread).c_str()); return thread->Suspend(); }
}
