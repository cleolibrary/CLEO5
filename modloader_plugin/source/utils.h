#pragma once
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

			output.insert(path.string());
		} 
		while (FindNextFileA(hSearch, &wfd));

		FindClose(hSearch);
	}

	static bool StrStartsWith(const std::string_view str, const std::string_view prefix, bool caseSensitive = true)
	{
		if (str.length() < prefix.length())
		{
			return false;
		}

		if (caseSensitive)
		{
			return strncmp(str.data(), prefix.data(), prefix.length()) == 0;
		}
		else
		{
			return _strnicmp(str.data(), prefix.data(), prefix.length()) == 0;
		}
	}

	static const std::string_view GetParentDirectory(const std::string_view str)
	{
		auto separatorPos = str.find_last_of('\\');

		if (separatorPos == std::string::npos)
		{
			return {};
		}

		return std::string_view(str.data(), separatorPos);
	}
}
