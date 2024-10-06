#pragma once
#include "CLEO_Utils.h"
#include <Windows.h>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>

namespace utils
{
	static void ListFiles(const char* searchPattern, std::set<std::string>& output, bool listDirs = true, bool listFiles = true)
	{
		WIN32_FIND_DATAA wfd = { 0 };
		HANDLE hSearch = FindFirstFileA(searchPattern, &wfd);
		if (hSearch == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			if (!listDirs && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				continue; // skip directories
			}

			if (!listFiles && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				continue; // skip files
			}

			auto path = std::filesystem::path(wfd.cFileName);
			if (!path.is_absolute()) // keep absolute in case somebody hooked the APIs to return so
				path = std::filesystem::path(searchPattern).parent_path() / path;

			auto result = path.string();
			CLEO::FilepathNormalize(result);

			output.insert(std::move(result));
		} 
		while (FindNextFileA(hSearch, &wfd));

		FindClose(hSearch);
	}
}
