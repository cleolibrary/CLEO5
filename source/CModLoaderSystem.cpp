#include "stdafx.h"
#include "CModLoaderSystem.h"
#include "stdafx.h"
#include "CModuleSystem.h"
#include <filesystem>

void CLEO::CModLoaderSystem::Init()
{
	auto modloaderDir = FS::path(Filepath_Root).append("modloader");

	active = GetModuleHandleA("modloader.asi") != INVALID_HANDLE_VALUE;
	active = active && FS::is_directory(modloaderDir);
}

void CLEO::CModLoaderSystem::ResolvePath(char* buff, size_t buffSize) const
{
	
}
