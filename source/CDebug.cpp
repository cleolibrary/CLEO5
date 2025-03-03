#include "stdafx.h"
#include "CDebug.h"
#include "CleoBase.h"
#include "CTimer.h"
#include <shellapi.h>

CDebug Debug;
using namespace CLEO;

void CDebug::Trace(CLEO::eLogLevel level, const char* msg)
{
    std::lock_guard<std::mutex> guard(mutex);

    // output to console
#ifdef _DEBUG
    OutputDebugString(msg);
    OutputDebugString("\n");
#endif

    // censor user name in paths like "C:\Users\xxx\Documents..."
    const char* UsersDir = "\\users\\";
    std::string msgStr = msg;
    StringToLower(msgStr);
    auto usersDirPos = strstr(msgStr.c_str(), UsersDir);
    if (usersDirPos != nullptr)
    {
        size_t userBegin = (size_t)(usersDirPos - msgStr.c_str()) + strlen(UsersDir);
        
        size_t userEnd = msgStr.find_first_of("\\/*\"<>:|?", userBegin); // till next path separator or any invalid filepath character
        if (userEnd == std::string::npos)
        {
            userEnd = msgStr.length();
        }

        msgStr = msg; // restore original case
        msgStr.replace(userBegin, userEnd - userBegin, "[redacted]");
        msg = msgStr.c_str();
    }

    // output to callbacks
    if (CleoInstance.IsStarted())
    {
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::Log))
        {
            typedef void WINAPI callback(eLogLevel, const char*);
            ((callback*)func)(level, msg);
        }
    }

    // output to log file
    if (m_hFile.good())
    {
        // time stamp
        SYSTEMTIME t;
        GetLocalTime(&t);
        //void(__stdcall * GTA_GetLocalTime)(LPSYSTEMTIME lpSystemTime) = memory_pointer(0x0081E514); // use ingame function instead as GetLocalTime seems to be considered suspicious by some AV software
        //GTA_GetLocalTime(&t);

        char timestampStr[32];
        sprintf(timestampStr, "%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
        m_hFile << timestampStr;

        // add separator line if frame rendered since last log entry
        if (lastFrame != CTimer::m_FrameCounter)
        {
            m_hFile << std::endl << timestampStr;
            lastFrame = CTimer::m_FrameCounter;
        }

        // message
        auto message = std::string(msg);
        StringRemoveFormatting(message);
        m_hFile << message.c_str();

        m_hFile << std::endl;
    }
}

extern "C" 
{
    void WINAPI CLEO_Log(eLogLevel level, const char* msg)
    {
        Debug.Trace(level, msg);
    }
}
