#include "stdafx.h"
#include "CleoBase.h"
#include "CGameVersionManager.h"
#include "CCustomOpcodeSystem.h"
#include "ScmFunction.h"


namespace CLEO
{
	template<typename T> inline CRunningScript& operator>>(CRunningScript& thread, T*& pval);
	template<typename T> inline CRunningScript& operator<<(CRunningScript& thread, T* pval);
	template<typename T> inline CRunningScript& operator<<(CRunningScript& thread, memory_pointer pval);
	template<typename T> inline CRunningScript& operator>>(CRunningScript& thread, memory_pointer& pval);

	void(__thiscall * ProcessScript)(CRunningScript*);

	CRunningScript* CCustomOpcodeSystem::lastScript = nullptr;
	WORD CCustomOpcodeSystem::lastOpcode = 0xFFFF;
	WORD* CCustomOpcodeSystem::lastOpcodePtr = nullptr;
	WORD CCustomOpcodeSystem::lastCustomOpcode = 0;
	std::string CCustomOpcodeSystem::lastErrorMsg = {};
	WORD CCustomOpcodeSystem::prevOpcode = 0xFFFF;
	BYTE CCustomOpcodeSystem::handledParamCount = 0;

	// opcode handler for custom opcodes
	OpcodeResult __fastcall CCustomOpcodeSystem::customOpcodeHandler(CRunningScript *thread, int dummy, WORD opcode)
	{
		OpcodeResult result = OR_NONE;

		auto AfterOpcodeExecuted = [&]()
		{
			// execute registered callbacks
			OpcodeResult callbackResult = OR_NONE;
			for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptOpcodeProcessAfter))
			{
				typedef OpcodeResult WINAPI callback(CRunningScript*, DWORD, OpcodeResult);
				auto res = ((callback*)func)(thread, opcode, result);

				callbackResult = std::max(res, callbackResult); // store result with highest value from all callbacks
			}

