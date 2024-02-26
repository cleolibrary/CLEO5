#include <cstdio>
#include "CLEO.h"
#include "CLEO_Utils.h"
#include <string>

using namespace CLEO;

class IniFiles
{
public:
	IniFiles()
	{
		auto cleoVer = CLEO_GetVersion();
		if (cleoVer >= CLEO_VERSION)
		{
			// register opcodes
			CLEO_RegisterOpcode(0x0AF0, Script_InifileGetInt);
			CLEO_RegisterOpcode(0x0AF1, Script_InifileWriteInt);
			CLEO_RegisterOpcode(0x0AF2, Script_InifileGetFloat);
			CLEO_RegisterOpcode(0x0AF3, Script_InifileWriteFloat);
			CLEO_RegisterOpcode(0x0AF4, Script_InifileReadString);
			CLEO_RegisterOpcode(0x0AF5, Script_InifileWriteString);
		}
		else
		{
			std::string err(128, '\0');
			sprintf(err.data(), "This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
			MessageBox(HWND_DESKTOP, err.data(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
		}
	}

	// resused globals to cut down allocations
	static char section[128];
	static char key[128];

	static OpcodeResult WINAPI Script_InifileGetInt(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF0=4,%4d% = get_int_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		auto result = GetPrivateProfileInt(section, key, 0x80000000, path);

		OPCODE_WRITE_PARAM_INT(result);
		OPCODE_CONDITION_RESULT(result != 0x80000000);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileWriteInt(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF1=4,write_int %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_INT();
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		char strValue[32];
		_itoa(value, strValue, 10);
		auto result = WritePrivateProfileString(section, key, strValue, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileGetFloat(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF2=4,%4d% = get_float_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		auto value = 0.0f;
		char strValue[32];
		auto result = GetPrivateProfileString(section, key, NULL, strValue, sizeof(strValue), path);
		if (result)
		{
			value = (float)atof(strValue);
			OPCODE_WRITE_PARAM_FLOAT(value);
		}
		else
		{
			OPCODE_SKIP_PARAMS(1);
		}
		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileWriteFloat(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF3=4,write_float %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_FLOAT();
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		char strValue[32];
		sprintf(strValue, "%g", value);
		auto result = WritePrivateProfileString(section, key, strValue, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileReadString(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF4=4,%4d% = read_string_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		char strValue[MAX_STR_LEN];
		auto result = GetPrivateProfileString(section, key, NULL, strValue, sizeof(strValue), path);
		if (result)
		{
			OPCODE_WRITE_PARAM_STRING(strValue);
		}
		else
		{
			OPCODE_SKIP_PARAMS(1);
		}
		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileWriteString(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF5=4,write_string %1s% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		char strValue[MAX_STR_LEN]; OPCODE_READ_PARAM_STRING_BUFF(strValue, sizeof(strValue));
		auto path = OPCODE_READ_PARAM_FILEPATH();
		OPCODE_READ_PARAM_STRING_BUFF(section, sizeof(section));
		OPCODE_READ_PARAM_STRING_BUFF(key, sizeof(key));

		auto result = WritePrivateProfileString(section, key, strValue, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}
} iniFiles;

char IniFiles::section[128] = { 0 };
char IniFiles::key[128] = { 0 };
