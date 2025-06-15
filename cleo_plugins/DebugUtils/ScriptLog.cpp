#include "ScriptLog.h"
#include "CLEO_Utils.h"
#include "crc32.h" // from CLEO core
#include <CCheat.h>
#include <CGame.h>
#include <CHud.h>
#include <CMenuManager.h>
#include <CTheScripts.h>
#include <CTimer.h>
#include <memory>
#include <sstream>
#include <string>

using namespace CLEO;


ScriptLog* ScriptLog::g_Instance = nullptr;

CLEO::SCRIPT_VAR* GetScriptVar(CLEO::CRunningScript* script, bool global, size_t index)
{
    SCRIPT_VAR* vars;
    if (global)
        vars = (SCRIPT_VAR*)CTheScripts::ScriptSpace;
    else
        vars = script->IsMission() ? (SCRIPT_VAR*)CTheScripts::LocalVariablesForCurrentMission : script->LocalVar;

    return &vars[index];
}

struct ScriptParamInfo
{
    eDataType type = eDataType::DT_INVALID;
    eArrayDataType arrayType = eArrayDataType::ADT_NONE;
    WORD varIndex = 0;
    WORD arrayIndexVar = 0; // index of variable storing array index
    WORD arraySize = 0;
    WORD arrayFlags = 0;

    SCRIPT_VAR value = { 0 };
    size_t stringLen = 0;

    ScriptParamInfo() = default;

    // Create from script
    // Advances instruction pointer forward
    // Ommits all param processing functions/hooks
    ScriptParamInfo(CLEO::CRunningScript* script)
    {
        // type
        type = script->PeekDataType();
        arrayType = IsArray(type) ? script->PeekArrayDataType() : eArrayDataType::ADT_NONE;
        script->IncPtr(sizeof(eDataType));

        // type properties
        if (arrayType != eArrayDataType::ADT_NONE)
        {
            varIndex = script->ReadDataVarIndex();
            arrayIndexVar = script->ReadDataArrayIndexVar();
            arraySize = script->ReadDataArraySize();
            arrayFlags = script->ReadDataArrayFlags();

            if (arrayFlags & ADTF_INDEX_GLOBAL) arrayIndexVar /= 4; // stored as byte offset
        }
        else if (IsVariable(type) || IsVarString(type))
        {
            varIndex = script->ReadDataVarIndex();
        }

        // value
        size_t varStride; // num of variable slots
        switch (type)
        {
            case DT_VAR_TEXTLABEL:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_LVAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL_ARRAY:
                varStride = 2;
                break;

            case DT_VAR_STRING:
            case DT_VAR_STRING_ARRAY:
            case DT_LVAR_STRING:
            case DT_LVAR_STRING_ARRAY:
                varStride = 4;
                break;

            case DT_VAR:
            case DT_VAR_ARRAY:
            case DT_LVAR:
            case DT_LVAR_ARRAY:
                varStride = 1;
                break;

            default:
                varStride = 0;
        }

        bool isGlobalVar = false;
        switch (type)
        {
            // immediate numbers
            case DT_BYTE: value.dwParam = script->ReadDataByte(); break;
            case DT_WORD: value.dwParam = script->ReadDataWord(); break;
            case DT_DWORD: value.dwParam = script->ReadDataDword(); break;
            case DT_FLOAT: value.dwParam = script->ReadDataDword(); break; // 4 bytes

            // immediate texts
            case DT_TEXTLABEL:
            case DT_STRING:
                value.pParam = script->CurrentIP;
                stringLen = strnlen(value.pcParam, type == DT_TEXTLABEL ? 8 : 16);
                script->IncPtr(8);
                break;

            case DT_VARLEN_STRING:
                stringLen = script->ReadDataByte();
                value.pParam = script->CurrentIP;
                script->IncPtr(stringLen);
                break;

            // global variables
            case DT_VAR:
            case DT_VAR_ARRAY:
            case DT_VAR_TEXTLABEL:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_VAR_STRING:
            case DT_VAR_STRING_ARRAY:
                isGlobalVar = true;
                varIndex /= 4; // stored as byte offset
                [[fallthrough]];

            // local variables
            case DT_LVAR:
            case DT_LVAR_ARRAY:
            case DT_LVAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL_ARRAY:
            case DT_LVAR_STRING:
            case DT_LVAR_STRING_ARRAY:
            {
                SCRIPT_VAR* var;
                if (arrayType == ADT_NONE)
                {
                    var = GetScriptVar(script, isGlobalVar, varIndex);
                }
                else
                {
                    auto idxVar = GetScriptVar(script, arrayFlags & ADTF_INDEX_GLOBAL, arrayIndexVar);
                    var = GetScriptVar(script, isGlobalVar, varIndex + idxVar->nParam * varStride);
                }

                if (IsVarString(type))
                {
                    value.pParam = var; // store pointer to variable contents
                    stringLen = strnlen(value.pcParam, varStride * sizeof(SCRIPT_VAR));
                }
                else
                    value = *var;
            }
            break;
        }
    }

