#include "stdafx.h"
#include "CCustomScript.h"
#include "CleoBase.h"
#include "crc32.h"
#include "CScriptEngine.h"
#include "ScriptUtils.h"

using namespace CLEO;


// TODO: Consider split into 2 classes: CCustomExternalScript, CCustomChildScript
CCustomScript::CCustomScript(const char* szFileName, bool bIsMiss, CRunningScript* parent, int label)
    : CRunningScript(), m_saveEnabled(false), m_ok(false),
    m_compatVer(CLEO_VER_CUR)
{
    TRACE(""); // separator
    TRACE("Loading custom script '%s'...", szFileName);

    bIsCustom = true;
    bIsMission = bUseMissionCleanup = bIsMiss;

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

            if (parent != nullptr)
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
                CleoInstance.ScriptEngine.missionIndex = -1;
                BaseIP = CurrentIP = CleoInstance.ScriptEngine.missionBlock; // TODO: there should be check length <= missionBlock size
            }
            else
            {
                BaseIP = CurrentIP = new BYTE[length];
            }
            is.read(reinterpret_cast<char*>(BaseIP), length);

            m_codeSize = length;
            m_codeSize -= GetExtraInfoSize(reinterpret_cast<BYTE*>(BaseIP), m_codeSize); // just the code without extra SCM data at the end

            m_codeChecksum = crc32(reinterpret_cast<BYTE*>(BaseIP), length);

            // thread name from filename
            auto threadNamePath = path;
            if (threadNamePath.extension() == cs3_ext || threadNamePath.extension() == cs4_ext)
            {
                threadNamePath.replace_extension(cs_ext); // keep original extension even in compatibility modes
            }
            auto fName = threadNamePath.filename().string();

            memset(Name, '\0', sizeof(Name));
            if (!fName.empty())
            {
                auto len = std::min(fName.length(), sizeof(Name) - 1); // and text terminator
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

void CCustomScript::ShutdownThisScript()
{
    ((::CRunningScript*)this)->ShutdownThisScript(); // CRunningScript from Plugin SDK
}

bool CCustomScript::GetDebugMode() const
{
    return bIsCustom ? m_debugMode : CleoInstance.ScriptEngine.NativeScriptsDebugMode;
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
    return bIsCustom ? m_scriptFileDir.c_str() : CleoInstance.ScriptEngine.MainScriptFileDir.c_str();
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
    return bIsCustom ? m_scriptFileName.c_str() : CleoInstance.ScriptEngine.MainScriptFileName.c_str();
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
    return bIsCustom ? m_workDir.c_str() : CleoInstance.ScriptEngine.MainScriptCurWorkDir.c_str();
}

void CCustomScript::SetWorkDir(const char* directory)
{
    if (directory == nullptr || strlen(directory) == 0)
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
    enum class VPref { None, Game, User, Script, Cleo, Modules } virtualPrefix = VPref::None;
    if (!fsPath.empty())
    {
        const auto root = fsPath.begin()->string(); // first path element
        const auto r = root.c_str();

        if (_strcmpi(r, DIR_GAME) == 0) virtualPrefix = VPref::Game;
        else if (_strcmpi(r, DIR_USER) == 0) virtualPrefix = VPref::User;
        else if (_strcmpi(r, DIR_SCRIPT) == 0 && !IsLegacyScript((CRunningScript*)this)) virtualPrefix = VPref::Script;
        else if (_strcmpi(r, DIR_CLEO) == 0) virtualPrefix = VPref::Cleo;
        else if (_strcmpi(r, DIR_MODULES) == 0) virtualPrefix = VPref::Modules;
    }

    // not virtual
    if (virtualPrefix == VPref::None)
    {
        if (fsPath.is_relative())
        {
            if (customWorkDir != nullptr)
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
    switch (virtualPrefix)
    {
        case VPref::User: resolved = Filepath_User; break;
        case VPref::Script: resolved = GetScriptFileDir(); break;
        case VPref::Game: resolved = Filepath_Game; break;
        case VPref::Cleo: resolved = Filepath_Cleo; break;
        case VPref::Modules: resolved = Filepath_Cleo + "\\cleo_modules"; break;
        default: resolved = "<error>"; break; // should never happen
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

    if (memcmp(threadName.c_str(), fileName, threadName.length()) != 0) // thread name no longer same as filename (was set with 03A4)
    {
        ss << "'" << threadName << "' from ";
    }

    ss << "'" << fileName << "'";

    if (currLineInfo)
    {
        ss << " at ";

        if (false)
        {
            // TODO: get Sanny's SMC extra info
            ss << "line " << 0;
            ss << " - ";
            ss << "CODE";
        }
        else
        {
            auto offset = CLEO_GetScriptBaseRelativeOffset((CLEO::CRunningScript*)this, (BYTE*)CCustomOpcodeSystem::lastOpcodePtr);
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

            // add previously executed command info if available
            if (CCustomOpcodeSystem::prevOpcode != 0xFFFF)
            {
                ss << " \nPreviously called command: ";

                auto commandName = CleoInstance.OpcodeInfoDb.GetCommandName(CCustomOpcodeSystem::prevOpcode);
                if (commandName)
                    ss << commandName;
                else
                    ss << "[" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << CCustomOpcodeSystem::prevOpcode << "]";
            }
        }
    }

    return ss.str();
}
