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

    public:
        CPluginSystem() = default;
        CPluginSystem(const CPluginSystem&) = delete; // no copying
        ~CPluginSystem();

        void LoadPlugins();
        size_t GetNumPlugins() const;
    };
}
