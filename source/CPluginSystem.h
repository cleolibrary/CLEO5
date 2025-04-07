#pragma once
#include <windows.h>
#include <list>
#include <string>

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

    public:
        CPluginSystem() = default;
        CPluginSystem(const CPluginSystem&) = delete; // no copying
        ~CPluginSystem();

        void LoadPlugins();
        void UnloadPlugins();
        size_t GetNumPlugins() const;

        void LogLoadedPlugins() const;
    };
}
