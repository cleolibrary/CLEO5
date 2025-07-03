#include "CLEO.h"
#include "CLEO_Utils.h"
#include <CCheat.h>

using namespace CLEO;


class Input
{
	static constexpr size_t Key_Code_Max = 0xFF;
	static constexpr BYTE Key_Down_Flag = 0x80; // top bit

public:
	std::array<BYTE, Key_Code_Max + 1>* keyStatesCurr, *keyStatesPrev;

	Input()
	{
		keyStatesCurr = new std::array<BYTE, Key_Code_Max + 1>();
		keyStatesPrev = new std::array<BYTE, Key_Code_Max + 1>();

		auto cleoVer = CLEO_GetVersion();
		if (cleoVer >= CLEO_VERSION)
		{
			// register opcodes
			CLEO_RegisterOpcode(0x0AB0, opcode_0AB0); // is_key_pressed
			CLEO_RegisterOpcode(0x0ADC, opcode_0ADC); // test_cheat
			CLEO_RegisterOpcode(0x2880, opcode_2880); // is_key_just_pressed
			CLEO_RegisterOpcode(0x2881, opcode_2881); // get_key_pressed_in_range
			CLEO_RegisterOpcode(0x2882, opcode_2882); // get_key_just_pressed_in_range

			// register event callbacks
			CLEO_RegisterCallback(eCallbackId::GameProcessBefore, OnGameProcessBefore);
		}
		else
		{
			auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
			MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
		}
	}

	~Input()
	{
		delete keyStatesCurr;
		delete keyStatesPrev;

		CLEO_UnregisterCallback(eCallbackId::GameProcessBefore, OnGameProcessBefore);
	}

	static void __stdcall OnGameProcessBefore()
	{
		// refresh keys info
		std::swap(g_instance.keyStatesCurr, g_instance.keyStatesPrev);
		GetKeyboardState(g_instance.keyStatesCurr->data());
	}

	// is_key_pressed
	// is_key_pressed {keyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_0AB0(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key < 0 || key > Key_Code_Max)
		{
			LOG_WARNING(thread, "Invalid key code (%d) used in script %s", key, ScriptInfoStr(thread).c_str());
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		bool isDown = g_instance.keyStatesCurr->at(key) & Key_Down_Flag;

		thread->SetConditionResult(isDown);
		return OR_CONTINUE;
	}

	// test_cheat
	// test_cheat {input} [string] (logical)
	static OpcodeResult __stdcall opcode_0ADC(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(text, sizeof(CCheat::m_CheatString));

		_strrev(_buff_text); // reverse
		auto len = strlen(_buff_text);
		if (_strnicmp(_buff_text, CCheat::m_CheatString, len) == 0)
		{
			CCheat::m_CheatString[0] = '\0'; // consume the cheat
			OPCODE_CONDITION_RESULT(true);
			return OR_CONTINUE;
		}

		OPCODE_CONDITION_RESULT(false);
		return OR_CONTINUE;
	}

	// is_key_just_pressed
	// is_key_just_pressed {keyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_2880(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key < 0 || key > Key_Code_Max)
		{
			LOG_WARNING(thread, "Invalid key code (%d) used in script %s", key, ScriptInfoStr(thread).c_str());
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		bool wasDown = g_instance.keyStatesPrev->at(key) & Key_Down_Flag;
		bool isDown = g_instance.keyStatesCurr->at(key) & Key_Down_Flag;

		OPCODE_CONDITION_RESULT(!wasDown && isDown);
		return OR_CONTINUE;
	}

	// get_key_pressed_in_range
	// [var keyCode: KeyCode] = get_key_pressed_in_range {minKeyCode} [KeyCode] {maxKeyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_2881(CRunningScript* thread)
	{
		auto keyMin = OPCODE_READ_PARAM_INT();
		auto keyMax = OPCODE_READ_PARAM_INT();

		if (keyMin < 0 || keyMin > Key_Code_Max)
		{
			LOG_WARNING(thread, "Invalid value (%d) of 'minKeyCode' argument in script %s", keyMin, ScriptInfoStr(thread).c_str());
			OPCODE_SKIP_PARAMS(1); // no result
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (keyMax < 0 || keyMax > Key_Code_Max || keyMax < keyMin)
		{
			LOG_WARNING(thread, "Invalid value (%d) of 'maxKeyCode' argument in script %s", keyMin, ScriptInfoStr(thread).c_str());
			OPCODE_SKIP_PARAMS(1); // no result
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		for (auto key = keyMin; key <= keyMax; key++)
		{
			bool isDown = g_instance.keyStatesCurr->at(key) & Key_Down_Flag;

			if (isDown)
			{
				OPCODE_WRITE_PARAM_INT(key);
				OPCODE_CONDITION_RESULT(true);
				return OR_CONTINUE;
			}
		}

		OPCODE_SKIP_PARAMS(1); // no result
		OPCODE_CONDITION_RESULT(false);
		return OR_CONTINUE;
	}

	// get_key_just_pressed_in_range
	// [var keyCode: KeyCode] = get_key_just_pressed_in_range {minKeyCode} [KeyCode] {maxKeyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_2882(CRunningScript* thread)
	{
		auto keyMin = OPCODE_READ_PARAM_INT();
		auto keyMax = OPCODE_READ_PARAM_INT();

		if (keyMin < 0 || keyMin > Key_Code_Max)
		{
			LOG_WARNING(thread, "Invalid value (%d) of 'minKeyCode' argument in script %s", keyMin, ScriptInfoStr(thread).c_str());
			OPCODE_SKIP_PARAMS(1); // no result
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (keyMax < 0 || keyMax > Key_Code_Max || keyMax < keyMin)
		{
			LOG_WARNING(thread, "Invalid value (%d) of 'maxKeyCode' argument in script %s", keyMin, ScriptInfoStr(thread).c_str());
			OPCODE_SKIP_PARAMS(1); // no result
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		for (auto key = keyMin; key <= keyMax; key++)
		{
			bool wasDown = g_instance.keyStatesPrev->at(key) & Key_Down_Flag;
			bool isDown = g_instance.keyStatesCurr->at(key) & Key_Down_Flag;

			if (!wasDown && isDown)
			{
				OPCODE_WRITE_PARAM_INT(key);
				OPCODE_CONDITION_RESULT(true);
				return OR_CONTINUE;
			}
		}

		OPCODE_SKIP_PARAMS(1); // no result
		OPCODE_CONDITION_RESULT(false);
		return OR_CONTINUE;
	}

} g_instance;

