#pragma once
#include <mutex>

std::string stringPrintf(const char* format, ...);

namespace CLEO
{
    class CRunningScript;
}

class CDebug
{
public:
    CDebug() : m_hFile(Filepath_Log)
    {
        Trace(CLEO::eLogLevel::Default, "Log started.");

#ifdef _DEBUG
        Trace(CLEO::eLogLevel::Default, "CLEO v%s DEBUG", CLEO_VERSION_STR);
#else
        Trace(CLEO::eLogLevel::Default, "CLEO v%s", CLEO_VERSION_STR);
#endif
    }

    ~CDebug()
    {
        Trace(CLEO::eLogLevel::Default, "Log finished.");
    }
    
    void Trace(CLEO::eLogLevel level, const char* format, ...);
    void Trace(const CLEO::CRunningScript* thread, CLEO::eLogLevel level, const char* format, ...);
    void Error(const char* format, ...);
    
private:
    std::mutex mutex;
    std::ofstream m_hFile;
    const char* TraceVArg(CLEO::eLogLevel level, const char* format, va_list args);
};

extern CDebug Debug;
