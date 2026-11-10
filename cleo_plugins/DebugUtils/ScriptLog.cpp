// Log content generation and the actual write to cleo_script.log.

#include "ScriptLog.h"
#include "ScriptUtils.h" // from CLEO core
#include <CRunningScript.h>
#include <CTheScripts.h>
#include <CTimer.h>
#include <cstring>
#include <string>

using namespace CLEO;

void ScriptLog::SetCurrScript(CLEO::CRunningScript* script)
{
    // close previous script's log block
    if (m_currScript != nullptr && script != m_currScript && state != LoggingState::Disabled && m_currScriptLogging)
    {
        LogAppend("  |-- Executed in ");
        LogAppendNum(size_t(clock() - m_currScriptStartTime) * 1000 / CLOCKS_PER_SEC);
        LogAppend(" ms");
        LogLine("  |");
    }

    // find index of this script in active scripts queue
    int scriptIdx = -1;
    int i         = 0;
    for (auto s = (CLEO::CRunningScript*)CTheScripts::pActiveScripts; s; s = s->Next)
    {
        i++; // 1-based display
        if (s == script)
        {
            scriptIdx = i;
            break;
        }
    }

    // update m_currScriptLogging state
    if (script)
    {
        m_currScriptLogging = true;

        // custom scripts
        if (m_currScriptLogging && !script->IsCustom() && scriptIdx != -1) // native scripts in active scripts queue
        {
            switch (logCustomScriptsOnly)
            {
            case -1:
                m_currScriptLogging = m_customMain;
                break; // automatic: log if not defaul main.scm
            case 1:
                m_currScriptLogging = false;
                break; // only custom
                // default: // log all
            }
        }

        // debug mode scripts
        if (m_currScriptLogging && logDebugScriptsOnly)
        {
            m_currScriptLogging =
                CLEO_GetScriptDebugMode(script) ||                      // debug mode on
                (*(WORD*)script->GetBytePointer()) == COMMAND_DEBUG_ON; // debug mode just about to be enabled
        }
    }
    else
        m_currScriptLogging = false;

    // start new script's log block
    if (script != nullptr && script != m_currScript && state != LoggingState::Disabled && m_currScriptLogging)
    {
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

            if (StringStartsWith(filename, CLEO_GetUserDirectory(), false))
            {
                FilepathRemoveParent(filename, CLEO_GetUserDirectory());
                filename = "user:\\" + filename;
            }
            else
                FilepathRemoveParent(filename, CLEO_GetGameDirectory());
        }
        else
        {
            filename = "?";
        }

        LogNewLine();
        LogAppend(" -|-- Script ");
        LogAppendNum(scriptIdx);
        LogAppend(": ");
        LogAppend(filename);

        // Add flags in parentheses
        bool hasFlags = false;
        if (script->IsCustom() || script->IsMission() || CLEO_GetScriptDebugMode(script))
        {
            LogAppend(" - ");
            if (script->IsCustom())
            {
                LogAppend("custom");
                hasFlags = true;
            }
            if (script->IsMission())
            {
                if (hasFlags) LogAppend(", ");
                LogAppend("mission");
                hasFlags = true;
            }
            if (CLEO_GetScriptDebugMode(script))
            {
                if (hasFlags) LogAppend(", ");
                LogAppend("debug");
                hasFlags = true;
            }
        }

        auto ver = CLEO_GetScriptVersion(script);
        if (ver != eCLEO_Version::CLEO_VER_CUR)
        {
            LogAppend(hasFlags ? ", " : " - ");
            switch (ver)
            {
            case eCLEO_Version::CLEO_VER_3:
                LogAppend("CLEO3 compat mode");
                break;
            case eCLEO_Version::CLEO_VER_4:
                LogAppend("CLEO4 compat mode");
                break;
            case eCLEO_Version::CLEO_VER_5:
                LogAppend("CLEO5 compat mode");
                break;
            default:
                LogAppendHex(ver);
                break;
            }
        }

        // call stack info
        auto cleoStack = CLEO_GetCleoCallStackSize(script);
        if (cleoStack || script->SP)
        {
            LogNewLine();
            LogAppend(" -|   Call Stacks - cleo_call: ");
            LogAppendNum(cleoStack);
            LogAppend(", gosub: ");
            LogAppendNum(script->SP);

            bool savingEnabled = !script->IsCustom() || false; // TODO: check CLEO save enabled for custom script
            if (savingEnabled && cleoStack)
            {
                LogAppend(" \\ BUG: Script saving enabled. Saving during cleo_call would result in errors!");
            }

            bool wastedBustedCheck = script->IsMission() && script->bWastedBustedCheck;
            if (wastedBustedCheck && cleoStack)
            {
                LogAppend(
                    " \\ BUG: Mission death-arrest-check active. Executing death-arrest procedure during cleo_call "
                    "would result in errors!"
                );
            }
            else if (wastedBustedCheck && script->SP == 0) // Rockstar's bug?
            {
                LogAppend(
                    " \\ BUG: Mission death-arrest-check requires at least one GOSUB level. Executing death-arrest "
                    "procedure now would result in errors!"
                );
            }
        }

        // 'wait' command in progress?
        if (script->WakeTime > CTimer::m_snTimeInMilliseconds)
        {
            LogNewLine();
            LogAppend("  |   Sleeping for ");
            LogAppendNum(script->WakeTime - CTimer::m_snTimeInMilliseconds);
            LogAppend(" ms more");
        }

        LogNewLine();
    }

    // update/reset script stats
    if (script != m_currScript || script == nullptr)
    {
        m_currScript          = script;
        m_currScriptStartTime = clock();
        m_prevCommand         = 0xFFFF;
    }
}