			return (callbackResult != OR_NONE) ? callbackResult : result;
		};

		prevOpcode = (thread != lastScript) ? 0xFFFF : lastOpcode;
		lastScript = thread;
		lastOpcode = opcode;
		lastOpcodePtr = (WORD*)thread->GetBytePointer() - 1; // rewind to the opcode start
		handledParamCount = 0;

		// prevent past code execution
		if (thread->IsCustom() && !IsLegacyScript(thread))
		{
			auto cs = (CCustomScript*)thread;
			auto endPos = cs->GetBasePointer() + cs->GetCodeSize();
			if ((BYTE*)lastOpcodePtr == endPos || (BYTE*)lastOpcodePtr == (endPos - 1)) // consider script can end with incomplete opcode
			{
				SHOW_ERROR_COMPAT("Code execution past script end in script %s\nThis usually happens when [004E] command is missing.\nScript suspended.", ScriptInfoStr(thread).c_str());
				result = thread->Suspend();
				return AfterOpcodeExecuted();
			}
		}

		// execute registered callbacks
		for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptOpcodeProcessBefore))
		{
			typedef OpcodeResult WINAPI callback(CRunningScript*, DWORD);
			result = ((callback*)func)(thread, opcode);

			if(result != OR_NONE)
				break; // processed
		}

		if(result == OR_NONE) // opcode not proccessed yet
		{
			if(opcode > LastCustomOpcode)
			{
				SHOW_ERROR("Opcode [%04X] out of supported range! \nCalled in script %s\nScript suspended.", opcode, ScriptInfoStr(thread).c_str());
				result = thread->Suspend();
				return AfterOpcodeExecuted();
			}

			CustomOpcodeHandler handler = customOpcodeProc[opcode];
			if(handler != nullptr)
			{
				lastCustomOpcode = opcode;
				result = handler(thread);
				return AfterOpcodeExecuted();
			}

			// Not registered as custom opcode. Call game's original handler

			if (opcode > LastOriginalOpcode)
			{
				auto extensionMsg = CleoInstance.OpcodeInfoDb.GetExtensionMissingMessage(opcode);
				if (!extensionMsg.empty()) extensionMsg = " " + extensionMsg;

				SHOW_ERROR("Custom opcode [%04X] not registered!%s\nCalled in script %s\nScript suspended.",
					opcode,
					extensionMsg.c_str(),
					ScriptInfoStr(thread).c_str());

				result = thread->Suspend();
				return AfterOpcodeExecuted();
			}

			size_t tableIdx = opcode / 100; // 100 opcodes peer handler table
			result = originalOpcodeHandlers[tableIdx](thread, opcode);

			if(result == OR_ERROR)
			{
				auto extensionMsg = CleoInstance.OpcodeInfoDb.GetExtensionMissingMessage(opcode);
				if (!extensionMsg.empty()) extensionMsg = " " + extensionMsg;

				SHOW_ERROR("Opcode [%04X] not found!%s\nCalled in script %s\nScript suspended.",
					opcode,
					extensionMsg.c_str(),
					ScriptInfoStr(thread).c_str());

				result = thread->Suspend();
				return AfterOpcodeExecuted();
			}
		}

		return AfterOpcodeExecuted();
	}

	void CCustomOpcodeSystem::FinalizeScriptObjects()
	{
		TRACE("Cleaning up script data...");

		CleoInstance.CallCallbacks(eCallbackId::ScriptsFinalize);

		// clean up after opcode_0AB1
		ScmFunction::Clear();
	}

	void CCustomOpcodeSystem::Inject(CCodeInjector& inj)
	{
		TRACE("Injecting CustomOpcodeSystem...");
		CGameVersionManager& gvm = CleoInstance.VersionManager;

		// replace all handlers in original table
		// store original opcode handlers for later use
		auto handlersTable = (OpcodeHandler*)::CRunningScript::CommandHandlerTable;
		for(size_t i = 0; i < OriginalOpcodeHandlersCount; i++)
		{
			originalOpcodeHandlers[i] = handlersTable[i];
			handlersTable[i] = (OpcodeHandler)customOpcodeHandler;
		}

		// initialize and apply new handlers table
		for (size_t i = 0; i < CustomOpcodeHandlersCount; i++)
		{
			customOpcodeHandlers[i] = (OpcodeHandler)customOpcodeHandler;
		}

		inj.MemoryWrite(gvm.TranslateMemoryAddress(MA_OPCODE_HANDLER_REF_1), &customOpcodeHandlers);
		inj.MemoryWrite(gvm.TranslateMemoryAddress(MA_OPCODE_HANDLER_REF_2), &customOpcodeHandlers);
	}

	void CCustomOpcodeSystem::Init()
	{
		if (initialized) return;

		TRACE(""); // separator
		TRACE("Initializing CLEO core opcodes...");

		CLEO_RegisterOpcode(0x004E, opcode_004E);
		CLEO_RegisterOpcode(0x0051, opcode_0051);
		CLEO_RegisterOpcode(0x0417, opcode_0417);
		CLEO_RegisterOpcode(0x0A92, opcode_0A92);
		CLEO_RegisterOpcode(0x0A93, opcode_0A93);
		CLEO_RegisterOpcode(0x0A94, opcode_0A94);
		CLEO_RegisterOpcode(0x0A95, opcode_0A95);
		CLEO_RegisterOpcode(0x0AA0, opcode_0AA0);
		CLEO_RegisterOpcode(0x0AA1, opcode_0AA1);
		CLEO_RegisterOpcode(0x0AA9, opcode_0AA9);
		CLEO_RegisterOpcode(0x0AB1, opcode_0AB1);
		CLEO_RegisterOpcode(0x0AB2, opcode_0AB2);
		CLEO_RegisterOpcode(0x0AB3, opcode_0AB3);
		CLEO_RegisterOpcode(0x0AB4, opcode_0AB4);

		CLEO_RegisterOpcode(0x0DD5, opcode_0DD5); // get_platform

		CLEO_RegisterOpcode(0x2000, opcode_2000); // get_cleo_arg_count
		// 2001 free
		CLEO_RegisterOpcode(0x2002, opcode_2002); // cleo_return_with
		CLEO_RegisterOpcode(0x2003, opcode_2003); // cleo_return_fail

		initialized = true;
	}

	CCustomOpcodeSystem::~CCustomOpcodeSystem()
	{
		TRACE(""); // separator
		TRACE("Custom Opcode System finalized:");
		TRACE(" Last opcode executed: %04X", lastOpcode);
		TRACE(" Previous opcode executed: %04X", prevOpcode);
	}

	CCustomOpcodeSystem::OpcodeHandler CCustomOpcodeSystem::originalOpcodeHandlers[OriginalOpcodeHandlersCount];
	CCustomOpcodeSystem::OpcodeHandler CCustomOpcodeSystem::customOpcodeHandlers[CustomOpcodeHandlersCount];
	CustomOpcodeHandler CCustomOpcodeSystem::customOpcodeProc[LastCustomOpcode + 1];

	bool CCustomOpcodeSystem::RegisterOpcode(WORD opcode, CustomOpcodeHandler callback)
	{
		if (opcode > LastCustomOpcode)
		{
			SHOW_ERROR("Can not register [%04X] opcode! Out of supported range.", opcode);
			return false;
		}

		CustomOpcodeHandler& dst = customOpcodeProc[opcode];
		if (*dst != nullptr)
		{
			LOG_WARNING(0, "Opcode [%04X] already registered! Replacing...", opcode);
		}

		dst = callback;
		TRACE("Opcode [%04X] registered", opcode);
		return true;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, DWORD& uval)
	{
		CScriptEngine::GetScriptParams(&thread, 1);
		uval = opcodeParams[0].dwParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, DWORD uval)
	{
		opcodeParams[0].dwParam = uval;
		CScriptEngine::SetScriptParams(&thread, 1);
		return thread;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, int& nval)
	{
		CScriptEngine::GetScriptParams(&thread, 1);
		nval = opcodeParams[0].nParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, int nval)
	{
		opcodeParams[0].nParam = nval;
		CScriptEngine::SetScriptParams(&thread, 1);
		return thread;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, float& fval)
	{
		CScriptEngine::GetScriptParams(&thread, 1);
		fval = opcodeParams[0].fParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, float fval)
	{
		opcodeParams[0].fParam = fval;
		CScriptEngine::SetScriptParams(&thread, 1);
		return thread;
	}

	template<typename T>
	inline CRunningScript& operator>>(CRunningScript& thread, T *& pval)
	{
		GetScriptParams(&thread, 1);
		pval = reinterpret_cast<T *>(opcodeParams[0].pParam);
		return thread;
	}

	template<typename T>
	inline CRunningScript& operator<<(CRunningScript& thread, T *pval)
	{
		opcodeParams[0].pParam = (void *)(pval);
		SetScriptParams(&thread, 1);
		return thread;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, memory_pointer& pval)
	{
		CScriptEngine::GetScriptParams(&thread, 1);
		pval = opcodeParams[0].pParam;
		return thread;
	}

	template<typename T>
	inline CRunningScript& operator<<(CRunningScript& thread, memory_pointer pval)
	{
		opcodeParams[0].pParam = pval;
		SetScriptParams(&thread, 1);
		return thread;
	}

	const char* ReadStringParam(CRunningScript *thread, char* buff, int buffSize)
	{
		if (buffSize > 0) buff[buffSize - 1] = '\0'; // buffer always terminated
		return GetScriptStringParam(thread, 0, buff, buffSize - 1); // minus terminator
	}

	// write output\result string parameter
	bool WriteStringParam(CRunningScript* thread, const char* str)
	{
		auto target = GetStringParamWriteBuffer(thread);
		return WriteStringParam(target, str);
	}

	bool WriteStringParam(const StringParamBufferInfo& target, const char* str)
	{
		CCustomOpcodeSystem::lastErrorMsg.clear();

		if (str != nullptr && (size_t)str <= CCustomOpcodeSystem::MinValidAddress)
		{
			CCustomOpcodeSystem::lastErrorMsg = StringPrintf("Writing string from invalid '0x%X' pointer", target.data);
			return false;
		}

		if ((size_t)target.data <= CCustomOpcodeSystem::MinValidAddress)
		{
			CCustomOpcodeSystem::lastErrorMsg = StringPrintf("Writing string into invalid '0x%X' pointer argument", target.data);
			return false;
		}

		if (target.size == 0)
		{
			return false;
		}

		bool addTerminator = target.needTerminator;
		size_t buffLen = target.size - addTerminator;
		size_t length = str == nullptr ? 0 : strlen(str);

		if (buffLen > length) addTerminator = true; // there is space left for terminator

		length = std::min(length, buffLen);
		if (length > 0) std::memcpy(target.data, str, length);
		if (addTerminator) target.data[length] = '\0';

		return true;
	}

	StringParamBufferInfo GetStringParamWriteBuffer(CRunningScript* thread)
	{
		StringParamBufferInfo result;
		CCustomOpcodeSystem::lastErrorMsg.clear();

		auto paramType = thread->PeekDataType();
		if (IsImmInteger(paramType) || IsVariable(paramType))
		{
			// address to output buffer
			CScriptEngine::GetScriptParams(thread, 1);

			if (opcodeParams[0].dwParam <= CCustomOpcodeSystem::MinValidAddress)
			{
				CCustomOpcodeSystem::lastErrorMsg = StringPrintf("Writing string into invalid '0x%X' pointer argument", opcodeParams[0].dwParam);
				return result; // error
			}

			result.data = opcodeParams[0].pcParam;
			result.size = 0x7FFFFFFF; // user allocated memory block can be any size
			result.needTerminator = true;

			return result;
		}
		else
		if (IsVarString(paramType))
		{
			switch(paramType)
			{
				// short string variable
				case DT_VAR_TEXTLABEL:
				case DT_LVAR_TEXTLABEL:
				case DT_VAR_TEXTLABEL_ARRAY:
				case DT_LVAR_TEXTLABEL_ARRAY:
					result.data = (char*)CScriptEngine::GetScriptParamPointer(thread);
					result.size = 8;
					result.needTerminator = false;
					return result;

				// long string variable
				case DT_VAR_STRING:
				case DT_LVAR_STRING:
				case DT_VAR_STRING_ARRAY:
				case DT_LVAR_STRING_ARRAY:
					result.data = (char*)CScriptEngine::GetScriptParamPointer(thread);
					result.size = 16;
					result.needTerminator = false;
					return result;
			}
		}

		CCustomOpcodeSystem::lastErrorMsg = StringPrintf("Writing string, got argument %s", ToKindStr(paramType));
		CLEO_SkipOpcodeParams(thread, 1); // skip unhandled param
		return result; // error
	}

	// perform 'sprintf'-operation for parameters, passed through SCM
	int ReadFormattedString(CRunningScript *thread, char *outputStr, DWORD len, const char *format)
	{
		unsigned int written = 0;
		const char *iter = format;
		char* outIter = outputStr;
		char bufa[MAX_STR_LEN + 1], fmtbufa[64], *fmta;

		// invalid input arguments
		if(outputStr == nullptr || len == 0)
		{
			LOG_WARNING(thread, "ReadFormattedString invalid input arg(s) in script %s", ScriptInfoStr(thread).c_str());
			SkipUnusedVarArgs(thread);
			return -1; // error
		}

		if(len > 1 && format != nullptr)
		{
			while (*iter)
			{
				while (*iter && *iter != '%')
				{
					if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
					*outIter++ = *iter++;
				}

				if (*iter == '%')
				{
					// end of format string
					if (iter[1] == '\0')
					{
						LOG_WARNING(thread, "ReadFormattedString encountered incomplete format specifier in script %s", ScriptInfoStr(thread).c_str());
						SkipUnusedVarArgs(thread);
						return -1; // error
					}

					// escaped % character
					if (iter[1] == '%')
					{
						if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
						*outIter++ = '%';
						iter += 2;
						continue;
					}

					//get flags and width specifier
					fmta = fmtbufa;
					*fmta++ = *iter++;
					while (*iter == '0' ||
						   *iter == '+' ||
						   *iter == '-' ||
						   *iter == ' ' ||
						   *iter == '*' ||
						   *iter == '#')
					{
						if (*iter == '*')
						{
							//get width
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							CScriptEngine::GetScriptParams(thread, 1);
							_itoa_s(opcodeParams[0].dwParam, bufa, 10);

							char* buffiter = bufa;
							while (*buffiter)
								*fmta++ = *buffiter++;
						}
						else
							*fmta++ = *iter;
						iter++;
					}

					//get immidiate width value
					while (isdigit(*iter))
						*fmta++ = *iter++;

					//get precision
					if (*iter == '.')
					{
						*fmta++ = *iter++;
						if (*iter == '*')
						{
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							CScriptEngine::GetScriptParams(thread, 1);
							_itoa_s(opcodeParams[0].dwParam, bufa, 10);

							char* buffiter = bufa;
							while (*buffiter)
								*fmta++ = *buffiter++;
						}
						else
							while (isdigit(*iter))
								*fmta++ = *iter++;
					}
					//get size
					if (*iter == 'h' || *iter == 'l')
						*fmta++ = *iter++;

					switch (*iter)
					{
					case 's':
					{
						if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;

						const char* str = ReadStringParam(thread, bufa, sizeof(bufa));
						if(str == nullptr) // read error
						{
							static const char none[] = "(INVALID_STR)";
							str = none;
						}
						
						while (*str)
						{
							if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
							*outIter++ = *str++;
						}
						iter++;
						break;
					}

					case 'c':
						if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
						if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
						CScriptEngine::GetScriptParams(thread, 1);
						*outIter++ = (char)opcodeParams[0].nParam;
						iter++;
						break;

					default:
					{
						/* For non wc types, use system sprintf and append to wide char output */
						/* FIXME: for unrecognised types, should ignore % when printing */
						if (*iter == 'p' || *iter == 'P')
						{
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							CScriptEngine::GetScriptParams(thread, 1);
							sprintf_s(bufa, "%08X", opcodeParams[0].dwParam);
						}
						else
						{
							*fmta++ = *iter;
							*fmta = '\0';
							if (*iter == 'a' || *iter == 'A' ||
								*iter == 'e' || *iter == 'E' ||
								*iter == 'f' || *iter == 'F' ||
								*iter == 'g' || *iter == 'G')
							{
								if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
								CScriptEngine::GetScriptParams(thread, 1);
								sprintf_s(bufa, fmtbufa, opcodeParams[0].fParam);
							}
							else
							{
								if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
								CScriptEngine::GetScriptParams(thread, 1);
								sprintf_s(bufa, fmtbufa, opcodeParams[0].pParam);
							}
						}
						char* bufaiter = bufa;
						while (*bufaiter)
						{
							if (written++ >= len) goto _ReadFormattedString_OutOfMemory;
							*outIter++ = *bufaiter++;
						}
						iter++;
						break;
					}
					}
				}
			}
		}

		if (written >= len)
		{
		_ReadFormattedString_OutOfMemory: // jump here on error

			LOG_WARNING(thread, "Target buffer too small (%d) to read whole formatted string in script %s", len, ScriptInfoStr(thread).c_str());
			SkipUnusedVarArgs(thread);
			outputStr[len - 1] = '\0';
			return -1; // error
		}

		// still more var-args available
		if (thread->PeekDataType() != DT_END)
		{
			LOG_WARNING(thread, "More params than slots in formatted string in script %s", ScriptInfoStr(thread).c_str());
		}
		SkipUnusedVarArgs(thread); // skip terminator too

		outputStr[written] = '\0';
		return (int)written;

	_ReadFormattedString_ArgMissing: // jump here on error
		LOG_WARNING(thread, "Less params than slots in formatted string in script %s", ScriptInfoStr(thread).c_str());
		thread->IncPtr(); // skip vararg terminator
		outputStr[written] = '\0';
		return -1; // error
	}

	OpcodeResult CCustomOpcodeSystem::CleoReturnGeneric(WORD opcode, CRunningScript* thread, bool returnArgs, DWORD returnArgCount, bool strictArgCount)
	{
		auto cs = reinterpret_cast<CCustomScript*>(thread);

		ScmFunction* scmFunc = ScmFunction::Get(cs->GetScmFunction());
		if (scmFunc == nullptr)
		{
			SHOW_ERROR("Invalid Cleo Call reference. [%04X] possibly used without preceding [0AB1] in script %s\nScript suspended.", opcode, cs->GetInfoStr().c_str());
			return thread->Suspend();
		}

		// store return arguments
		static SCRIPT_VAR arguments[32];
		static bool argumentIsStr[32];
		std::forward_list<std::string> stringParams; // scope guard for strings
		if (returnArgs)
		{
			if (returnArgCount > 32)
			{
				SHOW_ERROR("Opcode [%04X] has too many (%d) args in script %s\nScript suspended.", opcode, returnArgCount, cs->GetInfoStr().c_str());
				return thread->Suspend();
			}

			auto nVarArg = GetVarArgCount(thread);
			if (returnArgCount > nVarArg)
			{
				SHOW_ERROR("Opcode [%04X] declared %d args, but %d was provided in script %s\nScript suspended.", opcode, returnArgCount, nVarArg, ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}

			for (DWORD i = 0; i < returnArgCount; i++)
			{
				SCRIPT_VAR* arg = arguments + i;
				argumentIsStr[i] = false;

				auto paramType = (eDataType)*thread->GetBytePointer();
				if (IsImmInteger(paramType) || IsVariable(paramType))
				{
					*thread >> arg->dwParam;
				}
				else if (paramType == DT_FLOAT)
				{
					*thread >> arg->fParam;
				}
				else if (IsImmString(paramType) || IsVarString(paramType))
				{
					argumentIsStr[i] = true;

					OPCODE_READ_PARAM_STRING(str);
					stringParams.emplace_front(str);
					arg->pcParam = stringParams.front().data();
				}
				else
				{
					SHOW_ERROR("Invalid argument type '0x%02X' in opcode [%04X] in script %s\nScript suspended.", paramType, opcode, ScriptInfoStr(thread).c_str());
					return thread->Suspend();
				}
			}
		}

		// handle program flow
		scmFunc->Return(cs); // jump back to cleo_call, right after last input param. Return slot var args starts here
		delete scmFunc;

		if (returnArgs)
		{
			DWORD returnSlotCount = GetVarArgCount(cs);
			if (returnSlotCount != returnArgCount)
			{
				if (strictArgCount)
				{
					SHOW_ERROR_COMPAT("Opcode [%04X] returned %d params, while function caller expected %d in script %s\nScript suspended.", opcode, returnArgCount, returnSlotCount, cs->GetInfoStr().c_str());
					return cs->Suspend();
				}
				else
				{
					LOG_WARNING(thread, "Opcode [%04X] returned %d params, while function caller expected %d in script %s", opcode, returnArgCount, returnSlotCount, cs->GetInfoStr().c_str());
				}
			}

			// set return args
			for (DWORD i = 0; i < std::min<DWORD>(returnArgCount, returnSlotCount); i++)
			{
				auto arg = (SCRIPT_VAR*)thread->GetBytePointer();

				auto paramType = *(eDataType*)arg;
				if (IsVarString(paramType))
				{
					OPCODE_WRITE_PARAM_STRING(arguments[i].pcParam);
				} 
				else if (IsVariable(paramType))
				{
					if (argumentIsStr[i]) // source was string, write it into provided buffer ptr
					{
						OPCODE_WRITE_PARAM_STRING(arguments[i].pcParam);
					}
					else
						*thread << arguments[i].dwParam;
				}
				else
				{
					SHOW_ERROR("Invalid output argument type '0x%02X' in opcode [%04X] in script %s\nScript suspended.", paramType, opcode, ScriptInfoStr(thread).c_str());
					return thread->Suspend();
				}
			}
		}

		SkipUnusedVarArgs(thread); // skip var args terminator too

		return OR_CONTINUE;
	}

	inline void ThreadJump(CRunningScript *thread, int off)
	{
		thread->SetIp(off < 0 ? thread->GetBasePointer() - off : CleoInstance.ScriptEngine.scmBlock + off);
	}

	void SkipUnusedVarArgs(CRunningScript *thread)
	{
		while (thread->PeekDataType() != DT_END)
			CLEO_SkipOpcodeParams(thread, 1);

		thread->IncPtr(); // skip terminator
	}

	DWORD GetVarArgCount(CRunningScript* thread)
	{
		// store state
		const auto ip = thread->GetBytePointer();
		const auto handledParams = CleoInstance.OpcodeSystem.handledParamCount;

		DWORD count = 0;
		while (thread->PeekDataType() != DT_END)
		{
			CLEO_SkipOpcodeParams(thread, 1);
			count++;
		}

		// restore state
		thread->SetIp(ip);
		CleoInstance.OpcodeSystem.handledParamCount = handledParams;

		return count;
	}

	/************************************************************************/
	/*						Opcode definitions								*/
	/************************************************************************/

	// terminate_this_script
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_004E(CRunningScript* thread)
	{
		CleoInstance.ScriptEngine.RemoveScript(thread);
		return OR_INTERRUPT;
	}

	// GOSUB return
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0051(CRunningScript* thread)
	{
		if (thread->SP == 0 && !IsLegacyScript(thread)) // CLEO5 - allow use of GOSUB `return` to exit cleo calls too
		{
			OPCODE_CONDITION_RESULT(false);
			return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x0051, thread, false); // try CLEO's function return
		}

		if (thread->SP == 0)
		{
			SHOW_ERROR("`return` used without preceding `gosub` call in script %s\nScript suspended.", ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		size_t tableIdx = 0x0051 / 100; // 100 opcodes peer handler table
		return originalOpcodeHandlers[tableIdx](thread, 0x0051); // call game's original
	}

	// load_and_launch_mission_internal
	// load_and_launch_mission_internal {index} [int]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0417(CRunningScript* thread)
	{
		CleoInstance.ScriptEngine.missionIndex = CLEO_PeekIntOpcodeParam(thread);

		size_t tableIdx = 0x0417 / 100; // 100 opcodes peer handler table
		return originalOpcodeHandlers[tableIdx](thread, 0x0417); // call game's original
	}

	// stream_custom_script
	// stream_custom_script {scriptFileName} [string] [arguments]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0A92(CRunningScript *thread)
	{
		OPCODE_READ_PARAM_STRING(path);

		auto filename = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(path, DIR_CLEO); // legacy: default search location is game\cleo directory
		TRACE("[0A92] Starting new custom script %s from thread named '%s'", filename.c_str(), thread->GetName().c_str());

		auto cs = new CCustomScript(filename.c_str(), false, thread);
		thread->SetConditionResult(cs && cs->IsOk());
		if (cs && cs->IsOk())
		{
			CleoInstance.ScriptEngine.AddCustomScript(cs);
			((::CRunningScript*)thread)->ReadParametersForNewlyStartedScript((::CRunningScript*)cs);
		}
		else
		{
			if (cs) delete cs;
			SkipUnusedVarArgs(thread);
			LOG_WARNING(0, "Failed to load script '%s' in script ", filename.c_str(), ScriptInfoStr(thread).c_str());
		}

		return OR_CONTINUE;
	}

	// terminate_this_custom_script
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0A93(CRunningScript *thread)
	{
		CCustomScript *cs = reinterpret_cast<CCustomScript *>(thread);
		if (thread->IsMission() || !cs->IsCustom())
		{
			LOG_WARNING(0, "Incorrect usage of opcode [0A93] in script '%s'. Use [004E] instead.", ScriptInfoStr(thread).c_str());
			return OR_CONTINUE; // legacy behavior
		}

		CleoInstance.ScriptEngine.RemoveScript(thread);
		return OR_INTERRUPT;
	}

	// load_and_launch_custom_mission
	// load_and_launch_custom_mission {scriptFileName} [string] [arguments]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0A94(CRunningScript *thread)
	{
		OPCODE_READ_PARAM_STRING(path);

		auto filename = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(path, DIR_CLEO); // legacy: default search location is game\cleo directory
		filename += ".cm"; // add custom mission extension
		TRACE("[0A94] Starting new custom mission '%s' from thread named '%s'", filename.c_str(), thread->GetName().c_str());

		auto cs = new CCustomScript(filename.c_str(), true, thread);
		thread->SetConditionResult(cs && cs->IsOk());
		if (cs && cs->IsOk())
		{
			CleoInstance.ScriptEngine.AddCustomScript(cs);
			CTheScripts::WipeLocalVariableMemoryForMissionScript();
			auto fakeScriptAddress = (BYTE*)missionLocals - offsetof(CRunningScript, LocalVar); // TODO: maybe copy params ourself instead?
			((::CRunningScript*)thread)->ReadParametersForNewlyStartedScript((::CRunningScript*)fakeScriptAddress);
		}
		else
		{
			if (cs) delete cs;
			SkipUnusedVarArgs(thread);
			LOG_WARNING(0, "[0A94] Failed to load mission '%s' from script '%s'.", filename.c_str(), thread->GetName().c_str());
		}

		return OR_CONTINUE;
	}

	// save_this_custom_script
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0A95(CRunningScript *thread)
	{
		if (thread->IsCustom())
		{
			reinterpret_cast<CCustomScript*>(thread)->EnableSaving();
		}
		return OR_CONTINUE;
	}

	// gosub_if_false
	// gosub_if_false [label]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AA0(CRunningScript *thread)
	{
		auto offset = OPCODE_READ_PARAM_INT();

		if (thread->GetConditionResult()) return OR_CONTINUE;

		thread->PushStack(thread->GetBytePointer());
		ThreadJump(thread, offset);
		return OR_CONTINUE;
	}

	// return_if_false
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AA1(CRunningScript *thread)
	{
		if (thread->GetConditionResult()) return OR_CONTINUE;

		if (thread->SP == 0)
		{
			SHOW_ERROR("`return_if_false` used without preceding `gosub` call in script %s\nScript suspended.", ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		thread->SetIp(thread->PopStack());
		return OR_CONTINUE;
	}

	// is_game_version_original
	// is_game_version_original (logical)
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AA9(CRunningScript *thread)
	{
		auto gameVer = CleoInstance.VersionManager.GetGameVersion();
		auto scriptVer = CLEO_GetScriptVersion(thread);

		bool result = (gameVer == GV_US10) ||
			(scriptVer <= CLEO_VER_4_MIN && gameVer == GV_EU10);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	// cleo_call
	// cleo_call [label] {numParams} [int] {params} [arguments]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AB1(CRunningScript *thread)
	{
		int label = 0;
		std::string moduleTxt;

		auto paramType = thread->PeekDataType();
		if (IsImmInteger(paramType) || IsVariable(paramType))
		{
			*thread >> label; // label offset
		}
		else if (IsImmString(paramType) || IsVarString(paramType))
		{
			char tmp[MAX_STR_LEN + 1];
			auto str = ReadStringParam(thread, tmp, sizeof(tmp)); // string with module and export name
			if (str != nullptr) moduleTxt = str;
		}
		else
		{
			SHOW_ERROR("Invalid type of first argument in opcode [0AB1], in script %s", ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}
	
		ScmFunction* scmFunc = new ScmFunction(thread);
		
		// parse module reference text
		if (!moduleTxt.empty())
		{
			auto pos = moduleTxt.find('@');
			if (pos == moduleTxt.npos)
			{
				SHOW_ERROR("Invalid module reference '%s' in opcode [0AB1] in script %s \nScript suspended.", moduleTxt.c_str(), ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
			auto strExport = std::string_view(moduleTxt.data(), pos);
			auto strModule = std::string_view(moduleTxt.data() + pos + 1);

			// get module's file absolute path
			auto modulePath = std::string(strModule);
			modulePath = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(modulePath.c_str(), DIR_SCRIPT); // by default search relative to current script location

			// get export reference
			auto scriptRef = CleoInstance.ModuleSystem.GetExport(modulePath, strExport);
			if (!scriptRef.Valid())
			{
				SHOW_ERROR("Not found module '%s' export '%s', requested by opcode [0AB1] in script %s", modulePath.c_str(), moduleTxt.c_str(), ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}

			reinterpret_cast<CCustomScript*>(thread)->SetScriptFileDir(FS::path(modulePath).parent_path().string().c_str());
			reinterpret_cast<CCustomScript*>(thread)->SetScriptFileName(FS::path(modulePath).filename().string().c_str());
			thread->SetBaseIp(scriptRef.base);
			label = scriptRef.offset;
		}

		// "number of input parameters" opcode argument
		DWORD nParams = 0;
		paramType = thread->PeekDataType();
		if (paramType != DT_END)
		{
			if (IsImmInteger(paramType))
			{
				*thread >> nParams;
			}
			else
			{
				SHOW_ERROR("Invalid type (%s) of the 'input param count' argument in opcode [0AB1] in script %s \nScript suspended.", ToKindStr(paramType), ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
		}
		if (nParams)
		{
			auto nVarArg = GetVarArgCount(thread);
			if (nParams > nVarArg) // if less it means there are return params too
			{
				SHOW_ERROR("Opcode [0AB1] declared %d input args, but provided %d in script %s\nScript suspended.", nParams, nVarArg, ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}

			if (nParams > 32)
			{
				SHOW_ERROR("Argument count %d is out of supported range (32) of opcode [0AB1] in script %s", nParams, ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
		}
		scmFunc->callArgCount = (BYTE)nParams;

		static SCRIPT_VAR arguments[32];
		SCRIPT_VAR* locals = thread->IsMission() ? missionLocals : thread->GetVarPtr();
		SCRIPT_VAR* localsEnd = locals + 32;
		SCRIPT_VAR* storedLocals = scmFunc->savedTls;

		// collect arguments
		for (DWORD i = 0; i < nParams; i++)
		{
			SCRIPT_VAR* arg = arguments + i;

			auto paramType = thread->PeekDataType();
			if (IsImmInteger(paramType) || IsVariable(paramType))
			{
				*thread >> arg->dwParam;
			}
			else if(paramType == DT_FLOAT)
			{
				*thread >> arg->fParam;
			}
			else if (IsImmString(paramType) || IsVarString(paramType))
			{
				// imm string texts exists in script code, but without terminator character.
				// For strings stored in variables there is no guarantee these will end with terminator.
				// In both cases copy is necessary to create proper c-string
				char tmp[MAX_STR_LEN + 1];
				auto str = ReadStringParam(thread, tmp, sizeof(tmp));
				scmFunc->stringParams.emplace_back(str);
				arg->pcParam = (char*)scmFunc->stringParams.back().c_str();
			}
			else
			{
				SHOW_ERROR("Invalid argument type '0x%02X' in opcode [0AB1] in script %s\nScript suspended.", paramType, ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
		}

		// all arguments read
		scmFunc->retnAddress = thread->GetBytePointer();

		// pass arguments as new scope local variables
		memcpy(locals, arguments, nParams * sizeof(SCRIPT_VAR));

		// initialize (clear) rest of new scope local variables
		if (CLEO_GetScriptVersion(thread) >= CLEO_VER_4_MIN) // CLEO 3 did not cleared local variables
		{
			for (DWORD i = nParams; i < 32; i++)
			{
				thread->SetIntVar(i, 0); // fill with zeros
			}
		}

		// jump to label
		ThreadJump(thread, label); // script offset
		return OR_CONTINUE;
	}

	// cleo_return
	// cleo_return {numRet} [int] {retParams} [arguments]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AB2(CRunningScript *thread)
	{
		auto returnParamCount = OPCODE_PEEK_VARARG_COUNT();
		if (returnParamCount)
		{
			auto paramType = thread->PeekDataType();
			if (!IsImmInteger(paramType))
			{
				SHOW_ERROR("Invalid type of first argument in opcode [0AB2], in script %s", ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
			DWORD declaredParamCount; *thread >> declaredParamCount;

			if (returnParamCount - 1 < declaredParamCount) // minus 'num args' itself
			{
				SHOW_ERROR("Opcode [0AB2] declared %d return args, but provided %d in script %s\nScript suspended.", declaredParamCount, returnParamCount - 1, ScriptInfoStr(thread).c_str());
				return thread->Suspend();
			}
			else if (returnParamCount - 1 > declaredParamCount) // more args than needed, not critical
			{
				LOG_WARNING(thread, "Opcode [0AB2] declared %d return args, but provided %d in script %s", declaredParamCount, returnParamCount - 1, ScriptInfoStr(thread).c_str());
			}

			returnParamCount = declaredParamCount;
		}

		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x0AB2, thread, true, returnParamCount, !IsLegacyScript(thread));
	}

	// set_cleo_shared_var
	// set_cleo_shared_var {index} [int] {value} [any]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AB3(CRunningScript *thread)
	{
		auto varIdx = OPCODE_READ_PARAM_INT();

		const auto VarCount = _countof(CScriptEngine::CleoVariables);
		if (varIdx < 0 || varIdx >= VarCount)
		{
			SHOW_ERROR("Variable index '%d' out of supported range in script %s\nScript suspended.", varIdx, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		CleoInstance.ScriptEngine.CleoVariables[varIdx] = OPCODE_READ_PARAM_ANY32();
		return OR_CONTINUE;
	}

	// get_cleo_shared_var
	// [var result: any] = get_cleo_shared_var {index} [int]
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0AB4(CRunningScript *thread)
	{
		auto varIdx = OPCODE_READ_PARAM_INT();

		const auto VarCount = _countof(CScriptEngine::CleoVariables);
		if (varIdx < 0 || varIdx >= VarCount)
		{
			SHOW_ERROR("Variable index '%d' out of supported range in script %s\nScript suspended.", varIdx, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		OPCODE_WRITE_PARAM_ANY32(CleoInstance.ScriptEngine.CleoVariables[varIdx]);
		return OR_CONTINUE;
	}

	// get_platform
	// [var platform: Platform] = get_platform
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0DD5(CRunningScript* thread)
	{
		OPCODE_WRITE_PARAM_INT(PLATFORM_WINDOWS);
		return OR_CONTINUE;
	}

	// get_cleo_arg_count
	// [var count: int] = get_cleo_arg_count
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_2000(CRunningScript* thread)
	{
		auto cs = reinterpret_cast<CCustomScript*>(thread);

		ScmFunction* scmFunc = ScmFunction::Get(cs->GetScmFunction());
		if (scmFunc == nullptr)
		{
			SHOW_ERROR("Quering argument count without preceding CLEO function call in script %s\nScript suspended.", cs->GetInfoStr().c_str());
			return thread->Suspend();
		}

		OPCODE_WRITE_PARAM_INT(scmFunc->callArgCount);
		return OR_CONTINUE;
	}

	// cleo_return_with
	// cleo_return_with {conditionResult} [bool] {retArgs} [arguments] (logical)
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_2002(CRunningScript* thread)
	{
		auto argCount = OPCODE_PEEK_VARARG_COUNT();
		if (argCount < 1)
		{
			SHOW_ERROR("Opcode [2002] missing condition result argument in script %s\nScript suspended.", ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		auto result = OPCODE_READ_PARAM_BOOL();
		argCount--;

		OPCODE_CONDITION_RESULT(result);
		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x2002, thread, true, argCount);
	}

	// cleo_return_fail
	// cleo_return_fail [arguments] (logical)
	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_2003(CRunningScript* thread)
	{
		auto argCount = OPCODE_PEEK_VARARG_COUNT();
		if (argCount != 0) // argument(s) not supported yet
		{
			SHOW_ERROR("Too many arguments of opcode [2003] in script %s\nScript suspended.", ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		OPCODE_CONDITION_RESULT(false);
		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x2003, thread);
	}
}
