#pragma once
#include "..\cleo_sdk\CLEO.h"
#include <filesystem>

template<typename T>
void FilesWalk(const char* directory, const char* extension, T callback)
{
    std::string searchPath = directory;
    if (searchPath.back() != '\\' && searchPath.back() != '/') searchPath.push_back('\\');
    searchPath += "*";
    searchPath += extension;

    auto list = CLEO::CLEO_ListDirectory(nullptr, searchPath.c_str(), false, true);
    for (DWORD i = 0; i < list.count; i++)
    {
        auto fsPath = FS::path(list.paths[i]);
        callback(list.paths[i], fsPath.filename().string().c_str());
    }
    CLEO::CLEO_ListDirectoryFree(list);
}
