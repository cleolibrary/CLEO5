#pragma once
#include "../cleo_sdk/CLEO.h"
#include <vector>

namespace CLEO
{
	struct ScriptDeleteDelegate
	{
		std::vector<FuncScriptDeleteDelegateT> funcs;

		template<class FuncScriptDeleteDelegateT> void operator+=(FuncScriptDeleteDelegateT mFunc)
		{
			funcs.push_back(mFunc);
		}

		template<class FuncScriptDeleteDelegateT> void operator-=(FuncScriptDeleteDelegateT mFunc)
		{
			funcs.erase(std::remove(funcs.begin(), funcs.end(), mFunc), funcs.end());
		}

		void operator()(CRunningScript* script)
		{
			for (auto& f : funcs) f(script);
		}
	};
}
