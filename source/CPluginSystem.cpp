#include "stdafx.h"
#include "CPluginSystem.h"
#include "CleoBase.h"


using namespace CLEO;

CPluginSystem::~CPluginSystem()
{
    UnloadPlugins();
}

void CPluginSystem::LoadPlugins(bool fromModLoader)
{
    if (!fromModLoader && pluginsLoaded) return; // plugins already loaded
    if (fromModLoader && mlPluginsLoaded) return; // plugins from ModLoader already loaded

    if (fromModLoader) mlPluginsLoaded = true;
    else pluginsLoaded = true;

    auto& modLoader = GetInstance().ModLoaderSystem;
    if (fromModLoader && !modLoader.IsActive()) return; // ModLoader not present

    std::set<std::string> names;
    std::vector<std::string> paths;
    std::set<std::string> skippedPaths;

    // load plugins from main CLEO directory
    auto ScanPluginsDir = [&](std::string path, const std::string prefix, const std::string extension)
    {
        auto pattern = path + '\\' + prefix + '*' + extension;

        auto files = fromModLoader ?
            modLoader.ListCleoFiles(pattern.c_str()) :
            CLEO_ListDirectory(nullptr, pattern.c_str(), false, true);

        for (DWORD i = 0; i < files.count; i++)
        {
            if (std::find(paths.begin(), paths.end(), files.strings[i]) != paths.end())
                continue; // file already listed

            if (skippedPaths.find(files.strings[i]) != skippedPaths.end())
                continue; // file already skipped

            auto name = FS::path(files.strings[i]).filename().string();
            name = name.substr(prefix.length()); // cut off prefix
            name.resize(name.length() - extension.length()); // cut off extension

            // case insensitive search in already listed plugin names
            auto found = std::find_if(names.begin(), names.end(), [&](const std::string& s) {
                return _stricmp(s.c_str(), name.c_str()) == 0;
            });

            if (found == names.end())
            {
                names.insert(name);
                paths.emplace_back(files.strings[i]);
                TRACE(" - '%s'", files.strings[i]);
            }
            else
            {
                skippedPaths.emplace(files.strings[i]);
                LOG_WARNING(0, " - '%s' skipped, duplicate of `%s` plugin", files.strings[i], name.c_str());
            }
        }

        if (fromModLoader) modLoader.StringListFree(files);
        else CLEO_StringListFree(files);
    };

    if (fromModLoader)
    {
        TRACE(""); // separator
        TRACE("Listing CLEO plugins from ModLoader:");

        // ModLoader loads *.cleo plugins itself
        auto files = modLoader.ListCleoFiles("*.cleo");
        mlPluginCount = files.count;
        modLoader.StringListFree(files);
        if (mlPluginCount > 0)
        {
            TRACE(" - %d CLEO plugin(s) already loaded by ModLoader", mlPluginCount);
        }
        else
        {
            TRACE(" - nothing found");
        }
    }
    else
    {
        TRACE(""); // separator
        TRACE("Listing CLEO plugins:");
        ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "SA.", ".cleo");
        ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "", ".cleo"); // legacy plugins in new location
        ScanPluginsDir(Filepath_Cleo, "", ".cleo"); // legacy plugins in old location

        if (paths.empty()) TRACE(" - nothing found");
    }

    // reverse order, so opcodes from CLEO5 plugins can overwrite opcodes from legacy plugins
    for (auto it = paths.crbegin(); it != paths.crend(); it++)
    {
        const auto filename = it->c_str();
        TRACE(""); // separator
        TRACE("Loading plugin '%s'", filename);

        HMODULE hlib = LoadLibrary(filename);
        if (!hlib)
        {
            LOG_WARNING(0, "Error loading plugin '%s'", filename);
            continue;
        }

        plugins.emplace_back(filename, hlib);
    }
}

void CPluginSystem::UnloadPlugins()
{
    if (!pluginsLoaded) return;

    TRACE(""); // separator
    TRACE("Unloading CLEO plugins:");
    for (const auto& plugin : plugins)
    {
        TRACE(" - Unloading '%s' at 0x%08X", plugin.name.c_str(), plugin.handle);
        FreeLibrary(plugin.handle);
    }
    TRACE("CLEO plugins unloaded");

    plugins.clear();
    pluginsLoaded = false;
 }

size_t CPluginSystem::GetNumPlugins() const
{
    return plugins.size() + mlPluginCount;
}

