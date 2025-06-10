#pragma once

namespace CLEO
{
    class CCustomScript : public CRunningScript
    {
        friend class CScriptEngine;
        friend class CCustomOpcodeSystem;
        friend struct ThreadSavingInfo;

        DWORD m_codeSize;
        DWORD m_codeChecksum;

        bool m_saveEnabled;
        bool m_ok;
        eCLEO_Version m_compatVer;
        CCustomScript* m_parentScript;
        std::list<CCustomScript*> m_childScripts;

        bool m_debugMode; // debug opcodes enabled

        std::string m_scriptFileDir;
        std::string m_scriptFileName;
        std::string m_workDir;

    public:
        inline SCRIPT_VAR* GetVarsPtr() { return LocalVar; }
        inline bool IsOk() const { return m_ok; }
        inline DWORD GetCodeSize() const { return m_codeSize; }
        inline DWORD GetCodeChecksum() const { return m_codeChecksum; }
        inline void EnableSaving(bool en = true) { m_saveEnabled = en; }
        inline void SetCompatibility(eCLEO_Version ver) { m_compatVer = ver; }
        inline eCLEO_Version GetCompatibility() const { return m_compatVer; }

        CCustomScript(const char* szFileName, bool bIsMiss = false, CRunningScript* parent = nullptr, int label = 0);
        CCustomScript(const CCustomScript&) = delete; // no copying
        ~CCustomScript();

        void AddScriptToList(CRunningScript** queuelist);
        void RemoveScriptFromList(CRunningScript** queuelist);

        void ShutdownThisScript();

        // debug related utils enabled?
        bool GetDebugMode() const;
        void SetDebugMode(bool enabled);

        // absolute path to directory where script's source file is located
        const char* GetScriptFileDir() const;
        void SetScriptFileDir(const char* directory);

        // filename with type extension of script's source file
        const char* GetScriptFileName() const;
        void SetScriptFileName(const char* filename);

        // absolute path to the script file
        std::string GetScriptFileFullPath() const;

        // current working directory of this script. Can be changed ith 0A99
        const char* GetWorkDir() const;
        void SetWorkDir(const char* directory);

        // create absolute file path
        std::string ResolvePath(const char* path, const char* customWorkDir = nullptr) const;

        // get short info text about script
        std::string GetInfoStr(bool currLineInfo = true) const;
    };
}

