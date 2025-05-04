#include "stdafx.h"
#include "CleoBase.h"
#include "CGameVersionManager.h"
#include "CCustomOpcodeSystem.h"
#include "ScmFunction.h"


#define OPCODE_VALIDATE_STR_ARG_WRITE(x) if((void*)x == nullptr) { SHOW_ERROR("%s in script %s \nScript suspended.", CCustomOpcodeSystem::lastErrorMsg.c_str(), ((CCustomScript*)thread)->GetInfoStr().c_str()); return thread->Suspend(); }
#define OPCODE_READ_FORMATTED_STRING(thread, buf, bufSize, format) if(ReadFormattedString(thread, buf, bufSize, format) == -1) { SHOW_ERROR("%s in script %s \nScript suspended.", CCustomOpcodeSystem::lastErrorMsg.c_str(), ((CCustomScript*)thread)->GetInfoStr().c_str()); return thread->Suspend(); }

namespace CLEO 
{
	template<typename T> inline CRunningScript& operator>>(CRunningScript& thread, T*& pval);
	template<typename T> inline CRunningScript& operator<<(CRunningScript& thread, T* pval);
	template<typename T> inline CRunningScript& operator<<(CRunningScript& thread, memory_pointer pval);
	template<typename T> inline CRunningScript& operator>>(CRunningScript& thread, memory_pointer& pval);


	OpcodeResult __stdcall opcode_004E(CRunningScript* thread); // terminate_this_script
	OpcodeResult __stdcall opcode_0051(CRunningScript * thread); // GOSUB return
	OpcodeResult __stdcall opcode_0417(CRunningScript* thread); // load_and_launch_mission_internal

	OpcodeResult __stdcall opcode_0A92(CRunningScript* thread); // stream_custom_script
	OpcodeResult __stdcall opcode_0A93(CRunningScript* thread); // terminate_this_custom_script
	OpcodeResult __stdcall opcode_0A94(CRunningScript* thread); // load_and_launch_custom_mission
	OpcodeResult __stdcall opcode_0A95(CRunningScript* thread); // save_this_custom_script
	OpcodeResult __stdcall opcode_0AA0(CRunningScript* thread); // gosub_if_false
	OpcodeResult __stdcall opcode_0AA1(CRunningScript* thread); // return_if_false
	OpcodeResult __stdcall opcode_0AA9(CRunningScript* thread); // is_game_version_original
	OpcodeResult __stdcall opcode_0AB0(CRunningScript* thread); // is_key_pressed
	OpcodeResult __stdcall opcode_0AB1(CRunningScript* thread); // cleo_call
	OpcodeResult __stdcall opcode_0AB2(CRunningScript* thread); // cleo_return
	OpcodeResult __stdcall opcode_0AB3(CRunningScript* thread); // set_cleo_shared_var
	OpcodeResult __stdcall opcode_0AB4(CRunningScript* thread); // get_cleo_shared_var

	OpcodeResult __stdcall opcode_0ADC(CRunningScript* thread); // test_cheat

	OpcodeResult __stdcall opcode_0DD5(CRunningScript* thread); // get_platform

