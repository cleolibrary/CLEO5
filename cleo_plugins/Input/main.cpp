#include "CLEO.h"
#include "CLEO_Utils.h"
#include <CCheat.h>
#include <CControllerConfigManager.h>
#include <CTimer.h>

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
			CLEO_RegisterOpcode(0x2080, opcode_2080); // is_key_just_pressed
			CLEO_RegisterOpcode(0x2081, opcode_2081); // get_key_pressed_in_range
			CLEO_RegisterOpcode(0x2082, opcode_2082); // get_key_just_pressed_in_range
			CLEO_RegisterOpcode(0x2083, opcode_2083); // emulate_key_press
			CLEO_RegisterOpcode(0x2084, opcode_2084); // emulate_key_release
			CLEO_RegisterOpcode(0x2085, opcode_2085); // get_button_assigned_key

			// register event callbacks
			CLEO_RegisterCallback(eCallbackId::GameProcessBefore, OnGameProcessBefore);
			CLEO_RegisterCallback(eCallbackId::DrawingFinished, OnDrawingFinished);
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
		CLEO_UnregisterCallback(eCallbackId::DrawingFinished, OnDrawingFinished);
	}

	// refresh keys info
	void CheckKeyboard()
	{
		std::swap(keyStatesCurr, keyStatesPrev);
		GetKeyboardState(keyStatesCurr->data());
	}

	static void __stdcall OnGameProcessBefore()
	{
		g_instance.CheckKeyboard();
	}

	static void __stdcall OnDrawingFinished()
	{
		if (CTimer::m_UserPause) // main menu visible
		{
			g_instance.CheckKeyboard(); // update for ambient scripts running in menu
		}
	}

	// is_key_pressed
	// is_key_pressed {keyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_0AB0(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == -1) // KeyCode::None
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			LOG_WARNING(thread, "Invalid key code (%d) used in script %s", key, ScriptInfoStr(thread).c_str()); // legacy opcode, just warning
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
	static OpcodeResult __stdcall opcode_2080(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == -1) // KeyCode::None
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
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
	static OpcodeResult __stdcall opcode_2081(CRunningScript* thread)
	{
		auto keyMin = OPCODE_READ_PARAM_INT();
		auto keyMax = OPCODE_READ_PARAM_INT();

		if (keyMin < 0 || keyMin > Key_Code_Max)
		{
			SHOW_ERROR("Invalid value (%d) of 'minKeyCode' argument in script %s\nScript suspended.", keyMin, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}
		if (keyMax < 0 || keyMax > Key_Code_Max || keyMax < keyMin)
		{
			SHOW_ERROR("Invalid value (%d) of 'maxKeyCode' argument in script %s\nScript suspended.", keyMin, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
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
	static OpcodeResult __stdcall opcode_2082(CRunningScript* thread)
	{
		auto keyMin = OPCODE_READ_PARAM_INT();
		auto keyMax = OPCODE_READ_PARAM_INT();

		if (keyMin < 0 || keyMin > Key_Code_Max)
		{
			SHOW_ERROR("Invalid value (%d) of 'minKeyCode' argument in script %s\nScript suspended.", keyMin, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}
		if (keyMax < 0 || keyMax > Key_Code_Max || keyMax < keyMin)
		{
			SHOW_ERROR("Invalid value (%d) of 'maxKeyCode' argument in script %s\nScript suspended.", keyMin, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
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

	// emulate_key_press
	// emulate_key_press {keyCode} [KeyCode]
	static OpcodeResult __stdcall opcode_2083(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == -1) // KeyCode::None
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			SHOW_ERROR("Invalid key code (%d) used in script %s", key, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		INPUT input = { 0 };
		if (key >= VK_LBUTTON && key <= VK_XBUTTON2) // mouse keys
		{
			input.type = INPUT_MOUSE;
			switch(key)
			{
				case VK_LBUTTON: input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;
				case VK_MBUTTON: input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
				case VK_RBUTTON: input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break;

				case VK_XBUTTON1:
					input.mi.dwFlags = MOUSEEVENTF_XDOWN;
					input.mi.mouseData = XBUTTON1;
					break;

				case VK_XBUTTON2:
					input.mi.dwFlags = MOUSEEVENTF_XDOWN;
					input.mi.mouseData = XBUTTON2;
					break;
			}
		}
		else // keyboard
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;

			if (key != VK_RETURN) // would be maped to numpad enter
			{
				input.ki.wScan = MapVirtualKey(key, 0);
				input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
			}
		}
		SendInput(1, &input, sizeof(input));

		return OR_CONTINUE;
	}

	// emulate_key_release
	// emulate_key_release {keyCode} [KeyCode]
	static OpcodeResult __stdcall opcode_2084(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == -1) // KeyCode::None
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			SHOW_ERROR("Invalid key code (%d) used in script %s", key, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		INPUT input = { 0 };
		if (key >= VK_LBUTTON && key <= VK_XBUTTON2) // mouse keys
		{
			input.type = INPUT_MOUSE;
			switch (key)
			{
				case VK_LBUTTON: input.mi.dwFlags = MOUSEEVENTF_LEFTUP; break;
				case VK_MBUTTON: input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
				case VK_RBUTTON: input.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break;

				case VK_XBUTTON1:
					input.mi.dwFlags = MOUSEEVENTF_XUP;
					input.mi.mouseData = XBUTTON1;
					break;

				case VK_XBUTTON2:
					input.mi.dwFlags = MOUSEEVENTF_XUP;
					input.mi.mouseData = XBUTTON2;
					break;
			}
		}
		else // keyboard
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = KEYEVENTF_KEYUP;

			if (key != VK_RETURN) // would be maped to numpad enter
			{
				input.ki.wScan = MapVirtualKey(key, 0);
				input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
			}
		}
		SendInput(1, &input, sizeof(input));

		return OR_CONTINUE;
	}

	// get_button_assigned_key
	// [var keyCode: KeyCode] = get_controller_assigned_key {action} [ControllerAction]
	static OpcodeResult __stdcall opcode_2085(CRunningScript* thread)
	{
		auto actionId = OPCODE_READ_PARAM_INT();

		if (actionId < 0 || actionId >= _countof(ControlsManager.m_actions))
		{
			SHOW_ERROR("Invalid value (%d) of 'action' argument in script %s\nScript suspended.", actionId, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		auto& action = ControlsManager.m_actions[actionId];

		// find top priority association in key mapping
		int keyCode = -1;
		uint priority = 0;
		for (size_t i = 0; i < _countof(action.keys); i++)
		{
			auto& k = action.keys[i];
			if (k.keyCode != rsMOUSEWHEELUPBUTTON && // no VK code for mouse up
				k.keyCode != rsMOUSEWHEELDOWNBUTTON && // no VK code for mouse down
				k.priority > priority)
			{
				keyCode = action.keys[i].keyCode;
				priority = action.keys[i].priority;
			}
		}

		// translate RsKeyCode to VirtualKey
		switch(keyCode)
		{
			case rsMOUSELEFTBUTTON: keyCode = VK_LBUTTON; break;
			case rsMOUSEMIDDLEBUTTON: keyCode = VK_MBUTTON; break;
			case rsMOUSERIGHTBUTTON: keyCode = VK_RBUTTON; break;
			//case rsMOUSEWHEELUPBUTTON:
			//case rsMOUSEWHEELDOWNBUTTON:
			case rsMOUSEX1BUTTON: keyCode = VK_XBUTTON1; break;
			case rsMOUSEX2BUTTON: keyCode = VK_XBUTTON2; break;

			case rsESC: keyCode = VK_ESCAPE; break;

			case rsF1: keyCode = VK_F1; break;
			case rsF2: keyCode = VK_F2; break;
			case rsF3: keyCode = VK_F3; break;
			case rsF4: keyCode = VK_F4; break;
			case rsF5: keyCode = VK_F5; break;
			case rsF6: keyCode = VK_F6; break;
			case rsF7: keyCode = VK_F7; break;
			case rsF8: keyCode = VK_F8; break;
			case rsF9: keyCode = VK_F9; break;
			case rsF10: keyCode = VK_F10; break;
			case rsF11: keyCode = VK_F11; break;
			case rsF12: keyCode = VK_F12; break;

			case rsINS: keyCode = VK_INSERT; break;
			case rsDEL: keyCode = VK_DELETE; break;
			case rsHOME: keyCode = VK_HOME; break;
			case rsEND: keyCode = VK_END; break;
			case rsPGUP: keyCode = VK_PRIOR; break;
			case rsPGDN: keyCode = VK_NEXT; break;

			case rsUP: keyCode = VK_UP; break;
			case rsDOWN: keyCode = VK_DOWN; break;
			case rsLEFT: keyCode = VK_LEFT; break;
			case rsRIGHT: keyCode = VK_RIGHT; break;

			case rsDIVIDE: keyCode = VK_DIVIDE; break;
			case rsTIMES: keyCode = VK_MULTIPLY; break;
			case rsPLUS: keyCode = VK_ADD; break;
			case rsMINUS: keyCode = VK_SUBTRACT; break;
			case rsPADDEL: keyCode = VK_DECIMAL; break;
			case rsPADEND: keyCode = VK_NUMPAD1; break;
			case rsPADDOWN: keyCode = VK_NUMPAD2; break;
			case rsPADPGDN: keyCode = VK_NUMPAD3; break;
			case rsPADLEFT: keyCode = VK_NUMPAD4; break;
			case rsPAD5: keyCode = VK_NUMPAD5; break;
			case rsNUMLOCK: keyCode = VK_NUMLOCK; break;
			case rsPADRIGHT: keyCode = VK_NUMPAD6; break;
			case rsPADHOME: keyCode = VK_NUMPAD7; break;
			case rsPADUP: keyCode = VK_NUMPAD8; break;
			case rsPADPGUP: keyCode = VK_NUMPAD9; break;
			case rsPADINS: keyCode = VK_NUMPAD0; break;
			case rsPADENTER: keyCode = VK_RETURN; break; // not quite same

			case rsSCROLL: keyCode = VK_SCROLL; break;
			case rsPAUSE: keyCode = VK_PAUSE; break;

			case rsBACKSP: keyCode = VK_BACK; break;
			case rsTAB: keyCode = VK_TAB; break;
			case rsCAPSLK: keyCode = VK_CAPITAL; break;
			case rsENTER: keyCode = VK_RETURN; break;
			case rsLSHIFT: keyCode = VK_LSHIFT; break;
			case rsRSHIFT: keyCode = VK_RSHIFT; break;
			case rsSHIFT: keyCode = VK_SHIFT; break;
			case rsLCTRL: keyCode = VK_LCONTROL; break;
			case rsRCTRL: keyCode = VK_RCONTROL; break;
			case rsLALT: keyCode = VK_LMENU; break;
			case rsRALT: keyCode = VK_RMENU; break;
			case rsLWIN: keyCode = VK_LWIN; break;
			case rsRWIN: keyCode = VK_RWIN; break;
			case rsAPPS: keyCode = VK_APPS; break;
		}
		
		OPCODE_WRITE_PARAM_INT(keyCode);
		return OR_CONTINUE;
	}

} g_instance;

