#include "ScriptLog.h"
#include "CLEO_Utils.h"
#include "crc32.h" // from CLEO core
#include "ScriptUtils.h" // from CLEO core
#include <CCheat.h>
#include <CGame.h>
#include <CHud.h>
#include <CMenuManager.h>
#include <CRunningScript.h>
#include <CTheScripts.h>
#include <CTimer.h>
#include <memory>
#include <sstream>
#include <string>

using namespace CLEO;


ScriptLog* ScriptLog::g_Instance = nullptr;

ScriptLog::ScriptLog()
{
    assert(g_Instance == nullptr);
    g_Instance = this;

    m_logBuffer = new std::stringstream(std::string(Initial_Buff_Size, '\0'));
    m_logFileBuffer = new std::stringstream(std::string(Initial_Buff_Size, '\0'));

    m_logFilePath = CLEO_GetGameDirectory();
    m_logFilePath += "\\cleo\\.cleo_script_log.xml";
    LogFileDelete();

    CLEO_RegisterCallback(eCallbackId::GameBegin, callbackGameBegin);
    CLEO_RegisterCallback(eCallbackId::GameProcessBefore, callbackGameProcessBefore);
    CLEO_RegisterCallback(eCallbackId::GameProcessAfter, callbackGameProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessBefore, callbackScriptProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessAfter, callbackScriptProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcessBefore, callbackScriptOpcodeProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcessAfter, callbackScriptOpcodeProcessAfter);
    CLEO_RegisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);
    CLEO_RegisterCallback(eCallbackId::MainWindowFocus, callbackMainWindowFocus);

    LoadConfig();

    std::string path = CLEO_GetGameDirectory();
    path += "\\cleo\\.config\\sa.json";
    m_opcodeDatabase.LoadCommands(path.c_str());

    path = CLEO_GetGameDirectory();
    path += "\\cleo\\.config\\enums.json";
    m_opcodeDatabase.LoadEnums(path.c_str());

    // TODO: start file writer thread
}

ScriptLog::~ScriptLog()
{
    // TODO: stop file writer thread

    m_patchSetConditionResult.Apply(); // undo hook

    CLEO_UnregisterCallback(eCallbackId::GameBegin, callbackGameBegin);
    CLEO_UnregisterCallback(eCallbackId::GameProcessBefore, callbackGameProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::GameProcessAfter, callbackGameProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessBefore, callbackScriptProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessAfter, callbackScriptProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcessBefore, callbackScriptOpcodeProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcessAfter, callbackScriptOpcodeProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);
    CLEO_UnregisterCallback(eCallbackId::MainWindowFocus, callbackMainWindowFocus);

    LogWriteFile(); // flush to file on exit or crash

    delete m_logBuffer;
    delete m_logFileBuffer;
}

size_t ScriptLog::CurrScriptCommandCount() const
{
    return m_currScriptCommandCount;
}

size_t ScriptLog::CurrScriptElapsedSeconds() const
{
    return (clock() - m_currScriptStartTime) / CLOCKS_PER_SEC;
}

void ScriptLog::LoadConfig(bool keepState)
{
    auto config = GetConfigFilename();

    if (!keepState)
    {
        switch (GetPrivateProfileInt("ScriptLog", "Enabled", 0, config.c_str()))
        {
            case 0:
                state = LoggingState::Disabled;
                break;

            case 1:
            default:
                state = LoggingState::OnCrash;
                break;

            case 2:
                state = LoggingState::Full;
                break;
        }
    }

    int size = GetPrivateProfileInt("ScriptLog", "MaxSize", 100, config.c_str());
    size = std::max(size, 0);
    maxFileSize = size;
    maxFileSize *= 1024 * 1024; // to MB

    logCustomScriptsOnly = GetPrivateProfileInt("ScriptLog", "OnlyCustomScripts", -1, config.c_str());
    logOffsets = GetPrivateProfileInt("ScriptLog", "Offsets", 0, config.c_str()) != 0;
    logOpcodes = GetPrivateProfileInt("ScriptLog", "Opcodes", 0, config.c_str()) != 0;
}

