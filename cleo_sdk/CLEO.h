/*
	CLEO 5 header file
	Copyright (c) 2023 Alien, Deji, Junior_Djjr, Miran, Seemann
*/
#pragma once

#ifdef __cplusplus
#include <string>
#endif
#include <wtypes.h>

#define CLEO_VERSION_MAIN	5
#define CLEO_VERSION_MAJOR	0
#define CLEO_VERSION_MINOR	0

#define CLEO_VERSION ((CLEO_VERSION_MAIN << 24)|(CLEO_VERSION_MAJOR << 16)|(CLEO_VERSION_MINOR << 8)) // 0x0v0v0v00

#define __TO_STR(x) #x
#define TO_STR(x) __TO_STR(x)
#define CLEO_VERSION_STR TO_STR(CLEO_VERSION_MAIN.CLEO_VERSION_MAJOR.CLEO_VERSION_MINOR) // "v.v.v"

namespace CLEO
{
// result of CLEO_GetScriptVersion
enum eCLEO_Version : DWORD
{
	CLEO_VER_3 = 0x03000000,
	CLEO_VER_4_MIN = 0x04000000,
	CLEO_VER_4_2 = 0x04020000,
	CLEO_VER_4_3 = 0x04030000,
	CLEO_VER_4_4 = 0x04040000,
	CLEO_VER_4 = CLEO_VER_4_4,
	CLEO_VER_5 = 0x05000000,
	CLEO_VER_CUR = CLEO_VERSION
};

// result of CLEO_GetGameVersion
enum eGameVersion : int
{
	GV_US10 = 0, // 1.0 us
	GV_US11 = 1, // 1.01 us - not supported
	GV_EU10 = 2, // 1.0 eu
	GV_EU11 = 3, // 1.01 eu
	GV_STEAM,
	GV_TOTAL,
	GV_UNK = -1 // any other
};

// operand types
enum eDataType : BYTE
{
	DT_END, // variable args end marker
	DT_DWORD, // literal int 32
	DT_VAR, // globalVar $
	DT_LVAR, // localVar @
	DT_BYTE, // literal int 8
	DT_WORD, // literal int 16
	DT_FLOAT, // literal float 32
	DT_VAR_ARRAY, // globalArr $(,)
	DT_LVAR_ARRAY, // localArr @(,)
	DT_TEXTLABEL, // literal string up to 7 chars
	DT_VAR_TEXTLABEL, // globalVarSString s$
	DT_LVAR_TEXTLABEL, // localVarSString @s
	DT_VAR_TEXTLABEL_ARRAY, // globalVarSStringArr s$(,)
	DT_LVAR_TEXTLABEL_ARRAY, // localVarSStringArr @s(,)
	DT_VARLEN_STRING, // literal vstring ""
	DT_STRING, // literal string up to 15 chars
	DT_VAR_STRING, // globalVarVString v$
	DT_LVAR_STRING, // localVarVString @v
	DT_VAR_STRING_ARRAY, // globalVarStringArr v$(,)
	DT_LVAR_STRING_ARRAY, // localVarStringArr @v(,)
	DT_INVALID = 0xFF // CLEO internal
};

enum eArrayDataType : BYTE
{
	ADT_INT, // variable with integer
	ADT_FLOAT, // variable with integer
	ADT_TEXTLABEL, // variable with short string (8 char)
	ADT_STRING, // variable with long string (16 char)
	ADT_NONE = 0xFF // CLEO internal
};
static const BYTE ArrayDataTypeMask = ADT_INT | ADT_FLOAT | ADT_TEXTLABEL | ADT_STRING; // array flags byte contains other info too. Type needs to be masked when read

static const char* ToStr(eDataType type)
{
	switch (type)
	{
	case DT_END: return "VArgEnd"; break;
	case DT_DWORD: return "Int32"; break;
	case DT_VAR: return "GlobVar"; break;
	case DT_LVAR: return "LocVar"; break;
	case DT_BYTE: return "Int8"; break;
	case DT_WORD: return "Int16"; break;
	case DT_FLOAT: return "Float32"; break;
	case DT_VAR_ARRAY: return "GlobVarArr"; break;
	case DT_LVAR_ARRAY: return "LocVarArr"; break;
	case DT_TEXTLABEL: return "STxt"; break;
	case DT_VAR_TEXTLABEL: return "GlobVarSTxt"; break;
	case DT_LVAR_TEXTLABEL: return "LocVarSTxt"; break;
	case DT_VAR_TEXTLABEL_ARRAY: return "GlobVarSTxtArr"; break;
	case DT_LVAR_TEXTLABEL_ARRAY: return "LocVarSTxtArr"; break;
	case DT_VARLEN_STRING: return "Txt"; break;
	case DT_STRING: return "LTxt"; break;
	case DT_VAR_STRING: return "GlobVarLTxt"; break;
	case DT_LVAR_STRING: return "LocVarLTxt"; break;
	case DT_VAR_STRING_ARRAY: return "GlobVarLTxtArr"; break;
	case DT_LVAR_STRING_ARRAY: return "LocVarLTxtArr"; break;
	default: return "corrupted";
	}
}
static bool IsImmInteger(eDataType type) // immediate/literal integer in code like 42
{
	switch (type)
	{
		case DT_BYTE:
		case DT_WORD:
		case DT_DWORD:
			return true;
	}
	return false;
}
static bool IsImmFloat(eDataType type) // immediate/literal float in code like 42.0
{
	return type == DT_FLOAT;
}
static bool IsImmString(eDataType type) // immediate/literal string in code like "text"
{
	switch (type)
	{
		case DT_STRING:
		case DT_TEXTLABEL:
		case DT_VARLEN_STRING:
			return true;
	}
	return false;
}
static bool IsVarString(eDataType type) // string variable
{
	switch (type)
	{
		case DT_LVAR_TEXTLABEL:
		case DT_LVAR_TEXTLABEL_ARRAY:
		case DT_LVAR_STRING:
		case DT_LVAR_STRING_ARRAY:
		case DT_VAR_TEXTLABEL:
		case DT_VAR_TEXTLABEL_ARRAY:
		case DT_VAR_STRING:
		case DT_VAR_STRING_ARRAY:
			return true;
	}
	return false;
}
static bool IsVariable(eDataType type) // can carry int, float, pointer to text
{
	switch (type)
	{
		case DT_VAR:
		case DT_VAR_ARRAY:
		case DT_LVAR:
		case DT_LVAR_ARRAY:
			return true;
	}
	return false;
}
static bool IsArray(eDataType type)
{
	switch (type)
	{
		case DT_LVAR_TEXTLABEL_ARRAY:
		case DT_LVAR_STRING_ARRAY:
		case DT_VAR_TEXTLABEL_ARRAY:
		case DT_VAR_STRING_ARRAY:
		case DT_VAR_ARRAY:
		case DT_LVAR_ARRAY:
			return true;
	}
	return false;
}
static const char* ToKindStr(eDataType type, eArrayDataType arrType = ADT_NONE)
{
	switch (type)
	{
		case DT_BYTE:
		case DT_WORD:
		case DT_DWORD:
			return "int"; break;

		case DT_FLOAT:
			return "float"; break;

		case DT_STRING:
		case DT_TEXTLABEL:
		case DT_LVAR_TEXTLABEL:
		case DT_LVAR_TEXTLABEL_ARRAY:
		case DT_LVAR_STRING:
		case DT_LVAR_STRING_ARRAY:
		case DT_VAR_TEXTLABEL:
		case DT_VAR_TEXTLABEL_ARRAY:
		case DT_VAR_STRING:
		case DT_VAR_STRING_ARRAY:
		case DT_VARLEN_STRING:
			return "string"; break;

		case DT_VAR:
		case DT_LVAR:
			return "variable"; break;

		case DT_VAR_ARRAY:
		case DT_LVAR_ARRAY:
			switch(arrType)
			{
				case ADT_INT:
					return "int"; break;

				case ADT_FLOAT:
					return "float"; break;

				case ADT_TEXTLABEL:
				case ADT_STRING:
					return "string"; break;

				default: return "variable";
			}

		case DT_END:
			return "varArgEnd"; break;

		default:
			return "corrupted"; break;
	}
}

const size_t MAX_STR_LEN = 0xff; // max length of string type parameter

union SCRIPT_VAR
{
	DWORD dwParam;
	short wParam;
	WORD usParam;
	BYTE ucParam;
	char cParam;
	bool bParam;
	int nParam;
	float fParam;
	void* pParam;
	char* pcParam;
};

enum eLogicalOperation : WORD
{
	NONE = 0, // just replace

