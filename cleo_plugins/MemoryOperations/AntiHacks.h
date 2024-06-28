#pragma once
#include "CLEO.h"
//#include "CFileMgr.h"
#include <string>

namespace AntiHacks
{
	// checks if specific function call should be allowed. Performs replacement action if needed
	static bool CheckCall(CLEO::CRunningScript* thread, void* function, void* object, CLEO::SCRIPT_VAR* args, size_t argCount, DWORD& result)
	{
		switch ((size_t)function)
		{
			case 0x005387D0: // CFileMgr::SetDir(const char* relPath) // TODO: get the address from Plugin SDK
			{
				// some older mods use CFileMgr::SetDir directly instead of 0A99 opcode. In older CLEO versions 0A99 supported only few predefined locations
				auto resolved = std::string(args[0].pcParam);
				resolved.resize(511);
				CLEO_ResolvePath(thread, resolved.data(), resolved.size());

				CLEO::CLEO_SetScriptWorkDir(thread, resolved.c_str());
				return false;
			}
			break;

			default:
				break;
		}

		return true; // allow
	}
};

