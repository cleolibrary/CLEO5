#pragma once
#include <windows.h>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace CLEO
{
    class CPluginSystem
    {
        struct PluginEntry
        {
            std::string name;
            HMODULE handle = nullptr;

            PluginEntry() = default;
            PluginEntry(std::string name, HMODULE handle) : name(name), handle(handle) {}
        };
        std::list<PluginEntry> plugins;
        bool pluginsLoaded = false;
        std::unordered_map<std::string, void*> exportsCache;

        static std::vector<HMODULE> GetProcessModules();
        static std::string GetModulePath(HMODULE module);

      public:
        CPluginSystem()                     = default;
        CPluginSystem(const CPluginSystem&) = delete; // no copying
        ~CPluginSystem();

        void LoadPlugins();
        void UnloadPlugins();
        size_t GetNumPlugins() const;

        // Find an exported function by name in .cleo plugins
        void* FindPluginExport(const char* name);

        void LogLoadedPlugins() const;
    };
} // namespace CLEO
