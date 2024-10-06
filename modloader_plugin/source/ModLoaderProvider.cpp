#include "ModLoaderProvider.h"
#include "utils.h"
#include "CLEO_Utils.h"
#include <windows.h>
#include <algorithm>

using namespace std;
using namespace CLEO;

const char* const ModLoaderProvider::Mod_Loader_Dir = "modloader\\";
const char* const ModLoaderProvider::Cleo_Dir = "cleo\\";


void ModLoaderProvider::Init(const char* gamePath)
{
	this->gamePath = gamePath;
	FilepathNormalize(this->gamePath);

	modLoaderPath = this->gamePath + '\\' + Mod_Loader_Dir;
	cleoDirPath = this->gamePath + '\\' + Cleo_Dir;
}

bool ModLoaderProvider::IsHandled(const modloader::file& file) const
{
	if (file.is_dir()) return false;

	if (IsScript(file) || // all kind of CLEO scripts
		file.is_ext("cleo") || // currently loaded by std.asi plugin anyway
		file.is_ext("fxt")) // CLEO text dictionaries
	{
		return true;
	}

	return false;
}

bool ModLoaderProvider::IsCleoScript(const modloader::file& file) const
{
	if (file.is_dir()) return false;

	if (file.is_ext("cs") ||
		file.is_ext("cs3") ||
		file.is_ext("cs4"))
	{
		return true;
	}

	return false;
}

bool CLEO::ModLoaderProvider::IsScript(const modloader::file& file) const
{
	if (file.is_dir()) return false;

	if (IsCleoScript(file) ||
		file.is_ext("cm") || // CLEO mission
		file.is_ext("s")) // used to store scripts that are not auto started
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
	if (files.empty() || path == nullptr)
	{
		return {};
	}

	string p = path;
	FilepathNormalize(p);

	if (!StringStartsWith(p, gamePath))
	{
		return {}; // the path is not within game directory
	}

	if (StringStartsWith(p, modLoaderPath))
	{
		return {}; // the path is already pointing inside 'modloader' directory, no need for resolving
	}

	auto script = GetCleoFile(scriptPath);
	if (script == nullptr)
	{
		return {}; // scriptPath is not part of any active mod
	}

	// build modded path relative to that mod's directory
	string modded;
	if (StringStartsWith(p, cleoDirPath)) // accessing cleo\*
	{
		// that script's cleo directory
		modded = GetScriptsCleoDir(scriptPath);

		auto relative = p.c_str() + cleoDirPath.length(); // cut off cleo location from absolute path
		if (strlen(relative) > 0)
		{
			modded += "\\";
			modded += relative;
		}
	}
	else // any other path
	{
		modded = modLoaderPath;
		modded += GetModName(*script);

		auto relative = p.c_str() + gamePath.length(); // cut off game location from absolute path
		if (strlen(relative) > 0)
		{
			modded += "\\";
			modded += relative;
		}
	}

	FilepathNormalize(modded);

	if (GetFileAttributesA(modded.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		return modded; // file/dir found in mod's folder
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
	FilepathNormalize(path);

	if (!StringStartsWith(path, modLoaderPath))
	{
		return nullptr; // not located inside 'modloader' directory
	}

	FilepathRemoveParent(path, gamePath); // cut game directory path prefix
	
	for (auto it = files.begin(); it != files.end(); it++)
	{
		if (_strcmpi((*it)->filepath(), path.c_str()) == 0)
		{
			return *it; // found
		}
	}

	return nullptr; // the script is not from ModLoader
}

string CLEO::ModLoaderProvider::GetScriptsCleoDir(const char* filePath) const
{
	auto script = GetCleoFile(filePath);
	if (script == nullptr || !IsScript(*script))
	{
		return {}; 
	}

	auto dir = string(FilepathGetParent(script->filedir())); // relative path within mod's directory

	// By default ModLoader simply deep searched for any CLEO script files inside mod directory and mapped them as laying in game's main cleo directory.
	// Do same thing, unless mod actually has some 'cleo' directory. This allows some more sophisticated mods to store subscripts in subdirectories, without MLoader autostarting them.
	const size_t Cleo_Len = strlen(Cleo_Dir) - 1; // without path separator
	auto found = dir.find(Cleo_Dir);
	while (found != string::npos)
	{
		// make sure it is full directory name, not just suffix
		if (found == 0 || dir[found - 1] == '\\')
		{
			dir.resize(found + Cleo_Len); // cut after "cleo"
			break;
		}

		found = dir.find(dir.c_str() + found + Cleo_Len); // search for another
	}

	// build absolute path
	auto path = modLoaderPath;
	path += GetModName(*script);

	if (!dir.empty())
	{
		path += "\\";
		path += dir;
	}

	return path;
}

std::set<std::string> ModLoaderProvider::ListFiles(const char* scriptPath, const char* searchPattern, bool listDirs, bool listFiles) const
{
	std::set<std::string> files;

	// regular search
	utils::ListFiles(searchPattern, files, listDirs, listFiles);

	auto script = GetCleoFile(scriptPath);
	if (script != nullptr) // the script is ModLoader's mod?
	{
		string path = searchPattern;
		FilepathNormalize(path);
		if (StringStartsWith(path, gamePath))
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

	FilepathNormalize(prefix);
	FilepathNormalize(suffix);

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

			if (!prefix.empty() && !StringStartsWith(path, prefix))
				continue; // prefix mismatch

			if (!suffix.empty() && strncmp(path.c_str() + path.length() - suffix.length(), suffix.c_str(), suffix.length()) != 0)
				continue; // suffix mismatch
		}

		found.insert(file->fullpath());
	}

	return found;
}

set<string> CLEO::ModLoaderProvider::ListCleoStartupScripts() const
{
	set<string> found;
	if (files.empty()) return found;

	for (const auto& file : files)
	{
		if (!IsCleoScript(*file))
		{
			continue;
		}

		string scriptPath = file->fullpath();
		FilepathNormalize(scriptPath);

		auto scriptDir = string(FilepathGetParent(scriptPath));
		auto cleoDir = GetScriptsCleoDir(scriptPath.c_str());

		if (scriptDir.compare(cleoDir) == 0)
		{
			found.insert(std::move(scriptPath));
		}
	}

	return found;
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
