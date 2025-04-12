#pragma once
#include "CScriptEngine.h"

namespace CLEO
{
    class CCustomScript : public CRunningScript
    {
        friend class CScriptEngine;
        friend struct ThreadSavingInfo;

        bool m_ok = false;

        eCLEO_Version m_compatVer = CLEO_VER_CUR;
        DWORD m_codeSize = 0;
        DWORD m_codeChecksum = 0;

        CCustomScript* m_parentThread = nullptr;
        std::list<CCustomScript*> m_childThreads = {};

        bool m_saveEnabled = false;

        // script draws
        bool m_drawingNow = false;
        bool m_useTextCommands = false;
        std::vector<tScriptRectangle> m_scriptDraws;
        std::array<CSprite2d, CScriptEngine::Script_Sprites_Capacity> m_scriptSprites = {}; // loaded textures
        std::vector<tScriptText> m_scriptTexts;

        bool m_debugMode = false; // debug opcodes enabled

        std::string m_scriptFileDir;
        std::string m_scriptFileName;
        std::string m_workDir;

    public:
        CCustomScript(const char* szFileName, bool bIsMiss = false, CRunningScript* parent = nullptr, int label = 0);
        CCustomScript(const CCustomScript&) = delete; // no copying
        ~CCustomScript();

        inline bool IsOk() const { return m_ok; }
        inline DWORD GetCodeSize() const { return m_codeSize; }
        inline DWORD GetCodeChecksum() const { return m_codeChecksum; }

        inline bool GetSavingEnabled() const { return m_saveEnabled; }
        inline void SetSavingEnabled(bool enabled) { m_saveEnabled = enabled; }

        inline eCLEO_Version GetCompatibility() const { return m_compatVer; }
        inline void SetCompatibility(eCLEO_Version ver) { m_compatVer = ver; }

        void AddScriptToList(CRunningScript** queuelist);
        void RemoveScriptFromList(CRunningScript** queuelist);

        void Process();
        void ShutdownThisScript();

        void Draw(uchar bBeforeFade);
        CSprite2d* GetScriptSprite(size_t index); // script drawing textures loaded by this script

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

