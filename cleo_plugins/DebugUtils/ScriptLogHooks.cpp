// Script Log lifecycle, configuration, and CLEO callbacks

#include "ScriptLog.h"
#include "CLEO_Utils.h"
#include "crc32.h"       // from CLEO core
#include "ScriptUtils.h" // from CLEO core
#include <CGame.h>
#include <CMenuManager.h>
#include <CRunningScript.h>
#include <CTheScripts.h>
#include <CTimer.h>
#include <cstring>
#include <string>

using namespace CLEO;

static void HOOK_SetConditionResult();

ScriptLog* ScriptLog::g_Instance = nullptr;

ScriptLog::ScriptLog()
{
    assert(g_Instance == nullptr);
    g_Instance = this;

    LoadConfig();

    m_logFilePath = CLEO_GetLogDirectory();
    m_logFilePath += "\\cleo_script.log";

    // React to config changes and schedule an update of the logging state at the end of the frame
    CLEO_RegisterCallback(eCallbackId::ConfigUpdate, callbackConfigUpdate);
    // Apply new state at the end of the frame, when current log buffer is flushed to file
    CLEO_RegisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);

    if (state != LoggingState::Disabled)
    {
        m_logBuffer.reserve(Initial_Buff_Size);
        RegisterCallbacks();
        EnsureDatabaseLoaded();
        LogFileDelete(); // start every session with a fresh file
    }
    // TODO: start file writer thread
}

ScriptLog::~ScriptLog()
{
    // TODO: stop file writer thread

    UninstallConditionHook();
    UnregisterCallbacks();

    CLEO_UnregisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);
    CLEO_UnregisterCallback(eCallbackId::ConfigUpdate, callbackConfigUpdate);

    if (state == LoggingState::OnCrash && !m_logBuffer.empty())
    {
        CLEO_Log(eLogLevel::Default, m_logBuffer.c_str());
    }

    if (state != LoggingState::Disabled)
    {
        LogWriteFile(); // flush to file on exit or crash
    }
}

void ScriptLog::RegisterCallbacks()
{
    CLEO_RegisterCallback(eCallbackId::GameBegin, (void*)callbackGameBegin);
    CLEO_RegisterCallback(eCallbackId::GameProcessBefore, (void*)callbackGameProcessBefore);
    CLEO_RegisterCallback(eCallbackId::GameProcessAfter, (void*)callbackGameProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessBefore, (void*)callbackScriptProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessAfter, (void*)callbackScriptProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcessBefore, (void*)callbackScriptOpcodeProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcessAfter, (void*)callbackScriptOpcodeProcessAfter);
    CLEO_RegisterCallback(eCallbackId::MainWindowFocus, (void*)callbackMainWindowFocus);
}

void ScriptLog::UnregisterCallbacks()
{
    CLEO_UnregisterCallback(eCallbackId::GameBegin, (void*)callbackGameBegin);
    CLEO_UnregisterCallback(eCallbackId::GameProcessBefore, (void*)callbackGameProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::GameProcessAfter, (void*)callbackGameProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessBefore, (void*)callbackScriptProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessAfter, (void*)callbackScriptProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcessBefore, (void*)callbackScriptOpcodeProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcessAfter, (void*)callbackScriptOpcodeProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::MainWindowFocus, (void*)callbackMainWindowFocus);
}

void ScriptLog::SetState(LoggingState newState)
{
    if (newState == state) return;

    if (newState == LoggingState::Disabled)
    {
        if (state == LoggingState::Full)
        {
            // flush whatever was recorded so far
            LogWriteFile(true);
        }

        m_logBuffer.clear();
        if (m_logFile.is_open()) m_logFile.close();

        UnregisterCallbacks();
        UninstallConditionHook();

        state = newState;
        return;
    }

    if (state == LoggingState::Disabled)
    {
        // start new session in the log file
        m_logBuffer.reserve(Initial_Buff_Size);
        EnsureDatabaseLoaded();
        RegisterCallbacks();

        SYSTEMTIME t;
        GetLocalTime(&t);
        LogSeparator();
        LogFormattedLine(
            "  Script log enabled at: %02d/%02d/%04d %02d:%02d:%02d", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute,
            t.wSecond
        );
    }

    state = newState;
}

