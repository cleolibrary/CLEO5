#pragma once
#include "CLEO.h"
#include <cstdio>
#include <wtypes.h>

static DWORD FUNC_fopen = 0;
static DWORD FUNC_fclose = 0;
static DWORD FUNC_fread = 0;
static DWORD FUNC_fwrite = 0;
static DWORD FUNC_fgetc = 0;
static DWORD FUNC_fgets = 0;
static DWORD FUNC_fputs = 0;
static DWORD FUNC_fseek = 0;
static DWORD FUNC_fprintf = 0;
static DWORD FUNC_ftell = 0;
static DWORD FUNC_fflush = 0;
static DWORD FUNC_feof = 0;
static DWORD FUNC_ferror = 0;

void InitializeFileFunc(CLEO::eGameVersion version)
{
	// GV_US10, GV_US11, GV_EU10, GV_EU11, GV_STEAM
	const DWORD MA_FOPEN_FUNCTION[] =	{ 0x008232D8,	0, 0x00823318, 0x00824098, 0x0085C75E };
	const DWORD MA_FCLOSE_FUNCTION[] =	{ 0x0082318B,	0, 0x008231CB, 0x00823F4B, 0x0085C396 };
	const DWORD MA_FGETC_FUNCTION[]	=	{ 0x008231DC,	0, 0x0082321C, 0x00823F9C, 0x0085C680 };
	const DWORD MA_FGETS_FUNCTION[]	= 	{ 0x00823798,	0, 0x008237D8, 0x00824558, 0x0085D00C };
	const DWORD MA_FPUTS_FUNCTION[]	= 	{ 0x008262B8,	0, 0x008262F8, 0x00826BA8, 0x008621F1 };
	const DWORD MA_FREAD_FUNCTION[]	= 	{ 0x00823521,	0, 0x00823561, 0x008242E1, 0x0085CD04 };
	const DWORD MA_FWRITE_FUNCTION[] =	{ 0x00823674,	0, 0x008236B4, 0x00824434, 0x0085CE7E };
	const DWORD MA_FSEEK_FUNCTION[]	= 	{ 0x0082374F,	0, 0x0082378F, 0x0082450F, 0x0085CF87 };
	const DWORD MA_FPRINTF_FUNCTION[] =	{ 0x00823A30,	0, 0x00823A70, 0x008247F0, 0x0085D464 };
	const DWORD MA_FTELL_FUNCTION[] = 	{ 0x00826261,	0, 0x008262A1, 0x00826B51, 0x00862183 };
	const DWORD MA_FFLUSH_FUNCTION[] =	{ 0x00823E86,	0, 0x00823EC6, 0x00824C46, 0x0085DDDD };
	const DWORD MA_FEOF_FUNCTION[] =	{ 0x008262A2,	0, 0x008262E2, 0x00826B92, 0x0085D193 };
	const DWORD MA_FERROR_FUNCTION[] =	{ 0x008262AD,	0, 0x008262ED, 0x00826B9D, 0x0085D1C2 };

	FUNC_fopen = MA_FOPEN_FUNCTION[version];
	FUNC_fclose = MA_FCLOSE_FUNCTION[version];
	FUNC_fread = MA_FREAD_FUNCTION[version];
	FUNC_fwrite = MA_FWRITE_FUNCTION[version];
	FUNC_fgetc = MA_FGETC_FUNCTION[version];
	FUNC_fgets = MA_FGETS_FUNCTION[version];
	FUNC_fputs = MA_FPUTS_FUNCTION[version];
	FUNC_fseek = MA_FSEEK_FUNCTION[version];
	FUNC_fprintf = MA_FPRINTF_FUNCTION[version];
	FUNC_ftell = MA_FTELL_FUNCTION[version];
	FUNC_fflush = MA_FFLUSH_FUNCTION[version];
	FUNC_feof = MA_FEOF_FUNCTION[version];
	FUNC_ferror = MA_FERROR_FUNCTION[version];
}

// Legacy modes for CLEO 3
bool is_legacy_handle(DWORD dwHandle) 
{ 
	return (dwHandle & 0x1) == 0; 
}