void ScriptLog::SetCurrScript(CLEO::CRunningScript* script)
{
    static std::string line; // keep for performance
    line.clear();

    // close previous script's log block
    if (state != LoggingState::Disabled && 
        m_currScript != nullptr && script != m_currScript &&
        (!logCustomScriptsOnly || m_currScript->IsCustom()))
    {
        if (m_currScriptCommandCount > 0)
        {
            line += '\n';
            if (m_processingGame) line += Block_Indent;
            line += Block_Indent;
            line += "Executed in ";
            StringAppendNum(line, size_t(clock() - m_currScriptStartTime) * 1000 / CLOCKS_PER_SEC);
            line += " ms";
        }

        line += '\n';
        if (m_processingGame) line += Block_Indent;
        line += "</script>\n";
    }

    // start new script's log block
    if (state != LoggingState::Disabled && 
        script != nullptr && script != m_currScript &&
        (!logCustomScriptsOnly || script->IsCustom()))
    {
        // find index of this script in active scripts queue
        int idx = -1;
        int i = 0;
        for (auto s = (CLEO::CRunningScript*)CTheScripts::pActiveScripts; s; s = s->Next)
        {
            i++; // 1-based display
            if (s == script)
            {
                idx = i;
                break;
            }
        }

        // script file path
        static std::string filename; // keep for performance

        auto name = CLEO_GetScriptFilename(script);
        if (name != nullptr)
        {
            filename = ".\\";
            filename.resize(MAX_PATH);
            CLEO_ResolvePath(script, filename.data(), filename.size());
            filename.resize(strlen(filename.c_str()));
            if (!filename.empty()) filename.push_back('\\');
            filename += name;
            FilepathNormalize(filename);
            FilepathRemoveParent(filename, CLEO_GetGameDirectory());
        }
        else
        {
            filename = "?";
        }

        line += '\n';
        if (m_processingGame) line += Block_Indent;
        line += "<script idx='";
        StringAppendNum(line, idx);
        line += "' name='";
        line += script->GetName();
        line += "' file='";
        line += filename;
        line += "' custom='";
        line += script->IsCustom() ? "yes" : "no";
        line += "' mission='";
        line += script->IsMission() ? "yes" : "no";
        line += "'>";

        // call stack info
        auto cleoStack = CLEO_GetScriptCleoStackSize(script);
        if (cleoStack || script->SP)
        {
            line += '\n';
            if (m_processingGame) line += Block_Indent;
            line += Block_Indent;
            line += "Call Stacks depth - cleo_call: ";
            StringAppendNum(line, cleoStack);
            line += ", gosub: ";
            StringAppendNum(line, script->SP);

            bool savingEnabled = !script->IsCustom() || false; // TODO: check CLEO save enabled for custom script
            if (savingEnabled && cleoStack)
            {
                line += " \\ BUG: Scrip saving enabled. Saving during cleo_call would result in errors!";
            }

            bool wastedBustedCheck = script->IsMission() && script->bWastedBustedCheck;
            if (wastedBustedCheck && cleoStack)
            {
                line += " \\ BUG: Mission death-arrest-check active. Executing death-arrest procedure during cleo_call would result in errors!";
            }
        }

        // 'wait' in progress?
        if (CTimer::m_snTimeInMilliseconds < script->WakeTime)
        {
            auto pause = script->WakeTime - CTimer::m_snTimeInMilliseconds;

            line += '\n';
            if (m_processingGame) line += Block_Indent;
            line += Block_Indent;
            line += "Sleeping for ";
            StringAppendNum(line, pause);
            line += " ms more";
        }
    }

    if (!line.empty()) LogLineAppend(line);

    // update/reset script stats
    if (script != m_currScript || script == nullptr)
    {
        m_currScript = script;
        m_currScriptStartTime = clock();
        m_currScriptCommandCount = 0;
        m_prevCommand = 0xFFFF;
    }
}

void ScriptLog::LogLine(const char* line)
{
    // TODO: thread lock
    *m_logBuffer << "\n" << line;
    // TODO: thread unlock
}

void ScriptLog::LogLine(const std::string& line)
{
    // TODO: thread lock
    *m_logBuffer << "\n" << line;
    // TODO: thread unlock
}

