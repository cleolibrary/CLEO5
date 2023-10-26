#include "stdafx.h"
#include "CleoBase.h"
#include "CFileMgr.h"
#include "CGame.h"

#include <filesystem>
#include <sstream>

namespace CLEO
{
    DWORD FUNC_AddScriptToQueue;
    DWORD FUNC_RemoveScriptFromQueue;
    DWORD FUNC_StopScript;
    DWORD FUNC_ScriptOpcodeHandler00;
    DWORD FUNC_GetScriptParams;
    DWORD FUNC_TransmitScriptParams;
    DWORD FUNC_SetScriptParams;
    DWORD FUNC_SetScriptCondResult;
    DWORD FUNC_GetScriptParamPointer1;
    DWORD FUNC_GetScriptStringParam;
    DWORD FUNC_GetScriptParamPointer2;

    void(__thiscall * AddScriptToQueue)(CRunningScript *, CRunningScript **queue);
    void(__thiscall * RemoveScriptFromQueue)(CRunningScript *, CRunningScript **queue);
    void(__thiscall * StopScript)(CRunningScript *);
    char(__thiscall * ScriptOpcodeHandler00)(CRunningScript *, WORD opcode);
    void(__thiscall * GetScriptParams)(CRunningScript *, int count);
    void(__thiscall * TransmitScriptParams)(CRunningScript *, CRunningScript *);
    void(__thiscall * SetScriptParams)(CRunningScript *, int count);
    void(__thiscall * SetScriptCondResult)(CRunningScript *, bool);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer1)(CRunningScript *);
    void(__thiscall * GetScriptStringParam)(CRunningScript *, char* buf, BYTE len);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer2)(CRunningScript *, int __unused__);

	void RunScriptDeleteDelegate(CRunningScript *script);

    void __fastcall _AddScriptToQueue(CRunningScript *pScript, int dummy, CRunningScript **queue)
    {
        _asm
        {
            push queue
            mov ecx, pScript
            call FUNC_AddScriptToQueue
        }
    }

    void __fastcall _RemoveScriptFromQueue(CRunningScript *pScript, int dummy, CRunningScript **queue)
    {
        _asm
        {
            push queue
            mov ecx, pScript
            call FUNC_RemoveScriptFromQueue
        }
    }

    void __fastcall _StopScript(CRunningScript *pScript)
    {
        _asm
        {
            mov ecx, pScript
            call FUNC_StopScript
        }
    }

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

    void __fastcall _GetScriptStringParam(CRunningScript *pScript, int dummy, char *buf, int len)
    {
        _asm
        {
            mov ecx, pScript
            push len
            push buf
            call FUNC_GetScriptStringParam
        }
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
    void(__cdecl * SaveScmData)();
    void(__cdecl * LoadScmData)();
    void(__cdecl * DrawScriptStuff)(char bBeforeFade);
    void(__cdecl * DrawScriptStuff_H)(char bBeforeFade);

    DWORD* GameTimer;
    extern "C"
    {
        eCLEO_Version WINAPI CLEO_GetScriptVersion(const CRunningScript* thread)
        {
            if (thread->IsCustom())
                return reinterpret_cast<const CCustomScript*>(thread)->GetCompatibility();
            else
                return CLEO::eCLEO_Version::CLEO_VER_CUR;
        }

        SCRIPT_VAR *opcodeParams;
        SCRIPT_VAR *missionLocals;
        CRunningScript *staticThreads;
    }

    BYTE *scmBlock;
    BYTE *MissionLoaded;
    BYTE *missionBlock;
    BOOL *onMissionFlag;
    CTexture *scriptSprites;
    BYTE *scriptDraws;
    WORD *numScriptDraws;
    WORD *numScriptTexts;
    BYTE *useTextCommands;
    BYTE *scriptTexts;

    CRunningScript **inactiveThreadQueue, **activeThreadQueue;
	CCustomScript *lastScriptCreated = nullptr;

    // called to initialise the scripts (after the main.scm has actually had a chance to set up)
    void OnInitScm1(void)
    {
        TRACE("Scripts initialized");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();

        GetInstance().ScriptEngine.Initialize();
        GetInstance().ModuleSystem.Clear();
        //GetInstance().ModuleSystem.LoadCleoModules(); // TODO: enbale if cleo_modules approved
        GetInstance().ScriptEngine.LoadCustomScripts(false);

        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScmInit1))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    // called on first load before the others
    void OnInitScm2(void)
    {
        TRACE("Scripts exclusively initialized");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();

        GetInstance().ScriptEngine.Initialize();
        GetInstance().ScriptEngine.LoadCustomScripts();

        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScmInit2))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    // called to load the scripts
    void OnInitScm3(void)
    {
        TRACE("Scripts loaded");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();

        GetInstance().ScriptEngine.Initialize();
        GetInstance().ScriptEngine.LoadCustomScripts(true);

        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScmInit3))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    extern "C" void __stdcall opcode_004E(CCustomScript *pScript)
    {
        if (pScript->IsCustom())
        {
            if (!pScript->IsMission())
            {
                TRACE("Incorrect usage of opcode [004E] in script %s.", pScript->GetName());
            }
            else *MissionLoaded = false;
            GetInstance().ScriptEngine.RemoveCustomScript(pScript);
        }
        else
        {
            if (pScript->IsMission()) *MissionLoaded = false;
            RemoveScriptFromQueue(pScript, activeThreadQueue);
            AddScriptToQueue(pScript, inactiveThreadQueue);
            StopScript(pScript);
        }
    }

    extern "C" void __declspec(naked) opcode_004E_hook(void)
    {
        __asm
        {
            push esi
            call opcode_004E
            pop edi
            mov al, 1
            pop esi
            mov ecx, [esp + 0x14]
            mov fs : 0, ecx
            add esp, 32
            ret 0x4
        }
    }

    void OnNewGame(void)
    {
        static struct CGangWeapons {
            BYTE _f0;
            BYTE _f1; // -
            DWORD weapon1;
            DWORD weapon2;
            DWORD weapon3;
        } *gangWeapons((CGangWeapons *)0xC0B870);	// 1.01 eu specific
        TRACE("New game started");
        gangWeapons[0].weapon1 = 22;
        gangWeapons[0].weapon2 = 28;
        gangWeapons[0].weapon3 = 0;

        gangWeapons[1].weapon1 = 22;
        gangWeapons[1].weapon2 = 0;
        gangWeapons[1].weapon3 = 0;

        gangWeapons[2].weapon1 = 22;
        gangWeapons[2].weapon2 = 0;
        gangWeapons[2].weapon3 = 0;

        gangWeapons[4].weapon1 = 24;
        gangWeapons[4].weapon2 = 28;
        gangWeapons[4].weapon3 = 0;

        gangWeapons[5].weapon1 = 24;
        gangWeapons[5].weapon2 = 0;
        gangWeapons[5].weapon3 = 0;

        gangWeapons[6].weapon1 = 22;
        gangWeapons[6].weapon2 = 30;
        gangWeapons[6].weapon3 = 0;

        gangWeapons[7].weapon1 = 22;
        gangWeapons[7].weapon2 = 28;
        gangWeapons[7].weapon3 = 0;

        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        GetInstance().SoundSystem.UnloadAllStreams();
        GetInstance().ScriptEngine.LoadCustomScripts();
    }

    void OnLoadScmData(void)
    {
        TRACE(__FUNCSIG__);
        LoadScmData();
    }

    void OnSaveScmData(void)
    {
        TRACE(__FUNCSIG__);
        GetInstance().ScriptEngine.SaveState();
        GetInstance().ScriptEngine.UnregisterAllScripts();
        SaveScmData();
        GetInstance().ScriptEngine.ReregisterAllScripts();
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
            hash(cs->dwChecksum), condResult(cs->bCondResult),
            logicalOp(cs->LogicalOp), notFlag(cs->NotFlag != false), ip_diff(cs->CurrentIP - reinterpret_cast<BYTE*>(cs->BaseIP))
        {
            sleepTime = cs->WakeTime >= *GameTimer ? 0 : cs->WakeTime - *GameTimer;
            std::copy(cs->LocalVar, cs->LocalVar + 32, tls);
            std::copy(cs->Timers, cs->Timers + 2, timers);
            std::copy(cs->Name, cs->Name + 8, threadName);
        }

        void Apply(CCustomScript *cs)
        {
            cs->dwChecksum = hash;
            std::copy(tls, tls + 32, cs->LocalVar);
            std::copy(timers, timers + 2, cs->Timers);
            cs->bCondResult = condResult;
            cs->WakeTime = *GameTimer + sleepTime;
            cs->LogicalOp = logicalOp;
            cs->NotFlag = notFlag;
            cs->CurrentIP = reinterpret_cast<BYTE*>(cs->BaseIP) + ip_diff;
            std::copy(threadName, threadName + 8, cs->Name);
            cs->bSaveEnabled = true;
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
        // run registered callbacks
        bool process = true;
        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScriptProcess))
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
        GetInstance().ScriptEngine.DrawScriptStuff(bBeforeFade);

        if(bBeforeFade)
            DrawScriptStuff_H(bBeforeFade);
        else
            DrawScriptStuff(bBeforeFade);

        // run registered callbacks
        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScriptDraw))
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
        if (*useTextCommands)
        {
            RestoreTextDrawDefaults();
            *numScriptTexts = 0;
            std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
            *numScriptDraws = 0;
            if (*useTextCommands == 1)
                *useTextCommands = 0;
        }
		
		ProcessScript(this);

        StoreScriptSpecifics();
    }
    void CCustomScript::Draw(char bBeforeFade)
    {
        // no point if this script doesn't draw
        if (script_draws.size() || script_texts.size())
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
        if (*numScriptDraws)
            script_draws.assign(scriptDraws, scriptDraws + (*numScriptDraws * DRAW_DATA_SIZE));
        else if (script_draws.size())
            script_draws.clear();
        if (*numScriptTexts)
            script_texts.assign(scriptTexts, scriptTexts + (*numScriptTexts * TEXT_DATA_SIZE));
        else if (script_texts.size())
            script_texts.clear();

        UseTextCommands = *useTextCommands;
        NumDraws = *numScriptDraws;
        NumTexts = *numScriptTexts;

        // restore SCM draws + texts
        if (numStoredDraws) std::copy(storedDraws, storedDraws + (numStoredDraws * DRAW_DATA_SIZE), scriptDraws);
        else std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
        if (numStoredTexts) std::copy(storedTexts, storedTexts + (numStoredTexts * TEXT_DATA_SIZE), scriptTexts);
        else RestoreTextDrawDefaults();
        *numScriptDraws = numStoredDraws;
        *numScriptTexts = numStoredTexts;
        *useTextCommands = storedUseTextCommands;
    }
    void CCustomScript::RestoreScriptDraws()
    {
        // store SCM draws + texts
        storedUseTextCommands = *useTextCommands;
        numStoredDraws = *numScriptDraws;
        numStoredTexts = *numScriptTexts;
        if (numStoredDraws)
            std::copy(scriptDraws, scriptDraws + (numStoredDraws *  DRAW_DATA_SIZE), storedDraws);
        if (numStoredTexts)
            std::copy(scriptTexts, scriptTexts + (numStoredTexts * TEXT_DATA_SIZE), storedTexts);

        // restore script draws + texts
        if (!script_draws.size()) *numScriptDraws = 0;
        else
        {
            std::copy(script_draws.begin(), script_draws.end(), scriptDraws);
            *numScriptDraws = NumDraws;
        }
        if (!script_texts.size()) *numScriptTexts = 0;
        else
        {
            std::copy(script_texts.begin(), script_texts.end(), scriptTexts);
            *numScriptTexts = NumTexts;
        }
        *useTextCommands = UseTextCommands;
    }

    bool CCustomScript::GetDebugMode() const
    {
        if (!bIsCustom)
            return GetInstance().ScriptEngine.NativeScriptsDebugMode;

        return bDebugMode;
    }

    void CCustomScript::SetDebugMode(bool enabled)
    {
        if (!bIsCustom)
            GetInstance().ScriptEngine.NativeScriptsDebugMode = enabled;
        else
            bDebugMode = enabled;
    }

    const char* CCustomScript::GetScriptFileDir() const
    {
        if(!bIsCustom)
            return GetInstance().ScriptEngine.MainScriptFileDir.c_str();

        return scriptFileDir.c_str();
    }

    void CCustomScript::SetScriptFileDir(const char* directory)
    {
        if (!bIsCustom)
            GetInstance().ScriptEngine.MainScriptFileDir = directory;
        else
            scriptFileDir = directory;
    }

    const char* CCustomScript::GetScriptFileName() const 
    {
        if (!bIsCustom)
            return GetInstance().ScriptEngine.MainScriptFileName.c_str();

        return scriptFileName.c_str();
    }

    void CCustomScript::SetScriptFileName(const char* filename) 
    {
        if (!bIsCustom)
            GetInstance().ScriptEngine.MainScriptFileName = filename;
        else
            scriptFileName = filename;
    }

    const char* CCustomScript::GetWorkDir() const
    {
        if (!bIsCustom)
            return GetInstance().ScriptEngine.MainScriptCurWorkDir.c_str();

        return workDir.c_str(); 
    }

    void CCustomScript::SetWorkDir(const char* directory)
    {
        if (!bIsCustom)
            GetInstance().ScriptEngine.MainScriptCurWorkDir = directory;
        else
            workDir = directory;
    }

    std::string CCustomScript::ResolvePath(const char* path, const char* customWorkDir) const
    {
        if (path == nullptr)
        {
            return {};
        }

        std::string result;
        if (strlen(path) < 2 || path[1] != ':') // does not start with drive letter
        {
            result = (customWorkDir != nullptr) ? customWorkDir : GetWorkDir();
            if (!result.empty() && result.back() == '\\') result.pop_back();

            if (strlen(path) > 0)
            {
                if(!result.empty()) result.push_back('\\');
                result.append(path);
            }
        }
        else
        {
            result = path;
        }

        // predefined CLEO paths starting with '[digit]:'
        if (result.length() < 2 || result[1] != ':' ||
            result[0] < DIR_GAME[0] || result[0] > DIR_MODULES[0]) // supported range
        {
            return result; // not predefined path prefix found
        }

        if (result[0] == DIR_USER[0]) // saves/settings location
        {
            return std::string(GetUserDirectory()) + &result[2]; // original path without '1:' prefix;
        }

        if (result[0] == DIR_SCRIPT[0]) // current script location
        {
            std::string resolved = ResolvePath(GetScriptFileDir());
            resolved += &result[2]; // original path without '2:' prefix;
            return resolved;
        }

        // game root directory
        std::string resolved = CFileMgr::ms_rootDirName;
        if(!resolved.empty() && resolved.back() == '\\') resolved.pop_back();

        if (result[0] == DIR_CLEO[0]) // cleo directory
        {
            resolved += "\\cleo";
        }
        else if (result[0] == DIR_MODULES[0]) // cleo modules directory
        {
            resolved += "\\cleo\\cleo_modules";
        }

        resolved += &result[2]; // original path without 'X:' prefix
        return resolved;
    }

    std::string CCustomScript::GetInfoStr(bool currLineInfo) const
    {
        std::ostringstream ss;

        auto threadName = GetName();
        auto fileName = GetScriptFileName();

        if(memcmp(threadName, fileName, strlen(threadName)) != 0) // thread name no longer same as filename (was set with 03A4)
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
                auto address = (DWORD)BaseIP;
                if (address == 0) address = GetInstance().VersionManager.TranslateMemoryAddress(MA_SCM_BLOCK);
                //address = (DWORD)CurrentIP - address; // processed position
                address = (DWORD)CCustomOpcodeSystem::lastOpcodePtr - address; // opcode position

                ss << "offset {" << address << "}"; // Sanny offsets style
                ss << " - ";
                ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << CCustomOpcodeSystem::lastOpcode << ": ...";
            }
        }

        return ss.str();
    }

    void CCustomScript::StoreScriptTextures()
    {
        // store this scripts textures + restore SCM textures + make sure this scripts textures arent cleared by another
        if (script_textures.size())
            script_textures.clear();
        for (int i = 0; i<NUM_STORED_SPRITES; ++i)
        {
            script_textures.push_back(*(RwTexture**)&scriptSprites[i]);
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
        if (!script_textures.size())
            std::fill((RwTexture**)scriptSprites, (RwTexture**)scriptSprites + NUM_STORED_SPRITES, nullptr);
        else
        {
            // restore textures for this script
            for (auto i = script_textures.begin(); i != script_textures.end(); ++i, ++n)
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
        CGameVersionManager& gvm = GetInstance().VersionManager;

        // Global Events crashfix
        //inj.MemoryWrite(0xA9AF6C, 0, 4);

        // Dirty hacks to keep compatibility with plugins + overcome VS thiscall restrictions
        FUNC_AddScriptToQueue = gvm.TranslateMemoryAddress(MA_ADD_SCRIPT_TO_QUEUE_FUNCTION);
        FUNC_RemoveScriptFromQueue = gvm.TranslateMemoryAddress(MA_REMOVE_SCRIPT_FROM_QUEUE_FUNCTION);
        FUNC_StopScript = gvm.TranslateMemoryAddress(MA_STOP_SCRIPT_FUNCTION);
        FUNC_ScriptOpcodeHandler00 = gvm.TranslateMemoryAddress(MA_SCRIPT_OPCODE_HANDLER0_FUNCTION);
        FUNC_GetScriptParams = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAMS_FUNCTION);
        FUNC_TransmitScriptParams = gvm.TranslateMemoryAddress(MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptParams = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptCondResult = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_COND_RESULT_FUNCTION);
        FUNC_GetScriptParamPointer1 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION);
        FUNC_GetScriptStringParam = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_STRING_PARAM_FUNCTION);
        FUNC_GetScriptParamPointer2 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION);

        AddScriptToQueue = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript**)>(_AddScriptToQueue);
        RemoveScriptFromQueue = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript**)>(_RemoveScriptFromQueue);
        StopScript = reinterpret_cast<void(__thiscall*)(CRunningScript*)>(_StopScript);
        ScriptOpcodeHandler00 = reinterpret_cast<char(__thiscall*)(CRunningScript*, WORD)>(_ScriptOpcodeHandler00);
        GetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_GetScriptParams);
        TransmitScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript*)>(_TransmitScriptParams);
        SetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_SetScriptParams);
        SetScriptCondResult = reinterpret_cast<void(__thiscall*)(CRunningScript*, bool)>(_SetScriptCondResult);
        GetScriptParamPointer1 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*)>(_GetScriptParamPointer1);
        GetScriptStringParam = reinterpret_cast<void(__thiscall*)(CRunningScript*, char*, BYTE)>(_GetScriptStringParam);
        GetScriptParamPointer2 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*, int)>(_GetScriptParamPointer2);

        InitScm = gvm.TranslateMemoryAddress(MA_INIT_SCM_FUNCTION);
        SaveScmData = gvm.TranslateMemoryAddress(MA_SAVE_SCM_DATA_FUNCTION);
        LoadScmData = gvm.TranslateMemoryAddress(MA_LOAD_SCM_DATA_FUNCTION);

        GameTimer = gvm.TranslateMemoryAddress(MA_GAME_TIMER);
        opcodeParams = gvm.TranslateMemoryAddress(MA_OPCODE_PARAMS);
        missionLocals = gvm.TranslateMemoryAddress(MA_MISSION_LOCALS);
        scmBlock = gvm.TranslateMemoryAddress(MA_SCM_BLOCK);
        MissionLoaded = gvm.TranslateMemoryAddress(MA_MISSION_LOADED);
        missionBlock = gvm.TranslateMemoryAddress(MA_MISSION_BLOCK);
        onMissionFlag = gvm.TranslateMemoryAddress(MA_ON_MISSION_FLAG);

        // Protect script dependencies
        auto addr = gvm.TranslateMemoryAddress(MA_CALL_PROCESS_SCRIPT);
        inj.MemoryReadOffset(addr.address + 1, ProcessScript);
        inj.ReplaceFunction(HOOK_ProcessScript, addr);

        scriptSprites = gvm.TranslateMemoryAddress(MA_SCRIPT_SPRITE_ARRAY);
        scriptDraws = gvm.TranslateMemoryAddress(MA_SCRIPT_DRAW_ARRAY);
        scriptTexts = gvm.TranslateMemoryAddress(MA_SCRIPT_TEXT_ARRAY);
        numScriptDraws = gvm.TranslateMemoryAddress(MA_NUM_SCRIPT_DRAWS);
        numScriptTexts = gvm.TranslateMemoryAddress(MA_NUM_SCRIPT_TEXTS);
        useTextCommands = gvm.TranslateMemoryAddress(MA_USE_TEXT_COMMANDS);

        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE).address + 1, CLEO::DrawScriptStuff);
        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE).address + 1, DrawScriptStuff_H);
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE));
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE));
        inj.MemoryWrite(gvm.TranslateMemoryAddress(MA_CODE_JUMP_FOR_TXD_STORE), OP_RET);

        inactiveThreadQueue = gvm.TranslateMemoryAddress(MA_INACTIVE_THREAD_QUEUE);
        activeThreadQueue = gvm.TranslateMemoryAddress(MA_ACTIVE_THREAD_QUEUE);
        staticThreads = gvm.TranslateMemoryAddress(MA_STATIC_THREADS);

        if (gvm.GetGameVersion() == GV_EU11)
        {
            inj.ReplaceFunction(OnInitScm3, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM3));
            inj.InjectFunction(OnNewGame, 0x5DEEA0);	// GV_EU11 specific
        }
        else
        {
            inj.ReplaceFunction(OnInitScm1, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM1));
            inj.ReplaceFunction(OnInitScm2, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM2));
            inj.ReplaceFunction(OnInitScm3, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM3));
        }

        inj.ReplaceFunction(OnLoadScmData, gvm.TranslateMemoryAddress(MA_CALL_LOAD_SCM_DATA));
        inj.ReplaceFunction(OnSaveScmData, gvm.TranslateMemoryAddress(MA_CALL_SAVE_SCM_DATA));
        inj.InjectFunction(&opcode_004E_hook, gvm.TranslateMemoryAddress(MA_OPCODE_004E));
    }

    CScriptEngine::CScriptEngine()
    {
        CustomMission = nullptr;
    }

    CScriptEngine::~CScriptEngine()
    {
        TRACE("Unloading scripts...");
        RemoveAllCustomScripts();
    }

    CleoSafeHeader safe_header;
    ThreadSavingInfo *safe_info;
    unsigned long *stopped_info;
    std::unique_ptr<ThreadSavingInfo[]> safe_info_utilizer;
    std::unique_ptr<unsigned long[]> stopped_info_utilizer;

    void CScriptEngine::Initialize()
    {
        if (CGame::bMissionPackGame == 0) // regular main game
        {
            MainScriptFileDir = std::string(DIR_GAME) + "\\data\\script";
            MainScriptFileName = "main.scm";
        }
        else // mission pack
        {
            MainScriptFileDir = std::string(DIR_USER) + "\\MPACK\\MPACK";
            MainScriptFileDir += std::to_string(CGame::bMissionPackGame);
            MainScriptFileName = "scr.scm";
        }

        NativeScriptsDebugMode = GetPrivateProfileInt("General", "DebugMode", 0, GetInstance().ConfigFilename.c_str()) != 0;
        MainScriptCurWorkDir = DIR_GAME;
    }

    void CScriptEngine::LoadCustomScripts(bool load_mode)
    {
        char safe_name[MAX_PATH];

        // steam offset is different, so get it manually for now
        CGameVersionManager& gvm = GetInstance().VersionManager;
        int nSlot = gvm.GetGameVersion() != GV_STEAM ? *(BYTE*)&MenuManager->m_nSelectedSaveGame : *((BYTE*)MenuManager + 0x15B);

        sprintf(safe_name, "./cleo/cleo_saves/cs%d.sav", nSlot);

        safe_info = nullptr;
        stopped_info = nullptr;
        safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;

        if (load_mode)
        {
            // load cleo saving file
            try
            {
                TRACE("Loading cleo safe %s", safe_name);
                std::ifstream ss(safe_name, std::ios::binary);
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
                TRACE("Loading of cleo safe %s failed: %s", safe_name, ex.what());
                safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;
                memset(CleoVariables, 0, sizeof(CleoVariables));
            }
        }
        else
        {
            memset(CleoVariables, 0, sizeof(CleoVariables));
        }

        // [game root]\cleo
        std::string scriptsDir = CFileMgr::ms_rootDirName;
        if (!scriptsDir.empty() && scriptsDir.back() != '\\') scriptsDir.push_back('\\');
        scriptsDir += "cleo";

        TRACE("Searching for cleo scripts");

        FilesWalk(scriptsDir.c_str(), cs_ext, [this](const char* fullPath, const char* filename) {
            auto cs = LoadScript(fullPath);
            cs->SetDebugMode(NativeScriptsDebugMode); // inherit from global state
        });

        FilesWalk(scriptsDir.c_str(), cs4_ext, [this](const char* fullPath, const char* filename) {
            auto cs = LoadScript(fullPath);
            if (cs) cs->SetCompatibility(CLEO_VER_4);
        });

        FilesWalk(scriptsDir.c_str(), cs3_ext, [this](const char* fullPath, const char* filename) {
            auto cs = LoadScript(fullPath);
            if (cs) cs->SetCompatibility(CLEO_VER_3);
        });

        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScriptsLoaded))
        {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
    }

    CCustomScript * CScriptEngine::LoadScript(const char * szFilePath)
    {
        auto cs = new CCustomScript(szFilePath);

        if (!cs || !cs->bOK)
        {
            TRACE("Loading of custom script %s failed", szFilePath);
            if (cs) delete cs;
            return nullptr;
        }

        cs->SetDebugMode(NativeScriptsDebugMode); // inherit from global state

        // check whether the script is in stop-list
        if (stopped_info)
        {
            for (size_t i = 0; i < safe_header.n_stopped_threads; ++i)
            {
                if (stopped_info[i] == cs->dwChecksum)
                {
                    TRACE("Custom script %s found in the stop-list", szFilePath);
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
                if (safe_info[i].hash == cs->dwChecksum)
                {
                    TRACE("Custom script %s found in the safe-list", szFilePath);
                    safe_info[i].Apply(cs);
                    break;
                }
            }
        }

        AddCustomScript(cs);
        return cs;
    }

    void CScriptEngine::SaveState()
    {
        try
        {
            std::list<CCustomScript *> savedThreads;
            std::for_each(CustomScripts.begin(), CustomScripts.end(), [this, &savedThreads](CCustomScript *cs) {
                if (cs->bSaveEnabled)
                    savedThreads.push_back(cs);
            });

            CleoSafeHeader header = { CleoSafeHeader::sign, savedThreads.size(), InactiveScriptHashes.size() };

            // steam offset is different, so get it manually for now
            CGameVersionManager& gvm = GetInstance().VersionManager;
            int nSlot = gvm.GetGameVersion() != GV_STEAM ? *(BYTE*)&MenuManager->m_nSelectedSaveGame : *((BYTE*)MenuManager + 0x15B);

            char safe_name[MAX_PATH];
            sprintf(safe_name, "./cleo/cleo_saves/cs%d.sav", nSlot);
            TRACE("Saving script engine state to the file %s", safe_name);

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

    CRunningScript *CScriptEngine::FindScriptNamed(const char *name)
    {
        for (auto script = *activeThreadQueue; script; script = script->GetNext())
        {
            if (_stricmp(name, script->GetName()) == 0)
                return script;
        }
        return nullptr;
    }
    CCustomScript *CScriptEngine::FindCustomScriptNamed(const char *name)
    {
        if (CustomMission)
        {
            if (_stricmp(name, CustomMission->Name) == 0) return CustomMission;
        }

        for (auto it = CustomScripts.begin(); it != CustomScripts.end(); ++it)
        {
            auto cs = *it;
            if (_stricmp(name, cs->Name) == 0)
                return cs;
        }

        return nullptr;
    }

    bool CScriptEngine::IsValidScriptPtr(const CRunningScript* ptr) const
    {
        for (auto script = *activeThreadQueue; script != nullptr; script = script->GetNext())
        {
            if (script == ptr)
                return true;
        }

        for (const auto script : CustomScripts)
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
            TRACE("Registering custom mission named %s", cs->Name);
            CustomMission = cs;
        }
        else
        {
            TRACE("Registering custom script named %s", cs->Name);
            CustomScripts.push_back(cs);
        }
        AddScriptToQueue(cs, activeThreadQueue);
        cs->SetActive(true);

        // run registered callbacks
        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScriptRegister))
        {
            typedef void WINAPI callback(CCustomScript*);
            ((callback*)func)(cs);
        }
    }

    void CScriptEngine::RemoveCustomScript(CCustomScript *cs)
    {
        // run registered callbacks
        for (void* func : GetInstance().GetCallbacks(eCallbackId::ScriptUnregister))
        {
            typedef void WINAPI callback(CCustomScript*);
            ((callback*)func)(cs);
        }

		if (cs->parentThread)
		{
			cs->BaseIP = 0; // don't delete BaseIP if child thread
		}
		for (auto childThread : cs->childThreads)
		{
			CScriptEngine::RemoveCustomScript(childThread);
		}
        if (cs == CustomMission)
        {
            TRACE("Unregistering custom mission named %s", cs->Name);
            RemoveScriptFromQueue(CustomMission, activeThreadQueue);
            ScriptsWaitingForDelete.push_back(cs);
            CustomMission->SetActive(false);
            CustomMission = nullptr;
            *MissionLoaded = false;
        }
        else
        {
            if (cs->bSaveEnabled)
            {
                InactiveScriptHashes.insert(cs->dwChecksum);
                TRACE("Stopping custom script named %s", cs->Name);
            }
            else
            {
                TRACE("Unregistering custom script named %s", cs->Name);
                ScriptsWaitingForDelete.push_back(cs);
            }

            //TRACE("Psyke!");

            CustomScripts.remove(cs);
            RemoveScriptFromQueue(cs, activeThreadQueue);
            cs->SetActive(false);

            /*if(!pScript->IsMission()) *MissionLoaded = false;
            RemoveScriptFromQueue(pScript, activeThreadQueue);
            AddScriptToQueue(pScript, inactiveThreadQueue);
            StopScript(pScript);*/
        }
    }

    void CScriptEngine::RemoveAllCustomScripts(void)
    {
        InactiveScriptHashes.clear();
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs) {
            TRACE("Unregistering custom script named %s", cs->Name);
            RemoveScriptFromQueue(cs, activeThreadQueue);
            //AddScriptToQueue(cs, inactiveThreadQueue);
            //if(cs->GetPrev()) cs->GetPrev()->SetNext(nullptr);
            //if(cs->GetNext()) cs->GetNext()->SetPrev(nullptr);
            //TRACE("Psyke!!");
            cs->SetActive(false);
            delete cs;
        });
        CustomScripts.clear();
        std::for_each(ScriptsWaitingForDelete.begin(), ScriptsWaitingForDelete.end(), [this](CCustomScript *cs) {
            TRACE("Deleting inactive script named %s", cs->Name);
            delete cs;
        });
        ScriptsWaitingForDelete.clear();
        if (CustomMission)
        {
            TRACE("Unregistering custom mission named %s", CustomMission->Name);
            RemoveScriptFromQueue(CustomMission, activeThreadQueue);
            CustomMission->SetActive(false);
            delete CustomMission;
            CustomMission = nullptr;
            *MissionLoaded = false;
        }
    }

    void CScriptEngine::UnregisterAllScripts()
    {
        TRACE("Unregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs) {
            RemoveScriptFromQueue(cs, activeThreadQueue);
            cs->SetActive(false);
        });
    }

    void CScriptEngine::ReregisterAllScripts()
    {
        TRACE("Reregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs) {
            AddScriptToQueue(cs, activeThreadQueue);
            cs->SetActive(true);
        });
    }

	// TODO: Consider split into 2 classes: CCustomExternalScript, CCustomChildScript
    CCustomScript::CCustomScript(const char *szFileName, bool bIsMiss, CCustomScript *parent, int label)
        : CRunningScript(), bSaveEnabled(false), bOK(false),
        LastSearchPed(0), LastSearchCar(0), LastSearchObj(0),
        CompatVer(CLEO_VER_CUR)
    {
        bIsCustom = true;
        bIsMission = bUseMissionCleanup = bIsMiss;
        UseTextCommands = 0;
        NumDraws = 0;
        NumTexts = 0;

        TRACE("Loading custom script %s...", szFileName);

        // store script file directory and name
        std::filesystem::path path = szFileName;
        path = std::filesystem::absolute(path);
        scriptFileDir = path.parent_path().string();
        scriptFileName = path.filename().string();

        workDir = DIR_GAME;

        try
        {
			std::ifstream is;
			if (label != 0) // Create external from label.
			{
				if (!parent)
					throw std::logic_error("Trying to create external thread from label without parent thread");

				BaseIP = parent->GetBasePointer();
				CurrentIP = parent->GetBasePointer() - label;
				memcpy(Name, parent->Name, sizeof(Name));
				dwChecksum = parent->dwChecksum;
				parentThread = parent;
				parent->childThreads.push_back(this);
			}
			else
			{
				using std::ios;
				std::ifstream is(szFileName, std::ios::binary);
				is.exceptions(std::ios::badbit | std::ios::failbit);
				std::size_t length;
				is.seekg(0, std::ios::end);
				length = (size_t)is.tellg();
				is.seekg(0, std::ios::beg);

				if (bIsMiss)
				{
					if (*MissionLoaded)
						throw std::logic_error("Starting of custom mission when other mission loaded");
					*MissionLoaded = 1;
					BaseIP = CurrentIP = missionBlock;
				}
				else {
					BaseIP = CurrentIP = new BYTE[length];
				}
				is.read(reinterpret_cast<char *>(BaseIP), length);

				auto fname = strrchr(szFileName, '\\') + 1;
				if (!fname) fname = strrchr(szFileName, '/') + 1;
				if (fname < szFileName) fname = szFileName;
				memcpy(Name, fname, sizeof(Name));
				Name[7] = '\0';
				dwChecksum = crc32(reinterpret_cast<BYTE *>(BaseIP), length);
			}
			lastScriptCreated = this;
            bOK = true;
        }
        catch (std::exception& e)
        {
            LOG_WARNING("Error during loading of custom script %s occured.\nError message: %s", szFileName, e.what());
        }
        catch (...)
        {
            LOG_WARNING("Unknown error during loading of custom script %s occured.", szFileName);
        }
    }

    CCustomScript::~CCustomScript()
    {
        if (BaseIP && !bIsMission) delete[] BaseIP;
		RunScriptDeleteDelegate(reinterpret_cast<CRunningScript*>(this));
		if (lastScriptCreated == this) lastScriptCreated = nullptr;
    }

	float VectorSqrMagnitude(CVector vector) { return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z; }
}
