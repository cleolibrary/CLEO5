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
            if (std::find(filenames.begin(), filenames.end(), files.strings[i]) != filenames.end())
                continue; // file already listed

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
                filenames.emplace_back(files.strings[i]);
                TRACE(" - '%s'", files.strings[i]);
            }
            else
            {
                LOG_WARNING(0, " - '%s' skipped, duplicate of `%s` plugin", files.strings[i], name.c_str());
            }
        }

        CLEO_StringListFree(files);
    };

    TRACE("Listing CLEO plugins:");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "SA.", ".cleo");
    ScanPluginsDir(FS::path(Filepath_Cleo).append("cleo_plugins").string(), "", ".cleo"); // legacy plugins in new location
    ScanPluginsDir(Filepath_Cleo, "", ".cleo"); // legacy plugins in old location

    // reverse order, so opcodes from CLEO5 plugins can overwrite opcodes from legacy plugins
    if (!filenames.empty())
    {
        for (auto it = filenames.crbegin(); it != filenames.crend(); it++)
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