FILE* convert_handle_to_file(DWORD dwHandle) 
{
	return (FILE*)(dwHandle & ~0x1);
}

DWORD convert_file_to_handle(FILE* file, bool legacy)
{
	if (file == nullptr) return 0;

	auto handle = (DWORD)file;
	if (!legacy) handle |= 0x1;
	return handle;
}

FILE* legacy_fopen(const char* szPath, const char* szMode)
{
	FILE* hFile;
	_asm
	{
		push szMode
		push szPath
		call FUNC_fopen
		add esp, 8
		mov hFile, eax
	}
	return hFile;
}

void legacy_fclose(FILE* hFile)
{
	_asm
	{
		push hFile
		call FUNC_fclose
		add esp, 4
	}
}

size_t legacy_fread(void* buf, size_t len, size_t count, FILE* stream)
{
	_asm
	{
		push stream
		push count
		push len
		push buf
		call FUNC_fread
		add esp, 0x10
	}
}

size_t legacy_fwrite(const void* buf, size_t len, size_t count, FILE* stream)
{
	_asm
	{
		push stream
		push count
		push len
		push buf
		call FUNC_fwrite
		add esp, 0x10
	}
}

char legacy_fgetc(FILE* stream)
{
	_asm
	{
		push stream
		call FUNC_fgetc
		add esp, 0x4
	}
}

char* legacy_fgets(char* pStr, int num, FILE* stream)
{
	_asm
	{
		push stream
		push num
		push pStr
		call FUNC_fgets
		add esp, 0xC
	}
}

int legacy_fputs(const char* pStr, FILE* stream)
{
	_asm
	{
		push stream
		push pStr
		call FUNC_fputs
		add esp, 0x8
	}
}

int legacy_fseek(FILE* stream, long int offs, int original)
{
	_asm
	{
		push stream
		push offs
		push original
		call FUNC_fseek
		add esp, 0xC
	}
}

int legacy_ftell(FILE* stream)
{
	_asm
	{
		push stream
		call FUNC_ftell
		add esp, 0x4
	}
}

/*int __declspec(naked) fprintf(FILE* stream, const char* format, ...)
{
	_asm jmp FUNC_fprintf
}*/

int legacy_fflush(FILE* stream)
{
	_asm
	{
		push stream
		call FUNC_fflush
		add esp, 0x4
	}
}

int legacy_feof(FILE* stream)
{
	_asm
	{
		push stream
		call FUNC_feof
		add esp, 0x4
	}
}

int legacy_ferror(FILE* stream)
{
	_asm
	{
		push stream
		call FUNC_ferror
		add esp, 0x4
	}
}

DWORD open_file(const char* path, const char* mode, bool legacy)
{
	FILE* hFile = legacy ? legacy_fopen(path, mode) : fopen(path, mode);
	return convert_file_to_handle(hFile, legacy);
}

void close_file(DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return;

	if (is_legacy_handle(handle))
		legacy_fclose(file);
	else 
		fclose(file);
}

DWORD file_get_size(DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return 0;

	auto savedPos = ftell(file);
	fseek(file, 0, SEEK_END);
	DWORD size = static_cast<DWORD>(ftell(file));
	fseek(file, savedPos, SEEK_SET);
	return size;
}

DWORD read_file(void* buf, DWORD size, DWORD count, DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return 0;

	if (is_legacy_handle(handle))
		return legacy_fread(buf, size, 1, file);
	else
		return fread(buf, size, 1, file);
}

DWORD write_file(const void* buf, DWORD size, DWORD count, DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return 0;

	if (is_legacy_handle(handle))
		return legacy_fwrite(buf, size, 1, file);
	else
		return fwrite(buf, size, 1, file);
}

char* file_get_s(char* buf, DWORD bufSize, DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return nullptr;

	if (is_legacy_handle(handle))
		return legacy_fgets(buf, bufSize, file);
	else
		return fgets(buf, bufSize, file);
}

void flush_file(DWORD handle)
{
	FILE* file = convert_handle_to_file(handle);
	if (file == nullptr) return;

	if (is_legacy_handle(handle))
		legacy_fflush(file);
	else
		fflush(file);
}
