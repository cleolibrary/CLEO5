#pragma once
#include <string>
#include <unordered_map>
#include <SimpleIni.h>

namespace CLEO
{
    class CConfigManager
    {
      public:
        // Read methods
        static int ReadInt(const char* section, const char* key, int defaultValue);
        static float ReadFloat(const char* section, const char* key, float defaultValue);
        static std::string ReadString(const char* section, const char* key, const char* defaultValue);
        static size_t ReadString(
            const char* section, const char* key, const char* defaultValue, char* buffer, size_t bufferSize
        );
        // Write methods
        static bool WriteInt(const char* section, const char* key, int value);
        static bool WriteFloat(const char* section, const char* key, float value);
        static bool WriteString(const char* section, const char* key, const char* value);

        // Get config file path
        static const char* GetConfigPath();

        // Clears the cached values and forces a re-read from disk on next access
        static void Reload();

      private:
        static CSimpleIniA ini;
        static bool iniLoaded;
        static std::unordered_map<std::string, std::string> cache;

        static bool SaveIni();
        static void EnsureLoaded();
        static void UpdateUserConfig(const char* defaultData, size_t defaultSize);
        static std::string StripInlineComment(const char* value);
        static std::string MakeKey(const char* section, const char* key);
        static const std::string& GetCachedValue(const char* section, const char* key);
    };
} // namespace CLEO