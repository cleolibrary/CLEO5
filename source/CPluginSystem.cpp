#include "stdafx.h"
#include "CPluginSystem.h"
#include "CleoBase.h"


using namespace CLEO;

CPluginSystem::~CPluginSystem()
{
    UnloadPlugins();
}

void CPluginSystem::LoadPlugins()
{
    if (pluginsLoaded) return; // already done

    std::set<std::string> names;
    std::vector<std::string> paths;
    std::set<std::string> skippedPaths;

    // get blacklist from config
    auto blacklistStr = std::string(512, '\0');
    GetPrivateProfileString("General", "PluginBlacklist", "", blacklistStr.data(), blacklistStr.size(), Filepath_Config.c_str());
    blacklistStr.resize(strlen(blacklistStr.data()));
    StringToLower(blacklistStr);
    std::vector<std::string> blacklist;
    StringSplit(blacklistStr, "\t ,", blacklist);

    // load plugins from main CLEO directory
    auto ScanPluginsDir = [&](std::string path, const std::string prefix, const std::string extension)
    {
        auto pattern = path + '\\' + prefix + '*' + extension;
        auto files = CLEO_ListDirectory(nullptr, pattern.c_str(), false, true);

        for (DWORD i = 0; i < files.count; i++)
        {
            if (std::find(paths.begin(), paths.end(), files.strings[i]) != paths.end())
                continue; // file already listed

            if (skippedPaths.find(files.strings[i]) != skippedPaths.end())
                continue; // file already skipped

            auto name = FS::path(files.strings[i]).filename().string();
            name = name.substr(prefix.length()); // cut off prefix
            name.resize(name.length() - extension.length()); // cut off extension

            // on blacklist?
            auto blName = FS::path(files.strings[i]).filename().string();
            StringToLower(blName);
            if (std::find(blacklist.begin(), blacklist.end(), blName) != blacklist.end())
            {
                skippedPaths.emplace(files.strings[i]);
                LOG_WARNING(0, " %s - skipped, blacklisted in config.ini", files.strings[i]);
                continue;
            }

            // case insensitive search in already listed plugin names
            auto found = std::find_if(names.begin(), names.end(), [&](const std::string& s) {
                return _stricmp(s.c_str(), name.c_str()) == 0;
            });

            // duplicated?
            if (found != names.end())
            {
                skippedPaths.emplace(files.strings[i]);
                LOG_WARNING(0, " %s - skipped, duplicate of '%s' plugin", files.strings[i], name.c_str());
                continue;
            }

            names.insert(name);
            paths.emplace_back(files.strings[i]);
            TRACE(" %s", files.strings[i]);
        }

        CLEO_StringListFree(files);
    };

    TRACE(""); // separator
    TRACE("Listing CLEO plugins:");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "SA.", ".cleo");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "", ".cleo"); // legacy plugins in new location
    ScanPluginsDir(Filepath_Cleo, "", ".cleo"); // legacy plugins in old location

    // reverse order, so opcodes from CLEO5 plugins can overwrite opcodes from legacy plugins
    if (!paths.empty())
    {
        for (auto it = paths.crbegin(); it != paths.crend(); it++)
        {
            std::string filename = *it;

            // ModLoader support: keep game dir relative paths relative
            FilepathRemoveParent(filename, Filepath_Game);

            TRACE(""); // separator
            TRACE("Loading plugin '%s'", filename.c_str());

            HMODULE hlib = LoadLibrary(filename.c_str());
            if (!hlib)
            {
                LOG_WARNING(0, "Error loading plugin '%s'", filename.c_str());
                continue;
            }

            plugins.emplace_back(filename, hlib);
        }
        TRACE(""); // separator
    }
    else
    {
        TRACE(" nothing found");
    }

    pluginsLoaded = true;
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
    return plugins.size();
}

void CLEO::CPluginSystem::LogLoadedPlugins() const
{
    auto process = GetCurrentProcess();

    DWORD buffSize = 0;
    if (!EnumProcessModules(process, nullptr, 0, &buffSize) || buffSize < sizeof(HMODULE))
    {
        return;
    }

    std::vector<HMODULE> modules(buffSize / sizeof(HMODULE));
    if (!EnumProcessModules(process, modules.data(), buffSize, &buffSize))
    {
        return;
    }

    TRACE(""); // separator
    TRACE("Loaded plugins summary:");

    for (const auto& m : modules)
    {
        std::string filename(512, '\0');
        if (GetModuleFileName(m, filename.data(), filename.size()))
        {
            filename.resize(strlen(filename.data()));

            if (StringEndsWith(filename, ".asi", false) || StringEndsWith(filename, ".cleo", false))
            {
                std::error_code err;
                auto fileSize = (size_t)FS::file_size(filename, err);

                FilepathRemoveParent(filename, Filepath_Game);

                TRACE(" %s (%zu bytes)", filename.c_str(), fileSize);
            }
        }
    }

    TRACE(""); // separator
}

