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
                std::set<std::pair<std::string, std::string>> found;

                FilesWalk(path.c_str(), extension.c_str(), [&](const char* fullPath, const char* filename)
                {
                    std::string name = filename;
                    name.resize(name.length() - extension.length()); // cut off file type
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });

                    found.insert({ fullPath, name });
                });

                // load with prefix first
                if (!prefix.empty())
                {
                    for (const auto& entry : found)
                    {
                        if (_strnicmp(entry.second.c_str(), prefix.c_str(), prefix.length()) != 0)
                        {
                            continue; // prefix missing
                        }

                        auto shortName = std::string(entry.second.c_str() + 3); // strip prefix

                        if (loaded.find(shortName) == loaded.end())
                        {
                            TRACE("Loading plugin '%s'", entry.first.c_str());
                            HMODULE hlib = LoadLibrary(entry.first.c_str());
                            if (!hlib)
                            {
                                LOG_WARNING(0, "Error loading plugin '%s'", entry.first.c_str());
                            }
                            else
                            {
                                loaded.insert(shortName);
                                plugins.push_back(hlib);
                            }
                        }
                        else
                        {
                            LOG_WARNING(0, "Plugin `%s` already loaded. Skipping '%s'", shortName.c_str(), entry.first.c_str());
                        }
                    }
                }

                // without prefix now
                for (const auto& entry : found)
                {
                    if (!prefix.empty() && _strnicmp(entry.second.c_str(), prefix.c_str(), prefix.length()) == 0)
                    {
                        continue; // has prefix
                    }

                    auto& shortName = entry.second;

                    if (loaded.find(shortName) == loaded.end())
                    {
                        TRACE("Loading plugin '%s'", entry.first.c_str());
                        HMODULE hlib = LoadLibrary(entry.first.c_str());
                        if (!hlib)
                        {
                            LOG_WARNING(0, "Error loading plugin '%s'", entry.first.c_str());
                        }
                        else
                        {
                            loaded.insert(shortName);
                            plugins.push_back(hlib);
                        }
                    }
                    else
                    {
                        LOG_WARNING(0, "Plugin `%s` already loaded. Skipping '%s'", shortName.c_str(), entry.first.c_str());
                    }
                }
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
