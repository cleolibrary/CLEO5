#include "stdafx.h"
#include "..\cleo_sdk\CLEO.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include "CModLoaderSystem.h"
#include "CModuleSystem.h"
#include "CScriptEngine.h"
#include <sstream>

using namespace std;


void CLEO::CModLoaderSystem::Init()
{
	gameDir = FS::weakly_canonical(FS::path(Filepath_Root));

	modloaderDir = gameDir;
	modloaderDir /= "modloader";

	active = GetModuleHandleA("modloader.asi") != INVALID_HANDLE_VALUE;
	active = active && FS::is_directory(modloaderDir);

	if (active)
	{
		TRACE("ModLoader detetected! Support activated.");
	}
}

void CLEO::CModLoaderSystem::Update()
{
	TRACE("Updating ModLoader data...");

	mods.clear();

	// load ModLoader configuration from ini file
	auto configFilepath = FS::path(Filepath_Root).append("modloader\\modloader.ini");

	char profile[64] = { 0 };
	if (GetPrivateProfileString("Folder.Config", "Profile", "", profile, sizeof(profile), configFilepath.string().c_str()) == 0)
	{
		LOG_WARNING(0, "Failed to read Profile from ModLoader configuration file!");
		return;
	}

	// Config
	std::string sectionConfig = StringPrintf("Profiles.%s.Config", profile);

	char ignoreAll[16] = { 0 };
	GetPrivateProfileString(sectionConfig.c_str(), "IgnoreAllMods", "", ignoreAll, sizeof(ignoreAll), configFilepath.string().c_str());
	if (_stricmp(ignoreAll, "true") == 0)
	{
		return; // all mods ignored - done
	}

	// IgnoreMods
	set<string> ignoreMods;
	string sectionIgnoreMods = StringPrintf("Profiles.%s.IgnoreMods", profile);
	char ignoreModsStr[4096] = { 0 };
	auto len = GetPrivateProfileSection(sectionIgnoreMods.c_str(), ignoreModsStr, sizeof(ignoreModsStr), configFilepath.string().c_str());
	if (len > 2) // not empty double terminated string
	{
		// turn multistring into separate lines
		for (size_t i = 0; i < len - 2; i++) 
		{
			ignoreModsStr[i] = std::tolower(ignoreModsStr[i]);
			if (ignoreModsStr[i] == '\0') ignoreModsStr[i] = '\n';
		}

		stringstream ss(ignoreModsStr);
		char line[64];
		while (ss.getline(line, sizeof(line)))
		{
			ignoreMods.emplace(line);
		}
	}

	// list mod folders in ModLoader directory
	std::string sectionPriority = StringPrintf("Profiles.%s.Priority", profile);
	for (auto dir : FS::directory_iterator(modloaderDir))
	{
		if (dir.is_directory())
		{
			auto dirName = dir.path().filename().string();
			if (!dirName.empty() && dirName[0] != '.')
			{
				std::transform(dirName.begin(), dirName.end(), dirName.begin(), [](unsigned char c) { return std::tolower(c); });

				if (ignoreMods.find(dirName) == ignoreMods.end())
				{
					auto priority = GetPrivateProfileInt(sectionPriority.c_str(), dirName.c_str(), 50, configFilepath.string().c_str());
					AddMod(dirName.c_str(), priority);
				}
			}
		}
	}

	TRACE("%d active ModLoader mods detected.", std::distance(mods.begin(), mods.end()));
}

bool CLEO::CModLoaderSystem::IsActive() const
{
	return active && !mods.empty();
}

bool CLEO::CModLoaderSystem::HasMod(const char* name) const
{
	for (auto& mod : mods)
	{
		if (_stricmp(name, mod.name.c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

void CLEO::CModLoaderSystem::AddMod(const char* name, size_t priority)
{
	ModDesc mod;
	mod.name = name;
	mod.priority = priority;

	auto cleoPath = modloaderDir;
	cleoPath += "\\";
	cleoPath += name;
	cleoPath += "\\cleo";

	if (FS::is_directory(cleoPath))
	{
		mod.hasCleo = true;
	}

	mods.push_front(mod);
}

string CLEO::CModLoaderSystem::ResolvePath(CRunningScript* thread, const char* pathStr) const
{
	if (!IsActive()) return {};

	// is the script file in modloader directory?
	auto scriptDir = FS::path(((CLEO::CCustomScript*)thread)->GetScriptFileDir());
	auto [endLoader, _1] = std::mismatch(modloaderDir.begin(), modloaderDir.end(), scriptDir.begin());
	
	if (endLoader != modloaderDir.end()) return {}; // the script is not modloader's mod

	auto path = FS::weakly_canonical(FS::path(pathStr));

	// do the path starts with game root?
	auto [endGame, _2] = std::mismatch(gameDir.begin(), gameDir.end(), path.begin());
	if (endGame != gameDir.end()) return {}; // the path target is not within the game root directory

	auto modName = *(FS::proximate(scriptDir, modloaderDir).begin());
	if (!HasMod(modName.string().c_str())) return {}; // this mod is disabled/not present in modloader (but runs anyway?)

	auto relative = FS::proximate(path, gameDir);

	auto modedPath = modloaderDir;
	modedPath /= modName;
	modedPath /= relative;

	auto oriStatus = FS::status(path);
	bool oriExists = FS::is_regular_file(oriStatus) || FS::is_directory(oriStatus);

	auto modedStatus = FS::status(modedPath);
	bool modedExists = FS::is_regular_file(modedStatus) || FS::is_directory(modedStatus);

	if (!modedExists && oriExists) return {}; // file/dir not present in that mod, but exists in the game itself. Keep original path

	auto result = modedPath.string();
	
	// TODO: check if original path ended with path separator and add it then

	return result;
}