void ScriptLog::ApplyPendingState()
{
    if (!m_pendingState) return;

    auto pending = *m_pendingState;
    m_pendingState.reset();

    LoadConfig(true); // refresh the other switches as well
    SetState(pending);
}

ScriptLog::LoggingState ScriptLog::GetConfiguredState()
{
    switch (CLEO_GetConfigInt("DebugUtils.ScriptLog.Enabled", 0))
    {
    case 2:
        return LoggingState::Full;
    case 1:
        return LoggingState::OnCrash;
    default:
        return LoggingState::Disabled;
    }
}

void ScriptLog::OnConfigUpdate(const char* section, const char* key)
{
    // only our own keys are interesting; nullptr means "reload everything"
    if (section != nullptr && std::strcmp(section, "Plugins") != 0) return;
    if (key != nullptr && std::strncmp(key, "DebugUtils.ScriptLog.", 21) != 0) return;

    // Changing the state (subbing/unsubbing) is not safe here, defer to the end of the frame
    auto requested = GetConfiguredState();
    m_pendingState = (requested == state) ? std::nullopt : std::optional<LoggingState>(requested);
}

void ScriptLog::LoadConfig(bool keepState)
{
    if (!keepState) state = GetConfiguredState();

    int size    = CLEO_GetConfigInt("DebugUtils.ScriptLog.MaxSize", 250);
    size        = std::max(size, 0);
    maxFileSize = size;
    maxFileSize *= 1024 * 1024; // to MB

    logCustomScriptsOnly = CLEO_GetConfigInt("DebugUtils.ScriptLog.OnlyCustomScripts", -1);
    logDebugScriptsOnly  = CLEO_GetConfigInt("DebugUtils.ScriptLog.OnlyDebugScripts", 0) != 0;
    logOffsets           = CLEO_GetConfigInt("DebugUtils.ScriptLog.Offsets", 1) != 0;
    logOpcodes           = CLEO_GetConfigInt("DebugUtils.ScriptLog.Opcodes", 0) != 0;
}

void ScriptLog::OnGameBegin(DWORD saveSlot)
{
    if (state == LoggingState::Disabled) return;

    SetCurrScript(nullptr);

    LoadConfig(true);

    // time stamp
    SYSTEMTIME t;
    GetLocalTime(&t);
    char timeStamp[32];
    sprintf_s(
        timeStamp, "%02d/%02d/%04d %02d:%02d:%02d.%03d", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond,
        t.wMilliseconds
    );

    LogSeparator();
    LogAppend("  Game started at: ");
    LogAppend(timeStamp);

    if (CGame::bMissionPackGame)
    {
        // find name of the mission pack
        const char* packName = nullptr;
        for (const auto& pack : FrontEndMenuManager.m_MissionPacks)
        {
            if (pack.id == FrontEndMenuManager.m_nSelectedMissionPack)
            {
                packName = pack.name;
                break;
            }
        }
        LogLine("  Mission Pack: ");
        LogAppendNum(FrontEndMenuManager.m_nSelectedMissionPack);
        LogLine("  Mission Pack Name: ");
        LogAppend(packName ? packName : "???");
        LogNewLine();
        m_customMain = true;
    }
    else
    {
        // verify main.scm
        const size_t Orig_Main_Size        = 200000;
        const size_t Orig_Main_Code_Offset = 55976; // skip global variables
        const size_t Orig_Main_Hash        = 0xbd4e2fcf;

        auto hash    = crc32(CLEO_GetScmMainData() + Orig_Main_Code_Offset, Orig_Main_Size - Orig_Main_Code_Offset);
        m_customMain = hash != Orig_Main_Hash; // hash of original main.scm

        if (saveSlot != -1)
        {
            LogLine("  Save Slot: ");
            LogAppendNum(saveSlot + 1); // 1-based index
        }
        LogLine("  Main Script: ");
        LogAppend(m_customMain ? "modified" : "original");
        LogNewLine();
    }
    LogSeparator();
}

