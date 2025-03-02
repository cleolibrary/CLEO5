#pragma once
#include "CLEO.h"
#include <stdio.h>
#include <wtypes.h>

class File
{
public:
	static void initialize(CLEO::eGameVersion version);

	static DWORD open(const char* filename, const char* mode, bool legacy);
	static void close(DWORD handle);

	static bool isOk(DWORD handle);

	static DWORD getSize(DWORD handle);
	static bool seek(DWORD handle, int offset, DWORD orign);
	static DWORD getPos(DWORD handle);
	static bool isEndOfFile(DWORD handle);

	static DWORD read(DWORD handle, void* buffer, DWORD size);
	static char readChar(DWORD handle);
	static char* readString(DWORD handle, char* buffer, DWORD bufferSize);

	static DWORD write(DWORD handle, const void* buffer, DWORD size);
	static bool writeString(DWORD handle, const char* text);
	static DWORD scan(DWORD handle, const char* format, void** outputParams);
	static bool flush(DWORD handle);

	static bool isLegacy(DWORD handle); // Legacy modes for CLEO 3
	static FILE* handleToFile(DWORD handle);
	
private:
	static DWORD FUNC_fopen;
	static DWORD FUNC_fclose;
	static DWORD FUNC_fread;
	static DWORD FUNC_fwrite;
	static DWORD FUNC_fgetc;
	static DWORD FUNC_fgets;
	static DWORD FUNC_fputs;
	static DWORD FUNC_fseek;
	static DWORD FUNC_fprintf;
	static DWORD FUNC_ftell;
	static DWORD FUNC_fflush;
	static DWORD FUNC_feof;
	static DWORD FUNC_ferror;

	static DWORD fileToHandle(FILE* file, bool legacy);
	static void updateState(DWORD handle);
};
