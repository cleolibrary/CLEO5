// Extra helpers for CLEO plugins that work with game entity handles.
// Plugin-SDK is required to build.
//
// Add following lines to "Additional Include Directories" in project config:
//   $(PLUGIN_SDK_DIR)\plugin_sa\
//   $(PLUGIN_SDK_DIR)\shared\game\
//   $(PLUGIN_SDK_DIR)\plugin_sa\game_sa\
//
// Add following lines to "Preprocesor Definitions":
//   GTASA
//   TARGET_NAME=R"($(TargetName))"
//
// Requires adding "CPools.cpp" from GTA Plugin SDK to the project files.

#pragma once
#include "CLEO_Utils.h"
#include "CPools.h" // from GTA Plugin SDK
namespace CLEO
{
    static bool IsObjectHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pObjectPool->m_nSize) return false; // index out of bounds

        if (CPools::ms_pObjectPool->m_byteMap[index].IntValue() != flags) return false; // flags mismatch

        return true;
    }

    static bool IsPedHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pPedPool->m_nSize) return false; // index out of bounds

        if (CPools::ms_pPedPool->m_byteMap[index].IntValue() != flags) return false; // flags mismatch

        return true;
    }

    static bool IsVehicleHandleValid(DWORD handle)
    {
        // get handle info
        auto flags = handle & 0xFF;
        auto index = handle >> 8;

        if (index >= (DWORD)CPools::ms_pVehiclePool->m_nSize) return false; // index out of bounds

        if (CPools::ms_pVehiclePool->m_byteMap[index].IntValue() != flags) return false; // flags mismatch

        return true;
    }

#define OPCODE_READ_PARAM_OBJECT_HANDLE()                                                                              \
    _readParam(thread).dwParam;                                                                                        \
    if (!_paramWasInt())                                                                                               \
    {                                                                                                                  \
        SUSPEND_INPUT_TYPE("integer");                                                                                 \
    }                                                                                                                  \
    else if (!IsObjectHandleValid(_paramsArray[0].dwParam))                                                            \
    {                                                                                                                  \
        SUSPEND("Invalid object handle '0x%X' input argument %s", _paramsArray[0].dwParam, GetParamInfo().c_str());    \
    }

#define OPCODE_READ_PARAM_PED_HANDLE()                                                                                 \
    _readParam(thread).dwParam;                                                                                        \
    if (!_paramWasInt())                                                                                               \
    {                                                                                                                  \
        SUSPEND_INPUT_TYPE("integer");                                                                                 \
    }                                                                                                                  \
    else if (!IsPedHandleValid(_paramsArray[0].dwParam))                                                               \
    {                                                                                                                  \
        SUSPEND("Invalid character handle '0x%X' input argument %s", _paramsArray[0].dwParam, GetParamInfo().c_str()); \
    }

#define OPCODE_READ_PARAM_VEHICLE_HANDLE()                                                                             \
    _readParam(thread).dwParam;                                                                                        \
    if (!_paramWasInt())                                                                                               \
    {                                                                                                                  \
        SUSPEND_INPUT_TYPE("integer");                                                                                 \
    }                                                                                                                  \
    else if (!IsVehicleHandleValid(_paramsArray[0].dwParam))                                                           \
    {                                                                                                                  \
        SUSPEND("Invalid vehicle handle '0x%X' input argument %s", _paramsArray[0].dwParam, GetParamInfo().c_str());   \
    }

} // namespace CLEO
