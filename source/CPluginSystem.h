#pragma once
#include <windows.h>
#include <list>
#include <string>
#include <unordered_map>

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
        std::unordered_map<std::string, FARPROC> exportsCache;

      public:
        CPluginSystem()                     = default;
        CPluginSystem(const CPluginSystem&) = delete; // no copying
        ~CPluginSystem();

        void LoadPlugins();
        void UnloadPlugins();
        size_t GetNumPlugins() const;

        // Find an exported function by name in .cleo plugins
        FARPROC FindPluginExport(const char* name);

        void LogLoadedPlugins() const;
    };
} // namespace CLEO
