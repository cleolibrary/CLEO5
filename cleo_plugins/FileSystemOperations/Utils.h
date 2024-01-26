#pragma once
#include "CLEO.h"
#include <string>
#include <wtypes.h>

std::string stringPrintf(const char* format, ...)
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

std::string scriptInfoStr(CLEO::CRunningScript* thread)
{
    std::string info(1024, '\0');
    CLEO_GetScriptInfoStr(thread, true, info.data(), info.length());
    return std::move(info);
}

const char* TraceVArg(CLEO::eLogLevel level, const char* format, va_list args)
{
    static char szBuf[1024];
    vsprintf(szBuf, format, args); // put params into format
    CLEO_Log(level, szBuf);
    return szBuf;
}

void Trace(CLEO::eLogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    TraceVArg(level, format, args);
    va_end(args);
}

void Trace(const CLEO::CRunningScript* thread, CLEO::eLogLevel level, const char* format, ...)
{
    if (CLEO_GetScriptVersion(thread) < CLEO::eCLEO_Version::CLEO_VER_5)
    {
        return; // do not log this in older versions
    }

    va_list args;
    va_start(args, format);
    TraceVArg(level, format, args);
    va_end(args);
}

void ShowError(const char* format, ...)
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

#define TRACE(format,...) {Trace(CLEO::eLogLevel::Default, format, __VA_ARGS__);}
#define LOG_WARNING(script, format, ...) {Trace(script, CLEO::eLogLevel::Error, format, __VA_ARGS__);}
#define SHOW_ERROR(a,...) {ShowError(a, __VA_ARGS__);}

static const size_t MinValidAddress = 0x10000; // used for validation of pointers received from scripts. First 64kb are for sure reserved by Windows.
#define OPCODE_VALIDATE_POINTER(x) if((size_t)x <= MinValidAddress) { auto info = scriptInfoStr(thread); SHOW_ERROR("Invalid '0x%X' pointer param in script %s \nScript suspended.", x, info.c_str()); return thread->Suspend(); }
