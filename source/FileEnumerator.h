#pragma once
#include "..\cleo_sdk\CLEO.h"

template <typename T> void FilesWalk(const char *directory, const char *extension, T callback)
{
    auto filePattern = std::string(directory);
    filePattern += "\\*";
    filePattern += extension;

    auto list = CLEO::CLEO_ListDirectory(nullptr, filePattern.c_str(), false, true);
    for (DWORD i = 0; i < list.count; i++)
    {
        auto fsPath = FS::path(list.strings[i]);
        callback(list.strings[i], fsPath.filename().string().c_str());
    }
    CLEO::CLEO_StringListFree(list);
}
