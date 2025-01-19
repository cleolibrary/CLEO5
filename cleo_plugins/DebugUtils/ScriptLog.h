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

    void ProcessCommand(CRunningScript* thread)
    {
        if (this->thread != thread) Begin(thread);

        commandCounter++;
    }

    size_t GetElapsedSeconds()
    {
        return (clock() - startTime) / CLOCKS_PER_SEC;
    }
};

