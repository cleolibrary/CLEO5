#include "stdafx.h"
#include "CDebug.h"
#include "CleoBase.h"
#include <shellapi.h>

CDebug Debug;
using namespace CLEO;

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

void CDebug::Trace(CLEO::eLogLevel level, const char* msg)
{
    std::lock_guard<std::mutex> guard(mutex);

    static char szBuf[1024];

    // time stamp
    SYSTEMTIME t;

    //GetLocalTime(&t);
    void (__stdcall * GTA_GetLocalTime)(LPSYSTEMTIME lpSystemTime) = memory_pointer(0x0081E514); // use ingame function instead as GetLocalTime seems to be considered suspicious by some AV software
    GTA_GetLocalTime(&t);

    sprintf(szBuf, "%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    char* stampEnd = szBuf + strlen(szBuf);

    strcpy(stampEnd, msg);

    // output to file
    if(m_hFile.good())
        m_hFile << szBuf << std::endl;

    // output to console
#ifdef _DEBUG
    OutputDebugString(szBuf);
    OutputDebugString("\n");
#endif

    // output to callbacks
    if (CleoInstance.IsStarted())
    {
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::Log))
        {
            typedef void WINAPI callback(eLogLevel, const char*);
            ((callback*)func)(level, stampEnd);
        }
    }
}

extern "C" 
{
    void WINAPI CLEO_Log(eLogLevel level, const char* msg)
    {
        Debug.Trace(level, msg);
    }
}
