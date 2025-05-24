#pragma once
#include "CLEO.h"
#include "ScriptDrawsState.h"
#include <map>


class ScriptDrawing
{
public:
    void Initialize();

    void ScriptProcessingBegin(CLEO::CRunningScript* script);
    void ScriptProcessingEnd(CLEO::CRunningScript* script);
    void ScriptUnregister(CLEO::CRunningScript* script);

    void Draw(bool beforeFade); // draw buffered script draws to screen

    RwTexture* GetScriptTexture(CLEO::CRunningScript* script, DWORD slot);

private:
    bool m_initialized = false;

    ScriptDrawsState m_globalDrawingState;
    CLEO::CRunningScript* m_currCustomScript = nullptr; // currently processed script
    std::map<CLEO::CRunningScript*, ScriptDrawsState> m_scriptDrawingStates; // buffered script draws

    void(__cdecl* CHud__DrawScriptText_Orig)(bool beforeFade) = nullptr; // original not hooked function
};

