#include "stdafx.h"
#include "CPluginSystem.h"
#include "CleoBase.h"


using namespace CLEO;

CPluginSystem::~CPluginSystem()
{
    std::for_each(plugins.begin(), plugins.end(), FreeLibrary);
}

void CPluginSystem::LoadPlugins()
{
    if (pluginsLoaded) return; // already done

    std::set<std::string> names;
    std::vector<std::string> filenames;

    // load plugins from main CLEO directory
    auto ScanPluginsDir = [&](std::string path, const std::string prefix, const std::string extension)
    {
        auto pattern = path + '\\' + prefix + '*' + extension;
        auto files = CLEO_ListDirectory(nullptr, pattern.c_str(), false, true);

        for (DWORD i = 0; i < files.count; i++)
        {
            auto name = FS::path(files.strings[i]).filename().string();
            name = name.substr(prefix.length()); // cut off prefix
            name.resize(name.length() - extension.length()); // cut off extension
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });

            if (names.find(name) == names.end())
            {
                names.insert(name);
                filenames.emplace_back(files.strings[i]);
                TRACE(" - '%s'", files.strings[i]);
            }
            else
            {
                LOG_WARNING(0, " - '%s' skipped, duplicate of `%s` plugin.", files.strings[i], name.c_str());
            }
        }

        CLEO_StringListFree(files);
    };

    TRACE("Listing CLEO plugins:");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "SA.", ".cleo");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "", ".cleo"); // legacy plugins in new location
    ScanPluginsDir(Filepath_Cleo, "", ".cleo"); // legacy plugins in old location

    // reverse order, so most important plugins can overwrite already registered opcodes
    if (!filenames.empty())
    {
        for (auto it = filenames.crbegin(); it < filenames.crend(); it++)
        {
            const auto filename = it->c_str();
            TRACE("Loading plugin '%s'", filename);

            HMODULE hlib = LoadLibrary(filename);
            if (!hlib)
            {
                LOG_WARNING(0, "Error loading plugin '%s'", filename);
                continue;
            }

            plugins.push_back(hlib);
        }
    }
    else
    {
        TRACE(" - nothing found");
    }

    pluginsLoaded = true;
}

size_t CPluginSystem::GetNumPlugins() const
{
    return plugins.size();
}

