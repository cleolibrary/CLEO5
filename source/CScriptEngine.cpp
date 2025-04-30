#include "stdafx.h"
#include "CleoBase.h"
#include "crc32.h"


namespace CLEO
{
    DWORD FUNC_ScriptOpcodeHandler00;
    DWORD FUNC_GetScriptParams;
    DWORD FUNC_TransmitScriptParams;
    DWORD FUNC_SetScriptParams;
    DWORD FUNC_SetScriptCondResult;
    DWORD FUNC_GetScriptParamPointer1;
    DWORD FUNC_GetScriptParamPointer2;

    char(__thiscall * ScriptOpcodeHandler00)(CRunningScript *, WORD opcode);
    void(__thiscall * GetScriptParams)(CRunningScript *, int count);
    void(__thiscall * TransmitScriptParams)(CRunningScript *, CRunningScript *);
    void(__thiscall * SetScriptParams)(CRunningScript *, int count);
    void(__thiscall * SetScriptCondResult)(CRunningScript *, bool);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer1)(CRunningScript *);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer2)(CRunningScript *, int __unused__);

    char __fastcall _ScriptOpcodeHandler00(CRunningScript *pScript, int dummy, WORD opcode)
    {
        int result;
        _asm
        {
            push opcode
            mov ecx, pScript
            call FUNC_ScriptOpcodeHandler00
            mov result, eax
        }
        return result;
    }

    void __fastcall _GetScriptParams(CRunningScript *pScript, int dummy, int count)
    {
        _asm
        {
            mov ecx, pScript
            push count
            call FUNC_GetScriptParams
        }

        CleoInstance.OpcodeSystem.handledParamCount += count;
    }

    void __fastcall _TransmitScriptParams(CRunningScript *pScript, int dummy, CRunningScript *pScriptB)
    {
        _asm
        {
            mov ecx, pScript
            push pScriptB
            call FUNC_TransmitScriptParams
        }
    }

    void __fastcall _SetScriptParams(CRunningScript *pScript, int dummy, int count)
    {
        _asm
        {
            mov ecx, pScript
            push count
            call FUNC_SetScriptParams
        }

        CleoInstance.OpcodeSystem.handledParamCount += count;
    }

    void __fastcall _SetScriptCondResult(CRunningScript *pScript, int dummy, int val)
    {
        _asm
        {
            mov ecx, pScript
            push val
            call FUNC_SetScriptCondResult
        }
    }

    SCRIPT_VAR * __fastcall _GetScriptParamPointer1(CRunningScript *pScript)
    {
        SCRIPT_VAR *result;
        _asm
        {
            mov ecx, pScript
            call FUNC_GetScriptParamPointer1
            mov result, eax
        }
        return (SCRIPT_VAR*)((size_t)result + pScript->GetBasePointer());
    }

    const char* __fastcall GetScriptStringParam(CRunningScript* thread, int dummy, char* buff, int buffLen)
    {
        if (buff == nullptr || buffLen < 0)
        {
            LOG_WARNING(0, "Invalid ReadStringParam input argument! Ptr: 0x%08X, Size: %d", buff, buffLen);
            CLEO_SkipOpcodeParams(thread, 1);
            return nullptr;
        }

        auto paramType = thread->PeekDataType();
        auto arrayType = IsArray(paramType) ? thread->PeekArrayDataType() : eArrayDataType::ADT_NONE;
        auto isVariableInt = IsVariable(paramType) && (arrayType == eArrayDataType::ADT_NONE || arrayType == eArrayDataType::ADT_INT);

        // integer address to text buffer
        if (IsImmInteger(paramType) || isVariableInt)
        {
            auto str = (char*)CLEO_PeekIntOpcodeParam(thread);
            CLEO_SkipOpcodeParams(thread, 1);

            if ((size_t)str <= CCustomOpcodeSystem::MinValidAddress)
            {
                LOG_WARNING(thread, "Invalid '0x%X' pointer of input string argument %s in script %s", str, GetParamInfo().c_str(), ScriptInfoStr(thread).c_str());
                return nullptr; // error
            }

            auto len = min((int)strlen(str), buffLen);
            memcpy(buff, str, len);
            if (len < buffLen) buff[len] = '\0'; // add terminator if possible
            return str; // pointer to original data
        }
        else if (paramType == DT_VARLEN_STRING)
        {
            CleoInstance.OpcodeSystem.handledParamCount++;
            thread->IncPtr(1); // already processed paramType

            DWORD length = *thread->GetBytePointer(); // as unsigned byte!
            thread->IncPtr(1); // length info

            char* str = (char*)thread->GetBytePointer();
            thread->IncPtr(length); // text data

            memcpy(buff, str, min(buffLen, (int)length));
            if ((int)length < buffLen) buff[length] = '\0'; // add terminator if possible
            return buff;
        }
        else if (IsImmString(paramType))
        {
            thread->IncPtr(1); // already processed paramType
            auto str = (char*)thread->GetBytePointer();

            switch (paramType)
            {
                case DT_TEXTLABEL:
                {
                    CleoInstance.OpcodeSystem.handledParamCount++;
                    memcpy(buff, str, min(buffLen, 8));
                    thread->IncPtr(8); // text data
                    return buff;
                }

                case DT_STRING:
                {
                    CleoInstance.OpcodeSystem.handledParamCount++;
                    memcpy(buff, str, min(buffLen, 16));
                    thread->IncPtr(16); // ext data
                    return buff;
                }
            }
        }
        else if (IsVarString(paramType))
        {
            switch (paramType)
            {
                // short string variable
                case DT_VAR_TEXTLABEL:
                case DT_LVAR_TEXTLABEL:
                case DT_VAR_TEXTLABEL_ARRAY:
                case DT_LVAR_TEXTLABEL_ARRAY:
                {
                    auto str = (char*)GetScriptParamPointer(thread);
                    memcpy(buff, str, min(buffLen, 8));
                    if (buffLen > 8) buff[8] = '\0'; // add terminator if possible
                    return buff;
                }

                // long string variable
                case DT_VAR_STRING:
                case DT_LVAR_STRING:
                case DT_VAR_STRING_ARRAY:
                case DT_LVAR_STRING_ARRAY:
                {
                    auto str = (char*)GetScriptParamPointer(thread);
                    memcpy(buff, str, min(buffLen, 16));
                    if (buffLen > 16) buff[16] = '\0'; // add terminator if possible
                    return buff;
                }
            }
        }

        // unsupported param type
        LOG_WARNING(thread, "Argument %s expected to be string, got %s in script %s", GetParamInfo().c_str(), ToKindStr(paramType, arrayType), ScriptInfoStr(thread).c_str());
        CLEO_SkipOpcodeParams(thread, 1); // try skip unhandled param
        return nullptr; // error
    }

    SCRIPT_VAR* GetScriptParamPointer(CRunningScript* thread)
    {
        SCRIPT_VAR* ptr = GetScriptParamPointer2(thread, 0);
        CleoInstance.OpcodeSystem.handledParamCount++; // TODO: hook game's GetScriptParamPointer1 and GetScriptParamPointer2 procedures so this is always incremented
        return ptr;
    }

    SCRIPT_VAR * __fastcall _GetScriptParamPointer2(CRunningScript *pScript, int dummy, int unused)
    {
        _asm
        {
            mov ecx, pScript
            push unused
            call FUNC_GetScriptParamPointer2
        }
    }

    void(__cdecl * InitScm)();
    void(__cdecl * DrawScriptStuff)(char bBeforeFade);
    void(__cdecl * DrawScriptStuff_H)(char bBeforeFade);

    BYTE *scmBlock;
    BYTE *missionBlock;
    int MissionIndex;
    BOOL *onMissionFlag;
    CTexture *scriptSprites;
    BYTE *scriptDraws;
    BYTE *scriptTexts;

    CRunningScript **inactiveThreadQueue, **activeThreadQueue;

    void OnLoadScmData(void)
    {
        TRACE("Loading scripts save data...");
        CTheScripts::Load();
    }

    void OnSaveScmData(void)
    {
        TRACE("Saving scripts save data...");
        CleoInstance.ScriptEngine.SaveState();
        CleoInstance.ScriptEngine.UnregisterAllScripts();
        CTheScripts::Save();
        CleoInstance.ScriptEngine.ReregisterAllScripts();
    }

    struct CleoSafeHeader
    {
        const static unsigned sign;
        unsigned signature;
        unsigned n_saved_threads;
        unsigned n_stopped_threads;
    };

    const unsigned CleoSafeHeader::sign = 0x31345653;

    struct ThreadSavingInfo
    {
        unsigned long hash;
        SCRIPT_VAR tls[32];
        unsigned timers[2];
        bool condResult;
        unsigned sleepTime;
        eLogicalOperation logicalOp;
        bool notFlag;
        ptrdiff_t ip_diff;
        char threadName[8];

        ThreadSavingInfo(CCustomScript *cs) :
            hash(cs->m_codeChecksum), condResult(cs->bCondResult),
            logicalOp(cs->LogicalOp), notFlag(cs->NotFlag != false), ip_diff(cs->CurrentIP - reinterpret_cast<BYTE*>(cs->BaseIP))
        {
            sleepTime = cs->WakeTime >= CTimer::m_snTimeInMilliseconds ? 0 : cs->WakeTime - CTimer::m_snTimeInMilliseconds;
            std::copy(cs->LocalVar, cs->LocalVar + 32, tls);
            std::copy(cs->Timers, cs->Timers + 2, timers);
            std::copy(cs->Name, cs->Name + 8, threadName);
        }

        void Apply(CCustomScript *cs)
        {
            cs->m_codeChecksum = hash;
            std::copy(tls, tls + 32, cs->LocalVar);
            std::copy(timers, timers + 2, cs->Timers);
            cs->bCondResult = condResult;
            cs->WakeTime = CTimer::m_snTimeInMilliseconds + sleepTime;
            cs->LogicalOp = logicalOp;
            cs->NotFlag = notFlag;
            cs->CurrentIP = reinterpret_cast<BYTE*>(cs->BaseIP) + ip_diff;
            std::copy(threadName, threadName + 8, cs->Name);
            cs->EnableSaving(true);
        }

        ThreadSavingInfo() { }
    };

    SCRIPT_VAR CScriptEngine::CleoVariables[0x400];

    template<typename T>
    void inline ReadBinary(std::istream& s, T& buf)
    {
        s.read(reinterpret_cast<char *>(&buf), sizeof(T));
    }

    template<typename T>
    void inline ReadBinary(std::istream& s, T *buf, size_t size)
    {
        s.read(reinterpret_cast<char *>(buf), sizeof(T) * size);
    }

    template<typename T>
    void inline WriteBinary(std::ostream& s, const T& data)
    {
        s.write(reinterpret_cast<const char *>(&data), sizeof(T));
    }

    template<typename T>
    void inline WriteBinary(std::ostream& s, const T*data, size_t size)
    {
        s.write(reinterpret_cast<const char *>(data), sizeof(T) * size);
    }

    void __fastcall HOOK_ProcessScript(CCustomScript * pScript, int)
    {
        CleoInstance.ScriptEngine.GameBegin(); // all initialized and ready to process scripts

        // run registered callbacks
        bool process = true;
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptProcess))
        {
            typedef bool WINAPI callback(CRunningScript*);
            process = process && ((callback*)func)(pScript);
        }
        if (!process)
        {
            return; // skip this script
        }

        if (pScript->IsCustom()) pScript->Process();
        else ProcessScript(pScript);
    }

    void HOOK_DrawScriptStuff(char bBeforeFade)
    {
        CleoInstance.ScriptEngine.DrawScriptStuff(bBeforeFade);

        if(bBeforeFade)
            DrawScriptStuff_H(bBeforeFade);
        else
            DrawScriptStuff(bBeforeFade);

        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptDraw))
        {
            typedef void WINAPI callback(bool);
            ((callback*)func)(bBeforeFade != 0);
        }
    }

