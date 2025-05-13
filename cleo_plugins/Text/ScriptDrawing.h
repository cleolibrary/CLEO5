#pragma once
#include "CLEO.h"


class ScriptDrawing
{
public:
    void Initialize();
    void BeginScriptProcessing(CLEO::CRunningScript* script);
    void EndScriptProcessing(CLEO::CRunningScript* script);
    void Draw(bool beforeFade); // draw buffered script draws to screen

private:
    bool m_initialized = false;
};

