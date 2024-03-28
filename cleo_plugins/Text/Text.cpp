﻿#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"
#include "CHud.h"
#include "CGame.h"
#include "CMessages.h"
#include "CModelInfo.h"
#include "CText.h"
#include "CTextManager.h"
#include <shlwapi.h>

using namespace CLEO;
using namespace plugin;

class Text
{
public:
	static CTextManager textManager;
	
	static char msgBuffLow[MAX_STR_LEN + 1];
	static char msgBuffHigh[MAX_STR_LEN + 1];
	static const size_t MsgBigStyleCount = 7;
	static char msgBuffBig[MsgBigStyleCount][MAX_STR_LEN + 1];

    Text()
    {
        auto cleoVer = CLEO_GetVersion();
        if (cleoVer < CLEO_VERSION)
        {
            auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
            MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
            return;
        }

        //register opcodes
		CLEO_RegisterOpcode(0x0ACA, opcode_0ACA); // print_help_string
		CLEO_RegisterOpcode(0x0ACB, opcode_0ACB); // print_big_string
		CLEO_RegisterOpcode(0x0ACC, opcode_0ACC); // print_string
		CLEO_RegisterOpcode(0x0ACD, opcode_0ACD); // print_string_now
		CLEO_RegisterOpcode(0x0ACE, opcode_0ACE); // print_help_formatted
		CLEO_RegisterOpcode(0x0ACF, opcode_0ACF); // print_big_formatted
		CLEO_RegisterOpcode(0x0AD0, opcode_0AD0); // print_formatted
		CLEO_RegisterOpcode(0x0AD1, opcode_0AD1); // print_formatted_now
							    
		CLEO_RegisterOpcode(0x0AD3, opcode_0AD3); // string_format
		CLEO_RegisterOpcode(0x0AD4, opcode_0AD4); // scan_string
		CLEO_RegisterOpcode(0x0ADB, opcode_0ADB); // get_name_of_vehicle_model
							    
		CLEO_RegisterOpcode(0x0ADE, opcode_0ADE); // get_text_label_string
		CLEO_RegisterOpcode(0x0ADF, opcode_0ADF); // add_text_label
		CLEO_RegisterOpcode(0x0AE0, opcode_0AE0); // remove_text_label
							    
		CLEO_RegisterOpcode(0x0AED, opcode_0AED); // string_float_format

		CLEO_RegisterOpcode(0x2600, opcode_2600); // is_text_empty
		CLEO_RegisterOpcode(0x2601, opcode_2601); // is_text_equal
		CLEO_RegisterOpcode(0x2602, opcode_2602); // is_text_in_text
		CLEO_RegisterOpcode(0x2603, opcode_2603); // is_text_prefix
		CLEO_RegisterOpcode(0x2604, opcode_2604); // is_text_sufix

		// register event callbacks
		CLEO_RegisterCallback(eCallbackId::GameBegin, OnGameBegin);
		CLEO_RegisterCallback(eCallbackId::GameEnd, OnGameEnd);

		// install hooks
		MemPatchJump(0x006A0050, &HOOK_CTextGet); // FUNC_CText__Get from CText.cpp
	}

	static void __stdcall OnGameBegin(DWORD saveSlot)
	{
		textManager.LoadFxts();
	}

	static void __stdcall OnGameEnd()
	{
		textManager.Clear();
	}

	// hook of game's CText::Get
	static const char* __fastcall HOOK_CTextGet(CText* text, int dummy, const char* gxt)
	{
		if ((gxt[0] == '\0') || (gxt[0] == ' ')) return "";

		auto result = Text::textManager.LocateFxt(gxt);
		if (result != nullptr) return result;

		bool found;
		result = text->tkeyMain.GetTextByLabel(gxt, &found);
		if (found) return result;

		if (text->missionTableLoaded || CGame::bMissionPackGame || text->haveTabl)
		{
			result = text->tkeyMission.GetTextByLabel(gxt, &found);
			if (found) return result;
		}

		return "";
	}

