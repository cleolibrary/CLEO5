#include "modloader\modloader.hpp"
#include <set>
#include <string>

namespace CLEO
{
    class CRunningScript;

    class ModLoaderProvider
    {
    public:
        ModLoaderProvider() = default;
        ~ModLoaderProvider() = default;

        void Init(const char* gamePath);

        bool IsHandled(const modloader::file&) const; // is this file interesting to the provider
        void AddFile(const modloader::file&);
        void RemoveFile(const modloader::file&);

        std::string ResolvePath(const char* scriptPath, const char* path) const;
        const modloader::file* GetCleoFile(const char* filePath) const;

        std::set<std::string> ListFiles(const char* scriptPath, const char* searchPattern, bool listDirs = true, bool listFiles = true) const;
        std::set<std::string> ListCleoFiles(const char* searchPattern) const;

    private:
        static const char* const Mod_Loader_Dir;

        std::string gamePath;
        std::string modLoaderPath;
        std::set<const modloader::file*> files;

        static void NormalizePath(std::string& path);
        static std::string_view GetModName(const modloader::file&);
    };
}
