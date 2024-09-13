#include "ModLoaderProvider.h"
#include "utils.h"
#include "..\..\cleo_sdk\CLEO.h"
#include "..\..\cleo_sdk\CLEO_Utils.h"
#include "modloader\modloader.h"
#include "modloader\modloader.hpp"
#include <wtypes.h>

class ModLoaderPlugin : public modloader::basic_plugin
{
public:
    const info& GetInfo();
    bool OnStartup();
    bool OnShutdown();
    int GetBehaviour(modloader::file&);
    bool InstallFile(const modloader::file&);
    bool ReinstallFile(const modloader::file&);
    bool UninstallFile(const modloader::file&);

    CLEO::ModLoaderProvider modloaderProvider;
} mlPlugin;
REGISTER_ML_PLUGIN(::mlPlugin);

const ModLoaderPlugin::info& ModLoaderPlugin::GetInfo()
{
    static const char* extable[] = { "cleo", "cm", "cs", "cs3", "cs4", ".fxt", nullptr };
    static const info xinfo = { "CLEO5", CLEO_VERSION_STR, "Miran", -1, extable };
    return xinfo;
}

bool ModLoaderPlugin::OnStartup()
{
    modloaderProvider.Init(loader->gamepath);
    return true;
}

bool ModLoaderPlugin::OnShutdown()
{
    return true;
}

int ModLoaderPlugin::GetBehaviour(modloader::file& file)
{
    return modloaderProvider.IsHandled(file) ? MODLOADER_BEHAVIOUR_CALLME : MODLOADER_BEHAVIOUR_NO;
}

bool ModLoaderPlugin::InstallFile(const modloader::file& file)
{
    modloaderProvider.AddFile(file);
    return false;
}

bool ModLoaderPlugin::ReinstallFile(const modloader::file& file)
{
    return false;
}

bool ModLoaderPlugin::UninstallFile(const modloader::file& file)
{
    modloaderProvider.RemoveFile(file);
    return false;
}

// search in CLEO related type files registered by mods
extern "C" __declspec(dllexport) CLEO::StringList ListCleoFiles(const char* searchPattern)
{
    auto found = mlPlugin.modloaderProvider.ListCleoFiles(searchPattern);
    return CLEO::CreateStringList(found);
}

// search for all scripts that appear to be in 'cleo' directory
extern "C" __declspec(dllexport) CLEO::StringList ListCleoStartupScripts()
{
    auto found = mlPlugin.modloaderProvider.ListCleoStartupScripts();
    return CLEO::CreateStringList(found);
}

// generic file search including files from loaded mods
// scriptPath can be null
extern "C" __declspec(dllexport) CLEO::StringList ListFiles(const char* scriptPath, const char* searchPattern, BOOL listDirs, BOOL listFiles)
{
    auto found = mlPlugin.modloaderProvider.ListFiles(scriptPath, searchPattern, listDirs, listFiles);
    return CLEO::CreateStringList(found);
}

// required as free can not be called from another module
extern "C" __declspec(dllexport) void StringListFree(CLEO::StringList list)
{
    if (list.count > 0 && list.strings != nullptr)
    {
        for (DWORD i = 0; i < list.count; i++)
        {
            free(list.strings[i]);
        }

        free(list.strings);
    }
}

extern "C" __declspec(dllexport) BOOL ResolvePath(const char* scriptPath, char* path, DWORD pathMaxSize)
{
    auto resolved = mlPlugin.modloaderProvider.ResolvePath(scriptPath, path);

    if (!resolved.empty())
    {
        if (resolved.length() >= pathMaxSize)
            resolved.resize(pathMaxSize - 1); // and terminator character

        std::memcpy(path, resolved.c_str(), resolved.length() + 1); // with terminator

        return true;
    }

    return false;
}
