#pragma once
#include <mutex>

class CDebug
{
public:
    CDebug() : m_hFile(Filepath_Log)
    {
        Trace(CLEO::eLogLevel::Default, "Log started.");

#ifdef _DEBUG
        CLEO::Trace(CLEO::eLogLevel::Default, "CLEO v%s DEBUG", CLEO_VERSION_STR);
#else
        CLEO::Trace(CLEO::eLogLevel::Default, "CLEO v%s", CLEO_VERSION_STR);
#endif
    }

    ~CDebug()
    {
        CLEO::Trace(CLEO::eLogLevel::Default, ""); // separator
        CLEO::Trace(CLEO::eLogLevel::Default, "Log finished.");
    }
    
    void Trace(CLEO::eLogLevel level, const char* msg);
    
private:
    unsigned int lastFrame = -1;
    std::mutex mutex;
    std::ofstream m_hFile;
 };

extern CDebug Debug;
