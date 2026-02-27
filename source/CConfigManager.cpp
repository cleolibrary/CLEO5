#include "stdafx.h"
#include "CConfigManager.h"
#include "resource.h"
#include <SimpleIni.h>

namespace CLEO
{
    static CSimpleIniA ini;
    static bool iniLoaded = false;

    std::unordered_map<std::string, std::string> CConfigManager::cache;

    HMODULE GetCurrentModule()
    {
        HMODULE hModule = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hModule);

        return hModule;
    }

    std::string CConfigManager::MakeKey(const char* section, const char* key)
    {
        return std::string(section) + "." + key;
    }

    const std::string& CConfigManager::GetCachedValue(const char* section, const char* key)
    {
        const auto cacheKey = MakeKey(section, key);
        const auto& it      = cache.find(cacheKey);
        if (it != cache.end()) return it->second;

        EnsureLoaded();

        return cache[cacheKey] = StripInlineComment(ini.GetValue(section, key, ""));
    }

    int CConfigManager::ReadInt(const char* section, const char* key, int defaultValue)
    {
        const std::string& value = GetCachedValue(section, key);
        if (value.empty()) return defaultValue;

        char* end;
        int result;

        if (value[0] == '0' && value[1] == 'x')
        {
            result = (int)strtoul(value.c_str(), &end, 16);
        }
        else
        {
            result = strtol(value.c_str(), &end, 10);
        }

        return (end == value.c_str()) ? defaultValue : result;
    }

    float CConfigManager::ReadFloat(const char* section, const char* key, float defaultValue)
    {
        const std::string& value = GetCachedValue(section, key);
        if (value.empty()) return defaultValue;

        char* end;
        const auto result = strtof(value.c_str(), &end);
        return (end == value.c_str()) ? defaultValue : result;
    }

    std::string CConfigManager::ReadString(const char* section, const char* key, const char* defaultValue)
    {
        const std::string& value = GetCachedValue(section, key);
        return value.empty() ? defaultValue : value;
    }

    size_t CConfigManager::ReadString(
        const char* section, const char* key, const char* defaultValue, char* buffer, size_t bufferSize
    )
    {
        if (buffer == nullptr || bufferSize == 0) return 0;

        const std::string& cached = GetCachedValue(section, key);
        const char* src           = cached.empty() ? defaultValue : cached.c_str();

        size_t len = strlen(src);
        if (len >= bufferSize) len = bufferSize - 1;

        memcpy(buffer, src, len);
        buffer[len] = '\0';
        return len;
    }

    bool CConfigManager::WriteInt(const char* section, const char* key, int value)
    {
        char buffer[32];
        _itoa_s(value, buffer, sizeof(buffer), 10);
        return WriteString(section, key, buffer);
    }

    bool CConfigManager::WriteFloat(const char* section, const char* key, float value)
    {
        char buffer[32];
        sprintf_s(buffer, "%g", value);
        return WriteString(section, key, buffer);
    }

    bool CConfigManager::WriteString(const char* section, const char* key, const char* value)
    {
        // check if cache already has the same value
        const auto cacheKey = MakeKey(section, key);
        if (cache.find(cacheKey) != cache.end() && cache[cacheKey] == value) return true;

        EnsureLoaded();

        if (ini.SetValue(section, key, value) < 0) return false;

        if (!SaveIni()) return false;

        cache[cacheKey] = value;
        return true;
    }

    const char* CConfigManager::GetConfigPath()
    {
        static const std::string path = GetCleoDirectory() + "\\.cleo_config.ini";
        return path.c_str();
    }

    std::string CConfigManager::StripInlineComment(const char* value)
    {
        if (!value) return {};

        std::string result(value);

        if (auto pos = result.find(';'); pos != std::string::npos)
        {
            result.erase(pos);
        }

        // trim trailing whitespace
        if (auto end = result.find_last_not_of(" \t"); end != std::string::npos)
        {
            result.erase(end + 1);
        }
        else
        {
            result.clear();
        }

        return result;
    }

    void CConfigManager::UpdateUserConfig(const char* defaultData, size_t defaultSize)
    {
        CSimpleIniA defaults;

        if (defaults.LoadData(defaultData, defaultSize) < 0) return;

        CSimpleIniA::TNamesDepend sections;
        defaults.GetAllSections(sections);
        sections.sort(CSimpleIniA::Entry::LoadOrder());

        bool modified = false;
        for (const auto& section : sections)
        {
            CSimpleIniA::TNamesDepend keys;
            defaults.GetAllKeys(section.pItem, keys);
            keys.sort(CSimpleIniA::Entry::LoadOrder());

            for (const auto& key : keys)
            {
                // skip if the user file already has this key
                if (ini.KeyExists(section.pItem, key.pItem)) continue;

                const char* value   = defaults.GetValue(section.pItem, key.pItem);
                const char* comment = key.pComment;

                ini.SetValue(section.pItem, key.pItem, value, comment);
                modified = true;
            }
        }

        if (modified)
        {
            SaveIni();
        }
    }

    void CConfigManager::EnsureLoaded()
    {
        if (iniLoaded) return;
        iniLoaded = true;

        const char* path  = GetConfigPath();
        const auto handle = GetCurrentModule();

        if (const auto iniFile = FindResource(handle, MAKEINTRESOURCE(IDR_DEFAULT_CONFIG), RT_RCDATA))
        {
            if (const auto resource = LoadResource(handle, iniFile))
            {
                const auto ptr  = (const char*)LockResource(resource);
                const auto size = SizeofResource(handle, iniFile);

                if (!FS::exists(path))
                {
                    // if config file does not exist, use embedded default
                    std::ofstream out(path, std::ios::binary);
                    if (out) out.write(ptr, size);
                    out.close();
                }
                else
                {
                    // merge any new keys from the embedded default into the user file
                    ini.LoadFile(path);
                    UpdateUserConfig(ptr, size);
                }
            }
        }
        ini.LoadFile(path);
    }

    bool CConfigManager::SaveIni()
    {
        const auto PADDING_SIZE = 43; // align values in INI file for better readability

        std::ofstream out(GetConfigPath(), std::ios::binary);
        if (!out) return false;

        // manually write INI file to preserve formatting

        CSimpleIniA::TNamesDepend sections;
        ini.GetAllSections(sections);
        sections.sort(CSimpleIniA::Entry::LoadOrder());

        for (const auto& section : sections)
        {
            out << "[" << section.pItem << "]" << std::endl;

            std::string lastGroup = {};

            CSimpleIniA::TNamesDepend keys;
            ini.GetAllKeys(section.pItem, keys);
            keys.sort(CSimpleIniA::Entry::LoadOrder());

            for (const auto& key : keys)
            {
                const char* value   = ini.GetValue(section.pItem, key.pItem);
                const char* comment = key.pComment;

                std::string group = key.pItem;
                if (auto pos = group.find_last_of('.'); pos != std::string::npos)
                {
                    group = group.substr(0, pos);
                }
                else
                {
                    group.clear();
                }

                // add a blank line when a new group of settings starts
                // (e.g. "DebugUtils.General", "MemoryOperations.Limits", etc)
                if (group != lastGroup)
                {
                    out << std::endl;
                    lastGroup = group;
                }

                std::string paddedKey = key.pItem;
                if (paddedKey.length() < PADDING_SIZE)
                {
                    paddedKey.append(PADDING_SIZE - paddedKey.length(), ' ');
                }

                out << paddedKey << " = " << value;

                if (comment) out << " ;" << comment;

                out << std::endl;
            }
            out << std::endl;
        }

        out.close();
        return true;
    }

} // namespace CLEO
