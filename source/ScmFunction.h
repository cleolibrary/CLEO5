#pragma once
#include "..\cleo_sdk\CLEO.h"
#include "CDebug.h"

#include <wtypes.h>
#include <list>
#include <string>

namespace CLEO
{
	struct ScmFunction
	{
		static const size_t Store_Size = 0x400;
		static ScmFunction* store[Store_Size];
		static size_t allocationPlace; // contains an index of last allocated object
		static ScmFunction* Get(unsigned short idx);
		static void Clear();

		unsigned short prevScmFunctionId, thisScmFunctionId;
		void* savedBaseIP;
		BYTE* retnAddress;
		BYTE* savedStack[8]; // gosub stack
		WORD savedSP;
		SCRIPT_VAR savedTls[32];
		std::list<std::string> stringParams; // texts with this scope lifetime
		bool savedCondResult;
		eLogicalOperation savedLogicalOp;
		bool savedNotFlag;
		void* moduleExportRef = 0; // modules switching. Points to modules baseIP in case if this is export call
		std::string savedScriptFileDir; // modules switching
		std::string savedScriptFileName; // modules switching

		void* operator new(size_t size);
		void operator delete(void* mem);
		ScmFunction(CRunningScript* thread);

		void Return(CRunningScript* thread);
	};
}
