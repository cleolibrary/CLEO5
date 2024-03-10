#include "stdafx.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include "CModLoaderSystem.h"
#include "CModuleSystem.h"
#include <sstream>

using namespace std;


void CLEO::CModLoaderSystem::Init()
{
	modloaderDir = FS::path(Filepath_Root).append("modloader");
	cleoDir = FS::path(Filepath_Cleo);

	active = GetModuleHandleA("modloader.asi") != INVALID_HANDLE_VALUE;
	active = active && FS::is_directory(modloaderDir);
}

void CLEO::CModLoaderSystem::Update()
{
	mods.clear();

	// load ModLoader configuration from ini file
	auto configFilepath = FS::path(Filepath_Root).append("modloader\\modloader.ini");

	char profile[64] = { 0 };
	if (GetPrivateProfileString("Folder.Config", "Profile", "", profile, sizeof(profile), configFilepath.string().c_str()) == 0)
	{
		LOG_WARNING(0, "Failed to read ModLoader configuration file!");
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
			std::transform(dirName.begin(), dirName.end(), dirName.begin(), [](unsigned char c) { return std::tolower(c); });

			if (!dirName.empty() && dirName[0] != '.')
			{
				if (ignoreMods.find(dirName) == ignoreMods.end())
				{
					auto priority = GetPrivateProfileInt(sectionPriority.c_str(), dirName.c_str(), 50, configFilepath.string().c_str());
					AddMod(dirName.c_str(), priority);
				}
			}
		}
	}

	int b = 0;
}

bool CLEO::CModLoaderSystem::IsActive() const
{
	return active && mods.empty();
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
}

string CLEO::CModLoaderSystem::ResolvePath(const char* pathStr) const
{
	if (!IsActive()) return pathStr;

	auto path = FS::path(pathStr);

	return pathStr;
}