void ScriptLog::OnGameProcessBefore()
{
    if (state == LoggingState::Disabled) return;
    if (CTimer::m_UserPause || CTimer::m_CodePause) return;

    // count active scripts

    size_t scriptCount = 0;
    auto next          = CTheScripts::pActiveScripts;
    while (next)
    {
        scriptCount++;
        next = next->m_pNext;
    }

    LogNewLine();
    LogAppend("--|-- Frame ");
    LogAppendNum(CTimer::m_FrameCounter);
    LogAppend(" - Active Scripts: ");
    LogAppendNum(scriptCount);
    LogLine("  |");
}

void ScriptLog::OnGameProcessAfter()
{
    SetCurrScript(nullptr);
}

bool ScriptLog::OnScriptProcessBefore(CLEO::CRunningScript* script)
{
    SetCurrScript(script);
    return true;
}

void ScriptLog::OnScriptProcessAfter(CLEO::CRunningScript* script)
{
    SetCurrScript(nullptr);
}

OpcodeResult ScriptLog::OnScriptOpcodeProcessBefore(CLEO::CRunningScript* script, DWORD opcode)
{
    // late installation: the first traced opcode runs after every ASI (SCRLog included)
    // had its chance to patch UpdateCompareFlag, so our hook stacks on top of it
    InstallConditionHook();

    SetCurrScript(script);

    if (state == LoggingState::Disabled || !m_currScriptLogging) return OR_NONE;

    m_currCommandReturnParams = nullptr;
    m_conditionResultUpdated  = false;
    m_conditionResultExpected =
        script->NotFlag || m_prevCommand == COMMAND_ANDOR || script->LogicalOp != eLogicalOperation::NONE;
    if (opcode == 0x0AB1) m_conditionResultExpected = false; // cleo_call: condition result set on return

    auto oriIP = script->CurrentIP;

    LogAppend("  |   "); // script content indentation

    // script offset
    if (logOffsets)
    {
        auto offset = CLEO_GetScriptBaseRelativeOffset(script, script->CurrentIP);
        offset -= sizeof(WORD); // command opcode was already consumed

        auto digits = offset <= 0 ? 1 : (int)(log10(offset) + 1);
        for (int i = 7 - digits; i > 0; i--)
            LogAppend(' '); // pad to constant width
        LogAppend('{');
        LogAppendNum(offset);
        LogAppend("}   ");
    }

    // command opcode
    if (logOpcodes)
    {
        LogAppend('{');
        LogAppendHex(opcode | (script->NotFlag ? 0x8000 : 0), 4);
        LogAppend(":} ");
    }

    // if block conditions indentation
    if (m_prevCommand == COMMAND_ANDOR || script->LogicalOp != eLogicalOperation::NONE) LogAppend(Script_Indent);

    // gosub scope indentation
    for (auto i = script->SP; i > 0; i--)
        LogAppend(Script_Indent);

    // cleo_call scope indentation
    for (auto i = CLEO_GetCleoCallStackSize(script); i > 0; i--)
        LogAppend(Script_Indent);

    // negation
    if (script->NotFlag) LogAppend("not ");

    auto command = m_opcodeDatabase.GetCommand((uint16_t)opcode);

    if (command == nullptr) // not documented in database?
    {
        LogAppend("command_");
        LogAppendHex(opcode, 4);
        m_prevCommand = (WORD)opcode;
        return OR_NONE;
    }

    // return parameters as "x, y = command ..."
    auto returnArgCount = command->arguments.size() - command->inputArguments;
    if (returnArgCount > 0)
    {
        for (size_t i = 0; i < command->arguments.size(); i++)
        {
            if (i < command->inputArguments)
            {
                if (script->PeekDataType() == eDataType::DT_END) break;

                if (command->arguments[i].type == OpcodeInfoDatabase::CommandArgumentType::Arguments)
                {
                    // when varArg param is present in command SB always compiles all following params as members of the
                    // varArg list we need to count all provided params and skip only those which are actualy optional
                    BYTE* argsIP = script->CurrentIP;
                    int args     = 0;
                    while (args < 32)
                    {
                        ScriptParamInfo paramInfo(script); // just skip
                        if (paramInfo.type != eDataType::DT_END)
                            args++;
                        else
                            break;
                    }

                    // skip optional args
                    script->CurrentIP = argsIP;
                    args -= command->arguments.size() - (i + 1); // substract expected remaining params
                    while (args > 0)
                    {
                        ScriptParamInfo paramInfo(script); // just skip
                        args--;
                    }
                }
                else
                    ScriptParamInfo paramInfo(script); // just skip
            }
            else
            {
                if (i == command->inputArguments)
                    m_currCommandReturnParams = script->CurrentIP; // keep pointer to return params start

                LogAppendSpace();
                LogAppendScriptParam(script, command, i, returnArgCount > 1, true, false);
            }
        }
        script->CurrentIP = oriIP; // restore original position

        LogAppend(" = ");
    }
    else if (!command->oper.empty() && !command->arguments.empty() && !command->IsComparison())
    {
        m_currCommandReturnParams = oriIP; // operator result is first param
    }

    // command name
    if (command->oper.empty())
    {
        LogAppend(command->nameLower);
    }

    // command params
    if (opcode == COMMAND_ANDOR)
    {
        ScriptParamInfo paramInfo(script);
        if (paramInfo.value.nParam >= eLogicalOperation::ORS_1)
            LogAppend(" or");
        else if (paramInfo.value.nParam >= eLogicalOperation::ANDS_1)
            LogAppend(" and");
    }
    else
    {
        for (size_t i = 0; i < command->inputArguments; i++)
        {
            if (script->PeekDataType() == eDataType::DT_END) break;

            LogAppendSpace();

            // operator
            if (!command->oper.empty())
            {
                if (command->inputArguments == 1 && i == 0) // %param
                {
                    LogAppend(command->oper);
                }
                else if (command->inputArguments == 2 && i == 1) // param % param
                {
                    LogAppend(command->oper);
                    if (!command->IsComparison() && command->oper.find('=') == std::string::npos)
                        LogAppend('='); // param %= param
                    LogAppend(" ");
                }
            }

            if (command->arguments[i].type != OpcodeInfoDatabase::CommandArgumentType::Arguments)
            {
                LogAppendScriptParam(script, command, i, true, true, true);
            }
            else // varArgs
            {
                auto type = script->PeekDataType();
                int count = 0;

                while (type != eDataType::DT_END && count < 32)
                {
                    if (count > 0) LogAppend(" ");
                    LogAppendScriptParam(script, command, i, count == 0, true, true);

                    count++;
                    type = script->PeekDataType();
                }
                script->IncPtr(sizeof(eDataType)); // var args terminator
            }
        }
    }

    script->CurrentIP = oriIP; // restore original position
    m_prevCommand     = (WORD)opcode;
    return OR_NONE;
}