    ~ScriptParamInfo() = default;

    eDataType GetBaseDataType() const
    {
        switch (type)
        {
            case DT_BYTE:
            case DT_WORD:
            case DT_DWORD:
                return DT_DWORD; // int

            case DT_FLOAT:
                return DT_FLOAT; // float

            case DT_VAR:
            case DT_LVAR:
            case DT_VAR_ARRAY:
            case DT_LVAR_ARRAY:
                return DT_VAR; // any 32 bit

            case DT_TEXTLABEL:
            case DT_VAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_LVAR_TEXTLABEL_ARRAY:
                return DT_TEXTLABEL; // 8 char string

            case DT_STRING:
            case DT_VAR_STRING:
            case DT_LVAR_STRING:
            case DT_VAR_STRING_ARRAY:
            case DT_LVAR_STRING_ARRAY:
            //case DT_VARLEN_STRING: other size
                return DT_STRING; // 16 char string

            default:
                return type;
        }
    }

    std::string_view GetText() const
    {
        if (IsImmString(type))
        {
            return std::string_view({ value.pcParam, stringLen });
        }
        
        if (IsVarString(type))
        {
            if (value.pParam == nullptr)
                return "NULL_PTR";
            else
            if (value.dwParam < MinValidAddress) 
                return "INVALID_PTR";
            else
                return { value.pcParam, strnlen(value.pcParam, 255) };
        }

        return "INVALID_TYPE"; // not a string type param
    }

    std::string ValueToString()
    {
        std::string result;
        ValueToString(result);
        return result;
    }

    void ValueToString(std::string& dest)
    {
        switch (type)
        {
            // int
            case DT_BYTE:
            case DT_WORD:
            case DT_DWORD:
                if (abs(value.nParam) >= MinValidAddress)
                {
                    dest += "0x";
                    StringAppendHex(dest, value.nParam);
                }
                else
                    StringAppendNum(dest, value.nParam);
                break;

            // float
            case DT_FLOAT:
                StringAppendPrintf(dest, "%g", value.fParam);
                break;

            // short string
            case DT_TEXTLABEL:
            case DT_VAR_TEXTLABEL:
            case DT_VAR_TEXTLABEL_ARRAY:
            case DT_LVAR_TEXTLABEL:
            case DT_LVAR_TEXTLABEL_ARRAY:
                dest += '\'';
                dest += GetText();
                dest += '\'';
                break;

            // long string
            case DT_STRING:
            case DT_VARLEN_STRING:
            case DT_VAR_STRING:
            case DT_VAR_STRING_ARRAY:
            case DT_LVAR_STRING:
            case DT_LVAR_STRING_ARRAY:
                dest += '\"';
                dest += GetText();
                dest += '\"';
                break;

            // variables
            case DT_VAR:
            case DT_LVAR:
            case DT_VAR_ARRAY:
            case DT_LVAR_ARRAY:
                if (abs(value.nParam) >= MinValidAddress)
                {
                    dest += "0x";
                    StringAppendHex(dest, value.nParam);
                }
                else
                    StringAppendNum(dest, value.nParam);
                break;
        }
    }
};

ScriptLog::ScriptLog()
{
    assert(g_Instance == nullptr);
    g_Instance = this;

    m_logBuffer = new std::ostringstream();
    m_logFileBuffer = new std::ostringstream();

    m_logFilePath = CLEO_GetGameDirectory();
    m_logFilePath += "\\cleo\\.cleo_script_log.xml";
    std::remove(m_logFilePath.c_str());

    CLEO_RegisterCallback(eCallbackId::GameBegin, callbackGameBegin);
    CLEO_RegisterCallback(eCallbackId::GameProcessBefore, callbackGameProcessBefore);
    CLEO_RegisterCallback(eCallbackId::GameProcessAfter, callbackGameProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessBefore, callbackScriptProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptProcessAfter, callbackScriptProcessAfter);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcess, callbackScriptOpcodeProcessBefore);
    CLEO_RegisterCallback(eCallbackId::ScriptOpcodeProcessFinished, callbackScriptOpcodeProcessAfter);
    CLEO_RegisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);

    LoadConfig();

    std::string path = CLEO_GetGameDirectory();
    path += "\\cleo\\.config\\sa.json";
    m_opcodeDatabase.LoadCommands(path.c_str());

    path = CLEO_GetGameDirectory();
    path += "\\cleo\\.config\\enums.txt";
    m_opcodeDatabase.LoadEnums(path.c_str());

    // TODO: start file writer thread
}

