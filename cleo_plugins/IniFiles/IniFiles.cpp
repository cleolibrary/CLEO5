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

	static OpcodeResult WINAPI Script_InifileGetInt(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF0=4,%4d% = get_int_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		char buff[32];
		if (GetPrivateProfileString(section, key, NULL, buff, sizeof(buff), path))
		{
			char* str;
			int base;
			if (StringStartsWith(buff, "0x", false)) // hex int
			{
				str = buff + 2;
				base = 16;
			}
			else // decimal int
			{
				str = buff;
				base = 10;
			}

			// parse
			char* end;
			int value = strtol(str, &end, base);
			if (end != str || // at least one number character consumed
				IsLegacyScript(thread)) // old CLEO reported success anyway with value 0
			{
				OPCODE_WRITE_PARAM_INT(value);
				OPCODE_CONDITION_RESULT(true);
				return OR_CONTINUE;
			}
		}

		// failed
		if (IsLegacyScript(thread))
		{
			OPCODE_WRITE_PARAM_INT(0x80000000); // CLEO4 behavior
		}
		else
		{
			OPCODE_SKIP_PARAMS(1);
		}

		OPCODE_CONDITION_RESULT(false);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileWriteInt(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF1=4,write_int %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_INT();
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		char strValue[32];
		_itoa(value, strValue, 10);
		auto result = WritePrivateProfileString(section, key, strValue, path);
		
		if (GetLastError() == ERROR_FILE_NOT_FOUND) // path points directory
		{
			result = false;
		}

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileGetFloat(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF2=4,%4d% = get_float_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		char buff[32];
		if (GetPrivateProfileString(section, key, NULL, buff, sizeof(buff), path))
		{
			char *str, *end;
			float value;
			if (StringStartsWith(buff, "0x", false)) // hex int
			{
				str = buff + 2;
				value = (float)strtol(str, &end, 16);
			}
			else // float
			{
				str = buff;
				value = strtof(str, &end);
			}

			if (end != str || // at least one number character consumed
				IsLegacyScript(thread)) // old CLEO reported success anyway with value 0
			{
				OPCODE_WRITE_PARAM_FLOAT(value);
				OPCODE_CONDITION_RESULT(true);
				return OR_CONTINUE;
			}
		}

		// failed
		OPCODE_SKIP_PARAMS(1);
		OPCODE_CONDITION_RESULT(false);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileWriteFloat(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF3=4,write_float %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_FLOAT();
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		char strValue[32];
		sprintf(strValue, "%g", value);
		auto result = WritePrivateProfileString(section, key, strValue, path);
		
		if (GetLastError() == ERROR_FILE_NOT_FOUND) // path points directory
		{
			result = false;
		}

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI Script_InifileReadString(CScriptThread* thread)
		/****************************************************************
		Opcode Format
		0AF4=4,%4d% = read_string_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

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
		OPCODE_READ_PARAM_STRING(strValue);
		OPCODE_READ_PARAM_FILEPATH(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		auto result = WritePrivateProfileString(section, key, strValue, path);

		if (GetLastError() == ERROR_FILE_NOT_FOUND) // path points directory
		{
			result = false;
		}

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}
} iniFiles;