	OpcodeResult __stdcall opcode_2000(CRunningScript* thread); // get_cleo_arg_count
	// 2001 free slot
	OpcodeResult __stdcall opcode_2002(CRunningScript* thread); // cleo_return_with
	OpcodeResult __stdcall opcode_2003(CRunningScript* thread); // cleo_return_fail

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
		prevOpcode = lastOpcode;

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
				SHOW_ERROR_COMPAT("Code execution past script end in script %s\nThis usually happens when [004E] command is missing.\nScript suspended.", ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}
		}

		// execute registered callbacks
		OpcodeResult result = OR_NONE;
		for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptOpcodeProcess))
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
				SHOW_ERROR("Opcode [%04X] out of supported range! \nCalled in script %s\nScript suspended.", opcode, ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}

			CustomOpcodeHandler handler = customOpcodeProc[opcode];
			if(handler != nullptr)
			{
				lastCustomOpcode = opcode;
				return handler(thread);
			}

			// Not registered as custom opcode. Call game's original handler

			if (opcode > LastOriginalOpcode)
			{
				auto extensionMsg = CleoInstance.OpcodeInfoDb.GetExtensionMissingMessage(opcode);
				if (!extensionMsg.empty()) extensionMsg = " " + extensionMsg;

				SHOW_ERROR("Custom opcode [%04X] not registered!%s\nCalled in script %s\nPreviously called opcode: [%04X]\nScript suspended.",
					opcode,
					extensionMsg.c_str(),
					((CCustomScript*)thread)->GetInfoStr().c_str(), 
					prevOpcode);

				return thread->Suspend();
			}

			size_t tableIdx = opcode / 100; // 100 opcodes peer handler table
			result = originalOpcodeHandlers[tableIdx](thread, opcode);

			if(result == OR_ERROR)
			{
				auto extensionMsg = CleoInstance.OpcodeInfoDb.GetExtensionMissingMessage(opcode);
				if (!extensionMsg.empty()) extensionMsg = " " + extensionMsg;

				SHOW_ERROR("Opcode [%04X] not found!%s\nCalled in script %s\nPreviously called opcode: [%04X]\nScript suspended.",
					opcode,
					extensionMsg.c_str(),
					((CCustomScript*)thread)->GetInfoStr().c_str(), 
					prevOpcode);

				return thread->Suspend();
			}
		}

		// execute registered callbacks
		OpcodeResult callbackResult = OR_NONE;
		for (void* func : CleoInstance.GetCallbacks(eCallbackId::ScriptOpcodeProcessFinished))
		{
			typedef OpcodeResult WINAPI callback(CRunningScript*, DWORD, OpcodeResult);
			auto res = ((callback*)func)(thread, opcode, result);

			callbackResult = std::max(res, callbackResult); // store result with highest value from all callbacks
		}

		return (callbackResult != OR_NONE) ? callbackResult : result;
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
		_OpcodeHandler* handlersTable = gvm.TranslateMemoryAddress(MA_OPCODE_HANDLER);
		for(size_t i = 0; i < OriginalOpcodeHandlersCount; i++)
		{
			originalOpcodeHandlers[i] = handlersTable[i];
			handlersTable[i] = (_OpcodeHandler)customOpcodeHandler;
		}

		// initialize and apply new handlers table
		for (size_t i = 0; i < CustomOpcodeHandlersCount; i++)
		{
			customOpcodeHandlers[i] = (_OpcodeHandler)customOpcodeHandler;
		}
		MemWrite(gvm.TranslateMemoryAddress(MA_OPCODE_HANDLER_REF), &customOpcodeHandlers);
		MemWrite(0x00469EF0, &customOpcodeHandlers); // TODO: game version translation
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
		CLEO_RegisterOpcode(0x0AB0, opcode_0AB0);
		CLEO_RegisterOpcode(0x0AB1, opcode_0AB1);
		CLEO_RegisterOpcode(0x0AB2, opcode_0AB2);
		CLEO_RegisterOpcode(0x0AB3, opcode_0AB3);
		CLEO_RegisterOpcode(0x0AB4, opcode_0AB4);
		CLEO_RegisterOpcode(0x0ADC, opcode_0ADC);

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

	CCustomOpcodeSystem::_OpcodeHandler CCustomOpcodeSystem::originalOpcodeHandlers[OriginalOpcodeHandlersCount];
	CCustomOpcodeSystem::_OpcodeHandler CCustomOpcodeSystem::customOpcodeHandlers[CustomOpcodeHandlersCount];
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
		GetScriptParams(&thread, 1);
		uval = opcodeParams[0].dwParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, DWORD uval)
	{
		opcodeParams[0].dwParam = uval;
		SetScriptParams(&thread, 1);
		return thread;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, int& nval)
	{
		GetScriptParams(&thread, 1);
		nval = opcodeParams[0].nParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, int nval)
	{
		opcodeParams[0].nParam = nval;
		SetScriptParams(&thread, 1);
		return thread;
	}

	inline CRunningScript& operator>>(CRunningScript& thread, float& fval)
	{
		GetScriptParams(&thread, 1);
		fval = opcodeParams[0].fParam;
		return thread;
	}

	inline CRunningScript& operator<<(CRunningScript& thread, float fval)
	{
		opcodeParams[0].fParam = fval;
		SetScriptParams(&thread, 1);
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
		GetScriptParams(&thread, 1);
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
			GetScriptParams(thread, 1);

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
					result.data = (char*)GetScriptParamPointer(thread);
					result.size = 8;
					result.needTerminator = false;
					return result;

				// long string variable
				case DT_VAR_STRING:
				case DT_LVAR_STRING:
				case DT_VAR_STRING_ARRAY:
				case DT_LVAR_STRING_ARRAY:
					result.data = (char*)GetScriptParamPointer(thread);
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
			LOG_WARNING(thread, "ReadFormattedString invalid input arg(s) in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
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
						LOG_WARNING(thread, "ReadFormattedString encountered incomplete format specifier in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
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
							char *buffiter = bufa;

							//get width
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							GetScriptParams(thread, 1);
							_itoa(opcodeParams[0].dwParam, buffiter, 10);
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
							char *buffiter = bufa;
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							GetScriptParams(thread, 1);
							_itoa(opcodeParams[0].dwParam, buffiter, 10);
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
						GetScriptParams(thread, 1);
						*outIter++ = (char)opcodeParams[0].nParam;
						iter++;
						break;

					default:
					{
						/* For non wc types, use system sprintf and append to wide char output */
						/* FIXME: for unrecognised types, should ignore % when printing */
						char *bufaiter = bufa;
						if (*iter == 'p' || *iter == 'P')
						{
							if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
							GetScriptParams(thread, 1);
							sprintf(bufaiter, "%08X", opcodeParams[0].dwParam);
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
								GetScriptParams(thread, 1);
								sprintf(bufaiter, fmtbufa, opcodeParams[0].fParam);
							}
							else
							{
								if (thread->PeekDataType() == DT_END) goto _ReadFormattedString_ArgMissing;
								GetScriptParams(thread, 1);
								sprintf(bufaiter, fmtbufa, opcodeParams[0].pParam);
							}
						}
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

			LOG_WARNING(thread, "Target buffer too small (%d) to read whole formatted string in script %s", len, ((CCustomScript*)thread)->GetInfoStr().c_str());
			SkipUnusedVarArgs(thread);
			outputStr[len - 1] = '\0';
			return -1; // error
		}

		// still more var-args available
		if (thread->PeekDataType() != DT_END)
		{
			LOG_WARNING(thread, "More params than slots in formatted string in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
		}
		SkipUnusedVarArgs(thread); // skip terminator too

		outputStr[written] = '\0';
		return (int)written;

	_ReadFormattedString_ArgMissing: // jump here on error
		LOG_WARNING(thread, "Less params than slots in formatted string in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
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
				SHOW_ERROR("Opcode [%04X] declared %d args, but %d was provided in script %s\nScript suspended.", opcode, returnArgCount, nVarArg, ((CCustomScript*)thread)->GetInfoStr().c_str());
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
					SHOW_ERROR("Invalid argument type '0x%02X' in opcode [%04X] in script %s\nScript suspended.", paramType, opcode, ((CCustomScript*)thread)->GetInfoStr().c_str());
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
					SHOW_ERROR("Invalid output argument type '0x%02X' in opcode [%04X] in script %s\nScript suspended.", paramType, opcode, ((CCustomScript*)thread)->GetInfoStr().c_str());
					return thread->Suspend();
				}
			}
		}

		SkipUnusedVarArgs(thread); // skip var args terminator too

		return OR_CONTINUE;
	}

	inline void ThreadJump(CRunningScript *thread, int off)
	{
		thread->SetIp(off < 0 ? thread->GetBasePointer() - off : scmBlock + off);
	}

	void SkipUnusedVarArgs(CRunningScript *thread)
	{
		while (thread->PeekDataType() != DT_END)
			CLEO_SkipOpcodeParams(thread, 1);

		thread->IncPtr(); // skip terminator
	}

	DWORD GetVarArgCount(CRunningScript* thread)
	{
		const auto ip = thread->GetBytePointer();

		DWORD count = 0;
		while (thread->PeekDataType() != DT_END)
		{
			CLEO_SkipOpcodeParams(thread, 1);
			count++;
		}

		thread->SetIp(ip); // restore
		return count;
	}

	/************************************************************************/
	/*						Opcode definitions								*/
	/************************************************************************/

	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_004E(CRunningScript* thread)
	{
		CleoInstance.ScriptEngine.RemoveScript(thread);
		return OR_INTERRUPT;
	}

	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0051(CRunningScript* thread) // GOSUB return
	{
		if (thread->SP == 0 && !IsLegacyScript(thread)) // CLEO5 - allow use of GOSUB `return` to exit cleo calls too
		{
			SetScriptCondResult(thread, false);
			return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x0051, thread, false); // try CLEO's function return
		}

		if (thread->SP == 0)
		{
			SHOW_ERROR("`return` used without preceding `gosub` call in script %s\nScript suspended.", ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		size_t tableIdx = 0x0051 / 100; // 100 opcodes peer handler table
		return originalOpcodeHandlers[tableIdx](thread, 0x0051); // call game's original
	}

	OpcodeResult __stdcall CCustomOpcodeSystem::opcode_0417(CRunningScript* thread) // load_and_launch_mission_internal
	{
		MissionIndex = CLEO_PeekIntOpcodeParam(thread);
		size_t tableIdx = 0x0417 / 100; // 100 opcodes peer handler table
		return originalOpcodeHandlers[tableIdx](thread, 0x0417); // call game's original
	}

	//0A92=-1,create_custom_thread %1d%
	OpcodeResult __stdcall opcode_0A92(CRunningScript *thread)
	{
		OPCODE_READ_PARAM_STRING(path);

		auto filename = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(path, DIR_CLEO); // legacy: default search location is game\cleo directory
		TRACE("[0A92] Starting new custom script %s from thread named '%s'", filename.c_str(), thread->GetName().c_str());

		auto cs = new CCustomScript(filename.c_str(), false, thread);
		SetScriptCondResult(thread, cs && cs->IsOk());
		if (cs && cs->IsOk())
		{
			CleoInstance.ScriptEngine.AddCustomScript(cs);
			TransmitScriptParams(thread, cs);
		}
		else
		{
			if (cs) delete cs;
			SkipUnusedVarArgs(thread);
			LOG_WARNING(0, "Failed to load script '%s' in script ", filename.c_str(), ((CCustomScript*)thread)->GetInfoStr().c_str());
		}

		return OR_CONTINUE;
	}

	//0A93=0,terminate_this_custom_script
	OpcodeResult __stdcall opcode_0A93(CRunningScript *thread)
	{
		CCustomScript *cs = reinterpret_cast<CCustomScript *>(thread);
		if (thread->IsMission() || !cs->IsCustom())
		{
			LOG_WARNING(0, "Incorrect usage of opcode [0A93] in script '%s'. Use [004E] instead.", ((CCustomScript*)thread)->GetInfoStr().c_str());
			return OR_CONTINUE; // legacy behavior
		}

		CleoInstance.ScriptEngine.RemoveScript(thread);
		return OR_INTERRUPT;
	}

	//0A94=-1,create_custom_mission %1d%
	OpcodeResult __stdcall opcode_0A94(CRunningScript *thread)
	{
		OPCODE_READ_PARAM_STRING(path);

		auto filename = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(path, DIR_CLEO); // legacy: default search location is game\cleo directory
		filename += ".cm"; // add custom mission extension
		TRACE("[0A94] Starting new custom mission '%s' from thread named '%s'", filename.c_str(), thread->GetName().c_str());

		auto cs = new CCustomScript(filename.c_str(), true, thread);
		SetScriptCondResult(thread, cs && cs->IsOk());
		if (cs && cs->IsOk())
		{
			CleoInstance.ScriptEngine.AddCustomScript(cs);
			memset(missionLocals, 0, 1024 * sizeof(SCRIPT_VAR)); // same as CTheScripts::WipeLocalVariableMemoryForMissionScript
			TransmitScriptParams(thread, (CRunningScript*)((BYTE*)missionLocals - 0x3C));
		}
		else
		{
			if (cs) delete cs;
			SkipUnusedVarArgs(thread);
			LOG_WARNING(0, "[0A94] Failed to load mission '%s' from script '%s'.", filename.c_str(), thread->GetName().c_str());
		}

		return OR_CONTINUE;
	}

	//0A95=0,enable_thread_saving
	OpcodeResult __stdcall opcode_0A95(CRunningScript *thread)
	{
		if (thread->IsCustom())
		{
			reinterpret_cast<CCustomScript*>(thread)->EnableSaving();
		}
		return OR_CONTINUE;
	}

	//0AA0=1,gosub_if_false %1p%
	OpcodeResult __stdcall opcode_0AA0(CRunningScript *thread)
	{
		int off;
		*thread >> off;
		if (thread->GetConditionResult()) return OR_CONTINUE;
		thread->PushStack(thread->GetBytePointer());
		ThreadJump(thread, off);
		return OR_CONTINUE;
	}

	//0AA1=0,return_if_false
	OpcodeResult __stdcall opcode_0AA1(CRunningScript *thread)
	{
		if (thread->GetConditionResult()) return OR_CONTINUE;
		thread->SetIp(thread->PopStack());
		return OR_CONTINUE;
	}

	//0AA9=0,  is_game_version_original
	OpcodeResult __stdcall opcode_0AA9(CRunningScript *thread)
	{
		auto gameVer = CleoInstance.VersionManager.GetGameVersion();
		auto scriptVer = CLEO_GetScriptVersion(thread);

		bool result = (gameVer == GV_US10) ||
			(scriptVer <= CLEO_VER_4_MIN && gameVer == GV_EU10);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	//0AB0=1,  key_pressed %1d%
	OpcodeResult __stdcall opcode_0AB0(CRunningScript *thread)
	{
		DWORD key;
		*thread >> key;

		SHORT(__stdcall * GTA_GetKeyState)(int nVirtKey) = memory_pointer(0x0081E64C); // use ingame function as GetKeyState might look like keylogger to some AV software
		bool isDown = (GTA_GetKeyState(key) & 0x8000) != 0;

		SetScriptCondResult(thread, isDown);
		return OR_CONTINUE;
	}

	//0AB1=-1,call_scm_func %1p%
	OpcodeResult __stdcall opcode_0AB1(CRunningScript *thread)
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
			SHOW_ERROR("Invalid type of first argument in opcode [0AB1], in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}
	
		ScmFunction* scmFunc = new ScmFunction(thread);
		
		// parse module reference text
		if (!moduleTxt.empty())
		{
			auto pos = moduleTxt.find('@');
			if (pos == moduleTxt.npos)
			{
				SHOW_ERROR("Invalid module reference '%s' in opcode [0AB1] in script %s \nScript suspended.", moduleTxt.c_str(), ((CCustomScript*)thread)->GetInfoStr().c_str());
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
				SHOW_ERROR("Not found module '%s' export '%s', requested by opcode [0AB1] in script %s", modulePath.c_str(), moduleTxt.c_str(), ((CCustomScript*)thread)->GetInfoStr().c_str());
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
				SHOW_ERROR("Invalid type (%s) of the 'input param count' argument in opcode [0AB1] in script %s \nScript suspended.", ToKindStr(paramType), ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}
		}
		if (nParams)
		{
			auto nVarArg = GetVarArgCount(thread);
			if (nParams > nVarArg) // if less it means there are return params too
			{
				SHOW_ERROR("Opcode [0AB1] declared %d input args, but provided %d in script %s\nScript suspended.", nParams, nVarArg, ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}

			if (nParams > 32)
			{
				SHOW_ERROR("Argument count %d is out of supported range (32) of opcode [0AB1] in script %s", nParams, ((CCustomScript*)thread)->GetInfoStr().c_str());
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
				SHOW_ERROR("Invalid argument type '0x%02X' in opcode [0AB1] in script %s\nScript suspended.", paramType, ((CCustomScript*)thread)->GetInfoStr().c_str());
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

	//0AB2=-1,cleo_return
	OpcodeResult __stdcall opcode_0AB2(CRunningScript *thread)
	{
		DWORD returnParamCount = GetVarArgCount(thread);
		if (returnParamCount)
		{
			auto paramType = (eDataType)*thread->GetBytePointer();
			if (!IsImmInteger(paramType))
			{
				SHOW_ERROR("Invalid type of first argument in opcode [0AB2], in script %s", ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}
			DWORD declaredParamCount; *thread >> declaredParamCount;

			if (returnParamCount - 1 < declaredParamCount) // minus 'num args' itself
			{
				SHOW_ERROR("Opcode [0AB2] declared %d return args, but provided %d in script %s\nScript suspended.", declaredParamCount, returnParamCount - 1, ((CCustomScript*)thread)->GetInfoStr().c_str());
				return thread->Suspend();
			}
			else if (returnParamCount - 1 > declaredParamCount) // more args than needed, not critical
			{
				LOG_WARNING(thread, "Opcode [0AB2] declared %d return args, but provided %d in script %s", declaredParamCount, returnParamCount - 1, ((CCustomScript*)thread)->GetInfoStr().c_str());
			}

			returnParamCount = declaredParamCount;
		}

		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x0AB2, thread, true, returnParamCount, !IsLegacyScript(thread));
	}

	//0AB3=2,set_cleo_shared_var %1d% = %2d%
	OpcodeResult __stdcall opcode_0AB3(CRunningScript *thread)
	{
		auto varIdx = OPCODE_READ_PARAM_INT();

		const auto VarCount = _countof(CScriptEngine::CleoVariables);
		if (varIdx < 0 || varIdx >= VarCount)
		{
			SHOW_ERROR("Variable index '%d' out of supported range in script %s\nScript suspended.", varIdx, ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		auto paramType = thread->PeekDataType();
		if (!IsImmInteger(paramType) &&
			!IsImmFloat(paramType) &&
			!IsVariable(paramType))
		{
			SHOW_ERROR("Invalid value type (%s) in script %s \nScript suspended.", ToKindStr(paramType), ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		GetScriptParams(thread, 1);
		CleoInstance.ScriptEngine.CleoVariables[varIdx].dwParam = opcodeParams[0].dwParam;
		return OR_CONTINUE;
	}

	//0AB4=2,%2d% = get_cleo_shared_var %1d%
	OpcodeResult __stdcall opcode_0AB4(CRunningScript *thread)
	{
		auto varIdx = OPCODE_READ_PARAM_INT();

		const auto VarCount = _countof(CScriptEngine::CleoVariables);
		if (varIdx < 0 || varIdx >= VarCount)
		{
			SHOW_ERROR("Variable index '%d' out of supported range in script %s\nScript suspended.", varIdx, ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		auto paramType = thread->PeekDataType();
		if (!IsVariable(paramType))
		{
			SHOW_ERROR("Invalid result argument type (%s) in script %s \nScript suspended.", ToKindStr(paramType), ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		opcodeParams[0].dwParam = CleoInstance.ScriptEngine.CleoVariables[varIdx].dwParam;
		CLEO_RecordOpcodeParams(thread, 1);
		return OR_CONTINUE;
	}

	//0ADC=1,  test_cheat %1d%
	OpcodeResult __stdcall opcode_0ADC(CRunningScript *thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(text, sizeof(CCheat::m_CheatString));
		
		_strrev(_buff_text); // reverse
		auto len = strlen(_buff_text);
		if (_strnicmp(_buff_text, CCheat::m_CheatString, len) == 0)
		{
			CCheat::m_CheatString[0] = '\0'; // consume the cheat
			SetScriptCondResult(thread, true);
			return OR_CONTINUE;
		}

		SetScriptCondResult(thread, false);
		return OR_CONTINUE;
	}

	//0DD5=1,%1d% = get_platform
	OpcodeResult __stdcall opcode_0DD5(CRunningScript* thread)
	{
		*thread << PLATFORM_WINDOWS;
		return OR_CONTINUE;
	}

	//2000=1, %1d% = get_cleo_arg_count
	OpcodeResult __stdcall opcode_2000(CRunningScript* thread)
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

	//2002=-1, cleo_return_with ...
	OpcodeResult __stdcall opcode_2002(CRunningScript* thread)
	{
		DWORD argCount = GetVarArgCount(thread);
		if (argCount < 1)
		{
			SHOW_ERROR("Opcode [2002] missing condition result argument in script %s\nScript suspended.", ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		DWORD result; *thread >> result;
		argCount--;
		SetScriptCondResult(thread, result != 0);

		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x2002, thread, true, argCount);
	}

	//2003=-1, cleo_return_fail
	OpcodeResult __stdcall opcode_2003(CRunningScript* thread)
	{
		DWORD argCount = GetVarArgCount(thread);
		if (argCount != 0) // argument(s) not supported yet
		{
			SHOW_ERROR("Too many arguments of opcode [2003] in script %s\nScript suspended.", ((CCustomScript*)thread)->GetInfoStr().c_str());
			return thread->Suspend();
		}

		SetScriptCondResult(thread, false);
		return CleoInstance.OpcodeSystem.CleoReturnGeneric(0x2003, thread);
	}
}
