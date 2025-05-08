#include "FileUtils.h"
#include "CLEO_Utils.h"
#include <string>

DWORD File::FUNC_fopen = 0;
DWORD File::FUNC_fclose = 0;
DWORD File::FUNC_fread = 0;
DWORD File::FUNC_fwrite = 0;
DWORD File::FUNC_fgetc = 0;
DWORD File::FUNC_fgets = 0;
DWORD File::FUNC_fputs = 0;
DWORD File::FUNC_fseek = 0;
DWORD File::FUNC_fprintf = 0;
DWORD File::FUNC_ftell = 0;
DWORD File::FUNC_fflush = 0;
DWORD File::FUNC_feof = 0;
DWORD File::FUNC_ferror = 0;

void File::initialize(CLEO::eGameVersion version)
{
	// GV_US10, GV_US11, GV_EU10, GV_EU11, GV_STEAM
	const DWORD MA_FOPEN_FUNCTION[] =	{ 0x008232D8, 0, 0x00823318, 0x00824098, 0x0085C75E };
	const DWORD MA_FCLOSE_FUNCTION[] =	{ 0x0082318B, 0, 0x008231CB, 0x00823F4B, 0x0085C396 };
	const DWORD MA_FGETC_FUNCTION[] =	{ 0x008231DC, 0, 0x0082321C, 0x00823F9C, 0x0085C680 };
	const DWORD MA_FGETS_FUNCTION[] =	{ 0x00823798, 0, 0x008237D8, 0x00824558, 0x0085D00C };
	const DWORD MA_FPUTS_FUNCTION[] =	{ 0x008262B8, 0, 0x008262F8, 0x00826BA8, 0x008621F1 };
	const DWORD MA_FREAD_FUNCTION[] =	{ 0x00823521, 0, 0x00823561, 0x008242E1, 0x0085CD04 };
	const DWORD MA_FWRITE_FUNCTION[] =	{ 0x00823674, 0, 0x008236B4, 0x00824434, 0x0085CE7E };
	const DWORD MA_FSEEK_FUNCTION[] =	{ 0x0082374F, 0, 0x0082378F, 0x0082450F, 0x0085CF87 };
	const DWORD MA_FPRINTF_FUNCTION[] = { 0x00823A30, 0, 0x00823A70, 0x008247F0, 0x0085D464 };
	const DWORD MA_FTELL_FUNCTION[] =	{ 0x00826261, 0, 0x008262A1, 0x00826B51, 0x00862183 };
	const DWORD MA_FFLUSH_FUNCTION[] =	{ 0x00823E86, 0, 0x00823EC6, 0x00824C46, 0x0085DDDD };
	const DWORD MA_FEOF_FUNCTION[] =	{ 0x008262A2, 0, 0x008262E2, 0x00826B92, 0x0085D193 };
	const DWORD MA_FERROR_FUNCTION[] =	{ 0x008262AD, 0, 0x008262ED, 0x00826B9D, 0x0085D1C2 };

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
	FUNC_ferror = MA_FERROR_FUNCTION[version];;
}

bool File::isLegacy(DWORD handle) { return (handle & 0x1) == 0; }

FILE* File::handleToFile(DWORD handle) { return (FILE*)(handle & ~0x1); }

DWORD File::fileToHandle(FILE* file, bool legacy)
{
	if (file == nullptr) return 0;

	auto handle = (DWORD)file;
	if (!legacy) handle |= 0x1;
	return handle;
}

void File::updateState(DWORD handle)
{
	// RW streams needs synchronization of read and write positions (https://en.wikibooks.org/wiki/C_Programming/stdio.h/fopen)
	if (isOk(handle) && !isEndOfFile(handle))
	{
		seek(handle, 0, SEEK_CUR);
	}
}

bool File::flush(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return false;

	int result = 0;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_fflush
			add esp, 0x4
			mov result, eax
		}
	}
	else
		result = fflush(file);

	return result == 0;
}

DWORD File::open(const char* filename, const char* mode, bool legacy)
{
	// validate the mode argument
	if (!legacy)
	{
		static char modeUpdated[12];
		const std::string allowed = "+abcnrtwxDRST"; // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-170

		bool valid = false;
		bool binary = false;
		bool text = false;
		auto modeLen = mode != nullptr ? strlen(mode) : 0;
		if (modeLen > 0 && modeLen < (sizeof(modeUpdated) - 1)) // keep space for extra binary mode char
		{
			valid = true;

			for (auto ch : std::string_view(mode))
			{
				if (allowed.find(ch) == std::string_view::npos)
				{
					valid = false;
					break; // invalid character
				}

				if (ch == 'b') binary = true;
				if (ch == 't') text = true;
			}

			if (binary && text) valid = false;

			// By default open as binary mode.
			// Generally text mode is not well documented in C and many file related functions has undefined behavior. For example 'ftell' returns invalid values.
			if (valid && (!binary && !text))
			{
				strcpy_s(modeUpdated, mode);
				strcat_s(modeUpdated, "b");
				mode = modeUpdated;
			}
		}

		if (!valid)
		{
			LOG_WARNING(0, "Invalid mode argument '%s' while opening file \"%s\" stream!", mode, filename);
			return 0; // invalid handle
		}
	}

	FILE* file = nullptr;
	if (legacy)
	{
		_asm
		{
			push mode
			push filename
			call FUNC_fopen
			add esp, 8
			mov file, eax
		}
	}
	else
		fopen_s(&file, filename, mode);

	return fileToHandle(file, legacy);
}

