
#include "ScriptDrawing.h"
#include "CLEO_Utils.h"
#include <CHud.h>
#include <CTheScripts.h>

using namespace CLEO;


void ScriptDrawing::ScriptProcessingBegin(CLEO::CRunningScript* script)
{
    if (!script->IsCustom()) return;

    m_globalDrawingState.Store();
    m_scriptDrawingStates[script].Apply(true); // initialize for new frame

    m_currCustomScript = script;
}

void ScriptDrawing::ScriptProcessingEnd(CLEO::CRunningScript* script)
{
    if (!script->IsCustom()) return;

    if (script->IsActive()) m_scriptDrawingStates[script].Store();
    m_globalDrawingState.Apply();

    m_currCustomScript = nullptr;
}

void ScriptDrawing::ScriptUnregister(CLEO::CRunningScript* script)
{
    if (!script->IsCustom()) return;

    m_scriptDrawingStates.erase(script);
}

void ScriptDrawing::Draw(bool beforeFade)
{
    if (std::all_of(m_scriptDrawingStates.cbegin(), m_scriptDrawingStates.cend(), [](auto& p) { return p.second.IsEmpty(); }))
    {
        return; // no custom scripts with draws
    }

    m_globalDrawingState.Store();

    for(auto& [script, state] : m_scriptDrawingStates)
    {
        if (state.IsEmpty()) continue;

        state.Apply();
        CHud::DrawScriptText(beforeFade);
    }

    m_globalDrawingState.Apply();
}

RwTexture* ScriptDrawing::GetScriptTexture(CLEO::CRunningScript* script, DWORD slot)
{
    if (script == nullptr || slot == 0 || slot > _countof(CTheScripts::ScriptSprites))
    {
        return nullptr; // invalid param
    }
    slot -= 1; // to 0-based index

    if (script->IsCustom())
    {
        if (script == m_currCustomScript)
        {
            return CTheScripts::ScriptSprites[slot].m_pTexture;
        }
        else
        {
            return (m_scriptDrawingStates.find(script) != m_scriptDrawingStates.end()) ? CTheScripts::ScriptSprites[slot].m_pTexture : nullptr;
        }
    }
    else
    {
        return (m_currCustomScript == nullptr) ? CTheScripts::ScriptSprites[slot].m_pTexture : m_globalDrawingState.sprites[slot].m_pTexture;
    }
}
