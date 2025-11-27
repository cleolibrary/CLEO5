#pragma once
#include <string>
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
    };
} // namespace CLEO