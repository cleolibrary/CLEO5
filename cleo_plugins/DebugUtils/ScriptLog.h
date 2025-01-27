#pragma once
#include "CLEO.h"
#include <ctime>

struct ScriptLog
{
    CRunningScript* thread = nullptr;
    clock_t startTime = 0;
    size_t commandCounter = 0;

    void Begin(CRunningScript* thread)
    {
        this->thread = thread;
        startTime = clock();
        commandCounter = 0;
    }

    void Clear()
    {
        thread = nullptr;
        startTime = 0;
        commandCounter = 0;
    }

    void ProcessCommand(CRunningScript* thread)
    {
        if (this->thread != thread) Begin(thread);

        commandCounter++;
    }

    inline size_t GetElapsedSeconds() const
    {
        return (clock() - startTime) / CLOCKS_PER_SEC;
    }
};

