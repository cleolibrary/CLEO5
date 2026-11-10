#pragma once
#include "CLEO.h"
#include "CLEO_Utils.h"
#include "OpcodeInfoDatabase.h" // from CLEO core
#include <ctime>
#include <fstream>
#include <deque>
#include <optional>
#include <string>

class ScriptLog
{
  public:
    ScriptLog();
    ScriptLog(const ScriptLog&) = delete; // no copying!
    ~ScriptLog();

    enum LoggingState
    {
        Disabled,
        OnCrash,
        Full
    } state                  = OnCrash;
    size_t maxFileSize       = 250 * 1024 * 1024; // 250 MB
    int logCustomScriptsOnly = false;
    bool logDebugScriptsOnly = false;
    bool logOffsets          = true;
    bool logOpcodes          = false;

    static LoggingState GetConfiguredState();
    void SetState(LoggingState newState);
    void LoadConfig(bool keepState = false);

  private:
    // One frame worth of tracing data
    static constexpr size_t Initial_Buff_Size = 256 * 1024; // 256 KB
    const char* Script_Indent                 = "    ";

    static ScriptLog* g_Instance;

    bool m_hookInstalled = false; // CRunningScript::UpdateCompareFlag is currently hooked
    // state requested by the config, applied at the next safe point (DrawingFinished)
    std::optional<LoggingState> m_pendingState;

    bool m_databaseLoaded = false;
    void EnsureDatabaseLoaded();

    OpcodeInfoDatabase m_opcodeDatabase;

    CLEO::CRunningScript* m_currScript = nullptr;
    clock_t m_currScriptStartTime      = 0;
    bool m_currScriptLogging           = true;
    BYTE* m_currCommandReturnParams    = nullptr;
    void SetCurrScript(CLEO::CRunningScript* script);

    void RegisterCallbacks();
    void UnregisterCallbacks();
    void ApplyPendingState();

    // UpdateCompareFlag hook machinery. Needed for backward compatibility with ScrLog
    void InstallConditionHook();
    void UninstallConditionHook();

    std::string m_logBuffer;
    std::string m_logFilePath;
    std::ofstream m_logFile;

    void LogLine(const std::string_view& str);
    void LogFormattedLine(const char* format, ...); // slow!

    // extend last log line
    void LogAppend(char ch);
    void LogAppend(const std::string_view& str);
    void LogAppendNum(int number, int padLen = 0);
    void LogAppendHex(int number, int padLen = 0);
    void LogAppendFloat(float number, int padLen = 0);
    void LogAppendSpace(); // add white char separator at end of the line if not present
    void LogSeparator();
    void LogNewLine();
    bool LogAppendScriptParam(
        CLEO::CRunningScript* script, const OpcodeInfoDatabase::Command* command, size_t paramIdx, bool logName,
        bool logVariable, bool logValue
    ); // return true if param was global variable

    void LogWriteFile(bool forceUpdate = false);
    void LogFileDelete();

    bool m_customMain  = false;
    WORD m_prevCommand = 0xFFFF;

    // event handlers
    static void SetConditionResult(CLEO::CRunningScript* script, bool state);
    CLEO::MemPatch m_patchSetConditionResult;
    bool m_conditionResultUpdated  = false;
    bool m_conditionResultValue    = false;
    bool m_conditionResultExpected = false;

    void OnGameBegin(DWORD saveSlot);
    static void __stdcall callbackGameBegin(DWORD saveSlot) { g_Instance->OnGameBegin(saveSlot); };

    void OnGameProcessBefore();
    static void __stdcall callbackGameProcessBefore() { g_Instance->OnGameProcessBefore(); };

    void OnGameProcessAfter();
    static void __stdcall callbackGameProcessAfter() { g_Instance->OnGameProcessAfter(); };

    bool OnScriptProcessBefore(CLEO::CRunningScript* script);
    static bool __stdcall callbackScriptProcessBefore(CLEO::CRunningScript* script)
    {
        return g_Instance->OnScriptProcessBefore(script);
    };

    void OnScriptProcessAfter(CLEO::CRunningScript* script);
    static void __stdcall callbackScriptProcessAfter(CLEO::CRunningScript* script)
    {
        g_Instance->OnScriptProcessAfter(script);
    };

    CLEO::OpcodeResult OnScriptOpcodeProcessBefore(CLEO::CRunningScript* script, DWORD opcode);
    static CLEO::OpcodeResult __stdcall callbackScriptOpcodeProcessBefore(CLEO::CRunningScript* script, DWORD opcode)
    {
        return g_Instance->OnScriptOpcodeProcessBefore(script, opcode);
    };

    CLEO::OpcodeResult OnScriptOpcodeProcessAfter(
        CLEO::CRunningScript* script, DWORD opcode, CLEO::OpcodeResult result
    );
    static CLEO::OpcodeResult __stdcall callbackScriptOpcodeProcessAfter(
        CLEO::CRunningScript* script, DWORD opcode, CLEO::OpcodeResult result
    )
    {
        return g_Instance->OnScriptOpcodeProcessAfter(script, opcode, result);
    };

    void OnConfigUpdate(const char* section, const char* key);
    static void __stdcall callbackConfigUpdate(const char* section, const char* key)
    {
        g_Instance->OnConfigUpdate(section, key);
    };

    void OnDrawingFinished();
    static void __stdcall callbackDrawingFinished() { g_Instance->OnDrawingFinished(); };

    void OnMainWindowFocus(bool active);
    static void __stdcall callbackMainWindowFocus(bool active) { g_Instance->OnMainWindowFocus(active); };
};
