#include <cstdio>
#include "CLEO.h"
#include "CLEO_Utils.h"
#include <string>

using namespace CLEO;

// In case of naked file names without parent path INI file APIs searchs in Windows directory. Add leading ".\" to prevent that.
static void fixIniFilepath(char* buff)
{
	if (!std::filesystem::path(buff).has_parent_path())
	{
		std::string filename = buff;
		strcpy_s(buff, 512, ".\\");
		strcat_s(buff, 512, filename.c_str());
	}
}

#define OPCODE_READ_PARAM_FILEPATH_INI(_varName) OPCODE_READ_PARAM_FILEPATH(_varName); fixIniFilepath(_buff_##_varName)

#define OPCODE_READ_PARAM_STRING_OR_ZERO(_varName) const char* ##_varName; \
if (IsLegacyScript(thread) && IsImmInteger(thread->PeekDataType()) && CLEO_PeekIntOpcodeParam(thread) == 0) \
{ \
	CLEO_SkipOpcodeParams(thread, 1); \
	##_varName = nullptr; \
} \
else \
{ \
	char _buff_##_varName[MAX_STR_LEN + 1]; \
	##_varName = _readParamText(thread, _buff_##_varName, MAX_STR_LEN + 1); \
	if (!_paramWasString()) { return OpcodeResult::OR_INTERRUPT; } \
};

class IniFiles
{
public:
	IniFiles()
	{
		if (!PluginCheckCleoVersion()) return;

		// register opcodes
		CLEO_RegisterOpcode(0x0AF0, Script_InifileGetInt);
		CLEO_RegisterOpcode(0x0AF1, Script_InifileWriteInt);
		CLEO_RegisterOpcode(0x0AF2, Script_InifileGetFloat);
		CLEO_RegisterOpcode(0x0AF3, Script_InifileWriteFloat);
		CLEO_RegisterOpcode(0x0AF4, Script_InifileReadString);
		CLEO_RegisterOpcode(0x0AF5, Script_InifileWriteString);
		CLEO_RegisterOpcode(0x2800, Script_InifileDeleteSection);
		CLEO_RegisterOpcode(0x2801, Script_InifileDeleteKey);
	}

	static OpcodeResult __stdcall Script_InifileGetInt(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF0=4,%4d% = get_int_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH_INI(path);
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

	static OpcodeResult __stdcall Script_InifileWriteInt(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF1=4,write_int %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_INT();
		OPCODE_READ_PARAM_FILEPATH_INI(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING_OR_ZERO(key);	// 0 deletes the whole section

		char strValue[32];
		_itoa_s(value, strValue, 10);
		auto result = WritePrivateProfileString(section, key, strValue, path);
		
		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult __stdcall Script_InifileGetFloat(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF2=4,%4d% = get_float_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH_INI(path);
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

	static OpcodeResult __stdcall Script_InifileWriteFloat(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF3=4,write_float %1d% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		auto value = OPCODE_READ_PARAM_FLOAT();
		OPCODE_READ_PARAM_FILEPATH_INI(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING_OR_ZERO(key); // 0 deletes the whole section

		char strValue[32];
		sprintf_s(strValue, "%g", value);
		auto result = WritePrivateProfileString(section, key, strValue, path);
		
		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult __stdcall Script_InifileReadString(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF4=4,%4d% = read_string_from_ini_file %1s% section %2s% key %3s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH_INI(path);
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

	static OpcodeResult __stdcall Script_InifileWriteString(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		0AF5=4,write_string %1s% to_ini_file %2s% section %3s% key %4s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_STRING_OR_ZERO(strValue); // 0 deletes the key
		OPCODE_READ_PARAM_FILEPATH_INI(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING_OR_ZERO(key);		// 0 deletes the whole section

		auto result = WritePrivateProfileString(section, key, strValue, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult __stdcall Script_InifileDeleteSection(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		2800=2,delete_section_from_ini_file %1s% section %2s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH_INI(path);
		OPCODE_READ_PARAM_STRING(section);

		auto result = WritePrivateProfileString(section, nullptr, nullptr, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}

	static OpcodeResult __stdcall Script_InifileDeleteKey(CRunningScript* thread)
		/****************************************************************
		Opcode Format
		2801=3,delete_key_from_ini_file %1s% section %2s%
		****************************************************************/
	{
		OPCODE_READ_PARAM_FILEPATH_INI(path);
		OPCODE_READ_PARAM_STRING(section);
		OPCODE_READ_PARAM_STRING(key);

		auto result = WritePrivateProfileString(section, key, nullptr, path);

		OPCODE_CONDITION_RESULT(result);
		return OR_CONTINUE;
	}
} iniFiles;

