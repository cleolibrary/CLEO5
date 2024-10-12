#include "stdafx.h"
#include "CModLoaderSystem.h"
#include "CCodeInjector.h"

using namespace std;
using namespace CLEO;

extern const BOOL (__cdecl* ResolvePath)(const char* scriptPath, char* path, DWORD pathMaxSize);
extern const StringList (__cdecl* ListFiles)(const char* scriptPath, const char* searchPattern, BOOL listDirs, BOOL listFiles);
extern const StringList (__cdecl* ListCleoFiles)(const char* directory, const char* filePattern);
extern const StringList (__cdecl* ListCleoStartupScripts)();
extern const void(__cdecl* StringListFree)(StringList);

bool CModLoaderSystem::IsInstalled()
{
	return GetModuleHandleA("modloader.asi") != NULL || FS::is_regular_file(Filepath_Game + "\\modloader.asi");
}

void CModLoaderSystem::Init()
{
	if (initialized) return;
	initialized = true;

	TRACE(""); // separator
	TRACE("ModLoader system initialization...");

	if (GetModuleHandleA("modloader.asi") == NULL)
	{
		TRACE(" ModLoader not detected");
		return;
	}
	TRACE(" ModLoader detected");

	auto provider = GetModuleHandleA("CLEO5_Provider.dll"); // CLEO's Mod Loader plugin
	if (provider == NULL)
	{
		SHOW_ERROR("CLEO's ModLoader plugin was not detected!\nPlease make sure CLEO5_Provider.dll is present.");
		return;
	}

	// get exported functions
	ResolvePath = memory_pointer(GetProcAddress(provider, "ResolvePath"));
	ListFiles = memory_pointer(GetProcAddress(provider, "ListFiles"));
	ListCleoFiles = memory_pointer(GetProcAddress(provider, "ListCleoFiles"));
	ListCleoStartupScripts = memory_pointer(GetProcAddress(provider, "ListCleoStartupScripts"));
	StringListFree = memory_pointer(GetProcAddress(provider, "StringListFree"));

	if (ResolvePath == nullptr || 
		ListFiles == nullptr || 
		ListCleoFiles == nullptr || 
		ListCleoStartupScripts == nullptr || 
		StringListFree == nullptr)
	{
		SHOW_ERROR("CLEO's ModLoader plugin initialization error! Plugin corrupted or outdated.");
		return;
	}

	TRACE(" ModLoader support active");
	active = true;
}

bool CModLoaderSystem::IsActive() const
{
	return active;
}
