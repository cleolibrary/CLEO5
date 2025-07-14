#pragma once
#include "CLEO.h"
#include "ScriptDrawsState.h"
#include <map>

using namespace CLEO;

class ScriptDrawing
{
public:
    void ScriptProcessingBegin(Script* script);
    void ScriptProcessingEnd(Script* script);
    void ScriptUnregister(Script* script);

    void Draw(bool beforeFade); // draw buffered script draws to screen

    RwTexture* GetScriptTexture(Script* script, DWORD slot);

private:
    ScriptDrawsState m_globalDrawingState;
    Script* m_currCustomScript = nullptr; // currently processed script
    std::map<Script*, ScriptDrawsState> m_scriptDrawingStates; // buffered script draws
};

