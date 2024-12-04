#pragma once
#include "CLEO.h"
#include <string>

namespace AntiHacks
{
	// checks if specific function call is allowed. Performs replacement action if needed
	static bool CheckCall(CLEO::CRunningScript* thread, void* function, void* object, CLEO::SCRIPT_VAR* args, size_t argCount, DWORD& result)
	{
		switch ((size_t)function)
		{
			// Setting work directory. Some older mods used these instead of 0A99 opcode
			case 0x005387D0: // CFileMgr::SetDir(const char* relPath)
			case 0x00836F1E: // _chdir(LPCSTR lpPathName)
			case 0x0085824C: // SetCurrentDirectoryA(LPCSTR lpPathName)
			{
				auto resolved = std::string(args[0].pcParam);
				resolved.resize(MAX_PATH);
				CLEO_ResolvePath(thread, resolved.data(), resolved.size());

				CLEO_SetScriptWorkDir(thread, resolved.c_str());
				return false; // block call
			}
			break;

			default:
				break;
		}

		return true; // allow call
	}
};

