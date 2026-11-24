#include "stdafx.h"
#include "CDebug.h"
#include "CleoBase.h"

CDebug Debug;
using namespace CLEO;

// Removes ALL occurences of the username following /users/ from the message
static std::string RedactUserName(const char* msg)
{
    if (msg == nullptr) return {};

    const std::string text = msg;

    static const char* UserSegment        = "users";
    static const size_t UserSegmentLength = 5;
    static const char* NameEndChars       = "\\/*\"<>:|?\r\n";

    std::string result;
    size_t copied     = 0;
    size_t searchFrom = 0;

    // case-insensitive search
    std::string lower = text;
    StringToLower(lower);

    // scan the line, replacing the name with [redacted]
    while (searchFrom < lower.length())
    {
        const size_t users = lower.find(UserSegment, searchFrom);
        if (users == std::string::npos) break;

        searchFrom = users + 1;

        const size_t separator = users + UserSegmentLength; // the character right after "users"

        if (users == 0 || (lower[users - 1] != '\\' && lower[users - 1] != '/') || separator >= lower.length() ||
            (lower[separator] != '\\' && lower[separator] != '/'))
        {
            // users must be followed by a separator
            continue;
        }

        const size_t nameBegin = separator + 1;

        // find where the username ends
        size_t nameEnd = lower.find_first_of(NameEndChars, nameBegin);
        if (nameEnd == std::string::npos) nameEnd = lower.length();
        if (nameEnd == nameBegin) continue; // empty name

        // append [redacted] and the preceeding text to the result
        result.append(text, copied, nameBegin - copied);
        result.append("[redacted]");

        copied     = nameEnd;
        searchFrom = nameEnd;
    }

    result.append(text, copied, std::string::npos);
    return result;
}

void CDebug::Trace(CLEO::eLogLevel level, const char* msg)
{
    // censor user name in paths like "C:\Users\xxx\Documents..."
    const std::string redactedMsg = RedactUserName(msg);

    msg = redactedMsg.c_str();

    // output to console
#ifdef _DEBUG
    OutputDebugString(msg);
    OutputDebugString("\n");
#endif

    // output to log file
    {
        std::lock_guard<std::mutex> guard(mutex);

        if (m_hFile.good())
        {
            // time stamp
            SYSTEMTIME t;
            GetLocalTime(&t);
            // void(__stdcall * GTA_GetLocalTime)(LPSYSTEMTIME lpSystemTime) = memory_pointer(0x0081E514); // use ingame
            // function instead as GetLocalTime seems to be considered suspicious by some AV software
            // GTA_GetLocalTime(&t);

            char timestampStr[32];
            sprintf_s(
                timestampStr, "%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute,
                t.wSecond, t.wMilliseconds
            );
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

        // release the scoped mutex
    }

    // running callbacks after the mutex is released to avoid the deadlock
    if (CleoInstance.IsStarted())
    {
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::Log))
        {
            typedef void WINAPI callback(eLogLevel, const char*);
            ((callback*)func)(level, msg);
        }
    }
}
