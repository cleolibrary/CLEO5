#include "stdafx.h"
#include "CConfigManager.h"
#include "resource.h"

namespace CLEO
{
    std::unordered_map<std::string, std::string> CConfigManager::cache;

    void CConfigManager::Initialize()
    {
        // get handle of the module containing this function (CLEO.asi)
        HMODULE hModule = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&CConfigManager::Initialize,
            &hModule
        );
        if (!hModule)
        {
            TRACE("Config initialization failed: could not obtain module handle");
            return;
        }

        // load embedded default config from resource
        HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCE(IDR_DEFAULT_CONFIG), RT_RCDATA);
        if (!hRes)
        {
            TRACE("Config initialization failed: embedded default config resource not found");
            return;
        }

        HGLOBAL hGlobal = LoadResource(hModule, hRes);
        if (!hGlobal)
        {
            TRACE("Config initialization failed: could not load embedded default config resource");
            return;
        }

        const char* defaultConfig = (const char*)LockResource(hGlobal);
        DWORD defaultConfigSize   = SizeofResource(hModule, hRes);
        if (!defaultConfig || defaultConfigSize == 0)
        {
            TRACE("Config initialization failed: embedded default config resource is empty");
            return;
        }

        // if config file does not exist, create it from the embedded default
        if (!FS::exists(Filepath_Config))
        {
            std::ofstream file(Filepath_Config, std::ios::binary);
            if (file.is_open())
            {
                file.write(defaultConfig, defaultConfigSize);
                TRACE("Config file created with default settings: %s", Filepath_Config.c_str());
            }
            return;
        }

        // config file exists - add any keys present in the default but missing from the user's file
        static const char sentinel[] = "\x01\x02\x03CLEO_KEY_NOT_FOUND";

        std::string section;
        std::istringstream stream(std::string(defaultConfig, defaultConfigSize));
        std::string line;
        while (std::getline(stream, line))
        {
            // strip carriage return
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // trim leading whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            if (start > 0) line = line.substr(start);

            // section header
            if (line[0] == '[')
            {
                size_t end = line.find(']');
                if (end != std::string::npos) section = line.substr(1, end - 1);
                continue;
            }

            // skip comments and empty lines
            if (line[0] == ';' || line[0] == '#') continue;

            // key = value pair
            size_t eq = line.find('=');
            if (eq == std::string::npos || section.empty()) continue;

            // extract and trim key
            std::string key = line.substr(0, eq);
            size_t keyEnd   = key.find_last_not_of(" \t");
            if (keyEnd == std::string::npos) continue;
            key = key.substr(0, keyEnd + 1);
            if (key.empty()) continue;

            // extract and trim value (strip inline comment)
            std::string value = line.substr(eq + 1);
            size_t valStart   = value.find_first_not_of(" \t");
            if (valStart != std::string::npos) value = value.substr(valStart);
            size_t commentPos = value.find(';');
            if (commentPos != std::string::npos) value = value.substr(0, commentPos);
            size_t valEnd = value.find_last_not_of(" \t");
            value         = (valEnd != std::string::npos) ? value.substr(0, valEnd + 1) : std::string();

            // check whether the key already exists using a unique sentinel default
            char existing[512];
            GetPrivateProfileStringA(
                section.c_str(), key.c_str(), sentinel, existing, sizeof(existing), Filepath_Config.c_str()
            );
            if (strcmp(existing, sentinel) == 0)
            {
                // key is absent – add it
                WritePrivateProfileStringA(
                    section.c_str(), key.c_str(), value.c_str(), Filepath_Config.c_str()
                );
                TRACE("Config: added missing key [%s] %s = %s", section.c_str(), key.c_str(), value.c_str());
            }
        }
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

        // Read from file and then cache
        char buffer[512];
        GetPrivateProfileString(section, key, "", buffer, sizeof(buffer), GetConfigPath());

        // remove comments starting with ;
        char* commentPos = strchr(buffer, ';');
        if (commentPos) *commentPos = '\0';

        return cache[cacheKey] = buffer;
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

        bool result = WritePrivateProfileString(section, key, value, GetConfigPath()) != 0;
        if (result) cache[cacheKey] = value;
        return result;
    }

    const char* CConfigManager::GetConfigPath()
    {
        return Filepath_Config.c_str();
    }

} // namespace CLEO