void ScriptLog::LogFormattedLine(const char* format, ...)
{
    static std::string line; // keep for performance
    line.clear();

    va_list args;

    va_start(args, format);
    auto len = std::vsnprintf(nullptr, 0, format, args);
    va_end(args);

    if (len <= 0) return; // empty or encoding error

    line.resize(len);

    va_start(args, format);
    std::vsnprintf(line.data(), len + 1, format, args);
    va_end(args);

    LogLine(line);
}

void ScriptLog::LogLineAppend(const char* line)
{
    // TODO: thread lock
    *m_logBuffer << line;
    // TODO: thread unlock
}

void ScriptLog::LogLineAppend(const std::string& line)
{
    // TODO: thread lock
    *m_logBuffer << line;
    // TODO: thread unlock
}

void ScriptLog::LogScriptParam(std::string& dest, CLEO::CRunningScript* script, const OpcodeInfoDatabase::Command* command, size_t paramIdx, bool logName, bool logVariable, bool logValue) const
{
    bool hasName = false;
    if (logName)
    {
        if (!command->arguments[paramIdx].name.empty())
        {
            hasName = true;
            dest += '{';
            dest += command->arguments[paramIdx].name;
            dest += '}';
        }
    }

    ScriptParamInfo paramInfo(script);

    bool hasVariable = false;
    if (logVariable)
    {
        bool isGlobalVar = false;
        switch(paramInfo.type)
        {
            case DT_END:
                /*hasVariable = true;
                if (hasName) dest += ' ';
                dest += "ArgsEnd";*/
                break;

            case DT_VAR:
            case DT_VAR_ARRAY:
            case DT_VAR_TEXTLABEL:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_VAR_STRING:
            case DT_VAR_STRING_ARRAY:
                isGlobalVar = true;
                [[fallthrough]];

            case DT_LVAR:
            case DT_LVAR_ARRAY:
            case DT_LVAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL_ARRAY:
            case DT_LVAR_STRING:
            case DT_LVAR_STRING_ARRAY:
                hasVariable = true;
                if (hasName) dest += ' ';
                if (isGlobalVar) dest += '$';
                StringAppendNum(dest, paramInfo.varIndex);
                if (!isGlobalVar) dest += '@';

                if (paramInfo.arrayType != eArrayType::AT_NONE)
                {
                    auto idxVar = GetScriptVar(script,
                        paramInfo.arrayFlags & ATF_INDEX_GLOBAL,
                        paramInfo.arrayIndexVar);

                    dest += "[ ";
                    if (paramInfo.arrayFlags & ATF_INDEX_GLOBAL) dest += '$';
                    StringAppendNum(dest, paramInfo.arrayIndexVar);
                    if ((paramInfo.arrayFlags & ATF_INDEX_GLOBAL) == 0) dest += '@';
                    dest += '(';
                    if (abs(idxVar->nParam) >= MinValidAddress)
                    {
                        dest += "0x";
                        StringAppendHex(dest, idxVar->dwParam);
                    }
                    else
                        StringAppendNum(dest, idxVar->nParam);
                    dest += ") ]";
                }
                break;
        }
    }

    // no value to print
    switch (paramInfo.type)
    {
        case DT_END:
            logValue = false;
    }

    if (logValue)
    {
        if (hasVariable) dest += '(';
        else if (hasName) dest += ' ';

        // pick presentation style according to param type declared in the mode
        switch (command->arguments[paramIdx].type)
        {
            case OpcodeInfoDatabase::CommandArgumentType::Other: // enum or class
            {
                auto e = m_opcodeDatabase.GetEnum(command->arguments[paramIdx].typeNameLower.c_str());
                if (e)
                {
                    if (e->IsNumeric())
                    {
                        if (!IsImmString(paramInfo.type) && !IsVarString(paramInfo.type))
                        {
                            auto entry = e->GetEntryName(paramInfo.value.nParam);
                            if (entry)
                            {
                                dest += e->name;
                                dest += '.';
                                dest += entry;
                            }
                            else
                                paramInfo.ValueToString(dest); // print according to param type in script
                        }
                        else
                            paramInfo.ValueToString(dest); // print according to param type in script
                    }
                    else // text enums
                    {
                        auto entry = e->GetEntryName(std::string(paramInfo.GetText()).c_str());
                        if (entry)
                        {
                            dest += e->name;
                            dest += '.';
                            dest += entry;
                        }
                        else
                            paramInfo.ValueToString(dest); // print according to param type in script
                    }
                }
                else // class or unknown enum
                {
                    paramInfo.ValueToString(dest); // print according to param type in script
                }
                break;
            }

            //case OpcodeInfoDatabase::CommandArgumentType::Any:
            //case OpcodeInfoDatabase::CommandArgumentType::Arguments:

            case OpcodeInfoDatabase::CommandArgumentType::Bool:
                switch (paramInfo.value.nParam)
                {
                    case 0: dest += "false"; break;
                    case 1: dest += "true"; break;
                    default: StringAppendNum(dest, paramInfo.value.nParam); break;
                }
                break;

            case OpcodeInfoDatabase::CommandArgumentType::Float:
                StringAppendFloat(dest, paramInfo.value.fParam);
                break;

            case OpcodeInfoDatabase::CommandArgumentType::GxtKey:
            case OpcodeInfoDatabase::CommandArgumentType::ZoneKey:
            case OpcodeInfoDatabase::CommandArgumentType::String:
            case OpcodeInfoDatabase::CommandArgumentType::String128:
                dest += (paramInfo.GetBaseType() == DT_TEXTLABEL) ? '\'' : '"';
                dest += paramInfo.GetText();
                dest += (paramInfo.GetBaseType() == DT_TEXTLABEL) ? '\'' : '"';
                break;
            
            case OpcodeInfoDatabase::CommandArgumentType::Int:
                if (paramInfo.GetBaseType() == DT_DWORD || paramInfo.GetBaseType() == DT_VAR)
                {
                    paramInfo.ValueToString(dest);
                }
                else
                {
                    dest += "0x";
                    StringAppendHex(dest, paramInfo.value.nParam);
                }
                break;
            
            case OpcodeInfoDatabase::CommandArgumentType::Label:
                dest += "@LABEL_";
                StringAppendNum(dest, abs(paramInfo.value.nParam));
                break;

            case OpcodeInfoDatabase::CommandArgumentType::ModelAny:
            case OpcodeInfoDatabase::CommandArgumentType::ModelChar:
            case OpcodeInfoDatabase::CommandArgumentType::ModelObject:
            case OpcodeInfoDatabase::CommandArgumentType::ModelVehicle:
                StringAppendNum(dest, abs(paramInfo.value.nParam));
                break;

            case OpcodeInfoDatabase::CommandArgumentType::ScriptId:
                dest += "0x";
                StringAppendHex(dest, paramInfo.value.nParam);
                break;

            default:
                paramInfo.ValueToString(dest); // print according to param type in script
        }

        if (hasVariable) dest += ')';
    }
}

