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
        bool IsCleoScript(const modloader::file&) const; // is CleoScript file? (*.cs or legacy variants)
        bool IsScript(const modloader::file&) const; // is filetype any of known to store CLEO scripts?
        void AddFile(const modloader::file&);
        void RemoveFile(const modloader::file&);

        std::string ResolvePath(const char* scriptPath, const char* path) const; // resolve regular absolute path to actual ModLoader file. CLEO's virtual paths are not supported there!
        const modloader::file* GetCleoFile(const char* filePath) const; // get descriptor of listed file
        std::string GetScriptsCleoDir(const char* filePath) const; // get cleo directory location for specific script

        std::set<std::string> ListFiles(const char* scriptPath, const char* searchPattern, bool listDirs = true, bool listFiles = true) const;
        std::set<std::string> ListCleoFiles(const char* searchPattern) const;
        std::set<std::string> ListCleoStartupScripts() const; // all CLEO scripts that appear to be in 'cleo' directory

    private:
        static const char* const Mod_Loader_Dir;
        static const char* const Cleo_Dir;

        std::string gamePath;
        std::string modLoaderPath;
        std::string cleoDirPath;
        std::set<const modloader::file*> files;

        static void NormalizePath(std::string& path);
        static std::string_view GetModName(const modloader::file&);
    };
}
