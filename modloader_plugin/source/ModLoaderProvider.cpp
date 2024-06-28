#include "ModLoaderProvider.h"
#include "utils.h"
#include <windows.h>
#include <algorithm>

using namespace std;
using namespace CLEO;

const char* const ModLoaderProvider::Mod_Loader_Dir = "modloader\\";


void ModLoaderProvider::Init(const char* gamePath)
{
	this->gamePath = gamePath;
	NormalizePath(this->gamePath);

	modLoaderPath = this->gamePath + Mod_Loader_Dir;
}

bool ModLoaderProvider::IsHandled(const modloader::file& file) const
{
	if (file.is_dir()) return false;

	if (file.is_ext("cm") ||
		file.is_ext("cs") ||
		file.is_ext("cs3") ||
		file.is_ext("cs4") ||
		file.is_ext("cleo") || // loaded by std.asi plugin anyway
		file.is_ext("cleo5"))
	{
		return true;
	}

	return false;
}

void ModLoaderProvider::AddFile(const modloader::file& file)
{
	files.insert(&file);
}

void ModLoaderProvider::RemoveFile(const modloader::file& file)
{
	files.erase(&file);
}

string ModLoaderProvider::ResolvePath(const char* scriptPath, const char* path) const
{
	if (files.empty() || path == nullptr || strlen(path) <= gamePath.length())
	{
		return {};
	}

	string p = path;
	NormalizePath(p);

	if (!utils::StrStartsWith(p, gamePath))
	{
		return {}; // the path is not within game directory
	}

	if (utils::StrStartsWith(p, modLoaderPath))
	{
		return {}; // the path is already pointing inside 'modloader' directory, no need for resolving
	}

	auto script = GetCleoFile(scriptPath);
	if (script == nullptr)
	{
		return {}; // scriptPath is not part of any active mod
	}

	// build moded path relative to that mod's directory
	string modded = modLoaderPath;
	modded += GetModName(*script);

	auto relative = p.c_str() + gamePath.length();
	if (strlen(relative) > 0)
	{
		modded += "\\";
		modded += relative;
	}

	if (GetFileAttributesA(modded.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		return modded;
	}

	if (GetFileAttributesA(p.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		return modded; // both paths do not exists, so the mod perhaps tries to create a new file/directory
	}

	return {};
}

const modloader::file* ModLoaderProvider::GetCleoFile(const char* filePath) const
{
	if (filePath == nullptr || strlen(filePath) <= modLoaderPath.length())
		return nullptr; // not located within 'modloader' directory

	string path = filePath;
	NormalizePath(path);

	if (!utils::StrStartsWith(path, modLoaderPath))
	{
		return nullptr; // not located inside 'modloader' directory
	}

	path = path.substr(gamePath.length()); // cut game directory path prefix
	auto scriptModFile = files.begin();
	while (true)
	{
		if (scriptModFile == files.end())
		{
			return {}; // the script is not from ModLoader
		}

		if (path == (*scriptModFile)->filebuffer())
		{
			return *scriptModFile; // found
		}

		scriptModFile++;
	}

	return nullptr;
}

std::set<std::string> ModLoaderProvider::ListFiles(const char* scriptPath, const char* searchPattern, bool listDirs, bool listFiles) const
{
	std::set<std::string> files;
	utils::ListFiles(searchPattern, files, listDirs, listFiles); // regular search

	if (scriptPath != nullptr)
	{
		string path = searchPattern;
		NormalizePath(path);
		if (utils::StrStartsWith(path, gamePath))
		{
			auto script = GetCleoFile(scriptPath); // the script is ModLoader's mod?
			if (script != nullptr)
			{
				// files inside mod directory will mask coresponding files in game
				std::set<std::string> masked;
				for (const auto& file : files)
				{
					auto resolved = ResolvePath(scriptPath, file.c_str());
					masked.insert(resolved.empty() ? file : resolved); // select original or mod's file
				}
				files = masked;

				// search again, but now with mod location immitating game's directory
				string moddedPattern = modLoaderPath;
				moddedPattern += GetModName(*script);
				moddedPattern += "\\";
				moddedPattern += searchPattern + gamePath.length(); // cut game directory path prefix

				utils::ListFiles(moddedPattern.c_str(), files, listDirs, listFiles);
			}
		}
	}

	return files;
}

set<string> ModLoaderProvider::ListCleoFiles(const char* searchPattern) const
{
	set<string> found;
	if (files.empty()) return found;

	string prefix, suffix;
	bool exact;

	auto pattern = string(searchPattern != nullptr ? searchPattern : "");
	auto pos = pattern.find('*'); // TODO: use real wildcards support
	if (pos != string::npos)
	{
		prefix = pattern.substr(0, pos);
		suffix = pattern.substr(pos + 1);
		exact = false;
	}
	else
	{
		prefix = pattern;
		exact = true;
	}

	NormalizePath(prefix);
	NormalizePath(suffix);

	for (const auto& file : files)
	{
		auto path = string(file->filedir());

		if (exact)
		{
			if (path != prefix) // requires exact match
				continue;
		}
		else
		{
			if (prefix.length() + suffix.length() > path.length())
				continue; // search pattern longer than actual file

			if (!prefix.empty() && !utils::StrStartsWith(path, prefix))
				continue; // prefix mismatch

			if (!suffix.empty() && strncmp(path.c_str() + path.length() - suffix.length(), suffix.c_str(), suffix.length()) != 0)
				continue; // suffix mismatch
		}

		found.insert(file->fullpath());
	}

	return found;
}

void ModLoaderProvider::NormalizePath(string& path)
{
	replace(path.begin(), path.end(), '/', '\\');
	transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return tolower(c); }); // to lower case
}

string_view ModLoaderProvider::GetModName(const modloader::file& file)
{
	auto start = file.buffer + strlen(Mod_Loader_Dir);

	size_t length;
	auto separator = strchr(start, '\\');
	if (separator != nullptr)
	{
		length = (size_t)separator - (size_t)start; // distance
	}
	else
	{
		length = strlen(start);
	}

	return std::string_view(start, length);
}
