#include "stdafx.h"
#include "CleoBase.h"


namespace CLEO
{
    CRunningScript* CScriptEngine::lastScript = nullptr;
    WORD CScriptEngine::lastOpcode = 0xFFFF;
    BYTE* CScriptEngine::lastOpcodePtr = nullptr;
    std::string CScriptEngine::lastErrorMsg = {};
    WORD CScriptEngine::prevOpcode = 0xFFFF;
    BYTE CScriptEngine::handledParamCount = 0;

    SCRIPT_VAR CScriptEngine::CleoVariables[0x400] = { 0 };

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

    void __cdecl CScriptEngine::HOOK_DrawScriptText(char beforeFade)
    {
        DrawScriptText_Orig(beforeFade);

        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptDraw))
        {
            typedef void WINAPI callback(bool);
            ((callback*)func)(beforeFade != 0);
        }
    }

    void CScriptEngine::HOOK_LoadScmData(void)
    {
        TRACE("Loading scripts save data...");
        CTheScripts::Load();
    }

    void CScriptEngine::HOOK_SaveScmData(void)
    {
        TRACE("Saving scripts save data...");
        CleoInstance.ScriptEngine.SaveState();
        CleoInstance.ScriptEngine.UnregisterAllCustomScripts();
        CTheScripts::Save();
        CleoInstance.ScriptEngine.ReregisterAllCustomScripts();
    }

    const char* __fastcall CScriptEngine::HOOK_GetScriptStringParam(CRunningScript* thread, int dummy, char* buff, int buffLen)
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

            if ((size_t)str <= MinValidAddress)
            {
                LOG_WARNING(thread, "Invalid '0x%X' pointer of input string argument %s in script %s", str, GetParamInfo().c_str(), ScriptInfoStr(thread).c_str());
                return nullptr; // error
            }

            auto len = std::min((int)strlen(str), buffLen);
            memcpy(buff, str, len);
            if (len < buffLen) buff[len] = '\0'; // add terminator if possible
            return str; // pointer to original data
        }
        else if (paramType == DT_VARLEN_STRING)
        {
            handledParamCount++;
            thread->IncPtr(1); // already processed paramType

            DWORD length = *thread->GetBytePointer(); // as unsigned byte!
            thread->IncPtr(1); // length info

            char* str = (char*)thread->GetBytePointer();
            thread->IncPtr(length); // text data

            memcpy(buff, str, std::min(buffLen, (int)length));
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
                    handledParamCount++;
                    memcpy(buff, str, std::min(buffLen, 8));
                    thread->IncPtr(8); // text data
                    return buff;
                }

                case DT_STRING:
                {
                    handledParamCount++;
                    memcpy(buff, str, std::min(buffLen, 16));
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
                    memcpy(buff, str, std::min(buffLen, 8));
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
                    memcpy(buff, str, std::min(buffLen, 16));
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

    void CScriptEngine::ReadScriptParams(CRunningScript* script, BYTE count)
    {
        ((::CRunningScript*)script)->CollectParameters(count);
        handledParamCount += count;
    }

    void CScriptEngine::WriteScriptParams(CRunningScript* script, BYTE count)
    {
        ((::CRunningScript*)script)->StoreParameters(count);
        handledParamCount += count;
    }

    SCRIPT_VAR* CScriptEngine::GetScriptParamPointer(CRunningScript* thread)
    {
        auto type = DT_DWORD; //thread->PeekDataType(); // ignored in GetPointerToScriptVariable anyway
        auto ptr = ((::CRunningScript*)thread)->GetPointerToScriptVariable(type);
        handledParamCount++; // TODO: hook game's GetPointerToScriptVariable so this is always incremented?
        return (SCRIPT_VAR*)ptr;
    }

    StringParamBufferInfo CScriptEngine::ReadStringParamWriteBuffer(CRunningScript* thread)
    {
        StringParamBufferInfo result;
        lastErrorMsg.clear();

        auto paramType = thread->PeekDataType();
        if (IsImmInteger(paramType) || IsVariable(paramType))
        {
            // address to output buffer
            ReadScriptParams(thread, 1);

            if (opcodeParams[0].dwParam <= MinValidAddress)
            {
                lastErrorMsg = StringPrintf("Writing string into invalid '0x%X' pointer argument", opcodeParams[0].dwParam);
                return result; // error
            }

            result.data = opcodeParams[0].pcParam;
            result.size = 0x7FFFFFFF; // user allocated memory block can be any size
            result.needTerminator = true;

            return result;
        }
        else
            if (IsVarString(paramType))
            {
                switch (paramType)
                {
                    // short string variable
                    case DT_VAR_TEXTLABEL:
                    case DT_LVAR_TEXTLABEL:
                    case DT_VAR_TEXTLABEL_ARRAY:
                    case DT_LVAR_TEXTLABEL_ARRAY:
                        result.data = (char*)GetScriptParamPointer(thread);
                        result.size = 8;
                        result.needTerminator = false;
                        return result;

                    // long string variable
                    case DT_VAR_STRING:
                    case DT_LVAR_STRING:
                    case DT_VAR_STRING_ARRAY:
                    case DT_LVAR_STRING_ARRAY:
                        result.data = (char*)GetScriptParamPointer(thread);
                        result.size = 16;
                        result.needTerminator = false;
                        return result;
                }
            }

        lastErrorMsg = StringPrintf("Writing string, got argument %s", ToKindStr(paramType));
        CLEO_SkipOpcodeParams(thread, 1); // skip unhandled param
        return result; // error
    }

    const char* CScriptEngine::ReadStringParam(CRunningScript* thread, char* buff, int buffSize)
    {
        if (buffSize > 0) buff[buffSize - 1] = '\0'; // buffer always terminated
        return HOOK_GetScriptStringParam(thread, 0, buff, buffSize - 1); // minus terminator
    }

    // perform sprintf-operation for parameters passed through SCM
    int CScriptEngine::ReadFormattedString(CRunningScript* thread, char* outputStr, DWORD len, const char* format)
    {
        unsigned int written = 0;
        const char* iter = format;
        char* outIter = outputStr;
        char bufa[MAX_STR_LEN + 1], fmtbufa[64], * fmta;

        // invalid input arguments
        if (outputStr == nullptr || len == 0)
        {
            LOG_WARNING(thread, "ReadFormattedString invalid input arg(s) in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
            SkipUnusedVarArgs(thread);
            return -1; // error
        }

        if (len > 1 && format != nullptr)
        {
            while (*iter)
            {
                while (*iter && *iter != '%')
                {
                    if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
                    *outIter++ = *iter++;
                }

                if (*iter == '%')
                {
                    // end of format string
                    if (iter[1] == '\0')
                    {
                        LOG_WARNING(thread, "ReadFormattedString encountered incomplete format specifier in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
                        SkipUnusedVarArgs(thread);
                        return -1; // error
                    }

                    // escaped % character
                    if (iter[1] == '%')
                    {
                        if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
                        *outIter++ = '%';
                        iter += 2;
                        continue;
                    }

                    //get flags and width specifier
                    fmta = fmtbufa;
                    *fmta++ = *iter++;
                    while (*iter == '0' ||
                        *iter == '+' ||
                        *iter == '-' ||
                        *iter == ' ' ||
                        *iter == '*' ||
                        *iter == '#')
                    {
                        if (*iter == '*')
                        {
                            //get width
                            if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                            ReadScriptParams(thread, 1);
                            _itoa_s(opcodeParams[0].dwParam, bufa, 10);

                            char* buffiter = bufa;
                            while (*buffiter)
                                *fmta++ = *buffiter++;
                        }
                        else
                            *fmta++ = *iter;
                        iter++;
                    }

                    //get immidiate width value
                    while (isdigit(*iter))
                        *fmta++ = *iter++;

                    //get precision
                    if (*iter == '.')
                    {
                        *fmta++ = *iter++;
                        if (*iter == '*')
                        {
                            if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                            ReadScriptParams(thread, 1);
                            _itoa_s(opcodeParams[0].dwParam, bufa, 10);

                            char* buffiter = bufa;
                            while (*buffiter)
                                *fmta++ = *buffiter++;
                        }
                        else
                            while (isdigit(*iter))
                                *fmta++ = *iter++;
                    }
                    //get size
                    if (*iter == 'h' || *iter == 'l')
                        *fmta++ = *iter++;

                    switch (*iter)
                    {
                    case 's':
                    {
                        if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;

                        const char* str = ReadStringParam(thread, bufa, sizeof(bufa));
                        if (str == nullptr) // read error
                        {
                            static const char none[] = "(INVALID_STR)";
                            str = none;
                        }

                        while (*str)
                        {
                            if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
                            *outIter++ = *str++;
                        }
                        iter++;
                        break;
                    }

                    case 'c':
                        if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
                        if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                        ReadScriptParams(thread, 1);
                        *outIter++ = (char)opcodeParams[0].nParam;
                        iter++;
                        break;

                    default:
                    {
                        // For non wc types, use system sprintf and append to wide char output
                        // FIXME: for unrecognised types, should ignore % when printing
                        if (*iter == 'p' || *iter == 'P')
                        {
                            if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                            ReadScriptParams(thread, 1);
                            sprintf_s(bufa, "%08X", opcodeParams[0].dwParam);
                        }
                        else
                        {
                            *fmta++ = *iter;
                            *fmta = '\0';
                            if (*iter == 'a' || *iter == 'A' ||
                                *iter == 'e' || *iter == 'E' ||
                                *iter == 'f' || *iter == 'F' ||
                                *iter == 'g' || *iter == 'G')
                            {
                                if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                                ReadScriptParams(thread, 1);
                                sprintf_s(bufa, fmtbufa, opcodeParams[0].fParam);
                            }
                            else
                            {
                                if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
                                ReadScriptParams(thread, 1);
                                sprintf_s(bufa, fmtbufa, opcodeParams[0].pParam);
                            }
                        }
                        char* bufaiter = bufa;
                        while (*bufaiter)
                        {
                            if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
                            *outIter++ = *bufaiter++;
                        }
                        iter++;
                        break;
                    }
                    }
                }
            }
        }

        if (written >= len)
        {
        _ReadFormattedString_OutOfMemory: // jump here on error

            LOG_WARNING(thread, "Target buffer too small (%d) to read whole formatted string in script %s", len, ((CCustomScript*)thread)->GetInfoStr().c_str());
            SkipUnusedVarArgs(thread);
            outputStr[len - 1] = '\0';
            return -1; // error
        }

        // still more var-args available
        if (thread->PeekDataType() != DT_END)
        {
            LOG_WARNING(thread, "More params than slots in formatted string in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
        }
        SkipUnusedVarArgs(thread); // skip terminator too

        outputStr[written] = '\0';
        return (int)written;

    _ReadFormattedString_ArgMissing: // jump here on error
        LOG_WARNING(thread, "Less params than slots in formatted string in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
        thread->IncPtr(); // skip vararg terminator
        outputStr[written] = '\0';
        return -1; // error
    }

    // write output\result string parameter
    bool CScriptEngine::WriteStringParam(CRunningScript* thread, const char* str)
    {
        auto target = ReadStringParamWriteBuffer(thread);
        return WriteStringParam(target, str);
    }

    bool CScriptEngine::WriteStringParam(const StringParamBufferInfo& target, const char* str)
    {
        lastErrorMsg.clear();

        if (str != nullptr && (size_t)str <= MinValidAddress)
        {
            lastErrorMsg = StringPrintf("Writing string from invalid '0x%X' pointer", target.data);
            return false;
        }

        if ((size_t)target.data <= MinValidAddress)
        {
            lastErrorMsg = StringPrintf("Writing string into invalid '0x%X' pointer argument", target.data);
            return false;
        }

        if (target.size == 0)
        {
            return false;
        }

        bool addTerminator = target.needTerminator;
        size_t buffLen = target.size - addTerminator;
        size_t length = str == nullptr ? 0 : strlen(str);

        if (buffLen > length) addTerminator = true; // there is space left for terminator

        length = std::min(length, buffLen);
        if (length > 0) std::memcpy(target.data, str, length);
        if (addTerminator) target.data[length] = '\0';

        return true;
    }

    DWORD CScriptEngine::GetVarArgCount(CRunningScript* thread)
    {
        const auto ip = thread->GetBytePointer();
        const auto handledParams = handledParamCount;

        DWORD count = 0;
        while (thread->PeekDataType() != DT_END)
        {
            CLEO_SkipOpcodeParams(thread, 1);
            count++;
        }

        // restore original state
        thread->SetIp(ip);
        handledParamCount = handledParams;

        return count;
    }

    void CScriptEngine::SkipUnusedVarArgs(CRunningScript* thread)
    {
        while (thread->PeekDataType() != DT_END)
            CLEO_SkipOpcodeParams(thread, 1);

        thread->IncPtr(); // skip terminator
    }

    void CScriptEngine::ThreadJump(CRunningScript* thread, int offset)
    {
        thread->SetIp(offset < 0 ? thread->GetBasePointer() - offset : CleoInstance.ScriptEngine.scmBlock + offset);
    }

    void CScriptEngine::DrawScriptText_Orig(char beforeFade)
    {
        if (beforeFade)
            CleoInstance.ScriptEngine.DrawScriptTextBeforeFade_Orig(beforeFade);
        else
            CleoInstance.ScriptEngine.DrawScriptTextAfterFade_Orig(beforeFade);
    }

    void __fastcall CScriptEngine::HOOK_ProcessScript(CLEO::CRunningScript* pScript)
    {
        CleoInstance.ScriptEngine.GameBegin(); // all initialized and ready to process scripts

        // run registered callbacks
        bool process = true;
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptProcessBefore))
        {
            typedef bool WINAPI callback(CRunningScript*);
            process = process && ((callback*)func)(pScript);
        }

        if (process)
        {
            CleoInstance.ScriptEngine.ProcessScript_Orig(pScript);
        }

        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptProcessAfter))
        {
            typedef void WINAPI callback(CRunningScript*);
            ((callback*)func)(pScript);
        }
    }

    void CScriptEngine::Inject(CCodeInjector& inj)
    {
        TRACE("Injecting ScriptEngine: Phase 1");
        CGameVersionManager& gvm = CleoInstance.VersionManager;
            
        // Global Events crashfix
        //inj.MemoryWrite(0xA9AF6C, 0, 4);

        opcodeParams = (SCRIPT_VAR*)ScriptParams; // from Plugin SDK's TheScripts.h
        missionLocals = (SCRIPT_VAR*)CTheScripts::LocalVariablesForCurrentMission;
        staticThreads = (CRunningScript*)CTheScripts::ScriptsArray;

        // Protect script dependencies
        inj.ReplaceFunction(HOOK_ProcessScript, gvm.TranslateMemoryAddress(MA_CALL_PROCESS_SCRIPT), &ProcessScript_Orig);

        inj.ReplaceFunction(HOOK_DrawScriptText, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE), &DrawScriptTextAfterFade_Orig);
        inj.ReplaceFunction(HOOK_DrawScriptText, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE), &DrawScriptTextBeforeFade_Orig);

        inj.ReplaceFunction(HOOK_LoadScmData, gvm.TranslateMemoryAddress(MA_CALL_LOAD_SCM_DATA));
        inj.ReplaceFunction(HOOK_SaveScmData, gvm.TranslateMemoryAddress(MA_CALL_SAVE_SCM_DATA));
    }

    void CScriptEngine::InjectLate(CCodeInjector& inj)
    {
        TRACE("Injecting ScriptEngine: Phase 2");
        CGameVersionManager& gvm = CleoInstance.VersionManager;

        inj.InjectFunction(HOOK_GetScriptStringParam, gaddrof(::CRunningScript::ReadTextLabelFromScript));

        // limit adjusters support: get adresses from (possibly) patched references
        inj.MemoryRead(gvm.TranslateMemoryAddress(MA_SCM_BLOCK_REF), scmBlock);
        inj.MemoryRead(gvm.TranslateMemoryAddress(MA_MISSION_BLOCK_REF), missionBlock);
    }

    CScriptEngine::~CScriptEngine()
    {
        GameEnd();

        TRACE(""); // separator
        TRACE("Script Engine finalized:");
        TRACE(" Last opcode executed: %04X", lastOpcode);
        TRACE(" Previous opcode executed: %04X", prevOpcode);
    }

    CleoSafeHeader safe_header;
    ThreadSavingInfo *safe_info;
    unsigned long *stopped_info;
    std::unique_ptr<ThreadSavingInfo[]> safe_info_utilizer;
    std::unique_ptr<unsigned long[]> stopped_info_utilizer;

    void CScriptEngine::GameBegin()
    {
        auto& activeScriptsListHead = (CRunningScript*&)CTheScripts::pActiveScripts; // reference, but with type casted to CLEO's CRunningScript

        if(gameInProgress) return; // already started
        if(activeScriptsListHead == nullptr) return; // main gamescript not loaded yet 
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
        auto head = activeScriptsListHead;
        auto tail = head;
        while (tail->Next) tail = tail->Next;

        // load custom scripts as new list
        activeScriptsListHead = nullptr;
        LoadCustomScripts();

        // append custom scripts list to the back
        if (activeScriptsListHead != nullptr)
        {
            tail->Next = activeScriptsListHead;
            activeScriptsListHead->Previous = tail;
        }

        activeScriptsListHead = head; // restore original
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

    CCustomScript* CScriptEngine::LoadScript(const char * szFilePath)
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
        if (fromThread) fromThread->SetConditionResult(cs && cs->IsOk());
        if (cs && cs->IsOk())
        {
            AddCustomScript(cs);
            if (fromThread) ((::CRunningScript*)fromThread)->ReadParametersForNewlyStartedScript((::CRunningScript*)cs);
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
            sprintf_s(safe_name, "./cleo/cleo_saves/cs%d.sav", FrontEndMenuManager.m_nSelectedSaveGame);
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
            for (auto script = (CRunningScript*)CTheScripts::pActiveScripts; script; script = script->GetNext())
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
        for (auto script = (CRunningScript*)CTheScripts::pActiveScripts; script; script = script->GetNext())
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
        for (auto script = (CRunningScript*)CTheScripts::pActiveScripts; script != nullptr; script = script->GetNext())
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
        for (auto script = (CRunningScript*)CTheScripts::pActiveScripts; script != nullptr; script = script->GetNext())
        {
            if (script == ptr)
                return true;
        }

        for (auto script = (CLEO::CRunningScript*)CTheScripts::pIdleScripts; script != nullptr; script = script->GetNext())
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

        cs->AddScriptToList((CRunningScript**)&CTheScripts::pActiveScripts);
        cs->SetActive(true);

        // run registered callbacks
        for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptRegister))
        {
            typedef void WINAPI callback(CCustomScript*);
            ((callback*)func)(cs);
        }
    }

    void CScriptEngine::RemoveScript(CRunningScript* script)
    {
        if (script->IsMission()) CTheScripts::bAlreadyRunningAMissionScript = false;

        if (script->IsCustom())
        {
            RemoveCustomScript((CCustomScript*)script);
        }
        else // native script
        {
            auto cs = (CCustomScript*)script;
            cs->RemoveScriptFromList((CRunningScript**)&CTheScripts::pActiveScripts);
            cs->AddScriptToList((CRunningScript**)&CTheScripts::pIdleScripts);
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

        if (cs == CustomMission)
        {
            CustomMission = nullptr;
            CTheScripts::bAlreadyRunningAMissionScript = false; // on_mission
        }

        if (cs->m_parentScript)
        {
            cs->BaseIP = 0; // don't delete BaseIP if child thread
        }

        for (auto childThread : cs->m_childScripts)
        {
            RemoveScript(childThread);
        }

        cs->SetActive(false);
        cs->RemoveScriptFromList((CRunningScript**)&CTheScripts::pActiveScripts);
        CustomScripts.remove(cs);

        if (cs->m_saveEnabled && !cs->IsMission())
        {
            TRACE("Stopping custom script named '%s'", cs->GetName().c_str());
            InactiveScriptHashes.insert(cs->GetCodeChecksum());
        }
        else
        {
            TRACE("Unregistering custom %s named '%s'", cs->IsMission() ? "mission" : "script", cs->GetName().c_str());
            ScriptsWaitingForDelete.push_back(cs);
        }
    }

    void CScriptEngine::RemoveAllCustomScripts(void)
    {
        TRACE("Unloading scripts...");

        if (CustomMission)
        {
            RemoveCustomScript(CustomMission);
        }

        while (!CustomScripts.empty())
        {
            RemoveCustomScript(CustomScripts.back());
        }

        for (auto& script : ScriptsWaitingForDelete)
        {
            TRACE(" Deleting inactive script named '%s'", script->GetName().c_str());
            delete script;
        }
        ScriptsWaitingForDelete.clear();
    }

    void CScriptEngine::UnregisterAllCustomScripts()
    {
        TRACE("Unregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs)
        {
            cs->RemoveScriptFromList((CRunningScript**)&CTheScripts::pActiveScripts);
            cs->SetActive(false);
        });
    }

    void CScriptEngine::ReregisterAllCustomScripts()
    {
        TRACE("Reregistering all custom scripts");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs)
        {
            cs->AddScriptToList((CRunningScript**)&CTheScripts::pActiveScripts);
            cs->SetActive(true);
        });
    }
}
