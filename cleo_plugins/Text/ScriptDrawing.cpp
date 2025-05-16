
#include "ScriptDrawing.h"
#include "CLEO_Utils.h"
#include <CTheScripts.h>


using namespace CLEO;

void ScriptDrawing::Initialize()
{
    if (m_initialized) return;

    CHud__DrawScriptText_Orig = (void(__cdecl*)(bool))CLEO_GetMemoryAddress("CHud::DrawScriptText original");

    m_initialized = true;
}

void ScriptDrawing::BeginGame()
{
    m_globalDrawingState.Apply(true); // initialize clear state
    CTheScripts::UseTextCommands = eUseTextCommandState::DISABLED;
}

void ScriptDrawing::BeginScriptProcessing(CLEO::CRunningScript* script)
{
    if (!script->IsCustom()) return;

    m_globalDrawingState.Store();
    m_scriptDrawingStates[script].Apply(true); // initialize clear state
}

void ScriptDrawing::EndScriptProcessing(CLEO::CRunningScript* script)
{
    if (!script->IsCustom()) return;

    m_scriptDrawingStates[script].Store();
    m_globalDrawingState.Apply();
}

void ScriptDrawing::Draw(bool beforeFade)
{
    if (!m_initialized) return;

    if (std::all_of(m_scriptDrawingStates.cbegin(), m_scriptDrawingStates.cend(), [](auto& p) { return p.second.IsEmpty(); }))
    {
        return; // no custom scripts with draws
    }

    m_globalDrawingState.Store();

    for(auto& [script, state] : m_scriptDrawingStates)
    {
        if (state.IsEmpty()) continue;

        state.Apply();
        CHud__DrawScriptText_Orig(beforeFade);
    }

    m_globalDrawingState.Apply();
}