	//0ACA=1,show_text_box %1d%
	static OpcodeResult __stdcall opcode_0ACA(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(text);

		CHud::SetHelpMessage(text, true, false, false);
		return OR_CONTINUE;
	}

	//0ACB=3,show_styled_text %1d% time %2d% style %3d%
	static OpcodeResult __stdcall opcode_0ACB(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(text);
		auto time = OPCODE_READ_PARAM_INT();
		auto style = OPCODE_READ_PARAM_INT();

		auto styleIdx = std::clamp(style, 0, (int)MsgBigStyleCount - 1);
		strncpy(msgBuffBig[styleIdx], text, sizeof(msgBuffBig[styleIdx]));
		CMessages::AddBigMessage(msgBuffBig[styleIdx], time, style - 1);
		return OR_CONTINUE;
	}

	//0ACC=2,show_text_lowpriority %1d% time %2d%
	static OpcodeResult __stdcall opcode_0ACC(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(text);
		auto time = OPCODE_READ_PARAM_INT();

		strncpy(msgBuffLow, text, sizeof(msgBuffLow));
		CMessages::AddMessage(msgBuffLow, time, false, false);
		return OR_CONTINUE;
	}

	//0ACD=2,show_text_highpriority %1d% time %2d%
	static OpcodeResult __stdcall opcode_0ACD(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(text);
		auto time = OPCODE_READ_PARAM_INT();

		strncpy(msgBuffHigh, text, sizeof(msgBuffHigh));
		CMessages::AddMessageJumpQ(msgBuffHigh, time, false, false);
		return OR_CONTINUE;
	}

	//0ACE=-1,show_formatted_text_box %1d%
	static OpcodeResult __stdcall opcode_0ACE(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_FORMATTED(text);

		CHud::SetHelpMessage(text, true, false, false);
		return OR_CONTINUE;
	}

	//0ACF=-1,show_formatted_styled_text %1d% time %2d% style %3d%
	static OpcodeResult __stdcall opcode_0ACF(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(format);
		auto time = OPCODE_READ_PARAM_INT();
		auto style = OPCODE_READ_PARAM_INT();
		OPCODE_READ_PARAMS_FORMATTED(format, text);

		auto styleIdx = std::clamp(style, 0, (int)MsgBigStyleCount - 1);
		strncpy(msgBuffBig[styleIdx], text, sizeof(msgBuffBig[styleIdx]));
		CMessages::AddBigMessage(msgBuffBig[styleIdx], time, style - 1);
		return OR_CONTINUE;
	}

	//0AD0=-1,show_formatted_text_lowpriority %1d% time %2d%
	static OpcodeResult __stdcall opcode_0AD0(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(format);
		auto time = OPCODE_READ_PARAM_INT();
		OPCODE_READ_PARAMS_FORMATTED(format, text);

		strncpy(msgBuffLow, text, sizeof(msgBuffLow));
		CMessages::AddMessage(msgBuffLow, time, false, false);
		return OR_CONTINUE;
	}

	//0AD1=-1,show_formatted_text_highpriority %1d% time %2d%
	static OpcodeResult __stdcall opcode_0AD1(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(format);
		auto time = OPCODE_READ_PARAM_INT();
		OPCODE_READ_PARAMS_FORMATTED(format, text);

		strncpy(msgBuffHigh, text, sizeof(msgBuffHigh));
		CMessages::AddMessageJumpQ(msgBuffHigh, time, false, false);
		return OR_CONTINUE;
	}

	//0AD3=-1,string %1d% format %2d% ...
	static OpcodeResult __stdcall opcode_0AD3(CRunningScript* thread)
	{
		auto result = OPCODE_READ_PARAM_OUTPUT_VAR_STRING();
		OPCODE_READ_PARAM_STRING_FORMATTED(text);

		OPCODE_WRITE_PARAM_VAR_STRING(result, text);
		return OR_CONTINUE;
	}

