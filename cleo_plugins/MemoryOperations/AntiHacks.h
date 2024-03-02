#pragma once
#include "CLEO.h"
//#include "CFileMgr.h"

namespace AntiHacks
{
	// checks if ceratin function call should be allowed. Performs replacement action if needed
	static bool CheckCall(CLEO::CRunningScript* thread, void* function, void* object, CLEO::SCRIPT_VAR* args, size_t argCount, DWORD& result)
	{
		switch ((size_t)function)
		{
			case 0x005387D0: // CFileMgr::SetDir(const char* relPath) // TODO: get the address from Plugin SDK
			{
				// some older mods use CFileMgr::SetDir directly instead of 0A99 opcode. Lack of functionality back in the day?
				CLEO::CLEO_SetScriptWorkDir(thread, args[0].pcParam);
				return false;
			}
			break;

			default:
				break;
		}

		return true; // allow
	}
};

