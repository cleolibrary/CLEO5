#pragma once
#include "CLEO.h"

struct ScriptLog
{
    size_t commandCounter = 0;

    void Reset()
    {
        commandCounter = 0;
    }
};