	AND_2 = 1, // AND operation on results of next two conditional opcodes
	AND_3,
	AND_4,
	AND_5,
	AND_6,
	AND_7,
	AND_END,
	
	OR_2 = 21, // OR operation on results of next two conditional opcodes
	OR_3,
	OR_4,
	OR_5,
	OR_6,
	OR_7,
	OR_END,
};
static eLogicalOperation& operator--(eLogicalOperation& o)
{
	if (o == eLogicalOperation::NONE) return o; // can not be decremented anymore
	if (o == eLogicalOperation::OR_2) return o = eLogicalOperation::NONE;
	
	auto val = static_cast<WORD>(o); // to number
	val--;
	return o = static_cast<eLogicalOperation>(val);
}

// CLEO virtual path prefixes. Expandable with CLEO_ResolvePath
const char DIR_GAME[] = "root:"; // game root directory
const char DIR_USER[] = "userfiles:"; // game save directory
const char DIR_SCRIPT[] = "."; // current script directory
const char DIR_CLEO[] = "cleo:"; // game\cleo directory
const char DIR_MODULES[] = "modules:"; // game\cleo\modules directory

// argument of CLEO_RegisterCallback
enum class eCallbackId : DWORD
{
	GameBegin, // void WINAPI OnGameBegin(DWORD saveSlot); // -1 if not started from save
	GameProcess, // void WINAPI OnGameProcess(); // called once every frame during gameplay
	GameEnd, // void WINAPI OnGameEnd();
	ScriptsLoaded, // void WINAPI OnScriptsLoaded();
	ScriptsFinalize, // void WINAPI OnScriptsFinalize();
	ScriptRegister, // void WINAPI OnScriptRegister(CRunningScript* pScript); // called after script creation
	ScriptUnregister, // void WINAPI OnScriptUnregister(CRunningScript* pScript); // called before script deletion
	ScriptProcess, // bool WINAPI OnScriptProcess(CRunningScript* pScript); // return false to skip this script processing
	ScriptOpcodeProcess, // OpcodeResult WINAPI OnScriptOpcodeProcess(CRunningScript* pScript, DWORD opcode); // return other than OR_NONE to signal that opcode was handled in the callback
	ScriptOpcodeProcessFinished, // OpcodeResult WINAPI OnScriptOpcodeProcessFinished(CRunningScript* pScript, DWORD opcode, OpcodeResult result); // return other than OR_NONE to overwrite original result
	ScriptDraw, // void WINAPI OnScriptDraw(bool beforeFade);
	DrawingFinished, // void WINAPI OnDrawingFinished(); // called after game rendered everything and before presenting screen buffer
	Log, // void OnLog(eLogLevel level, const char* msg);
	MainWindowFocus, // void WINAPI OnMainWindowFocus(bool active); // called when game main window focus changes
};

// used by CLEO_Log and Log callback
enum class eLogLevel : DWORD
{
	None,
	Error, // errors and warnings
	Debug, // debug mode / user traces
	Default // all log messages
};

enum OpcodeResult : char
{
	OR_NONE = -2,
	OR_ERROR = -1,
	OR_CONTINUE = 0,
	OR_INTERRUPT = 1,
};

typedef int SCRIPT_HANDLE;
typedef SCRIPT_HANDLE HANDLE_ACTOR, ACTOR, HACTOR, PED, HPED, HANDLE_PED;
typedef SCRIPT_HANDLE HANDLE_CAR, CAR, HCAR, VEHICLE, HVEHICLE, HANDLE_VEHICLE;
typedef SCRIPT_HANDLE HANDLE_OBJECT, OBJECT, HOBJECT;
 
#pragma pack(push,1)
#ifdef __cplusplus
class CRunningScript
{
public:
#else
struct CRunningScript
{
#endif
 	CRunningScript* Next;		// 0x00 next script in queue
 	CRunningScript* Previous;	// 0x04 previous script in queue
 	char Name[8];				// 0x08 name of script, given by 03A4 opcode
 	void* BaseIP;				// 0x10 pointer to begin of script in memory
 	BYTE* CurrentIP;			// 0x14 current instruction pointer
 	BYTE* Stack[8];				// 0x18 return stack for 0050, 0051
 	WORD SP;					// 0x38 current item in stack
	BYTE _pad3A[2];				// 0x3A padding
 	SCRIPT_VAR LocalVar[32];	// 0x3C script's local variables
 	DWORD Timers[2];			// 0xBC script's timers
 	bool bIsActive;				// 0xC4 is script active
 	bool bCondResult;			// 0xC5 condition result
 	bool bUseMissionCleanup;	// 0xC6 clean mission
 	bool bIsExternal;			// 0xC7 is thread external (from script.img)
 	bool bTextBlockOverride;	// 0xC8
	BYTE bExternalType;			// 0xC9
	BYTE _padCA[2];				// 0xCA padding
 	DWORD WakeTime;				// 0xCC time, when script starts again after 0001 opcode
 	eLogicalOperation LogicalOp;// 0xD0 opcode 00D6 parameter
 	bool NotFlag;				// 0xD2 opcode & 0x8000 != 0
 	bool bWastedBustedCheck;	// 0xD3 wasted_or_busted check flag
 	bool bWastedOrBusted;		// 0xD4 is player wasted or busted
	char _padD5[3];				// 0xD5 padding
 	void* SceneSkipIP;			// 0xD8 scene skip label ptr
 	bool bIsMission;			// 0xDC is this script mission
	WORD ScmFunction;			// 0xDD CLEO's previous scmFunction id
	bool bIsCustom;				// 0xDF is this CLEO script

#ifdef __cplusplus
public:
	CRunningScript()
	{
		strcpy(Name, "noname");
		BaseIP = 0;
		Previous = 0;
		Next = 0;
		CurrentIP = 0;
		memset(Stack, 0, sizeof(Stack));
		SP = 0;
		WakeTime = 0;
		bIsActive = 0;
		bCondResult = 0;
		bUseMissionCleanup = 0;
		bIsExternal = 0;
		bTextBlockOverride = 0;
		bExternalType = -1;
		memset(LocalVar, 0, sizeof(LocalVar));
		LogicalOp = eLogicalOperation::NONE;
		NotFlag = 0;
		bWastedBustedCheck = 1;
		bWastedOrBusted = 0;
		SceneSkipIP = 0;
		bIsMission = 0;
		ScmFunction = 0;
		bIsCustom = 0;
	}

