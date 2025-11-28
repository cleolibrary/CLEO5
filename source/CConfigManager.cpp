#include "stdafx.h"
#include "CConfigManager.h"

namespace CLEO
{

    int CConfigManager::ReadInt(const char* section, const char* key, int defaultValue)
    {
        char buffer[32];
        GetPrivateProfileString(section, key, "", buffer, sizeof(buffer), CConfigManager::GetConfigPath());

        if (buffer[0] == '\0') return defaultValue;

        char* end;
        // Try to parse as hex if it starts with "0x", otherwise parse as decimal
        int base   = (buffer[0] == '0' && buffer[1] == 'x') ? 16 : 10;
        long value = strtol(buffer, &end, base);

        if (end == buffer) // no conversion performed
            return defaultValue;

        return (int)value;
    }

    float CConfigManager::ReadFloat(const char* section, const char* key, float defaultValue)
    {
        char buffer[32];
        GetPrivateProfileString(section, key, "", buffer, sizeof(buffer), CConfigManager::GetConfigPath());

        if (buffer[0] == '\0') return defaultValue;

        char* end;
        float value = strtof(buffer, &end);

        if (end == buffer) // no conversion performed
            return defaultValue;

        return value;
    }

    std::string CConfigManager::ReadString(const char* section, const char* key, const char* defaultValue)
    {
        char buffer[512];
        GetPrivateProfileString(
            section, key, defaultValue ? defaultValue : "", buffer, sizeof(buffer), CConfigManager::GetConfigPath()
        );
        return std::string(buffer);
    }

    size_t CConfigManager::ReadString(
        const char* section, const char* key, const char* defaultValue, char* buffer, size_t bufferSize
    )
    {
        if (buffer == nullptr || bufferSize == 0) return 0;

        return GetPrivateProfileString(
            section, key, defaultValue ? defaultValue : "", buffer, (DWORD)bufferSize, CConfigManager::GetConfigPath()
        );
    }

    bool CConfigManager::WriteInt(const char* section, const char* key, int value)
    {
        char buffer[32];
        _itoa_s(value, buffer, 10);
        return WritePrivateProfileString(section, key, buffer, CConfigManager::GetConfigPath()) != 0;
    }

    bool CConfigManager::WriteFloat(const char* section, const char* key, float value)
    {
        char buffer[32];
        sprintf_s(buffer, "%g", value);
        return WritePrivateProfileString(section, key, buffer, CConfigManager::GetConfigPath()) != 0;
    }

    bool CConfigManager::WriteString(const char* section, const char* key, const char* value)
    {
        return WritePrivateProfileString(section, key, value, CConfigManager::GetConfigPath()) != 0;
    }

    const char* CConfigManager::GetConfigPath()
    {
        return Filepath_Config.c_str();
    }

} // namespace CLEO