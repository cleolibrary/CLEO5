#include "CLEO.h"
#include "CLEO_Utils.h"
#include <CCheat.h>
#include <CControllerConfigManager.h>
#include <CMessages.h>
#include <CText.h>
#include <CTimer.h>
#include <map>

using namespace CLEO;


class Input
{
	static constexpr int Key_Code_None = -1;
	static constexpr size_t Key_Code_Max = 0xFF;
	static constexpr BYTE Key_Flag_Down = 0x80; // top bit

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
			CLEO_RegisterOpcode(0x2085, opcode_2085); // get_controller_key
			CLEO_RegisterOpcode(0x2086, opcode_2086); // get_key_name

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

	static void SendKeyEvent(BYTE vKey, bool down)
	{
		INPUT input = { 0 };
		if (vKey >= VK_LBUTTON && vKey <= VK_XBUTTON2) // mouse keys
		{
			input.type = INPUT_MOUSE;
			switch (vKey)
			{
				case VK_LBUTTON: input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
				case VK_MBUTTON: input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
				case VK_RBUTTON: input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;

				case VK_XBUTTON1:
					input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
					input.mi.mouseData = XBUTTON1;
					break;

				case VK_XBUTTON2:
					input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
					input.mi.mouseData = XBUTTON2;
					break;
			}
		}
		else // keyboard
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = vKey;
			input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

			switch(vKey)
			{
				case VK_RETURN: // would be mapped to numpad's enter
				case VK_LSHIFT: // maps to VK_SHIFT
				case VK_LCONTROL: // maps to VK_CONTROL
				case VK_LMENU: // maps to VK_MENU
					break; // do not use scan code

				default:
					input.ki.wScan = MapVirtualKey(vKey, MAPVK_VK_TO_VSC_EX);
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
			}
		}

		SendInput(1, &input, sizeof(input));
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

		if (key == Key_Code_None)
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

		bool isDown = g_instance.keyStatesCurr->at(key) & Key_Flag_Down;