	bool IsActive() const { return bIsActive; }
	bool IsExternal() const { return bIsExternal; }
	bool IsMission() const { return bIsMission; }
	bool IsCustom() const { return bIsCustom; } // is this CLEO Script?
	std::string GetName() const { auto str = std::string(Name, Name + 8); str.resize(strlen(str.c_str())); return str; } // make sure it is always null terminated
	BYTE* GetBasePointer() const { return (BYTE*)BaseIP; }
	BYTE* GetBytePointer() const { return CurrentIP; }
	void SetIp(void* ip) { CurrentIP = (BYTE*)ip; }
	void SetBaseIp(void* ip) { BaseIP = ip; }
	CRunningScript* GetNext() const { return Next; }
	CRunningScript* GetPrev() const { return Previous; }
	void SetIsExternal(bool b) { bIsExternal = b; }
	void SetActive(bool b) { bIsActive = b; }
	OpcodeResult Suspend() { WakeTime = 0xFFFFFFFF; return OpcodeResult::OR_INTERRUPT; } // suspend script execution forever
	void SetNext(CRunningScript* v) { Next = v; }
	void SetPrev(CRunningScript* v) { Previous = v; }
	SCRIPT_VAR* GetVarPtr() { return LocalVar; }
	SCRIPT_VAR* GetVarPtr(int i) { return &LocalVar[i]; }
	int* GetIntVarPtr(int i) { return (int*)&LocalVar[i].dwParam; }
	int GetIntVar(int i) const { return LocalVar[i].dwParam; }
	void SetIntVar(int i, int v) { LocalVar[i].dwParam = v; }
	void SetFloatVar(int i, float v) { LocalVar[i].fParam = v; }
	char GetByteVar(int i) const { return LocalVar[i].bParam; }
	bool GetConditionResult() const { return bCondResult != 0; }
	bool GetNotFlag() const { return NotFlag; }
	void SetNotFlag(bool state) { NotFlag = state; }

