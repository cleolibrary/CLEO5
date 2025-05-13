
#include "ScriptDrawing.h"
#include "CLEO_Utils.h"
#include <CTheScripts.h>

using namespace CLEO;


void ScriptDrawing::Initialize()
{
    if (m_initialized)
    {
        return;
    }

    m_initialized = true;
}

void ScriptDrawing::BeginScriptProcessing(CLEO::CRunningScript* script)
{
}

void ScriptDrawing::EndScriptProcessing(CLEO::CRunningScript* script)
{
}

void ScriptDrawing::Draw(bool beforeFade)
{
}