CLEO::OpcodeResult ScriptLog::OnScriptOpcodeProcessAfter(
    CLEO::CRunningScript* script, DWORD opcode, CLEO::OpcodeResult result
)
{
    if (state == LoggingState::Disabled || !m_currScriptLogging) return result;

    auto command = m_opcodeDatabase.GetCommand((uint16_t)opcode);
    if (!command)
    {
        LogNewLine();
        return result;
    }

    bool hasComment    = false;
    bool hasReturnVal  = false;
    bool warnGlobalVar = false;

    // print values of return arguments
    if (m_currCommandReturnParams != nullptr)
    {
        auto oriIp        = script->CurrentIP;
        script->CurrentIP = m_currCommandReturnParams;

        size_t i     = command->oper.empty() ? command->inputArguments : 0;
        size_t count = command->oper.empty() ? command->arguments.size() : 1;
        for (; i < count; i++)
        {
            if (!hasReturnVal)
                LogAppend(hasComment ? " -> " : " // -> ");
            else
                LogAppend(", "); // separator
            hasComment = true;

            auto isGlobalVar = LogAppendScriptParam(script, command, i, false, false, true);
            hasReturnVal     = true;

            if (isGlobalVar && script->IsCustom()) warnGlobalVar = true;
        }
        script->CurrentIP = oriIp;
    }

    // condition result value
    if (m_conditionResultUpdated)
    {
        if (!hasReturnVal)
            LogAppend(hasComment ? " -> " : " // -> ");
        else
            LogAppend(", "); // separator
        hasComment = true;

        LogAppend("condition: ");
        LogAppend(m_conditionResultValue ? "true" : "false");
        hasReturnVal = true;
    }

    // extra annotations
    if (command->isNop)
    {
        LogAppend(hasComment ? ", " : " // ");
        hasComment = true;
        LogAppend("NOP command");
    }

    if (m_conditionResultExpected && !m_conditionResultUpdated &&
        opcode != 0x0AB1) // assume cleo_call always sets condition result
    {
        LogAppend(hasComment ? ", " : " // ");
        hasComment = true;
        LogAppend("BUG: non-logical command");
    }

    if (warnGlobalVar)
    {
        LogAppend(hasComment ? ", " : " // ");
        hasComment = true;
        LogAppend("BUG?: Custom Script writes global variable");
    }

    LogNewLine();

    return result;
}