void ScriptLog::LogWriteFile(bool forceUpdate)
{
    static size_t fileSize = 0;

    // close stream so system and applications notice it was updated
    if (forceUpdate)
    {
        m_logFile.flush();
        m_logFile.close();
    }

    // TODO: thread lock
    std::swap(m_logBuffer, m_logFileBuffer);
    // unlock

    auto buff = m_logFileBuffer->rdbuf();
    if (!buff)
    {
        return; // empty
    }

    auto len = (size_t)buff->pubseekoff(0, std::ios_base::cur, std::ios_base::out);
    if (len == 0)
    {
        return; // empty
    }

    if (maxFileSize > 0 && fileSize > maxFileSize)
    {
        LogFileDelete();
        LOG_WARNING(0, "CLEO script log file cleared");
    }

    if (!m_logFile.is_open())
    {
        m_logFile.clear(); // clear flags
        m_logFile.open(m_logFilePath.c_str(), std::ios::app);
    }

    if (m_logFile.is_open())
    {
        auto view = buff->_Get_buffer_view(); // view() introduced in C++20
        m_logFile << std::string_view(view._Ptr, len) << std::endl;
        fileSize = (size_t)m_logFile.tellp();
    }
    else
    {
        LOG_WARNING(0, "Failed to open script log file!");
    }

    buff->pubseekpos(0);
    m_logFileBuffer->clear(); // reset flags
}