		thread->SetConditionResult(isDown);
		return OR_CONTINUE;
	}

	// test_cheat
	// test_cheat {input} [string] (logical)
	static OpcodeResult __stdcall opcode_0ADC(CRunningScript* thread)
	{
		OPCODE_READ_PARAM_STRING_LEN(text, sizeof(CCheat::m_CheatString));

		auto len = strlen(_buff_text);
		if (len == 0)
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		_strrev(_buff_text); // reverse
		if (_strnicmp(_buff_text, CCheat::m_CheatString, len) != 0)
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		CCheat::m_CheatString[0] = '\0'; // consume the cheat
		OPCODE_CONDITION_RESULT(true);
		return OR_CONTINUE;
	}

	// is_key_just_pressed
	// is_key_just_pressed {keyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_2080(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == Key_Code_None)
		{
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			SHOW_ERROR("Invalid key code (%d) used in script %s\nScript suspended.", key, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		bool wasDown = g_instance.keyStatesPrev->at(key) & Key_Flag_Down;
		bool isDown = g_instance.keyStatesCurr->at(key) & Key_Flag_Down;

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
			bool isDown = g_instance.keyStatesCurr->at(key) & Key_Flag_Down;

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
			bool wasDown = g_instance.keyStatesPrev->at(key) & Key_Flag_Down;
			bool isDown = g_instance.keyStatesCurr->at(key) & Key_Flag_Down;

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

		if (key == Key_Code_None)
		{
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			SHOW_ERROR("Invalid key code (%d) used in script %s\nScript suspended.", key, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		SendKeyEvent(key, true);

		return OR_CONTINUE;
	}

	// emulate_key_release
	// emulate_key_release {keyCode} [KeyCode]
	static OpcodeResult __stdcall opcode_2084(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();

		if (key == Key_Code_None)
		{
			return OR_CONTINUE;
		}
		if (key < 0 || key > Key_Code_Max)
		{
			SHOW_ERROR("Invalid key code (%d) used in script %s\nScript suspended.", key, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		SendKeyEvent(key, false);

		return OR_CONTINUE;
	}

	// get_controller_key
	// [var keyCode: KeyCode] = get_controller_key {action} [ControllerAction] {altKeyIdx} [int] (logical)
	static OpcodeResult __stdcall opcode_2085(CRunningScript* thread)
	{
		auto actionId = OPCODE_READ_PARAM_INT();
		auto altKeyIdx = OPCODE_READ_PARAM_INT();

		if (actionId < 0 || actionId >= _countof(ControlsManager.m_actions))
		{
			SHOW_ERROR("Invalid value (%d) of 'action' argument in script %s\nScript suspended.", actionId, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}
		if (altKeyIdx < 0 || altKeyIdx >= _countof(CControllerAction::keys))
		{
			SHOW_ERROR("Invalid value (%d) of 'altKeyIdx' argument in script %s\nScript suspended.", actionId, ScriptInfoStr(thread).c_str());
			return thread->Suspend();
		}

		auto& action = ControlsManager.m_actions[actionId];

		// sort associated keys by priority
		std::map<unsigned int, DWORD> mapping;
		for (size_t i = 0; i < _countof(action.keys); i++)
		{
			auto& k = action.keys[i];

			if (k.keyCode == 0 || k.priority == 0) // key not assigned
				continue;

			if (k.keyCode == rsMOUSEWHEELUPBUTTON || k.keyCode == rsMOUSEWHEELDOWNBUTTON)
				continue; // there is no VK codes for mouse wheel rolling

			mapping[k.priority] = k.keyCode;
		}

		if (mapping.size() <= (size_t)altKeyIdx)
		{
			OPCODE_SKIP_PARAMS(1); // no key assigned
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		// get the alt key code
		auto it = std::next(mapping.begin(), altKeyIdx);
		auto keyCode = it->second;

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
		OPCODE_CONDITION_RESULT(true);
		return OR_CONTINUE;
	}

	// get_key_name
	// [var name: string] = get_key_name {keyCode} [KeyCode] (logical)
	static OpcodeResult __stdcall opcode_2086(CRunningScript* thread)
	{
		auto key = OPCODE_READ_PARAM_INT();
		
		static char buff[32];
		const char* name = nullptr;

		if ((key >= '0' && key <= '9') ||
			(key >= 'A' && key <= 'Z'))
		{
			// direct ASCII equivalent
			buff[0] = (char)key;
			buff[1] = '\0';
			name = buff;
		}
		else if (key >= VK_F1 && key <= VK_F24)
		{
			CMessages::InsertNumberInString(TheText.Get("FEC_FNC"), key - VK_F1 + 1, 0, 0, 0, 0, 0, buff);
			name = buff;
		}
		else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9)
		{
			CMessages::InsertNumberInString(TheText.Get("FEC_NMN"), key - VK_NUMPAD0, 0, 0, 0, 0, 0, buff);
			name = buff;
		}
		else
		{
			// based on CInputEvents::getEventKeyName
			switch(key)
			{
				case Key_Code_None: name = TheText.Get("FEC_UNB"); break;

				case VK_LBUTTON: name = TheText.Get("FEC_MSL"); break;
				case VK_RBUTTON: name = TheText.Get("FEC_MSR"); break; 
				//case VK_CANCEL
				case VK_MBUTTON: name = TheText.Get("FEC_MSM"); break;
				case VK_XBUTTON1: name = TheText.Get("FEC_MXO"); break;
				case VK_XBUTTON2: name = TheText.Get("FEC_MXT"); break;

				case VK_BACK: name = TheText.Get("FEC_BSP"); break;
				case VK_TAB: name = TheText.Get("FEC_TAB"); break;
				case VK_CLEAR: name = "CLEAR"; break;
				case VK_RETURN: name = TheText.Get("FEC_RTN"); break;
				case VK_SHIFT: name = TheText.Get("FEC_SFT"); break;
				case VK_CONTROL: name = "CTRL"; break;
				case VK_MENU: name = "ALT"; break;
				case VK_PAUSE: name = TheText.Get("FEC_PSB"); break; // FEC_PAS
				case VK_CAPITAL: name = TheText.Get("FEC_CLK"); break;
				//case VK_KANA
				//case VK_IME_ON
				//case VK_JUNJA
				//case VK_FINAL
				//case VK_HANJA
				//case VK_IME_OFF
				case VK_ESCAPE: name = "ESC"; break;
				//case VK_CONVERT
				//case VK_NONCONVERT
				//case VK_ACCEPT
				//case VK_MODECHANGE
				case VK_SPACE: name = TheText.Get("FEC_SPC"); break;
				case VK_PRIOR: name = TheText.Get("FEC_PGU"); break;
				case VK_NEXT: name = TheText.Get("FEC_PGD"); break;
				case VK_END: name = TheText.Get("FEC_END"); break;
				case VK_HOME: name = TheText.Get("FEC_HME"); break;
				case VK_LEFT: name = TheText.Get("FEC_LFA"); break;
				case VK_UP: name = TheText.Get("FEC_UPA"); break;
				case VK_RIGHT: name = TheText.Get("FEC_RFA"); break;
				case VK_DOWN: name = TheText.Get("FEC_DWA"); break;
				case VK_SELECT: name = "SELECT"; break;
				case VK_PRINT: name = "PRINT"; break;
				case VK_EXECUTE: name = "EXECUTE"; break;
				case VK_SNAPSHOT: name = "PRTSCR"; break;
				case VK_INSERT: name = TheText.Get("FEC_IRT"); break;
				case VK_DELETE: name = TheText.Get("FEC_DLL"); break;
				case VK_HELP: name = "HELP"; break;
				case VK_LWIN: name = TheText.Get("FEC_LWD"); break;
				case VK_RWIN: name = TheText.Get("FEC_RWD"); break;
				case VK_APPS: name = TheText.Get("FEC_WRC"); break;

				case '^': // German "double s" letter
					buff[0] = '|';
					buff[1] = '\0';
					name = buff;
				break;

				case VK_SLEEP: name = "SLEEP"; break;
				case VK_MULTIPLY: name = TheText.Get("FECSTAR"); break;
				case VK_ADD: name = TheText.Get("FEC_PLS"); break;
				//case VK_SEPARATOR ???
				case VK_SUBTRACT: name = TheText.Get("FEC_MIN"); break;
				case VK_DECIMAL: name = TheText.Get("FEC_DOT"); break;
				case VK_DIVIDE: name = TheText.Get("FEC_FWS"); break;
				//case VK_NAVIGATION_VIEW
				//case VK_NAVIGATION_MENU
				//case VK_NAVIGATION_UP
				//case VK_NAVIGATION_DOWN
				//case VK_NAVIGATION_LEFT
				//case VK_NAVIGATION_RIGHT
				//case VK_NAVIGATION_ACCEPT
				//case VK_NAVIGATION_CANCEL
				case VK_NUMLOCK: name = TheText.Get("FEC_NLK"); break;
				case VK_SCROLL: name = TheText.Get("FEC_SLK"); break;
				case VK_OEM_NEC_EQUAL: name = TheText.Get("FEC_ETR"); break;
				//case VK_OEM_FJ_JISHO
				//case VK_OEM_FJ_MASSHOU
				//case VK_OEM_FJ_TOUROKU
				//case VK_OEM_FJ_LOYA
				//case VK_OEM_FJ_ROYA
				case VK_LSHIFT: name = TheText.Get("FEC_LSF"); break;
				case VK_RSHIFT: name = TheText.Get("FEC_RSF"); break;
				case VK_LCONTROL: name = TheText.Get("FEC_LCT"); break;
				case VK_RCONTROL: name = TheText.Get("FEC_RCT"); break;
				case VK_LMENU:name = TheText.Get("FEC_LAL"); break;
				case VK_RMENU:name = TheText.Get("FEC_RAL"); break;
				//case VK_BROWSER_BACK
				//case VK_BROWSER_FORWARD
				//case VK_BROWSER_REFRESH
				//case VK_BROWSER_STOP
				//case VK_BROWSER_SEARCH
				//case VK_BROWSER_FAVORITES
				//case VK_BROWSER_HOME
				//case VK_VOLUME_MUTE
				//case VK_VOLUME_DOWN
				//case VK_VOLUME_UP
				//case VK_MEDIA_NEXT_TRACK
				//case VK_MEDIA_PREV_TRACK
				//case VK_MEDIA_STOP: in GTA some French letter?
				//case VK_MEDIA_PLAY_PAUSE
				//case VK_LAUNCH_MAIL
				//case VK_LAUNCH_MEDIA_SELECT
				//case VK_LAUNCH_APP1
				//case VK_LAUNCH_APP2
				case VK_OEM_1: name = ","; break;
				case VK_OEM_PLUS: name = "="; break;
				case VK_OEM_COMMA: name = ","; break;
				case VK_OEM_MINUS: name = "-"; break;
				case VK_OEM_PERIOD: name = "."; break;
				case VK_OEM_2: name = "/"; break;
				case VK_OEM_3: name = "`"; break;
				//case VK_GAMEPAD_A
				//case VK_GAMEPAD_B
				//case VK_GAMEPAD_X
				//case VK_GAMEPAD_Y
				//case VK_GAMEPAD_RIGHT_SHOULDER
				//case VK_GAMEPAD_LEFT_SHOULDER
				//case VK_GAMEPAD_LEFT_TRIGGER
				//case VK_GAMEPAD_RIGHT_TRIGGER
				//case VK_GAMEPAD_DPAD_UP
				//case VK_GAMEPAD_DPAD_DOWN
				//case VK_GAMEPAD_DPAD_LEFT
				//case VK_GAMEPAD_DPAD_RIGHT
				//case VK_GAMEPAD_MENU
				//case VK_GAMEPAD_VIEW
				//case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON
				//case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON
				//case VK_GAMEPAD_LEFT_THUMBSTICK_UP
				//case VK_GAMEPAD_LEFT_THUMBSTICK_DOWN
				//case VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT
				//case VK_GAMEPAD_LEFT_THUMBSTICK_LEFT
				//case VK_GAMEPAD_RIGHT_THUMBSTICK_UP
				//case VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN
				//case VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT
				//case VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT
				case VK_OEM_4: name = "["; break;
				case VK_OEM_5: name = "\\"; break;
				case VK_OEM_6: name = "]"; break;
				case VK_OEM_7: name = "'"; break;
				//case VK_OEM_8
				//case VK_OEM_AX
				//case VK_OEM_102
				//case VK_ICO_HELP
				//case VK_ICO_00
				//case VK_ICO_CLEAR
				//case VK_PACKET
				//case VK_OEM_RESET
				//case VK_OEM_JUMP
				//case VK_OEM_PA1
				//case VK_OEM_PA2
				//case VK_OEM_PA3
				//case VK_OEM_WSCTRL
				//case VK_OEM_CUSEL
				//case VK_OEM_ATTN
				//case VK_OEM_FINISH
				//case VK_OEM_COPY
				//case VK_OEM_AUTO
				//case VK_OEM_ENLW
				//case VK_OEM_BACKTAB
				//case VK_OEM_BACKTAB
				//case VK_ATTN
				//case VK_CRSEL
				//case VK_EXSEL
				//case VK_EREOF
				//case VK_PLAY
				//case VK_ZOOM
				//case VK_NONAME
				//case VK_PA1
				//case VK_OEM_CLEAR
			}
		}

		if (name == nullptr)
		{
			OPCODE_SKIP_PARAMS(1);
			OPCODE_CONDITION_RESULT(false);
			return OR_CONTINUE;
		}

		OPCODE_WRITE_PARAM_STRING(name);
		OPCODE_CONDITION_RESULT(true);
		return OR_CONTINUE;
	}

} g_instance;

