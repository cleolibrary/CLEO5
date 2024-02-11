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
            auto LoadPluginsDir = [&](std::string path, std::string extension)
            { 
                FilesWalk(path.c_str(), extension.c_str(), [&](const char* fullPath, const char* filename)
                {
                    std::string name = filename;
                    name.resize(name.length() - extension.length()); // cut off file type
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });

                    if (loaded.find(name) == loaded.end())
                    {
                        TRACE("Loading plugin '%s'", fullPath);
                        HMODULE hlib = LoadLibrary(fullPath);
                        if (!hlib)
                        {
                            LOG_WARNING(0, "Error loading plugin '%s'", fullPath);
                        }
                        else
                        {
                            loaded.insert(name);
                            plugins.push_back(hlib);
                        }
                    }
                    else
                    {
                        LOG_WARNING(0, "Plugin `%s` already loaded. Skipping '%s'", name.c_str(), fullPath);
                    }
                });
            };

            TRACE("Loading plugins...");
            LoadPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), ".cleo5"); // CLEO5 plugins
            LoadPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), ".cleo"); // legacy plugins
            LoadPluginsDir(Filepath_Cleo.c_str(), ".cleo"); // legacy plugins location
        }

        ~CPluginSystem()
        {
            std::for_each(plugins.begin(), plugins.end(), FreeLibrary);
        }

        inline size_t GetNumPlugins() { return plugins.size(); }
    };
}