void ScriptLog::LogFileDelete()
{
    if (m_logFile.is_open())
    {
        m_logFile.close();
    }

    // rename then remove
    // fixes lag when overwriting old file
    auto newName = m_logFilePath;
    StringAppendHex(newName, GetTickCount());
    if (std::rename(m_logFilePath.c_str(), newName.c_str()) == 0)
    {
        std::remove(newName.c_str());
    }
}

void __fastcall ScriptLog::HOOK_SetConditionResult(CLEO::CRunningScript* script, int dummy, bool state)
{
    if (script->NotFlag) state = !state;

    g_Instance->m_conditionResultUpdated = true;
    g_Instance->m_conditionResultValue = state;

    // restoring and calling orignal function code is more performance costly than reimplementing it here

    if (script->LogicalOp == eLogicalOperation::NONE)
    {
        script->bCondResult = state;
    }
    else if (script->LogicalOp >= eLogicalOperation::ANDS_1 && script->LogicalOp < eLogicalOperation::AND_END)
    {
        script->bCondResult &= state;
        --script->LogicalOp;
    }
    else if (script->LogicalOp >= eLogicalOperation::ORS_1 && script->LogicalOp < eLogicalOperation::OR_END)
    {
        script->bCondResult |= state;
        --script->LogicalOp;
    }
}

void ScriptLog::OnGameBegin(DWORD saveSlot)
{
    if (state == LoggingState::Disabled) return;

    SetCurrScript(nullptr);

    LoadConfig(true);

    if (CGame::bMissionPackGame)
    {
        // find name of the mission pack
        const char* packName = nullptr;
        for (size_t i = 0; i < _countof(FrontEndMenuManager.m_MissionPacks); i++)
        {
            if (FrontEndMenuManager.m_MissionPacks[i].id == FrontEndMenuManager.m_nSelectedMissionPack)
            {
                packName = FrontEndMenuManager.m_MissionPacks[i].name;
            }
        }

        LogFormattedLine("<newGameSession mPack='%d' mPackName='%s'/>\n", 
            FrontEndMenuManager.m_nSelectedMissionPack,
            packName ? packName : "???"
        );

        if (logCustomScriptsOnly == -1) logCustomScriptsOnly = false; // mission pack is custom main. Log it too
    }
    else
    {
        // verify main.scm
        const size_t Orig_Main_Size = 200000;
        const size_t Orig_Main_Code_Offset = 55976; // skip global variables
        const size_t Orig_Main_Hash = 0xbd4e2fcf;

        auto hash = crc32((BYTE*)CTheScripts::ScriptSpace + Orig_Main_Code_Offset, Orig_Main_Size - Orig_Main_Code_Offset);
        bool isMainOriginal = hash == Orig_Main_Hash; // hash of original main.scm

        if (saveSlot != -1)
        {
            LogFormattedLine("<newGameSession saveSlot='%d' mainScript='%s'/>\n",
                saveSlot + 1, // 1-based index
                isMainOriginal ? "original" : "modified" );
        }
        else
        {
            LogFormattedLine("<newGameSession mainScript='%s'/>\n",
                isMainOriginal ? "original" : "modified");
        }

        if (logCustomScriptsOnly == -1) // auto detect
        {
            logCustomScriptsOnly = isMainOriginal; // do not log original native scripts
        }
    }

    LogLine(""); // separator
}

