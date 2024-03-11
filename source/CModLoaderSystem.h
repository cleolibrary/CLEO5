
#include <filesystem>
#include <forward_list>
#include <set>
#include <string>

namespace CLEO
{
    class CRunningScript;

    class CModLoaderSystem
    {
        bool active = false; // ModLoader present?
        std::filesystem::path gameDir;
        std::filesystem::path modloaderDir;

        struct ModDesc 
        { 
            std::string name; 
            size_t priority = 50;
            bool hasCleo = false;
            std::set<std::string> cleoFiles;
        };
        std::forward_list<ModDesc> mods;

        void AddMod(const char* name, size_t priority);

    public:
        CModLoaderSystem() = default;
        ~CModLoaderSystem() = default;

        void Init();
        void Update(); // reload config and mods

        bool IsActive() const;
        bool HasMod(const char* name) const;

        std::string ResolvePath(CRunningScript* thread, const char* path) const;
    };
}