#define NUM_STORED_SPRITES 128
#define NUM_STORED_DRAWS 128
#define NUM_STORED_TEXTS 96
#define DRAW_DATA_SIZE 60
#define TEXT_DATA_SIZE 68
#define DRAW_ARRAY_SIZE NUM_STORED_DRAWS*DRAW_DATA_SIZE
#define TEXT_ARRAY_SIZE NUM_STORED_TEXTS*TEXT_DATA_SIZE
    CTexture storedSprites[NUM_STORED_SPRITES];
    BYTE storedDraws[DRAW_ARRAY_SIZE];
    BYTE storedTexts[TEXT_ARRAY_SIZE];
    BYTE storedUseTextCommands = 0;
    WORD numStoredDraws = 0;
    WORD numStoredTexts = 0;

    static void RestoreTextDrawDefaults()
    {
        for (int i = 0; i<NUM_STORED_TEXTS; ++i)
        {
            CTextDrawer * pText = (CTextDrawer*)&scriptTexts[i*TEXT_DATA_SIZE];
            pText->m_fScaleX = 0.48f;
            pText->m_fScaleY = 1.12f;
            pText->m_Colour = CRGBA(0xE1, 0xE1, 0xE1, 0xFF);
            pText->m_bJustify = false;
            pText->m_bAlignRight = false;
            pText->m_bCenter = false;
            pText->m_bBackground = false;
            pText->m_bUnk1 = false;
            pText->m_fLineHeight = 182.0f;
            pText->m_fLineWidth = 640.0f;
            pText->m_BackgroundColour = CRGBA(0x80, 0x80, 0x80, 0x80);
            pText->m_bProportional = true;
            pText->m_EffectColour = CRGBA(0, 0, 0, 0xFF);
            strncpy(pText->m_szGXT, "", 8);
            pText->m_ucShadow = 2;
            pText->m_ucOutline = 0;
            pText->m_bDrawBeforeFade = false;
            pText->m_nFont = 1;
            pText->m_fPosX = 0.0;
            pText->m_fPosY = 0.0;
            pText->m_nParam1 = -1;
            pText->m_nParam2 = -1;
        }
    }

    void CScriptEngine::DrawScriptStuff(char bBeforeFade)
    {
        for (auto i = CustomScripts.begin(); i != CustomScripts.end(); ++i)
        {
            auto script = *i;
            script->Draw(bBeforeFade);
        }
        if (auto script = GetCustomMission())
            script->Draw(bBeforeFade);
    }

    void CCustomScript::Process()
    {
        RestoreScriptSpecifics();

        bool bNeedDefaults = false;
        if (CTheScripts::UseTextCommands)
        {
            RestoreTextDrawDefaults();
            CTheScripts::NumberOfIntroTextLinesThisFrame = 0;
            std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
            CTheScripts::NumberOfIntroRectanglesThisFrame = 0;
            CTheScripts::UseTextCommands = false;
        }
		
		ProcessScript(this);

        StoreScriptSpecifics();
    }
    void CCustomScript::ShutdownThisScript()
    {
        ((::CRunningScript*)this)->ShutdownThisScript(); // CRunningScript from Plugin SDK
    }

    void CCustomScript::Draw(char bBeforeFade)
    {
        // no point if this script doesn't draw
        if (m_scriptDraws.size() || m_scriptTexts.size())
        {
            static CCustomScript * last;
            last = this;
            RestoreScriptDraws();
            RestoreScriptTextures();
            if (bBeforeFade) DrawScriptStuff_H(bBeforeFade);
            else DrawScriptStuff(bBeforeFade);
            StoreScriptDraws();
            StoreScriptTextures();
        }
    }
    void CCustomScript::StoreScriptDraws()
    {
        // store this scripts draws + texts
        if (CTheScripts::NumberOfIntroRectanglesThisFrame)
            m_scriptDraws.assign(scriptDraws, scriptDraws + (CTheScripts::NumberOfIntroRectanglesThisFrame * DRAW_DATA_SIZE));
        else if (m_scriptDraws.size())
            m_scriptDraws.clear();
        if (CTheScripts::NumberOfIntroTextLinesThisFrame)
            m_scriptTexts.assign(scriptTexts, scriptTexts + (CTheScripts::NumberOfIntroTextLinesThisFrame * TEXT_DATA_SIZE));
        else if (m_scriptTexts.size())
            m_scriptTexts.clear();

        m_useTextCommands = CTheScripts::UseTextCommands;
        m_numDraws = CTheScripts::NumberOfIntroRectanglesThisFrame;
        m_numTexts = CTheScripts::NumberOfIntroTextLinesThisFrame;

        // restore SCM draws + texts
        if (numStoredDraws) std::copy(storedDraws, storedDraws + (numStoredDraws * DRAW_DATA_SIZE), scriptDraws);
        else std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
        if (numStoredTexts) std::copy(storedTexts, storedTexts + (numStoredTexts * TEXT_DATA_SIZE), scriptTexts);
        else RestoreTextDrawDefaults();
        CTheScripts::NumberOfIntroRectanglesThisFrame = numStoredDraws;
        CTheScripts::NumberOfIntroTextLinesThisFrame = numStoredTexts;
        CTheScripts::UseTextCommands = storedUseTextCommands;
    }
    void CCustomScript::RestoreScriptDraws()
    {
        // store SCM draws + texts
        storedUseTextCommands = CTheScripts::UseTextCommands;
        numStoredDraws = CTheScripts::NumberOfIntroRectanglesThisFrame;
        numStoredTexts = CTheScripts::NumberOfIntroTextLinesThisFrame;
        if (numStoredDraws)
            std::copy(scriptDraws, scriptDraws + (numStoredDraws *  DRAW_DATA_SIZE), storedDraws);
        if (numStoredTexts)
            std::copy(scriptTexts, scriptTexts + (numStoredTexts * TEXT_DATA_SIZE), storedTexts);

        // restore script draws + texts
        if (!m_scriptDraws.size()) CTheScripts::NumberOfIntroRectanglesThisFrame = 0;
        else
        {
            std::copy(m_scriptDraws.begin(), m_scriptDraws.end(), scriptDraws);
            CTheScripts::NumberOfIntroRectanglesThisFrame = m_numDraws;
        }
        if (!m_scriptTexts.size()) CTheScripts::NumberOfIntroTextLinesThisFrame = 0;
        else
        {
            std::copy(m_scriptTexts.begin(), m_scriptTexts.end(), scriptTexts);
            CTheScripts::NumberOfIntroTextLinesThisFrame = m_numTexts;
        }
        CTheScripts::UseTextCommands = m_useTextCommands;
    }

    bool CCustomScript::GetDebugMode() const
    {
        if (!bIsCustom)
            return CleoInstance.ScriptEngine.NativeScriptsDebugMode;

        return m_debugMode;
    }

    void CCustomScript::SetDebugMode(bool enabled)
    {
        if (!bIsCustom)
            CleoInstance.ScriptEngine.NativeScriptsDebugMode = enabled;
        else
            m_debugMode = enabled;
    }

    const char* CCustomScript::GetScriptFileDir() const
    {
        if(!bIsCustom)
            return CleoInstance.ScriptEngine.MainScriptFileDir.c_str();

        return m_scriptFileDir.c_str();
    }

    void CCustomScript::SetScriptFileDir(const char* directory)
    {
        if (!bIsCustom)
            CleoInstance.ScriptEngine.MainScriptFileDir = directory;
        else
            m_scriptFileDir = directory;
    }

    const char* CCustomScript::GetScriptFileName() const 
    {
        if (!bIsCustom)
            return CleoInstance.ScriptEngine.MainScriptFileName.c_str();

        return m_scriptFileName.c_str();
    }

    void CCustomScript::SetScriptFileName(const char* filename) 
    {
        if (!bIsCustom)
            CleoInstance.ScriptEngine.MainScriptFileName = filename;
        else
            m_scriptFileName = filename;
    }

    std::string CCustomScript::GetScriptFileFullPath() const
    {
        std::string path = GetScriptFileDir();
        path += '\\';
        path += GetScriptFileName();
        return path;
    }

    const char* CCustomScript::GetWorkDir() const
    {
        if (!bIsCustom)
            return CleoInstance.ScriptEngine.MainScriptCurWorkDir.c_str();

        return m_workDir.c_str();
    }

    void CCustomScript::SetWorkDir(const char* directory)
    {
        if(directory == nullptr || strlen(directory) == 0)
            return;  // Already done. Empty path is relative path starting at current work dir

        auto resolved = ResolvePath(directory);

        if (!bIsCustom)
            CleoInstance.ScriptEngine.MainScriptCurWorkDir = resolved;
        else
            m_workDir = resolved;
    }

    std::string CCustomScript::ResolvePath(const char* path, const char* customWorkDir) const
    {
        if (path == nullptr)
        {
            return {};
        }

        auto fsPath = FS::path(path);

        // check for virtual path root
        enum class VPref{ None, Game, User, Script, Cleo, Modules } virtualPrefix = VPref::None;
        if(!fsPath.empty())
        {
            const auto root = fsPath.begin()->string(); // first path element
            const auto r = root.c_str();

            if(_strcmpi(r, DIR_GAME) == 0) virtualPrefix = VPref::Game;
            else if (_strcmpi(r, DIR_USER) == 0) virtualPrefix = VPref::User;
            else if (_strcmpi(r, DIR_SCRIPT) == 0 && !IsLegacyScript((CRunningScript*)this)) virtualPrefix = VPref::Script;
            else if (_strcmpi(r, DIR_CLEO) == 0) virtualPrefix = VPref::Cleo;
            else if (_strcmpi(r, DIR_MODULES) == 0) virtualPrefix = VPref::Modules;
        }

        // not virtual
        if(virtualPrefix == VPref::None)
        {
            if(fsPath.is_relative())
            {
                if(customWorkDir != nullptr)
                    fsPath = ResolvePath(customWorkDir) / fsPath;
                else
                    fsPath = GetWorkDir() / fsPath;
            }

            auto result = fsPath.string();
            FilepathNormalize(result, false);

            // ModLoader support: make paths withing game directory relative to it
            FilepathRemoveParent(result, Filepath_Game);

            return std::move(result);
        }

        // expand virtual paths
        FS::path resolved;
        switch(virtualPrefix)
        {
            case VPref::User: resolved = Filepath_User; break;
            case VPref::Script: resolved = GetScriptFileDir(); break;
            case VPref::Game: resolved = Filepath_Game; break;
            case VPref::Cleo: resolved = Filepath_Cleo; break;
            case VPref::Modules: resolved = Filepath_Cleo + "\\cleo_modules"; break;
            default : resolved = "<error>"; break; // should never happen
        }

        // append all but virtual prefix from original path
        for (auto it = ++fsPath.begin(); it != fsPath.end(); it++)
            resolved /= *it;

        auto result = resolved.string();
        FilepathNormalize(result, false);
        return std::move(result);
    }

    std::string CCustomScript::GetInfoStr(bool currLineInfo) const
    {
        std::ostringstream ss;

        auto threadName = GetName();
        auto fileName = GetScriptFileName();

        if(memcmp(threadName.c_str(), fileName, threadName.length()) != 0) // thread name no longer same as filename (was set with 03A4)
        {
            ss << "'" << threadName << "' from ";
        }

        ss << "'" << fileName << "'";

        if(currLineInfo)
        {
            ss << " at ";

            if(false)
            {
                // TODO: get Sanny's SMC extra info
                ss << "line " << 0;
                ss << " - ";
                ss << "CODE";
            }
            else
            {
                auto base = (DWORD)BaseIP;
                if (base == 0) base = (DWORD)scmBlock;
                auto currPos = (DWORD)CCustomOpcodeSystem::lastOpcodePtr;

                if (IsMission() && !IsCustom())
                {
                    if (currPos >= (DWORD)missionBlock)
                    {
                        // we are in mission code buffer
                        // native missions are loaded from script file into mission block area
                        currPos += ((DWORD*)CTheScripts::MultiScriptArray)[MissionIndex]; // start offset of this mission within source script file
                    }
                    else
                    {
                        base = (DWORD)scmBlock; // seems that mission uses main scm code
                    }
                }

                auto offset = currPos - base;

                ss << "offset {" << offset << "}"; // Sanny offsets style
                ss << " - ";
                ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << CCustomOpcodeSystem::lastOpcode;

                auto commandName = CleoInstance.OpcodeInfoDb.GetCommandName(CCustomOpcodeSystem::lastOpcode);
                if (commandName != nullptr)
                {
                    ss << ": " << commandName;
                }
                else
                {
                    ss << ": ...";
                }
            }
        }

        return ss.str();
    }

    void CCustomScript::StoreScriptTextures()
    {
        // store this scripts textures + restore SCM textures + make sure this scripts textures arent cleared by another
        if (m_scriptTextures.size())
            m_scriptTextures.clear();
        for (int i = 0; i<NUM_STORED_SPRITES; ++i)
        {
            m_scriptTextures.push_back(*(RwTexture**)&scriptSprites[i]);
            scriptSprites[i] = storedSprites[i];
        }

        //std::copy(scriptSprites, scriptSprites + NUM_STORED_SPRITES, storedSprites);
    }
    void CCustomScript::RestoreScriptTextures()
    {
        int n = 0;

        // store SCM textures
        for (int i = 0; i<NUM_STORED_SPRITES; ++i)
        {
            storedSprites[i] = scriptSprites[i];
        }
        //std::copy(scriptSprites, scriptSprites + NUM_STORED_SPRITES, storedSprites);

        // ensure SCM textures arent cleared - except by the SCM
        if (!m_scriptTextures.size())
            std::fill((RwTexture**)scriptSprites, (RwTexture**)scriptSprites + NUM_STORED_SPRITES, nullptr);
        else
        {
            // restore textures for this script
            for (auto i = m_scriptTextures.begin(); i != m_scriptTextures.end(); ++i, ++n)
            {
                if (n >= NUM_STORED_SPRITES) break;
                *(RwTexture**)(&scriptSprites[n]) = *i;
            }
        }
    }
    void CCustomScript::StoreScriptSpecifics()
    {
        StoreScriptDraws();
        StoreScriptTextures();
    }
    void CCustomScript::RestoreScriptSpecifics()
    {
        RestoreScriptDraws();
        RestoreScriptTextures();
    }

    void CScriptEngine::Inject(CCodeInjector& inj)
    {
        TRACE("Injecting ScriptEngine...");
        CGameVersionManager& gvm = CleoInstance.VersionManager;

        // Global Events crashfix
        //inj.MemoryWrite(0xA9AF6C, 0, 4);

        // Dirty hacks to keep compatibility with plugins + overcome VS thiscall restrictions
        FUNC_ScriptOpcodeHandler00 = gvm.TranslateMemoryAddress(MA_SCRIPT_OPCODE_HANDLER0_FUNCTION);
        FUNC_GetScriptParams = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAMS_FUNCTION);
        FUNC_TransmitScriptParams = gvm.TranslateMemoryAddress(MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptParams = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptCondResult = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_COND_RESULT_FUNCTION);
        FUNC_GetScriptParamPointer1 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION);
        FUNC_GetScriptParamPointer2 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION);

        ScriptOpcodeHandler00 = reinterpret_cast<char(__thiscall*)(CRunningScript*, WORD)>(_ScriptOpcodeHandler00);
        GetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_GetScriptParams);
        TransmitScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript*)>(_TransmitScriptParams);
        SetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_SetScriptParams);
        SetScriptCondResult = reinterpret_cast<void(__thiscall*)(CRunningScript*, bool)>(_SetScriptCondResult);
        GetScriptParamPointer1 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*)>(_GetScriptParamPointer1);
        GetScriptParamPointer2 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*, int)>(_GetScriptParamPointer2);

        opcodeParams = gvm.TranslateMemoryAddress(MA_OPCODE_PARAMS);
        missionLocals = gvm.TranslateMemoryAddress(MA_MISSION_LOCALS);
        scmBlock = gvm.TranslateMemoryAddress(MA_SCM_BLOCK);
        missionBlock = gvm.TranslateMemoryAddress(MA_MISSION_BLOCK);
        onMissionFlag = gvm.TranslateMemoryAddress(MA_ON_MISSION_FLAG);

        // Protect script dependencies
        auto addr = gvm.TranslateMemoryAddress(MA_CALL_PROCESS_SCRIPT);
        inj.MemoryReadOffset(addr.address + 1, ProcessScript);
        inj.ReplaceFunction(HOOK_ProcessScript, addr);
        scriptSprites = gvm.TranslateMemoryAddress(MA_SCRIPT_SPRITE_ARRAY);
        scriptDraws = gvm.TranslateMemoryAddress(MA_SCRIPT_DRAW_ARRAY);
        scriptTexts = gvm.TranslateMemoryAddress(MA_SCRIPT_TEXT_ARRAY);

        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE).address + 1, CLEO::DrawScriptStuff);
        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE).address + 1, DrawScriptStuff_H);
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE));
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE));
        inj.MemoryWrite(gvm.TranslateMemoryAddress(MA_CODE_JUMP_FOR_TXD_STORE), OP_RET);

        inactiveThreadQueue = gvm.TranslateMemoryAddress(MA_INACTIVE_THREAD_QUEUE);
        activeThreadQueue = gvm.TranslateMemoryAddress(MA_ACTIVE_THREAD_QUEUE);
        staticThreads = gvm.TranslateMemoryAddress(MA_STATIC_THREADS);

        inj.ReplaceFunction(OnLoadScmData, gvm.TranslateMemoryAddress(MA_CALL_LOAD_SCM_DATA));
        inj.ReplaceFunction(OnSaveScmData, gvm.TranslateMemoryAddress(MA_CALL_SAVE_SCM_DATA));
    }

    CScriptEngine::~CScriptEngine()
    {
        GameEnd();
    }

    CleoSafeHeader safe_header;
    ThreadSavingInfo *safe_info;
    unsigned long *stopped_info;
    std::unique_ptr<ThreadSavingInfo[]> safe_info_utilizer;
    std::unique_ptr<unsigned long[]> stopped_info_utilizer;

    void CScriptEngine::GameBegin()
    {
        if(gameInProgress) return; // already started
        if(activeThreadQueue == nullptr || activeThreadQueue[0] == nullptr) return; // main gamescript not loaded yet 
        gameInProgress = true;

        if (CGame::bMissionPackGame == 0) // regular main game
        {
            MainScriptFileDir = Filepath_Game + "\\data\\script";
            MainScriptFileName = "main.scm";
        }
        else // mission pack
        {
            MainScriptFileDir = Filepath_User + StringPrintf("\\MPACK\\MPACK%d", CGame::bMissionPackGame);
            MainScriptFileName = "scr.scm";
        }

        NativeScriptsDebugMode = GetPrivateProfileInt("General", "DebugMode", 0, Filepath_Config.c_str()) != 0;

        // global native scripts legacy mode
        int ver = GetPrivateProfileInt("General", "MainScmLegacyMode", 0, Filepath_Config.c_str());
        switch(ver)
        {
            case 3: NativeScriptsVersion = eCLEO_Version::CLEO_VER_3; break;
            case 4: NativeScriptsVersion = eCLEO_Version::CLEO_VER_4; break;
            default: 
                NativeScriptsVersion = eCLEO_Version::CLEO_VER_CUR;
                ver = 0;
            break;
        }
        if (ver != 0) TRACE("Legacy mode for native scripts active: CLEO%d", ver);

        MainScriptCurWorkDir = Filepath_Game;

        CleoInstance.ModuleSystem.LoadCleoModules();
        LoadState(CleoInstance.saveSlot);

        // keep already loaded scripts at front of processing queue
        auto head = *activeThreadQueue;

        auto tail = head;
        while (tail->Next) tail = tail->Next;

        // load custom scripts as new list
        *activeThreadQueue = nullptr;
        LoadCustomScripts();

        // append custom scripts list to the back
        if (*activeThreadQueue != nullptr)
        {
            tail->Next = *activeThreadQueue;
            (*activeThreadQueue)->Previous = tail;
        }

        *activeThreadQueue = head; // restore original
    }

    void CScriptEngine::GameEnd()
    {
        if (!gameInProgress) return;
        gameInProgress = false;

        RemoveAllCustomScripts();
        CleoInstance.ModuleSystem.Clear();
        memset(CleoVariables, 0, sizeof(CleoVariables));
    }

    void CScriptEngine::LoadCustomScripts()
    {
        TRACE(""); // separator
        TRACE("Listing CLEO scripts:");

        std::set<std::string> found;

        auto processFileList = [&](StringList fileList)
        {
            for (DWORD i = 0; i < fileList.count; i++)
            {
                const auto ext = FS::path(fileList.strings[i]).extension();
                if (ext == cs_ext || ext == cs3_ext || ext == cs4_ext)
                {
                    TRACE(" - '%s'", fileList.strings[i]);
                    found.emplace(fileList.strings[i]);
                }
            }
        };

        auto searchPattern = Filepath_Cleo + "\\*" + cs_ext;
        auto list = CLEO_ListDirectory(nullptr, searchPattern.c_str(), false, true);
        processFileList(list);
        CLEO_StringListFree(list);

        searchPattern = Filepath_Cleo + "\\*" + cs3_ext;
        list = CLEO_ListDirectory(nullptr, searchPattern.c_str(), false, true);
        processFileList(list);
        CLEO_StringListFree(list);

        searchPattern = Filepath_Cleo + "\\*" + cs4_ext;
        list = CLEO_ListDirectory(nullptr, searchPattern.c_str(), false, true);
        processFileList(list);
        CLEO_StringListFree(list);

        if (!found.empty())
        {
            TRACE("Starting CLEO scripts...");

            for (const auto& path : found)
            {
                LoadScript(path.c_str());
            }
        }
        else
        {
            TRACE(" - nothing found");
        }
    }

    CCustomScript * CScriptEngine::LoadScript(const char * szFilePath)
    {
        auto cs = new CCustomScript(szFilePath);

        if (!cs || !cs->IsOk())
        {
            TRACE("Loading of custom script '%s' failed", szFilePath);
            if (cs) delete cs;
            return nullptr;
        }

        // check whether the script is in stop-list
        if (stopped_info)
        {
            for (size_t i = 0; i < safe_header.n_stopped_threads; ++i)
            {
                if (stopped_info[i] == cs->m_codeChecksum)
                {
                    TRACE("Custom script '%s' found in the stop-list", szFilePath);
                    InactiveScriptHashes.insert(stopped_info[i]);
                    delete cs;
                    return nullptr;
                }
            }
        }

        // check whether the script is in safe-list
        if (safe_info)
        {
            for (size_t i = 0; i < safe_header.n_saved_threads; ++i)
            {
                if (safe_info[i].hash == cs->GetCodeChecksum())
                {
                    TRACE("Custom script '%s' found in the safe-list", szFilePath);
                    safe_info[i].Apply(cs);
                    break;
                }
            }
        }

        AddCustomScript(cs);
        return cs;
    }

    CCustomScript* CScriptEngine::CreateCustomScript(CRunningScript* fromThread, const char* script_name, int label)
    {
        auto filename = reinterpret_cast<CCustomScript*>(fromThread)->ResolvePath(script_name, DIR_CLEO); // legacy: default search location is game\cleo directory

        if (label != 0) // create from label
        {
            TRACE("Starting new custom script from thread named '%s' label 0x%08X", filename.c_str(), label);
        }
        else
        {
            TRACE("Starting new custom script '%s'", filename.c_str());
        }

        // if "label == 0" then "script_name" need to be the file name
        auto cs = new CCustomScript(filename.c_str(), false, fromThread, label);
        if (fromThread) SetScriptCondResult(fromThread, cs && cs->IsOk());
        if (cs && cs->IsOk())
        {
            AddCustomScript(cs);
            if (fromThread) TransmitScriptParams(fromThread, cs);
        }
        else
        {
            if (cs) delete cs;
            if (fromThread) SkipUnusedVarArgs(fromThread);
            LOG_WARNING(0, "Failed to load script '%s'", filename.c_str());
            return nullptr;
        }

        return cs;
    }

    void CScriptEngine::LoadState(int saveSlot)
    {
        memset(CleoVariables, 0, sizeof(CleoVariables));
        safe_info = nullptr;
        stopped_info = nullptr;
        safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;

        if(saveSlot == -1) return; // new game started

        auto saveFile = FS::path(Filepath_Cleo).append(StringPrintf("cleo_saves\\cs%d.sav", saveSlot)).string();

        // load cleo saving file
        try
        {
            TRACE(""); // separator
            TRACE("Loading cleo safe '%s'", saveFile.c_str());
            std::ifstream ss(saveFile.c_str(), std::ios::binary);
            if (ss.is_open())
            {
                ss.exceptions(std::ios::eofbit | std::ios::badbit | std::ios::failbit);
                ReadBinary(ss, safe_header);
                if (safe_header.signature != CleoSafeHeader::sign)
                    throw std::runtime_error("Invalid file format");
                safe_info = new ThreadSavingInfo[safe_header.n_saved_threads];
                safe_info_utilizer.reset(safe_info);
                stopped_info = new unsigned long[safe_header.n_stopped_threads];
                stopped_info_utilizer.reset(stopped_info);
                ReadBinary(ss, CleoVariables, 0x400);
                ReadBinary(ss, safe_info, safe_header.n_saved_threads);
                ReadBinary(ss, stopped_info, safe_header.n_stopped_threads);
                for (size_t i = 0; i < safe_header.n_stopped_threads; ++i)
                    InactiveScriptHashes.insert(stopped_info[i]);
                TRACE("Finished. Loaded %u cleo variables, %u saved threads info, %u stopped threads info",
                    0x400, safe_header.n_saved_threads, safe_header.n_stopped_threads);
            }
            else
            {
                memset(CleoVariables, 0, sizeof(CleoVariables));
            }
        }
        catch (std::exception& ex)
        {
            TRACE("Loading of cleo safe '%s' failed: %s", saveFile.c_str(), ex.what());
            safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;
            memset(CleoVariables, 0, sizeof(CleoVariables));
        }
    }

    void CScriptEngine::SaveState()
    {
        try
        {
            std::list<CCustomScript *> savedThreads;
            std::for_each(CustomScripts.begin(), CustomScripts.end(), [this, &savedThreads](CCustomScript *cs) {
                if (cs->m_saveEnabled)
                    savedThreads.push_back(cs);
            });

            CleoSafeHeader header = { CleoSafeHeader::sign, savedThreads.size(), InactiveScriptHashes.size() };

            char safe_name[MAX_PATH];
            sprintf(safe_name, "./cleo/cleo_saves/cs%d.sav", FrontEndMenuManager.m_nSelectedSaveGame);
            TRACE("Saving script engine state to the file '%s'", safe_name);

            CreateDirectory("cleo", NULL);
            CreateDirectory("cleo/cleo_saves", NULL);
            std::ofstream ss(safe_name, std::ios::binary);
            if (ss.is_open())
            {
                ss.exceptions(std::ios::failbit | std::ios::badbit);

                WriteBinary(ss, header);
                WriteBinary(ss, CleoVariables, 0x400);

                std::for_each(savedThreads.begin(), savedThreads.end(), [&savedThreads, &ss](CCustomScript *cs)
                {
                    ThreadSavingInfo savingInfo(cs);
                    WriteBinary(ss, savingInfo);
                });

                std::for_each(InactiveScriptHashes.begin(), InactiveScriptHashes.end(), [&ss](unsigned long hash) {
                    WriteBinary(ss, hash);
                });

                TRACE("Done. Saved %u cleo variables, %u saved threads, %u stopped threads",
                    0x400, header.n_saved_threads, header.n_stopped_threads);
            }
            else
            {
                TRACE("Failed to write save file '%s'!", safe_name);
            }
        }
        catch (std::exception& ex)
        {
            TRACE("Saving failed. %s", ex.what());
        }
    }

    CRunningScript* CScriptEngine::FindScriptNamed(const char* threadName, bool standardScripts, bool customScripts, size_t resultIndex)
    {
        if (standardScripts)
        {
            for (auto script = *activeThreadQueue; script; script = script->GetNext())
            {
                if (script->IsCustom()) 
                {
                    // skip custom scripts in the queue, they are handled separately
                    continue;
                }
                if (_strnicmp(threadName, script->Name, sizeof(script->Name)) == 0)
                {
                    if (resultIndex == 0) return script;
                    else resultIndex--;
                }
            }
        }

        if (customScripts)
        {
            if (CustomMission)
            {
                if (_strnicmp(threadName, CustomMission->Name, sizeof(CustomMission->Name)) == 0)
                {
                    if (resultIndex == 0) return CustomMission;
                    else resultIndex--;
                }
            }

            for (auto it = CustomScripts.begin(); it != CustomScripts.end(); ++it)
            {
                auto cs = *it;
                if (_strnicmp(threadName, cs->Name, sizeof(cs->Name)) == 0)
                {
                    if (resultIndex == 0) return cs;
                    else resultIndex--;
                }
            }
        }

        return nullptr;
    }

    CRunningScript* CScriptEngine::FindScriptByFilename(const char* path, size_t resultIndex)
    {
        if (path == nullptr) return nullptr;

        auto pathLen = strlen(path);
        auto CheckScript = [&](CRunningScript* script)
        {
            if (script == nullptr) return false;

            auto cs = (CCustomScript*)script;
            std::string scriptPath = cs->GetScriptFileFullPath();

            if (scriptPath.length() < pathLen) return false;

            auto startPos = scriptPath.length() - pathLen;
            if (_strnicmp(path, scriptPath.c_str() + startPos, pathLen) == 0)
            {
                if (startPos > 0 && path[startPos - 1] != '\\') return false; // whole file/dir name must match

                return true;
            }

            return false;
        };

        // standard scripts
        for (auto script = *activeThreadQueue; script; script = script->GetNext())
        {
            if (CheckScript(script))
            {
                if (resultIndex == 0) return script;
                else resultIndex--;
            }
        }

        // custom scripts
        if (CheckScript(CustomMission))
        {
            if (resultIndex == 0) return CustomMission;
            else resultIndex--;
        }

        for (auto it = CustomScripts.begin(); it != CustomScripts.end(); ++it)
        {
            auto cs = *it;
            if (CheckScript(cs))
            {
                if (resultIndex == 0) return cs;
                else resultIndex--;
            }
        }

        return nullptr;
    }

    bool CScriptEngine::IsActiveScriptPtr(const CRunningScript* ptr) const
    {
        for (auto script = *activeThreadQueue; script != nullptr; script = script->GetNext())
        {
            if (script == ptr)
                return ptr->IsActive();
        }

        for (const auto script : CustomScripts)
        {
            if (script == ptr)
                return ptr->IsActive();
        }

        return false;
    }

    bool CScriptEngine::IsValidScriptPtr(const CRunningScript* ptr) const
    {
        for (auto script = *activeThreadQueue; script != nullptr; script = script->GetNext())
        {
            if (script == ptr)
                return true;
        }

        for (auto script = *inactiveThreadQueue; script != nullptr; script = script->GetNext())
        {
            if (script == ptr)
                return true;
        }

        for (const auto script : CustomScripts)
        {
            if (script == ptr)
                return true;
        }

        for (const auto script : ScriptsWaitingForDelete)
        {
            if (script == ptr)
                return true;
        }

        return false;
    }

    void CScriptEngine::AddCustomScript(CCustomScript *cs)
    {
        if (cs->IsMission())
        {
            TRACE("Registering custom mission named '%s'", cs->GetName().c_str());
            CustomMission = cs;
        }
        else
        {
            TRACE("Registering custom script named '%s'", cs->GetName().c_str());
            CustomScripts.push_back(cs);
        }

        cs->AddScriptToList(activeThreadQueue);
        cs->SetActive(true);

        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptRegister))
        {
            typedef void WINAPI callback(CCustomScript*);
            ((callback*)func)(cs);
        }
    }

    void CScriptEngine::RemoveScript(CRunningScript* thread)
    {
        if (thread->IsMission()) CTheScripts::bAlreadyRunningAMissionScript = false;

        if (thread->IsCustom())
        {
            RemoveCustomScript((CCustomScript*)thread);
        }
        else // native script
        {
            auto cs = (CCustomScript*)thread;
            cs->RemoveScriptFromList(activeThreadQueue);
            cs->AddScriptToList(inactiveThreadQueue);
            cs->ShutdownThisScript();
        }
    }

    void CScriptEngine::RemoveCustomScript(CCustomScript *cs)
    {
        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptUnregister))
        {
            typedef void WINAPI callback(CCustomScript*);
            ((callback*)func)(cs);
        }

		if (cs->m_parentScript)
		{
			cs->BaseIP = 0; // don't delete BaseIP if child thread
		}
		for (auto childThread : cs->m_childScripts)
		{
			CScriptEngine::RemoveScript(childThread);
		}
        if (cs == CustomMission)
        {
            TRACE("Unregistering custom mission named '%s'", cs->GetName().c_str());
            CustomMission->RemoveScriptFromList(activeThreadQueue);
            ScriptsWaitingForDelete.push_back(cs);
            CustomMission->SetActive(false);
            CustomMission = nullptr;
            CTheScripts::bAlreadyRunningAMissionScript = false;
        }
        else
        {
            if (cs->m_saveEnabled)
            {
                InactiveScriptHashes.insert(cs->GetCodeChecksum());
                TRACE("Stopping custom script named '%s'", cs->GetName().c_str());
            }
            else
            {
                TRACE("Unregistering custom script named '%s'", cs->GetName().c_str());
                ScriptsWaitingForDelete.push_back(cs);
            }

            CustomScripts.remove(cs);
            cs->RemoveScriptFromList(activeThreadQueue);
            cs->SetActive(false);

            /*if(!pScript->IsMission()) *MissionLoaded = false;
            RemoveScriptFromQueue(pScript, activeThreadQueue);
            AddScriptToQueue(pScript, inactiveThreadQueue);
            StopScript(pScript);*/
        }
    }

    void CScriptEngine::RemoveAllCustomScripts(void)
    {
        TRACE("Unloading scripts...");

        InactiveScriptHashes.clear();

        UnregisterAllScripts();
        CustomScripts.clear();

        std::for_each(ScriptsWaitingForDelete.begin(), ScriptsWaitingForDelete.end(), [this](CCustomScript *cs) 
        {
            TRACE(" Deleting inactive script named '%s'", cs->GetName().c_str());
            delete cs;
        });
        ScriptsWaitingForDelete.clear();

        if (CustomMission)
        {
            TRACE(" Unregistering custom mission named '%s'", CustomMission->GetName().c_str());
            CustomMission->RemoveScriptFromList(activeThreadQueue);
            CustomMission->SetActive(false);
            delete CustomMission;
            CustomMission = nullptr;
            CTheScripts::bAlreadyRunningAMissionScript = false;
        }
    }

    void CScriptEngine::UnregisterAllScripts()
    {
        TRACE("Unregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs)
        {
            cs->RemoveScriptFromList(activeThreadQueue);
            cs->SetActive(false);
        });
    }

    void CScriptEngine::ReregisterAllScripts()
    {
        TRACE("Reregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs)
        {
            cs->AddScriptToList(activeThreadQueue);
            cs->SetActive(true);
        });
    }

    // TODO: Consider split into 2 classes: CCustomExternalScript, CCustomChildScript
    CCustomScript::CCustomScript(const char *szFileName, bool bIsMiss, CRunningScript *parent, int label)
        : CRunningScript(), m_saveEnabled(false), m_ok(false),
        m_compatVer(CLEO_VER_CUR)
    {
        TRACE(""); // separator
        TRACE("Loading custom script '%s'...", szFileName);

        bIsCustom = true;
        bIsMission = bUseMissionCleanup = bIsMiss;
        m_useTextCommands = 0;
        m_numDraws = 0;
        m_numTexts = 0;

        try
        {
            std::ifstream is;
            if (label != 0) // Create external from label.
            {
                if (!parent)
                    throw std::logic_error("Trying to create external thread from label without parent thread");

                if (!parent->IsCustom())
                    throw std::logic_error("Only custom threads can spawn children threads from label");

                auto cs = (CCustomScript*)parent;

                m_compatVer = cs->GetCompatibility();
                m_debugMode = cs->GetDebugMode();
                m_scriptFileDir = cs->GetScriptFileDir();
                m_scriptFileName = cs->GetScriptFileName();
                m_workDir = cs->GetWorkDir();

                BaseIP = cs->GetBasePointer();
                CurrentIP = cs->GetBasePointer() - label;
                memcpy(Name, cs->Name, sizeof(Name));
                m_codeChecksum = cs->m_codeChecksum;
                m_parentScript = cs;
                cs->m_childScripts.push_back(this);
            }
            else
            {
                // store script file directory and name
                std::string pathStr = szFileName;
                FilepathNormalize(pathStr, false);
                FS::path path = pathStr;

                // file exists?
                if (!FS::is_regular_file(path))
                {
                    if (path.extension() == cs_ext)
                    {
                        // maybe it was renamed to enable compatibility mode?
                        auto compatPath = path;

                        compatPath.replace_extension(cs4_ext);
                        if (FS::is_regular_file(compatPath))
                        {
                            path = compatPath;
                        }
                        else
                        {
                            compatPath.replace_extension(cs3_ext);
                            if (FS::is_regular_file(compatPath))
                            {
                                path = compatPath;
                            }
                            else
                            {
                                throw std::logic_error("File does not exists");
                            }
                        }
                    }
                    else
                        throw std::logic_error("File does not exists");
                }

                // deduce compatibility mode from filetype extension
                if (path.extension() == cs4_ext)
                    m_compatVer = CLEO_VER_4;
                else
                if (path.extension() == cs3_ext)
                    m_compatVer = CLEO_VER_3;

                if (m_compatVer == CLEO_VER_CUR && parent != nullptr)
                {
                    // inherit compatibility mode from parent
                    m_compatVer = CLEO_GetScriptVersion(parent);

                    // try loading file with same compatibility mode filetype extension
                    auto compatPath = path;
                    if (m_compatVer == CLEO_VER_4)
                    {
                        compatPath.replace_extension(cs4_ext);
                        if (FS::is_regular_file(compatPath))
                            path = compatPath;
                    }
                    else
                    if (m_compatVer == CLEO_VER_3)
                    {
                        compatPath.replace_extension(cs3_ext);
                        if (FS::is_regular_file(compatPath))
                            path = compatPath;
                    }
                }

                m_scriptFileDir = path.parent_path().string();
                m_scriptFileName = path.filename().string();

                if(parent != nullptr)
                {
                    m_debugMode = ((CCustomScript*)parent)->GetDebugMode();
                    m_workDir = ((CCustomScript*)parent)->GetWorkDir();
                }
                else
                {
                    m_debugMode = CleoInstance.ScriptEngine.NativeScriptsDebugMode; // global setting
                    m_workDir = Filepath_Game; // game root
                }

                using std::ios;
                std::ifstream is(path.string().c_str(), std::ios::binary);
                is.exceptions(std::ios::badbit | std::ios::failbit);
                std::size_t length;
                is.seekg(0, std::ios::end);
                length = (size_t)is.tellg();
                is.seekg(0, std::ios::beg);

                if (bIsMiss)
                {
                    if (CTheScripts::bAlreadyRunningAMissionScript)
                        throw std::logic_error("Starting of custom mission when other mission loaded");

                    CTheScripts::bAlreadyRunningAMissionScript = 1;
                    MissionIndex = -1;
                    BaseIP = CurrentIP = missionBlock; // TODO: there should be check length <= missionBlock size
                }
                else
                {
                    BaseIP = CurrentIP = new BYTE[length];
                }
                is.read(reinterpret_cast<char *>(BaseIP), length);

                m_codeSize = length;
                m_codeChecksum = crc32(reinterpret_cast<BYTE*>(BaseIP), length);

                // thread name from filename
                auto threadNamePath = path;
                if(threadNamePath.extension() == cs3_ext || threadNamePath.extension() == cs4_ext)
                {
                    threadNamePath.replace_extension(cs_ext); // keep original extension even in compatibility modes
                }
                auto fName = threadNamePath.filename().string();

                memset(Name, '\0', sizeof(Name));
                if(!fName.empty())
                {
                    auto len = min(fName.length(), sizeof(Name) - 1); // and text terminator
                    memcpy(Name, fName.c_str(), len);
                }
            }
            CleoInstance.ScriptEngine.LastScriptCreated = this;
            m_ok = true;
        }
        catch (std::exception& e)
        {
            LOG_WARNING(0, "Error during loading of custom script '%s' occured.\nError message: %s", szFileName, e.what());
        }
        catch (...)
        {
            LOG_WARNING(0, "Unknown error during loading of custom script '%s' occured.", szFileName);
        }
    }

    CCustomScript::~CCustomScript()
    {
        if (BaseIP && !bIsMission) delete[] BaseIP;
        CleoInstance.OpcodeSystem.scriptDeleteDelegate(this);

        if (CleoInstance.ScriptEngine.LastScriptCreated == this) CleoInstance.ScriptEngine.LastScriptCreated = nullptr;
    }

    void CCustomScript::AddScriptToList(CRunningScript** queuelist)
    {
        ((::CRunningScript*)this)->AddScriptToList((::CRunningScript**)queuelist); // CRunningScript from Plugin SDK
    }

    void CCustomScript::RemoveScriptFromList(CRunningScript** queuelist)
    {
        ((::CRunningScript*)this)->RemoveScriptFromList((::CRunningScript**)queuelist); // CRunningScript from Plugin SDK
    }
}