void ScriptLog::OnGameProcessBefore()
{
    if (!CTimer::m_UserPause && !CTimer::m_CodePause) m_processingGame = true;

    // 'clog' cheat
    if (_strnicmp(CCheat::m_CheatString, "golc", 4) == 0) // reversed order
    {
        CCheat::m_CheatString[0] = '\0'; // consume the cheat

        state = (state == LoggingState::Full) ? LoggingState::OnCrash : LoggingState::Full; // toggle
        if (state == LoggingState::Full) LoadConfig(true); // refresh config from ini file

        if (state == LoggingState::Full)
        {
            CHud::SetHelpMessage("CLEO script log: ~g~~h~~h~~h~ON~s~", true, false, false);
            LogLine("Log: ON\n");
        }
        else
        {
            CHud::SetHelpMessage("CLEO script log: ~r~~h~~h~~h~OFF~s~", true, false, false);
            LogLine("Log: OFF");
            LogWriteFile(); // flush
        }
    }

    if (state == LoggingState::Disabled) return;

    // count active scripts
    if (m_processingGame)
    {
        size_t scriptCount = 0;
        auto next = CTheScripts::pActiveScripts;
        while (next)
        {
            scriptCount++;
            next = next->m_pNext;
        }

        static std::string line; // keep for performance
        line.clear();
        line += "<gameProcess frame='";
        StringAppendNum(line, CTimer::m_FrameCounter);
        line += "' activeScripts='";
        StringAppendNum(line, scriptCount);
        line += "'>";
        LogLine(line);
    }
}

void ScriptLog::OnGameProcessAfter()
{
    SetCurrScript(nullptr);

    if (state != LoggingState::Disabled)
    {
        if (m_processingGame) LogLine("</gameProcess>\n");
    }

    m_processingGame = false;
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
    SetCurrScript(script);
    m_currScriptCommandCount++;

    if (state == LoggingState::Disabled) return OR_NONE;
    if (logCustomScriptsOnly && !script->IsCustom()) return OR_NONE;

    m_currCommandReturnParams = nullptr;
    m_conditionResultUpdated = false;
    m_conditionResultExpected = script->NotFlag || m_prevCommand == COMMAND_ANDOR || script->LogicalOp != eLogicalOperation::NONE;
    if (opcode == 0x0AB1) m_conditionResultExpected = false; // cleo_call: condition result set on return

    auto oriIP = script->CurrentIP;

    static std::string line; // keep for performance
    line.clear();

    if (m_processingGame) line += Block_Indent; // game processing block indentation
    if (m_currScript) line += Block_Indent; // script processing block indentation

    // script offset
    if (logOffsets)
    {
        auto offset = CLEO_GetScriptBaseRelativeOffset(script, script->CurrentIP);
        offset -= sizeof(WORD); // command opcode was already consumed

        auto digits = offset <= 0 ? 1 : (int)(log10(offset) + 1);
        for (int i = 7 - digits; i > 0; i--) line += ' '; // pad to constant width
        line += '{';
        StringAppendNum(line, offset);
        line += "}   ";
    }

    // command opcode
    if (logOpcodes)
    {
        line += '{';
        StringAppendHex(line, opcode | (script->NotFlag ? 0x8000 : 0), 4);
        line += ":} ";
    }

    // if block conditions indentation
    if (m_prevCommand == COMMAND_ANDOR || script->LogicalOp != eLogicalOperation::NONE) line += Script_Indent;

    // gosub scope indentation
    for (auto i = script->SP; i > 0; i--) line += Script_Indent;

    // cleo_call scope indentation
    for (auto i = CLEO_GetScriptCleoStackSize(script); i > 0; i--) line += Script_Indent;

    // negation
    if (script->NotFlag) line += "not ";

    auto command = m_opcodeDatabase.GetCommand((uint16_t)opcode);

    if (command == nullptr) // not documented in database?
    {
        line += "command_";
        StringAppendHex(line, opcode, 4);
        LogLine(line);
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
                ScriptParamInfo paramInfo(script); // just skip
            }
            else
            {
                if (i == command->inputArguments) m_currCommandReturnParams = script->CurrentIP; // keep pointer to return params start

                if (!line.empty() && line.back() != ' ') line += ' '; // separator
                LogScriptParam(line, script, command, i, returnArgCount > 1, true, false);
            }
        }
        script->CurrentIP = oriIP; // restore original position

        line += " = ";
    }
    else if(!command->oper.empty() && !command->arguments.empty() && !command->IsComparison())
    {
        m_currCommandReturnParams = oriIP; // operator result is first param
    }

    // command name
    if (command->oper.empty())
    {
        line += command->nameLower;
    }

    // command params
    if (opcode == COMMAND_ANDOR)
    {
        ScriptParamInfo paramInfo(script);
        if (paramInfo.value.nParam >= eLogicalOperation::ORS_1)
            line += " or";
        else
        if (paramInfo.value.nParam >= eLogicalOperation::ANDS_1)
            line += " and";
    }
    else
    {
        for (size_t i = 0; i < command->inputArguments; i++)
        {
            if (!line.empty() && line.back() != ' ') line += ' '; // separator

            // operator
            if (!command->oper.empty())
            {
                if (command->inputArguments == 1 && i == 0) // %param
                {
                    line += command->oper;
                }
                else if (command->inputArguments == 2 && i == 1) // param % param
                {
                    line += command->oper;
                    if (!command->IsComparison() && command->oper.find('=') == std::string::npos) line += '='; // param %= param
                    line += " ";
                }
            }

            if (command->arguments[i].type != OpcodeInfoDatabase::CommandArgumentType::Arguments)
            {
                LogScriptParam(line, script, command, i, true, true, true);
            }
            else // varArgs
            {
                bool first = true;
                auto type = script->PeekDataType();
                while (type != eDataType::DT_END)
                {
                    if (!first) line += " ";
                    LogScriptParam(line, script, command, i, first, true, true);

                    first = false;
                    type = script->PeekDataType();
                }
            }
        }
    }

    LogLine(line);

    script->CurrentIP = oriIP; // restore original position
    m_prevCommand = (WORD)opcode;
    return OR_NONE;
}

