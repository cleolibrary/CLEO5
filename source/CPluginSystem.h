#pragma once
#include "FileEnumerator.h"
#include "CDebug.h"
#include <windows.h>
#include <algorithm>
#include <list>
#include <CMenuSystem.h>


namespace CLEO
{
    class CPluginSystem
    {
        std::list<HMODULE> plugins;
        bool pluginsLoaded = false;

        // Mod Loader
        bool mlPluginsLoaded = false;
        size_t mlPluginCount = 0; // legacy plugins loaded by Mod Loader itself

    public:
        CPluginSystem() = default;
        CPluginSystem(const CPluginSystem&) = delete; // no copying
        ~CPluginSystem();

        void LoadPlugins(bool modloader = false);
        size_t GetNumPlugins() const;
    };
}