	eDataType PeekDataType() const { return *(eDataType*)CurrentIP; }
	eArrayDataType PeekArrayDataType() const { BYTE t = *(CurrentIP + 1 + 2 + 2 + 1); t &= ArrayDataTypeMask; return (eArrayDataType) t; } // result valid only for array type params

	eDataType ReadDataType() { return (eDataType)ReadDataByte(); }
	short ReadDataVarIndex() { return ReadDataWord(); }
	short ReadDataArrayOffset() { return ReadDataWord(); }
	short ReadDataArrayIndex() { return ReadDataWord(); }
	short ReadDataArraySize() { return ReadDataByte(); }
	short ReadDataArrayFlags() { return ReadDataByte(); }
 
	void IncPtr(int n = 1) { CurrentIP += n; }
	int ReadDataByte() { char b = *CurrentIP; ++CurrentIP; return b; }
	short ReadDataWord() { short v = *(short*)CurrentIP; CurrentIP += 2; return v; }
	int ReadDataInt() { int i = *(int*)CurrentIP; CurrentIP += 4; return i; }

	void PushStack(BYTE* ptr) { Stack[SP++] = ptr; }
	BYTE* PopStack() { return Stack[--SP]; }

	WORD GetScmFunction() const { return ScmFunction; }
	void SetScmFunction(WORD id) { ScmFunction = id; }

#endif // __cplusplus
};
#pragma pack(pop)
static_assert(sizeof(CRunningScript) == 0xE0, "Invalid size of CRunningScript!");

// alias for legacy use
#ifdef __cplusplus
	typedef class CRunningScript CScriptThread;
#else
	typedef struct CRunningScript CScriptThread;
#endif

typedef OpcodeResult (CALLBACK* _pOpcodeHandler)(CRunningScript*);
typedef void(*FuncScriptDeleteDelegateT) (CRunningScript*);

#ifdef __cplusplus
extern "C" {
#endif	//__cplusplus

DWORD WINAPI CLEO_GetVersion();
eGameVersion WINAPI CLEO_GetGameVersion();

BOOL WINAPI CLEO_RegisterOpcode(WORD opcode, _pOpcodeHandler callback);
BOOL WINAPI CLEO_RegisterCommand(const char* commandName, _pOpcodeHandler callback); // uses cleo\.CONFIG\sa.json to obtain opcode number from name
void WINAPI CLEO_RegisterCallback(eCallbackId id, void* func);


// script utils
void WINAPI CLEO_GetScriptInfoStr(CRunningScript* thread, bool currLineInfo, char* buf, DWORD bufSize); // short text for displaying in error\log messages
void WINAPI CLEO_GetScriptParamInfoStr(int idexOffset, char* buf, DWORD bufSize); // short text with current+offset opcode parameter info (index and name if available)
eCLEO_Version WINAPI CLEO_GetScriptVersion(const CRunningScript* thread); // compatibility mode
LPCSTR WINAPI CLEO_GetScriptFilename(const CRunningScript* thread); // returns nullptr if provided script ptr is not valid

LPCSTR WINAPI CLEO_GetScriptWorkDir(const CRunningScript* thread);
void WINAPI CLEO_SetScriptWorkDir(CRunningScript* thread, const char* path);

void WINAPI CLEO_SetThreadCondResult(CRunningScript* thread, BOOL result);
void WINAPI CLEO_ThreadJumpAtLabelPtr(CRunningScript* thread, int labelPtr);

int WINAPI CLEO_GetOperandType(const CRunningScript* thread); // peek parameter data type. Returns int for legacy reason, should be eDataType.
DWORD WINAPI CLEO_GetVarArgCount(CRunningScript* thread); // peek remaining var-args count

extern SCRIPT_VAR* opcodeParams;
extern SCRIPT_VAR* missionLocals;

SCRIPT_VAR* WINAPI CLEO_GetOpcodeParamsArray(); // get pointer to 'SCRIPT_VAR[32] opcodeParams'. Used by Retrieve/Record opcode params functions
BYTE WINAPI CLEO_GetParamsHandledCount(); // number of already read/written opcode parameters since current opcode handler was called

// param read
SCRIPT_VAR* WINAPI CLEO_GetPointerToScriptVariable(CRunningScript* thread); // get pointer to the variable's data, nullptr if parameter is not variable. Advances script to next param
void WINAPI CLEO_RetrieveOpcodeParams(CRunningScript* thread, int count); // read multiple params. Stored in opcodeParams array
DWORD WINAPI CLEO_GetIntOpcodeParam(CRunningScript* thread);
float WINAPI CLEO_GetFloatOpcodeParam(CRunningScript* thread);
LPCSTR WINAPI CLEO_ReadStringOpcodeParam(CRunningScript* thread, char* buff = nullptr, int buffSize = 0); // read always null-terminated string into buffer, clamped to its size. If no buffer provided then internal, globally shared by all CLEO_ReadStringOpcodeParam calls, is used. Returns pointer to the result buffer or nullptr on fail
LPCSTR WINAPI CLEO_ReadStringPointerOpcodeParam(CRunningScript* thread, char* buff = nullptr, int buffSize = 0); // read always null-terminated string into buffer, clamped to its size. If no buffer provided then internal, globally shared by all CLEO_ReadStringPointerOpcodeParam calls, is used. WARNING: returned pointer may differ from buff and contain string longer than buffSize (ptr to original data source) 
void WINAPI CLEO_ReadStringParamWriteBuffer(CRunningScript* thread, char** outBuf, int* outBufSize, BOOL* outNeedsTerminator); // get info about the string opcode param, so it can be written latter. If outNeedsTerminator is not 0 then whole bufSize can be used as text characters. Advances script to next param
char* WINAPI CLEO_ReadParamsFormatted(CRunningScript* thread, const char* format, char* buf = nullptr, int bufSize = 0); // consumes all var-arg params and terminator
// get param value without advancing the script
DWORD WINAPI CLEO_PeekIntOpcodeParam(CRunningScript* thread);
float WINAPI CLEO_PeekFloatOpcodeParam(CRunningScript* thread);
SCRIPT_VAR* WINAPI CLEO_PeekPointerToScriptVariable(CRunningScript* thread); // get pointer to the variable's data, nullptr if parameter is not variable

// param skip without reading
void WINAPI CLEO_SkipOpcodeParams(CRunningScript* thread, int count);
void WINAPI CLEO_SkipUnusedVarArgs(CRunningScript* thread); // for var-args opcodes. Should be called even when all params were read (to skip var-arg terminator)

// param write
void WINAPI CLEO_RecordOpcodeParams(CRunningScript* thread, int count); // write multiple params from opcodeParams array
void WINAPI CLEO_SetIntOpcodeParam(CRunningScript* thread, DWORD value);
void WINAPI CLEO_SetFloatOpcodeParam(CRunningScript* thread, float value);
void WINAPI CLEO_WriteStringOpcodeParam(CRunningScript* thread, const char* str);


BOOL WINAPI CLEO_GetScriptDebugMode(const CRunningScript* thread); // debug mode features enabled for this script?
void WINAPI CLEO_SetScriptDebugMode(CRunningScript* thread, BOOL enabled);

CRunningScript* WINAPI CLEO_CreateCustomScript(CRunningScript* fromThread, const char* script_name, int label);
CRunningScript* WINAPI CLEO_GetLastCreatedCustomScript();
CRunningScript* WINAPI CLEO_GetScriptByName(const char* threadName, BOOL standardScripts, BOOL customScripts, DWORD resultIndex = 0); // can be called multiple times to find more scripts named threadName. resultIndex should be incremented until the method returns nullptr
CRunningScript* WINAPI CLEO_GetScriptByFilename(const char* path, DWORD resultIndex = 0); // can be absolute, partial path or just filename

// scripts deletion callback
void WINAPI CLEO_AddScriptDeleteDelegate(FuncScriptDeleteDelegateT func);
void WINAPI CLEO_RemoveScriptDeleteDelegate(FuncScriptDeleteDelegateT func);

DWORD WINAPI CLEO_GetScriptTextureById(CRunningScript* thread, int id); // ret RwTexture *

DWORD WINAPI CLEO_GetInternalAudioStream(CRunningScript* thread, DWORD stream); // arg CAudioStream *

// Should be always used when working with files. Provides ModLoader compatibility
void WINAPI CLEO_ResolvePath(CRunningScript* thread, char* inOutPath, DWORD pathMaxLen); // convert to absolute (file system) path
struct DirectoryList{ DWORD count; char** paths; };
DirectoryList WINAPI CLEO_ListDirectory(CRunningScript* thread, const char* searchPath, BOOL listDirs, BOOL listFiles); // thread can be null, searchPath can contain wildcards. After use CLEO_ListDirectoryFree must be called on returned DirectoryList to free allocated resources
void WINAPI CLEO_ListDirectoryFree(DirectoryList list); // releases resources allocated by CLEO_ListDirectory

void WINAPI CLEO_Log(eLogLevel level, const char* msg); // add message to log

#ifdef __cplusplus
}
#endif	//__cplusplus

} // CLEO namespace