CLEO::OpcodeResult ScriptLog::OnScriptOpcodeProcessAfter(CLEO::CRunningScript* script, DWORD opcode, CLEO::OpcodeResult result)
{
    if (state == LoggingState::Disabled) return result;
    if (logCustomScriptsOnly && !script->IsCustom()) return result;
    
    auto command = m_opcodeDatabase.GetCommand((uint16_t)opcode);
    if (!command) return result;

    static std::string line; // keep for performance
    line.clear();

    // extra annotations
    bool hasComment = false;

    if (command->isNop)
    {
        line += hasComment ? ", " : " // ";
        line += "NOP command";
        hasComment = true;
    }

    if (m_conditionResultExpected && !m_conditionResultUpdated && opcode != 0x0AB1) // assume cleo_call always sets condition result
    {
        line += hasComment ? ", " : " // ";
        line += "BUG: non-logical command";
        hasComment = true;
    }
    
    // print values of return arguments
    bool hasReturnVal = false;

    if (m_currCommandReturnParams != nullptr)
    {
        auto oriIp = script->CurrentIP;
        script->CurrentIP = m_currCommandReturnParams;

        size_t i = command->oper.empty() ? command->inputArguments : 0;
        size_t count = command->oper.empty() ? command->arguments.size() : 1;
        for (; i < count; i++)
        {
            if (!hasReturnVal) line += hasComment ? " -> " : " // -> ";
            else line += ", "; // separator

            LogScriptParam(line, script, command, i, false, false, true);

            hasComment = true;
            hasReturnVal = true;
        }
        script->CurrentIP = oriIp;
    }

    if (m_conditionResultUpdated)
    {
        if (!hasReturnVal) line += hasComment ? " -> " : " // -> ";
        else line += ", "; // separator

        line += "condition: ";
        line += m_conditionResultValue ? "true" : "false";

        hasComment = true;
        hasReturnVal = true;
    }

    if (hasComment) LogLineAppend(line);

    return result;
}

void ScriptLog::OnDrawingFinished()
{
    // late initialization
    if (!m_initialized)
    {
        m_patchSetConditionResult = MemPatchJump(gaddrof(::CRunningScript::UpdateCompareFlag), &HOOK_SetConditionResult);

        m_initialized = true;
    }

    // this should be done just at the end of scripts processing
    // but PluginSDK, SAMP etc. call single scripts/callbacks outside of the processing queue
    // make sure current script does not persists to next render frame
    SetCurrScript(nullptr);

    if (state == LoggingState::Full) LogWriteFile();

    if (state != LoggingState::Full) // keep only current frame
    {
        // TODO: thread lock
        if (m_logBuffer->rdbuf()) m_logBuffer->rdbuf()->pubseekpos(0);
        m_logBuffer->clear(); // reset flags
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