ScriptLog::~ScriptLog()
{
    // TODO: stop file writer thread

    CLEO_UnregisterCallback(eCallbackId::GameBegin, callbackGameBegin);
    CLEO_UnregisterCallback(eCallbackId::GameProcessBefore, callbackGameProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::GameProcessAfter, callbackGameProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessBefore, callbackScriptProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptProcessAfter, callbackScriptProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcess, callbackScriptOpcodeProcessBefore);
    CLEO_UnregisterCallback(eCallbackId::ScriptOpcodeProcessFinished, callbackScriptOpcodeProcessAfter);
    CLEO_UnregisterCallback(eCallbackId::DrawingFinished, callbackDrawingFinished);

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

void ScriptLog::LoadConfig(bool reload)
{
    auto config = GetConfigFilename();

    if (!reload)
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
    // close previous script's log block
    if (state != LoggingState::Disabled && 
        m_currScript != nullptr && script != m_currScript &&
        (!logCustomScriptsOnly || m_currScript->IsCustom()))
    {
        if (m_currScriptCommandCount > 0)
        {
            LogFormattedLine("%s\tExecuted in %zu ms",
                m_processingGame ? "\t" : "", // block indentation
                size_t(clock() - m_currScriptStartTime) * 1000 / CLOCKS_PER_SEC);
        }

        LogFormattedLine("%s</script>\n",
            m_processingGame ? "\t" : ""); // block indentation
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

        LogFormattedLine("%s<script idx='%d' name='%s' file='%s' custom='%s' mission='%s'>",
            m_processingGame ? "\t" : "", // block indentation
            idx,
            script->GetName().c_str(),
            filename.c_str(),
            script->IsCustom() ? "yes" : "no",
            script->IsMission() ? "yes" : "no");

        // call stack info
        auto cleoStack = CLEO_GetScriptCleoStackSize(script);
        if (cleoStack || script->SP)
        {
            LogFormattedLine("%s\tCall Stacks depth - cleo_call: %d, gosub: %d",
                m_processingGame ? "\t" : "", // block indentation
                script->SP,
                cleoStack);
        }

        // 'wait' in progress?
        if (CTimer::m_snTimeInMilliseconds < script->WakeTime)
        {
            auto pause = script->WakeTime - CTimer::m_snTimeInMilliseconds;

            LogFormattedLine("%s\tSleeping for %zu ms more",
                m_processingGame ? "\t" : "", // block indentation
                pause);
        }
    }

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
    *m_logBuffer << std::endl << line;
    // TODO: thread unlock
}

void ScriptLog::LogLine(const std::string& line)
{
    // TODO: thread lock
    *m_logBuffer << std::endl << line;
    // TODO: thread unlock
}

void ScriptLog::LogLineAppend(const char* line)
{
    // TODO: thread lock
    *m_logBuffer << line;
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
                hasVariable = true;
                if (hasName) dest += ' ';
                dest += "ArgsEnd";
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

                if (paramInfo.arrayType != eArrayDataType::ADT_NONE)
                {
                    auto idxVar = GetScriptVar(script,
                        paramInfo.arrayFlags & ADTF_INDEX_GLOBAL,
                        paramInfo.arrayIndexVar);

                    dest += "[ ";
                    if (paramInfo.arrayFlags & ADTF_INDEX_GLOBAL) dest += '$';
                    StringAppendNum(dest, paramInfo.arrayIndexVar);
                    if ((paramInfo.arrayFlags & ADTF_INDEX_GLOBAL) == 0) dest += '@';
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
                StringAppendPrintf(dest, "%g", paramInfo.value.fParam);
                break;

            case OpcodeInfoDatabase::CommandArgumentType::GxtKey:
            case OpcodeInfoDatabase::CommandArgumentType::ZoneGxt:
            case OpcodeInfoDatabase::CommandArgumentType::String:
            case OpcodeInfoDatabase::CommandArgumentType::String128:
                dest += (paramInfo.GetBaseDataType() == DT_TEXTLABEL) ? '\'' : '"';
                dest += paramInfo.GetText();
                dest += (paramInfo.GetBaseDataType() == DT_TEXTLABEL) ? '\'' : '"';
                break;
            
            case OpcodeInfoDatabase::CommandArgumentType::Int:
                if (abs(paramInfo.value.nParam) >= MinValidAddress)
                {
                    dest += "0x";
                    StringAppendHex(dest, paramInfo.value.nParam);
                }
                else
                    StringAppendNum(dest, paramInfo.value.nParam);
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

void ScriptLog::LogWriteFile()
{
    static size_t fileSize = 0;

    // TODO: thread lock
    std::swap(m_logBuffer, m_logFileBuffer);
    // unlock

    if (m_logFileBuffer->tellp() == 0) // empty
    {
        return;
    }

    int flags = std::ios::out;
    if (maxFileSize > 0 && fileSize > maxFileSize)
    {
        flags |= std::ios::trunc;
        LOG_WARNING(0, "CLEO script log file cleared");
    }
    else
        flags |= std::ios::app;

    m_logFile.open(m_logFilePath.c_str(), flags);

    if (m_logFile.is_open())
    {
        m_logFile << m_logFileBuffer << std::endl;
        m_logFileBuffer->clear();
        fileSize = (size_t)m_logFile.tellp();
        m_logFile.close();
        m_logFile.clear(); // clear flags
    }
    else
    {
        LOG_WARNING(0, "Failed to open script log file!");
    }

    m_logFileBuffer->clear();
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

        if (state == LoggingState::Full) CHud::SetHelpMessage("CLEO script log: ~g~~h~~h~~h~ON~s~", true, false, false);
        else CHud::SetHelpMessage("CLEO script log: ~r~~h~~h~~h~OFF~s~", true, false, false);
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

        LogFormattedLine("<gameProcess frame='%u' activeScripts='%u'>",
            CTimer::m_FrameCounter,
            scriptCount);
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

    auto oriIP = script->CurrentIP;

    static std::string line; // keep for performance
    line.clear();
    //line.reserve(128);

    if (m_processingGame) line += '\t'; // game processing block indentation
    if (m_currScript) line += '\t'; // script processing block indentation

    // script offset
    if (logOffsets)
    {
        auto base = (DWORD)script->BaseIP;
        if (base == 0) base = (DWORD)CTheScripts::ScriptSpace;
        auto currPos = (DWORD)script->GetBytePointer();
        if (script->IsMission() && !script->IsCustom())
        {
            if (currPos >= (DWORD)CTheScripts::MissionBlock)
            {
                // we are in mission code buffer
                // native missions are loaded from script file into mission block area
                // TODO: currPos += ((DWORD*)CTheScripts::MultiScriptArray)[missionIdx]; // start offset of this mission within source script file
            }
            else
            {
                base = (DWORD)CTheScripts::ScriptSpace; // seems that mission uses main scm code
            }
        }
        auto offset = currPos - sizeof(WORD) - base; // command opcode was already consumed

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
        return OR_NONE;
    }

    // command name
    line += command->nameLower;

    // command params
    if (opcode == COMMAND_ANDOR)
    {
        ScriptParamInfo paramInfo(script);
        if (paramInfo.value.nParam >= eLogicalOperation::OR_2)
            line += " or";
        else
        if (paramInfo.value.nParam >= eLogicalOperation::AND_2)
            line += " and";
    }
    else
    {
        for (size_t i = 0; i < command->arguments.size(); i++)
        {
            line += ' ';

            // operator before second param
            if (i == 1 && !command->oper.empty())
            {
                line += command->oper;
                line += ' ';
            }

            LogScriptParam(line, script, command, i, true, true, true);
        }
    }

    // comments
    std::deque<std::string> comments;

    if (command->isNop) comments.push_back("NOP command");
    if ((m_prevCommand == COMMAND_ANDOR || script->LogicalOp != eLogicalOperation::NONE) && !command->isCondition) comments.push_back("BUG: non-logical command!");

    if (!comments.empty())
    {
        bool first = true;
        for (auto& comment : comments)
        {
            if (first) { line += " // "; first = false; }
            else line += ", ";
            line += comment;
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

    bool first = true;

    static std::string line; // keep for performance
    line.clear();

    // print return values
    if (command->arguments.size() > command->inputArguments || command->isCondition)
    {
        line += " --> ";

        if (command->isCondition)
        {
            if (!first) { line += ", "; first = false; }
            line += "condition: ";
            line += script->bCondResult ? "true" : "false";
        }
    }

    if (!line.empty()) LogLineAppend(line.c_str());

    return result;
}

void ScriptLog::OnDrawingFinished()
{
    if (state == LoggingState::Full) LogWriteFile();

    if (state == LoggingState::OnCrash) // keep only current frame
    {
        // TODO: thread lock
        m_logBuffer->clear();
        // TODO: thread unlock
    }

    // this should be done just at the end of scripts processing
    // but PluginSDK, SAMP etc. call single scripts/callbacks outside of the processing queue
    // make sure current script does not persists to next render frame
    SetCurrScript(nullptr);
}
