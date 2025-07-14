#pragma once
#include "CLEO.h"
#include <ctime>

struct ScriptLog
{
    Script* script = nullptr;
    clock_t startTime = 0;
    size_t commandCounter = 0;

    void Begin(Script* script)
    {
        this->script = script;
        startTime = clock();
        commandCounter = 0;
    }

    void Clear()
    {
        script = nullptr;
        startTime = 0;
        commandCounter = 0;
    }

    void ProcessCommand(Script* script)
    {
        if (this->script != script) Begin(script);

        commandCounter++;
    }

    inline size_t GetElapsedSeconds() const
    {
        return (clock() - startTime) / CLOCKS_PER_SEC;
    }
};