void ScriptLog::OnDrawingFinished()
{
    ApplyPendingState(); // apply deferred state change: sub/unsub

    if (state == LoggingState::Disabled) return; // nothing to flush

    // this should be done just at the end of scripts processing
    // but PluginSDK, SAMP etc. call single scripts/callbacks outside of the processing queue
    // make sure current script does not persists to next render frame
    SetCurrScript(nullptr);

    if (state == LoggingState::Full) LogWriteFile();

    if (state != LoggingState::Full) // keep only current frame
    {
        // TODO: thread lock
        m_logBuffer.clear();
        // TODO: thread unlock
    }
}

void ScriptLog::OnMainWindowFocus(bool active)
{
    if (active == false && state == LoggingState::Full)
    {
        LogWriteFile(true); // flush
    }
}

void ScriptLog::InstallConditionHook()
{
    if (m_hookInstalled) return;

    // Store current UpdateCompareFlag prologue (can be another plugin's patch)
    // Then install our custom hook
    m_patchSetConditionResult = MemPatchJump(gaddrof(::CRunningScript::UpdateCompareFlag), &HOOK_SetConditionResult);
    m_hookInstalled           = true;
}

void ScriptLog::UninstallConditionHook()
{
    if (!m_hookInstalled) return;

    // apply original UpdateCompareFlag prologue
    m_patchSetConditionResult.Apply();
    m_hookInstalled = false;
}

static void __declspec(naked) HOOK_SetConditionResult()
{
    _asm
        {
            push ecx // save ecx
            push dword ptr[esp + 8] // state
            push ecx // script
            call ScriptLog::SetConditionResult // cdecl, caller cleans stack
            add esp, 8
            pop ecx // restore ecx
            retn 4
        }
}

void ScriptLog::SetConditionResult(CLEO::CRunningScript* script, bool state)
{
    g_Instance->m_conditionResultUpdated = true;
    g_Instance->m_conditionResultValue   = script->NotFlag ? !state : state;

    // restore original function code
    g_Instance->m_patchSetConditionResult.Apply();
    // call original function
    ((::CRunningScript*)script)->UpdateCompareFlag(state);
    // reinstall our hook
    g_Instance->m_patchSetConditionResult =
        MemPatchJump(gaddrof(::CRunningScript::UpdateCompareFlag), &HOOK_SetConditionResult);
}