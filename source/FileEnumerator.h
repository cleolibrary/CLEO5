#pragma once
#include <filesystem>

template<typename T>
void FilesWalk(const char* directory, const char* extension, T callback)
{
    /*try
    {
        for (auto& it : std::filesystem::directory_iterator(directory))
        {
            if (it.is_regular_file())
            {
                auto& filePath = it.path();

                if (extension != nullptr)
                {
                    if (_stricmp(filePath.extension().string().c_str(), extension) != 0)
                    {
                        continue;
                    }
                }

                auto result = std::filesystem::absolute(filePath);
                callback(result.string().c_str(), result.filename().string().c_str());
            }
        }
    }
    catch (const std::exception& ex)
    {
        TRACE("Error while iterating directory: %s", ex.what());
    }*/

    // Re-implemented with raw search APIs for compatibility with ModLoader.
    // The ModLoader should be updated anyway to solve potential file access problems in more advanced Cleo scripts
    
    std::string pattern = directory;
    if(!pattern.empty() && pattern.back() != '\\') pattern.push_back('\\');

    const size_t baseDirLen = pattern.length();
    
    pattern.push_back('*');
    if (extension != nullptr) pattern.append(extension);

    WIN32_FIND_DATA wfd = { 0 };
    HANDLE hSearch = FindFirstFile(pattern.c_str(), &wfd);

    if (hSearch == INVALID_HANDLE_VALUE)
    {
        TRACE("No files found in: %s", pattern.c_str());
        return;
    }
    do
    {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue; // skip directories
        }

        auto result = std::filesystem::weakly_canonical(pattern.substr(0, baseDirLen) + wfd.cFileName); // will use CWD if input path was relative!
        callback(result.string().c_str(), result.filename().string().c_str());

    } while (FindNextFile(hSearch, &wfd));

    FindClose(hSearch);
}