void ScriptLog::LogLine(const std::string_view& str)
{
    // TODO: thread lock
    LogNewLine();
    LogAppend(str);
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

void ScriptLog::LogAppend(char ch)
{
    // TODO: thread lock
    m_logBuffer += ch;
    // TODO: thread unlock
}

void ScriptLog::LogAppend(const std::string_view& str)
{
    // TODO: thread lock
    m_logBuffer += str;
    // TODO: thread unlock
}

void ScriptLog::LogAppendNum(int number, int padLen)
{
    // TODO: thread lock
    StringAppendNum(m_logBuffer, number, padLen);
    // TODO: thread unlock
}

void ScriptLog::LogAppendHex(int number, int padLen)
{
    // TODO: thread lock
    StringAppendHex(m_logBuffer, number, padLen);
    // TODO: thread unlock
}

void ScriptLog::LogAppendFloat(float number, int padLen)
{
    // TODO: thread lock
    StringAppendFloat(m_logBuffer, number, padLen);
    // TODO: thread unlock
}

void ScriptLog::LogAppendSpace()
{
    // TODO: thread lock
    if (!m_logBuffer.empty() && m_logBuffer.back() != ' ' && m_logBuffer.back() != '\t')
    {
        m_logBuffer += ' ';
    }
    // TODO: thread unlock
}

void ScriptLog::LogSeparator()
{
    LogAppend("------------------------------------------------");
    LogNewLine();
}

void ScriptLog::LogNewLine()
{
    LogAppend('\n');
}

bool ScriptLog::LogAppendScriptParam(
    CLEO::CRunningScript* script, const OpcodeInfoDatabase::Command* command, size_t paramIdx, bool logName,
    bool logVariable, bool logValue
)
{
    bool hasName = false;
    if (logName)
    {
        if (!command->arguments[paramIdx].name.empty())
        {
            hasName = true;
            LogAppend('{');
            LogAppend(command->arguments[paramIdx].name);
            LogAppend('}');
        }
    }

    ScriptParamInfo paramInfo(script);

    bool isGlobalVar;
    switch (paramInfo.type)
    {
    case DT_VAR:
    case DT_VAR_ARRAY:
    case DT_VAR_TEXTLABEL:
    case DT_VAR_TEXTLABEL_ARRAY:
    case DT_VAR_STRING:
    case DT_VAR_STRING_ARRAY:
        isGlobalVar = true;
        break;

    default:
        isGlobalVar = false;
    }

    bool hasVariable = false;
    if (logVariable)
    {
        switch (paramInfo.type)
        {
        case DT_END:
            /*hasVariable = true;
            if (hasName) LogAppend(' ';
            LogAppend("ArgsEnd";*/
            break;

        case DT_VAR:
        case DT_VAR_ARRAY:
        case DT_VAR_TEXTLABEL:
        case DT_VAR_TEXTLABEL_ARRAY:
        case DT_VAR_STRING:
        case DT_VAR_STRING_ARRAY:
        case DT_LVAR:
        case DT_LVAR_ARRAY:
        case DT_LVAR_TEXTLABEL:
        case DT_LVAR_TEXTLABEL_ARRAY:
        case DT_LVAR_STRING:
        case DT_LVAR_STRING_ARRAY:
            hasVariable = true;
            if (hasName) LogAppend(' ');
            if (isGlobalVar) LogAppend('$');
            LogAppendNum(paramInfo.varIndex);
            if (!isGlobalVar) LogAppend('@');

            if (paramInfo.arrayType != eArrayType::AT_NONE)
            {
                auto idxVar = GetScriptVar(script, paramInfo.arrayFlags & ATF_INDEX_GLOBAL, paramInfo.arrayIndexVar);

                LogAppend("[ ");
                if (paramInfo.arrayFlags & ATF_INDEX_GLOBAL) LogAppend('$');
                LogAppendNum(paramInfo.arrayIndexVar);
                if ((paramInfo.arrayFlags & ATF_INDEX_GLOBAL) == 0) LogAppend('@');
                LogAppend('(');
                if (abs(idxVar->nParam) >= MinValidAddress)
                {
                    LogAppend("0x");
                    LogAppendHex(idxVar->dwParam);
                }
                else
                    LogAppendNum(idxVar->nParam);
                LogAppend(") ]");
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
        if (hasVariable)
            LogAppend('(');
        else if (hasName)
            LogAppend(' ');

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
                            LogAppend(e->name);
                            LogAppend('.');
                            LogAppend(entry);
                        }
                        else
                        {
                            // TODO: thread lock
                            paramInfo.ValueToString(m_logBuffer); // print according to param type in script
                            // TODO: thread unlock
                        }
                    }
                    else
                    {
                        // TODO: thread lock
                        paramInfo.ValueToString(m_logBuffer); // print according to param type in script
                        // TODO: thread unlock
                    }
                }
                else // text enums
                {
                    auto entry = e->GetEntryName(std::string(paramInfo.GetText()).c_str());
                    if (entry)
                    {
                        LogAppend(e->name);
                        LogAppend('.');
                        LogAppend(entry);
                    }
                    else
                    {
                        // TODO: thread lock
                        paramInfo.ValueToString(m_logBuffer); // print according to param type in script
                        // TODO: thread unlock
                    }
                }
            }
            else // class or unknown enum
            {
                // TODO: thread lock
                paramInfo.ValueToString(m_logBuffer); // print according to param type in script
                // TODO: thread unlock
            }
            break;
        }

            // case OpcodeInfoDatabase::CommandArgumentType::Any:
            // case OpcodeInfoDatabase::CommandArgumentType::Arguments:

        case OpcodeInfoDatabase::CommandArgumentType::Bool:
            switch (paramInfo.value.nParam)
            {
            case 0:
                LogAppend("false");
                break;
            case 1:
                LogAppend("true");
                break;
            default:
                LogAppendNum(paramInfo.value.nParam);
                break;
            }
            break;

        case OpcodeInfoDatabase::CommandArgumentType::Float:
            LogAppendFloat(paramInfo.value.fParam);
            break;

        case OpcodeInfoDatabase::CommandArgumentType::GxtKey:
        case OpcodeInfoDatabase::CommandArgumentType::ZoneKey:
        case OpcodeInfoDatabase::CommandArgumentType::String:
        case OpcodeInfoDatabase::CommandArgumentType::String128:
            LogAppend((paramInfo.GetBaseType() == DT_TEXTLABEL) ? '\'' : '"');
            LogAppend(paramInfo.GetText());
            LogAppend((paramInfo.GetBaseType() == DT_TEXTLABEL) ? '\'' : '"');
            break;

        case OpcodeInfoDatabase::CommandArgumentType::Int:
            if (paramInfo.GetBaseType() == DT_DWORD || paramInfo.GetBaseType() == DT_VAR)
            {
                // TODO: thread lock
                paramInfo.ValueToString(m_logBuffer);
                // TODO: thread unlock
            }
            else // expected integer, got something else. Print as hex
            {
                LogAppend("0x");
                LogAppendHex(paramInfo.value.nParam);
            }
            break;

        case OpcodeInfoDatabase::CommandArgumentType::Label:
            LogAppend("@LABEL_");
            LogAppendNum(abs(paramInfo.value.nParam));
            break;

        case OpcodeInfoDatabase::CommandArgumentType::ModelAny:
        case OpcodeInfoDatabase::CommandArgumentType::ModelChar:
        case OpcodeInfoDatabase::CommandArgumentType::ModelObject:
        case OpcodeInfoDatabase::CommandArgumentType::ModelVehicle:
            LogAppendNum(paramInfo.value.nParam);
            break;

        case OpcodeInfoDatabase::CommandArgumentType::ScriptId:
            LogAppend("0x");
            LogAppendHex(paramInfo.value.nParam);
            break;

        default: {
            // TODO: thread lock
            paramInfo.ValueToString(m_logBuffer); // print according to param type in script
            // TODO: thread unlock
        }
        }

        if (hasVariable) LogAppend(')');
    }

    return isGlobalVar;
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
    // std::swap(m_logBuffer, m_logFileBuffer);
    // unlock
    std::string& m_logFileBuffer = m_logBuffer; // no double buffering in single thread

    if (m_logFileBuffer.empty())
    {
        return; // empty
    }

    if (maxFileSize > 0 && fileSize > maxFileSize)
    {
        LogFileDelete();
    }

    if (!m_logFile.is_open())
    {
        m_logFile.clear(); // clear flags
        m_logFile.open(m_logFilePath.c_str(), std::ios::app);
    }

    if (m_logFile.is_open())
    {
        m_logFile << m_logFileBuffer;
        fileSize = (size_t)m_logFile.tellp();
    }
    else
    {
        LOG_WARNING(0, "Failed to open script log file!");
    }

    m_logBuffer.clear();
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

void ScriptLog::EnsureDatabaseLoaded()
{
    if (m_databaseLoaded) return;

    std::string path = CLEO_GetCleoDirectory();
    path += "\\.config\\sa.json";
    m_opcodeDatabase.LoadCommands(path.c_str());

    path = CLEO_GetCleoDirectory();
    path += "\\.config\\enums.json";
    m_opcodeDatabase.LoadEnums(path.c_str());

    m_databaseLoaded = true;
}
