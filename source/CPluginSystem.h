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

    public:
        CPluginSystem()
        {
            std::set<std::string> loaded;
            auto LoadPluginsDir = [&](std::string path, std::string prefix, std::string extension)
            {
                std::set<std::pair<std::string, std::string>> filesWithPrefix;
                std::set<std::pair<std::string, std::string>> filesWithoutPrefix;

                FilesWalk(path.c_str(), extension.c_str(), [&](const char* fullPath, const char* filename)
                {
                    std::string name = filename;
                    name.resize(name.length() - extension.length()); // cut off file type
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return ::tolower(c); });

                    if (_strnicmp(name.c_str(), prefix.c_str(), prefix.length()) == 0)
                    {
                        filesWithPrefix.insert({ fullPath, name.c_str() + prefix.length() });
                    }
                    else
                    {
                        filesWithoutPrefix.insert({ fullPath, name });
                    }
                });

                auto loadLib = [&](const char* fullPath, const char* name)
                {
                    if (loaded.find(name) != loaded.end())
                    {
                        LOG_WARNING(0, "Plugin `%s` already loaded. Skipping '%s'", name, fullPath);
                        return;
                    }

                    TRACE("Loading plugin '%s'", fullPath);
                    HMODULE hlib = LoadLibrary(fullPath);
                    if (!hlib)
                    {
                        LOG_WARNING(0, "Error loading plugin '%s'", fullPath);
                        return;
                    }

                    loaded.insert(name);
                    plugins.push_back(hlib);
                };

                // load with prefix first
                for (const auto& entry : filesWithPrefix) loadLib(entry.first.c_str(), entry.second.c_str());
                for (const auto& entry : filesWithoutPrefix) loadLib(entry.first.c_str(), entry.second.c_str());
            };

            TRACE("Loading plugins...");
            LoadPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "SA.", ".cleo"); // prioritize with prefix
            LoadPluginsDir(Filepath_Cleo.c_str(), "SA.", ".cleo"); // legacy plugins location
        }

        ~CPluginSystem()
        {
            std::for_each(plugins.begin(), plugins.end(), FreeLibrary);
        }

        inline size_t GetNumPlugins() { return plugins.size(); }
    };
}
