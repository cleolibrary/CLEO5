#include "stdafx.h"
#include "CConfigManager.h"

namespace CLEO
{
    std::unordered_map<std::string, std::string> CConfigManager::cache;

    std::string CConfigManager::MakeKey(const char* section, const char* key)
    {
        return std::string(section) + "\0" + key;
    }

    const std::string& CConfigManager::GetCachedValue(const char* section, const char* key)
    {
        const auto cacheKey = MakeKey(section, key);
        const auto& it      = cache.find(cacheKey);
        if (it != cache.end()) return it->second;

        // Read from file and then cache
        char buffer[512];
        GetPrivateProfileString(section, key, "", buffer, sizeof(buffer), GetConfigPath());
        return cache[cacheKey] = buffer;
    }

    int CConfigManager::ReadInt(const char* section, const char* key, int defaultValue)
    {
        const std::string& value = GetCachedValue(section, key);
        if (value.empty()) return defaultValue;

        char* end;
        const auto base   = (value[0] == '0' && value[1] == 'x') ? 16 : 10;
        const auto result = strtol(value.c_str(), &end, base);
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
        bool result = WritePrivateProfileString(section, key, buffer, GetConfigPath()) != 0;
        if (result) cache[MakeKey(section, key)] = buffer;
        return result;
    }

    bool CConfigManager::WriteFloat(const char* section, const char* key, float value)
    {
        char buffer[32];
        sprintf_s(buffer, "%g", value);
        bool result = WritePrivateProfileString(section, key, buffer, GetConfigPath()) != 0;
        if (result) cache[MakeKey(section, key)] = buffer;
        return result;
    }

    bool CConfigManager::WriteString(const char* section, const char* key, const char* value)
    {
        bool result = WritePrivateProfileString(section, key, value, GetConfigPath()) != 0;
        if (result) cache[MakeKey(section, key)] = value;
        return result;
    }

    const char* CConfigManager::GetConfigPath()
    {
        return Filepath_Config.c_str();
    }

} // namespace CLEO