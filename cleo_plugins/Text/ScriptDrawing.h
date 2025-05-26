#pragma once
#include "CLEO.h"
#include "ScriptDrawsState.h"
#include <map>


class ScriptDrawing
{
public:
    void ScriptProcessingBegin(CLEO::CRunningScript* script);
    void ScriptProcessingEnd(CLEO::CRunningScript* script);
    void ScriptUnregister(CLEO::CRunningScript* script);

    void Draw(bool beforeFade); // draw buffered script draws to screen

    RwTexture* GetScriptTexture(CLEO::CRunningScript* script, DWORD slot);

private:
    ScriptDrawsState m_globalDrawingState;
    CLEO::CRunningScript* m_currCustomScript = nullptr; // currently processed script
    std::map<CLEO::CRunningScript*, ScriptDrawsState> m_scriptDrawingStates; // buffered script draws
};

