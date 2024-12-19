#pragma once
#include "CLEO.h"

struct ScriptLog
{
    CRunningScript* thread = nullptr;
    size_t commandCounter = 0;

    void Begin(CRunningScript* thread)
    {
        this->thread = thread;
        commandCounter = 0;
    }

    void ProcessCommand(CRunningScript* thread)
    {
        if (this->thread != thread) Begin(thread);

        commandCounter++;
    }
};

