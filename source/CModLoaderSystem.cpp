#include "stdafx.h"
#include "CModLoaderSystem.h"
#include "CCodeInjector.h"

using namespace std;
using namespace CLEO;

extern const void (__cdecl* StringListFree)(StringList);
extern const BOOL (__cdecl* ResolvePath)(const char* scriptPath, char* path, DWORD pathMaxSize);
extern const StringList (__cdecl* ListFiles)(const char* scriptPath, const char* searchPattern, BOOL listDirs, BOOL listFiles);
extern const StringList (__cdecl* ListCleoFiles)(const char* directory, const char* filePattern);


void CModLoaderSystem::Init()
{
	if (initialized) return;
	initialized = true;

	if (GetModuleHandleA("modloader.asi") == NULL)
	{
		return;
	}
	TRACE("ModLoader detected");

	auto provider = GetModuleHandleA("CLEO5_Provider.dll"); // CLEO's Mod Loader plugin
	if (provider == NULL)
	{
		SHOW_ERROR("CLEO's ModLoader plugin was not detected!\nPlease make sure CLEO5_Provider.dll is present.");
		return;
	}

	StringListFree = memory_pointer(GetProcAddress(provider, "StringListFree"));
	ResolvePath = memory_pointer(GetProcAddress(provider, "ResolvePath"));
	ListFiles = memory_pointer(GetProcAddress(provider, "ListFiles"));
	ListCleoFiles = memory_pointer(GetProcAddress(provider, "ListCleoFiles"));

	if (StringListFree == nullptr || ResolvePath == nullptr || ListFiles == nullptr || ListCleoFiles == nullptr)
	{
		SHOW_ERROR("CLEO's ModLoader plugin error!");
		return;
	}

	TRACE("ModLoader support activate");
	active = true;
}

bool CModLoaderSystem::IsActive() const
{
	return active;
}