	//0AD4=-1,%3d% = scan_string %1d% format %2d%  //IF and SET
	static OpcodeResult __stdcall opcode_0AD4(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(src);
		OPCODE_READ_PARAM_STRING(format);

		auto readCount = OPCODE_READ_PARAM_OUTPUT_VAR_INT(); // store_to

		// collect provided by caller store_to variables
		size_t outputParamCount = 0;
		SCRIPT_VAR* outputParams[35];
		struct StringParamDesc
		{
			bool used = false;
			StringParamBufferInfo target;
			std::string str;
		} stringParams[35];

		for (int i = 0; i < 35; i++)
		{
			auto paramType = thread->PeekDataType();

			if (paramType == DT_END)
			{
				outputParams[i] = nullptr;
				continue;
			}

			if (IsVarString(paramType))
			{
				if (IsLegacyScript(thread))
				{
					// older CLEOs did not carred about string variable size limitations
					// just give pointer to the variable's data and allow overflow depending on input data
					outputParams[i] = CLEO_GetPointerToScriptVariable(thread);
				}
				else
				{
					stringParams[i].used = true;
					stringParams[i].target = OPCODE_READ_PARAM_OUTPUT_VAR_STRING();

					stringParams[i].str.resize(MAX_STR_LEN); // temp storage
					outputParams[i] = (SCRIPT_VAR*)stringParams[i].str.data();
				}
			}
			else
			{
				outputParams[i] = OPCODE_READ_PARAM_OUTPUT_VAR_ANY32();
			}

			outputParamCount++;
		}
		CLEO_SkipUnusedVarArgs(thread); // and var args terminator

		*readCount = sscanf(src, format,
			outputParams[0], outputParams[1], outputParams[2], outputParams[3], outputParams[4], outputParams[5],
			outputParams[6], outputParams[7], outputParams[8], outputParams[9], outputParams[10], outputParams[11],
			outputParams[12], outputParams[13], outputParams[14], outputParams[15], outputParams[16], outputParams[17],
			outputParams[18], outputParams[19], outputParams[20], outputParams[21], outputParams[22], outputParams[23],
			outputParams[24], outputParams[25], outputParams[26], outputParams[27], outputParams[28], outputParams[29],
			outputParams[30], outputParams[31], outputParams[32], outputParams[33], outputParams[34]);

		// transfer string params to target variables
		for (auto& p : stringParams)
		{
			if (p.used) OPCODE_WRITE_PARAM_VAR_STRING(p.target, p.str.c_str());
		}

		OPCODE_CONDITION_RESULT(outputParamCount == *readCount);
		return OR_CONTINUE;
	}

	//0ADB=2,%2d% = car_model %1d% name
	static OpcodeResult __stdcall opcode_0ADB(CRunningScript* thread)
	{
		auto modelIndex = OPCODE_READ_PARAM_UINT();

		CVehicleModelInfo* model;
		// if 1.0 US, prefer GetModelInfo function  makes it compatible with fastman92's limit adjuster
		if (CLEO_GetGameVersion() == CLEO::GV_US10)
			model = plugin::CallAndReturn<CVehicleModelInfo*, 0x403DA0, int>(modelIndex);
		else
			model = reinterpret_cast<CVehicleModelInfo*>(CModelInfo::ms_modelInfoPtrs[modelIndex]);

		auto str = std::string(std::string_view(model->m_szGameName, sizeof(model->m_szGameName))); // to proper cstr

		OPCODE_WRITE_PARAM_STRING(str.c_str());
		return OR_CONTINUE;
	}

	//0ADE=2,%2d% = text_by_GXT_entry %1d%
	static OpcodeResult __stdcall opcode_0ADE(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(gxt, 7); // GXT labels can be max 7 character long

		auto txt = textManager.Get(gxt);

		if (IsVarString(thread->PeekDataType()))
		{
			OPCODE_WRITE_PARAM_STRING(txt);
		}
		else
		{
			OPCODE_WRITE_PARAM_PTR(txt); // address of the text
		}
		return OR_CONTINUE;
	}