void File::close(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return;

	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_fclose
			add esp, 4
		}
	}
	else
		fclose(file);
}

bool File::isOk(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return false;

	int result = 0;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_ferror
			add esp, 0x4
		}
	}
	else
		result = ferror(file);

	return result == 0;
}

DWORD File::getSize(DWORD handle)
{
	auto pos = getPos(handle);
	seek(handle, 0, SEEK_END);
	DWORD size = getPos(handle);
	seek(handle, pos, SEEK_SET);
	return size;
}

bool File::seek(DWORD handle, int offset, DWORD orign)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return false;

	int result = 0;
	if (isLegacy(handle))
	{
		auto off = offset; // 'offset' is keyword in asm
		_asm
		{
			push orign
			push off
			push file
			call FUNC_fseek
			add esp, 0xC
			mov result, eax
		}
	}
	else
		result = fseek(file, offset, orign);

	return result == 0;
}

DWORD File::getPos(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	DWORD pos = 0;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_ftell
			add esp, 0x4
			mov pos, eax
		}
	}
	else
		pos = (DWORD)ftell(file);

	return pos;
}

bool File::isEndOfFile(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return true;

	int result = 0;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_feof
			add esp, 0x4
			mov result, eax
		}
	}
	else
		result = feof(file);

	return result != 0;
}

DWORD File::read(DWORD handle, void* buffer, DWORD size)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	DWORD read = 0;
	if (isLegacy(handle))
	{
		auto siz = size; // 'size' is keyword in asm
		_asm
		{
			push file
			push siz
			push 1
			push buffer
			call FUNC_fread
			add esp, 0x10
			mov read, eax
		}
	}
	else
		read = fread(buffer, 1, size, file);

	updateState(handle);

	return read;
}

char File::readChar(DWORD handle)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	char result = '_';
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			call FUNC_fgetc
			add esp, 0x4
			mov result, al
		}
	}
	else
		result = fgetc(file);

	updateState(handle);

	return result;
}

char* File::readString(DWORD handle, char* buffer, DWORD bufferSize)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return nullptr;

	char* result = nullptr;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			push bufferSize
			push buffer
			call FUNC_fgets
			add esp, 0xC
			mov result, eax
		}
	}
	else
		result = fgets(buffer, bufferSize, file);

	updateState(handle);

	return result;
}

DWORD File::write(DWORD handle, const void* buffer, DWORD size)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	DWORD writen = 0;
	if (isLegacy(handle))
	{
		auto siz = size; // 'size' is keyword in asm
		_asm
		{
			push file
			push siz
			push 1
			push buffer
			call FUNC_fwrite
			add esp, 0x10
			mov writen, eax
		}
	}
	else
		writen = (DWORD)fwrite(buffer, 1, size, file);

	updateState(handle);

	return writen;
}

bool File::writeString(DWORD handle, const char* text)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	int result = 0;
	if (isLegacy(handle))
	{
		_asm
		{
			push file
			push text
			call FUNC_fputs
			add esp, 0x8
			mov result, eax
		}
	}
	else
		result = (DWORD)fputs(text, file);

	updateState(handle);

	return result >= 0;
}

DWORD File::scan(DWORD handle, const char* format, void** outputParams)
{
	FILE* file = handleToFile(handle);
	if (file == nullptr) return 0;

	int read = 0;
	if (isLegacy(handle))
	{
		// fscanf not existent in game's code. Emulate it

		size_t paramCount = 0;
		const char* f = format;
		while(*f != '\0')
		{
			if (*f == '%')
			{
				if (*(f + 1) != '%') // not escaped %
					paramCount++;
				else
					f++; // skip escaped
			}
			f++;
		}

		auto newFormat = std::string(format) + "%n"; // %n returns characters processed by
		DWORD charRead = 0;
		outputParams[paramCount] = &charRead;

		DWORD prevCharRead = 0;
		std::string readText;
		while(true)
		{
			readText += readChar(handle);

			prevCharRead = charRead;
			auto p = outputParams;
			read = sscanf_s(readText.c_str(), newFormat.c_str(),
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], // 10
				p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], // 20
				p[20], p[21], p[22], p[23], p[24], p[25], p[26], p[27], p[28], p[29], // 30
				p[30], p[31], p[32], p[33], p[34]); // 35

			if (!isOk(handle)) break;

			if (read == paramCount)
			{
				if (charRead == prevCharRead) // all params collected and scan doesn't consume input text anymore
				{
					seek(handle, -1, SEEK_CUR); // return the character not used by scan
					break;
				}

				if (isEndOfFile(handle))
				{
					break;
				}
			}
		}
	}
	else
	{
		auto p = outputParams;
		read = 	fscanf_s(file, format, 
			p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], // 10
			p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], // 20
			p[20], p[21], p[22], p[23], p[24], p[25], p[26], p[27], p[28], p[29], // 30
			p[30], p[31], p[32], p[33], p[34]); // 35
	}

	updateState(handle);

	return read;
}