	//0ADF=2,add_dynamic_GXT_entry %1d% text %2d%
	static OpcodeResult __stdcall opcode_0ADF(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(gxt, 7); // GXT labels can be max 7 character long
		OPCODE_READ_PARAM_STRING(txt);

		textManager.AddFxt(gxt, txt);
		return OR_CONTINUE;
	}

	//0AE0=1,remove_dynamic_GXT_entry %1d%
	static OpcodeResult __stdcall opcode_0AE0(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(gxt, 7); // GXT labels can be max 7 character long

		textManager.RemoveFxt(gxt);
		return OR_CONTINUE;
	}

	//0AED=3,%3d% = float %1d% to_string_format %2d%
	static OpcodeResult __stdcall opcode_0AED(CRunningScript* thread)
	{
		// this opcode is useless now
		auto val = OPCODE_READ_PARAM_FLOAT();
		OPCODE_READ_PARAM_STRING(format);

		char result[32];
		sprintf_s(result, sizeof(result), format, val);

		OPCODE_WRITE_PARAM_STRING(result);
		return OR_CONTINUE;
	}

	//2600=1,  is_text_empty %1s%
	static OpcodeResult __stdcall opcode_2600(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(str);

		OPCODE_CONDITION_RESULT(str[0] == '\0');
		return OR_CONTINUE;
	}

	//2601=3,  is_text_equal %1s% another %2s% ignore_case %3d%
	static OpcodeResult __stdcall opcode_2601(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(a);
		OPCODE_READ_PARAM_STRING(b);
		auto ignoreCase = OPCODE_READ_PARAM_BOOL();

		auto result = ignoreCase ? _stricmp(a, b) : strcmp(a, b);

		OPCODE_CONDITION_RESULT(result == 0);
		return OR_CONTINUE;
	}

	//2602=3,  is_text_in_text %1s% sub_text %2s% ignore_case %3d%
	static OpcodeResult __stdcall opcode_2602(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(str);
		OPCODE_READ_PARAM_STRING(substr);
		auto ignoreCase = OPCODE_READ_PARAM_BOOL();

		if (substr[0] == '\0')
		{
			OPCODE_CONDITION_RESULT(true);
			return OR_CONTINUE;
		}

		auto result = ignoreCase ? StrStrIA(str, substr) : strstr(str, substr);

		OPCODE_CONDITION_RESULT(result != nullptr);
		return OR_CONTINUE;
	}

	//2603=3,  is_text_prefix %1s% prefix %2s% ignore_case %3d%
	static OpcodeResult __stdcall opcode_2603(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(str);
		OPCODE_READ_PARAM_STRING(prefix);
		auto ignoreCase = OPCODE_READ_PARAM_BOOL();

		auto prefixLen = strlen(prefix);
		auto result = ignoreCase ? _strnicmp(str, prefix, prefixLen) : strncmp(str, prefix, prefixLen);

		OPCODE_CONDITION_RESULT(result == 0);
		return OR_CONTINUE;
	}

	//2604=3,  is_text_sufix %1s% sufix %2s% ignore_case %3d%
	static OpcodeResult __stdcall opcode_2604(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING(str);
		OPCODE_READ_PARAM_STRING(sufix);
		auto ignoreCase = OPCODE_READ_PARAM_BOOL();

		auto strLen = strlen(str);
		auto sufixLen = strlen(sufix);

		if (sufixLen > strLen)
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		auto offset = strLen - sufixLen;
		auto result = ignoreCase ? _stricmp(str + offset, sufix) : strcmp(str + offset, sufix);

		OPCODE_CONDITION_RESULT(result == 0);
		return OR_CONTINUE;
	}
} textInstance;

CTextManager Text::textManager;
char Text::msgBuffLow[MAX_STR_LEN + 1];
char Text::msgBuffHigh[MAX_STR_LEN + 1];
char Text::msgBuffBig[MsgBigStyleCount][MAX_STR_LEN + 1];
