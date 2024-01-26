#include "plugin.h"
#include "CLEO.h"
#include "FileUtils.h"
#include "Utils.h"

#include <set>

using namespace CLEO;
using namespace plugin;

#define OPCODE_VALIDATE_HANDLE(x) if((size_t)x <= MinValidAddress) { auto info = scriptInfoStr(thread); SHOW_ERROR("Invalid '0x%X' file handle param in script %s \nScript suspended.", x, info.c_str()); return thread->Suspend(); }

class FileSystemOperations 
{
public:
    static std::set<DWORD> m_hFiles;
    static std::set<HANDLE> m_hFileSearches;

    static void WINAPI OnFinalizeScriptObjects()
    {
        // clean up opened files
        for (auto handle : m_hFiles)
        {
            if (!is_legacy_handle(handle))
                fclose(convert_handle_to_file(handle));
        }
        m_hFiles.clear();

        // clean up file searches
        for (auto handle : m_hFileSearches) FindClose(handle);
        m_hFileSearches.clear();
    }

    FileSystemOperations() 
    {
        auto cleoVer = CLEO_GetVersion();
        if (cleoVer >= CLEO_VERSION)
        {
            InitializeFileFunc(CLEO_GetGameVersion()); // file utils

            //register opcodes
            CLEO_RegisterOpcode(0x0A9A, opcode_0A9A);
            CLEO_RegisterOpcode(0x0A9B, opcode_0A9B);
            CLEO_RegisterOpcode(0x0A9C, opcode_0A9C);
            CLEO_RegisterOpcode(0x0A9D, opcode_0A9D);
            CLEO_RegisterOpcode(0x0A9E, opcode_0A9E);
            CLEO_RegisterOpcode(0x0AD5, opcode_0AD5);
            CLEO_RegisterOpcode(0x0AD6, opcode_0AD6);
            CLEO_RegisterOpcode(0x0AD7, opcode_0AD7);
            CLEO_RegisterOpcode(0x0AD8, opcode_0AD8);
            CLEO_RegisterOpcode(0x0AD9, opcode_0AD9);
            CLEO_RegisterOpcode(0x0ADA, opcode_0ADA);
            CLEO_RegisterOpcode(0x0ADA, opcode_2200);

            CLEO_RegisterOpcode(0x0AAB, Script_FS_FileExists);
            CLEO_RegisterOpcode(0x0AE4, Script_FS_DirectoryExists);
            CLEO_RegisterOpcode(0x0AE5, Script_FS_CreateDirectory);
            CLEO_RegisterOpcode(0x0AE6, Script_FS_FindFirstFile);
            CLEO_RegisterOpcode(0x0AE7, Script_FS_FindNextFile);
            CLEO_RegisterOpcode(0x0AE8, Script_FS_FindClose);

            CLEO_RegisterOpcode(0x0B00, Script_FS_DeleteFile);
            CLEO_RegisterOpcode(0x0B01, Script_FS_DeleteDirectory);
            CLEO_RegisterOpcode(0x0B02, Script_FS_MoveFile);
            CLEO_RegisterOpcode(0x0B03, Script_FS_MoveDir);
            CLEO_RegisterOpcode(0x0B04, Script_FS_CopyFile);
            CLEO_RegisterOpcode(0x0B05, Script_FS_CopyDir);

            // register event callbacks
            CLEO_RegisterCallback(eCallbackId::ScriptsFinalize, OnFinalizeScriptObjects);
        }
        else
        {
            std::string err(128, '\0');
            sprintf(err.data(), "This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
            MessageBox(HWND_DESKTOP, err.data(), "FileSystemOperations.cleo", MB_SYSTEMMODAL | MB_ICONERROR);
        }
    }

    static std::string ReadPathParam(CRunningScript* thread)
    {
        std::string path(MAX_STR_LEN, '\0');
        CLEO_ReadStringOpcodeParam(thread, path.data(), MAX_STR_LEN);
        CLEO_ResolvePath(thread, path.data(), MAX_STR_LEN);
        path.resize(strlen(path.c_str()));
        return path;
    }

    //0A9A=3,%3d% = openfile %1d% mode %2d% // IF and SET
    static OpcodeResult WINAPI opcode_0A9A(CRunningScript* thread)
    {
        auto filename = ReadPathParam(thread);

        char mode[16];
        auto paramType = CLEO_GetOperandType(thread);
        if (IsImmInteger(paramType) || IsVariable(paramType))
        {
            // integer param (for backward compatibility with CLEO 3)
            union
            {
                DWORD uParam;
                char strParam[4];
            } param;
            param.uParam = CLEO_GetIntOpcodeParam(thread);
            strcpy(mode, param.strParam);
        }
        else
        {
            CLEO_ReadStringOpcodeParam(thread, mode, sizeof(mode));
        }

        // either CLEO 3 or CLEO 4 made a big mistake! (they differ in one major unapparent preference)
        // lets try to resolve this with a legacy mode
        bool legacy = CLEO_GetScriptVersion(thread) < CLEO_VER_4_3;

        auto handle = open_file(filename.c_str(), mode, legacy);
        if (handle == NULL)
        {
            CLEO_SetIntOpcodeParam(thread, NULL);
            CLEO_SetThreadCondResult(thread, false);
            return OR_CONTINUE;
        }

        m_hFiles.insert(handle);
        CLEO_SetIntOpcodeParam(thread, handle);
        CLEO_SetThreadCondResult(thread, true);
        return OR_CONTINUE;
    }

    //0A9B=1,closefile %1d%
    static OpcodeResult WINAPI opcode_0A9B(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        close_file(handle);
        m_hFiles.erase(handle);
        return OR_CONTINUE;
    }

    //0A9C=2,%2d% = file %1d% size
    static OpcodeResult WINAPI opcode_0A9C(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        auto size = file_get_size(handle);
        CLEO_SetIntOpcodeParam(thread, size);
        return OR_CONTINUE;
    }

    //0A9D=3,readfile %1d% size %2d% to %3d%
    static OpcodeResult WINAPI opcode_0A9D(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        DWORD size = CLEO_GetIntOpcodeParam(thread);
        SCRIPT_VAR* buf = CLEO_GetPointerToScriptVariable(thread);

        buf->dwParam = 0; // https://github.com/cleolibrary/CLEO4/issues/91
        read_file(buf, size, 1, handle);
        return OR_CONTINUE;
    }

    //0A9E=3,writefile %1d% size %2d% from %3d%
    static OpcodeResult WINAPI opcode_0A9E(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        DWORD size = CLEO_GetIntOpcodeParam(thread);
        SCRIPT_VAR* buf = CLEO_GetPointerToScriptVariable(thread);

        write_file(buf, size, 1, handle);
        flush_file(handle);
        return OR_CONTINUE;
    }

    // 0AAB=1, file_exists %1s%
    static OpcodeResult WINAPI Script_FS_FileExists(CRunningScript* thread)
    {
        auto filename = ReadPathParam(thread);

        DWORD fAttr = GetFileAttributes(filename.c_str());
        bool exists = (fAttr != INVALID_FILE_ATTRIBUTES) && !(fAttr & FILE_ATTRIBUTE_DIRECTORY);

        CLEO_SetThreadCondResult(thread, exists);
        return OR_CONTINUE;
    }

    //0AD5=3,file %1d% seek %2d% from_origin %3d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD5(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        int offset = (int)CLEO_GetIntOpcodeParam(thread);
        int origin = (int)CLEO_GetIntOpcodeParam(thread);

        FILE* file = convert_handle_to_file(handle);
        bool ok = fseek(file, offset, origin) == 0;
        CLEO_SetThreadCondResult(thread, ok);
        return OR_CONTINUE;
    }

    //0AD6=1,end_of_file %1d% reached
    static OpcodeResult WINAPI opcode_0AD6(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)

        FILE* file = convert_handle_to_file(handle);
        bool end = ferror(file) || feof(file) != 0;
        CLEO_SetThreadCondResult(thread, end);
        return OR_CONTINUE;
    }

    //0AD7=3,read_string_from_file %1d% to %2d% size %3d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD7(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)

        char* buffer = nullptr;
        int bufferSize = 0;
        DWORD needsTerminator = TRUE;
        CLEO_ReadStringParamWriteBuffer(thread, &buffer, &bufferSize, &needsTerminator);

        int size = CLEO_GetIntOpcodeParam(thread);

        if (is_legacy_handle(handle))
        {
            auto info = scriptInfoStr(thread);
            SHOW_ERROR("Opcode [0AD7] used in legacy mode in script %s\nScript suspended.", info.c_str());
            return thread->Suspend();
        }

        if (size <= 0)
        {
            CLEO_SetThreadCondResult(thread, false);
            return OR_CONTINUE;
        }

        std::vector<char> tmpBuff;
        tmpBuff.resize(size);
        auto data = tmpBuff.data();

        FILE* file = convert_handle_to_file(handle);
        bool ok = fgets(data, size, file) != nullptr;
        if(!ok)
        {
            CLEO_SetThreadCondResult(thread, false);
            return OR_CONTINUE;
        }

        // copy into result param
        int len = strlen(data);
        int resultSize = min(len, bufferSize - (int)needsTerminator);

        memcpy(buffer, data, resultSize);
        if(resultSize < bufferSize) buffer[resultSize] = '\0'; // terminate string whenever possible

        CLEO_SetThreadCondResult(thread, false);
        return OR_CONTINUE;
    }

    //0AD8=2,write_string_to_file %1d% from %2d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD8(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        auto text = CLEO_ReadStringOpcodeParam(thread);

        FILE* file = convert_handle_to_file(handle);
        bool ok = fputs(text, file) > 0;
        if (!ok)
        {
            CLEO_SetThreadCondResult(thread, false);
            return OR_CONTINUE;
        }

        fflush(file);
        CLEO_SetThreadCondResult(thread, true);
        return OR_CONTINUE;
    }

    //0AD9=-1,write_formated_text %2d% to_file %1d%
    static OpcodeResult WINAPI opcode_0AD9(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        auto format = CLEO_ReadStringOpcodeParam(thread);
        char text[MAX_STR_LEN]; CLEO_ReadParamsFormatted(thread, format, text, MAX_STR_LEN);
        
        FILE* file = convert_handle_to_file(handle);
        fputs(text, file);
        fflush(file);
        return OR_CONTINUE;
    }

    //0ADA=-1,%3d% = scan_file %1d% format %2d% //IF and SET
    static OpcodeResult WINAPI opcode_0ADA(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        auto format = CLEO_ReadStringOpcodeParam(thread);
        int* result = (int*)CLEO_GetPointerToScriptVariable(thread);

        size_t paramCount = 0;
        SCRIPT_VAR* ExParams[35];
        while (CLEO_GetOperandType(thread) != eDataType::DT_END)
        {
            // TODO: if target param is string variable it should be handled correctly
            ExParams[paramCount++] = CLEO_GetPointerToScriptVariable(thread);
        }
        CLEO_SkipUnusedVarArgs(thread); // var arg terminator

        FILE* file = convert_handle_to_file(handle);
        *result = fscanf(file, format,
            /* extra parameters (will be aligned automatically, but the limit of 35 elements maximum exists) */
            ExParams[0], ExParams[1], ExParams[2], ExParams[3], ExParams[4], ExParams[5],
            ExParams[6], ExParams[7], ExParams[8], ExParams[9], ExParams[10], ExParams[11],
            ExParams[12], ExParams[13], ExParams[14], ExParams[15], ExParams[16], ExParams[17],
            ExParams[18], ExParams[19], ExParams[20], ExParams[21], ExParams[22], ExParams[23],
            ExParams[24], ExParams[25], ExParams[26], ExParams[27], ExParams[28], ExParams[29],
            ExParams[30], ExParams[31], ExParams[32], ExParams[33], ExParams[34]);

        CLEO_SetThreadCondResult(thread, paramCount == *result);
        return OR_CONTINUE;
    }

    //2200=3,read_block_from_file %1d% size %2d% address %3d% // IF and SET
    static OpcodeResult WINAPI opcode_2200(CRunningScript* thread)
    {
        DWORD handle = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_HANDLE(handle)
        DWORD size = CLEO_GetIntOpcodeParam(thread);

        auto paramType = CLEO_GetOperandType(thread);
        if(!IsImmInteger(paramType) && !IsVariable(paramType))
        {
            auto info = scriptInfoStr(thread);
            SHOW_ERROR("Invalid type (0x%02X) of 'address' argument in opcode [2200] in script %s\nScript suspended.", paramType, info.c_str());
            return thread->Suspend();
        }
        DWORD target = CLEO_GetIntOpcodeParam(thread); OPCODE_VALIDATE_POINTER(target)

        if(size < 0)
        {
            auto info = scriptInfoStr(thread);
            SHOW_ERROR("Invalid size argument (%d) in opcode [2200] in script %s\nScript suspended.", size, info.c_str());
            return thread->Suspend();
        }

        if (size == 0)
        {
            CLEO_SetThreadCondResult(thread, true); // done
            return OR_CONTINUE;
        }

        auto readCount = read_file((void*)target, size, 1, handle);
        if (readCount != size)
        {
            CLEO_SetThreadCondResult(thread, false);
            return OR_CONTINUE;
        }

        CLEO_SetThreadCondResult(thread, true);
        return OR_CONTINUE;
    }

    // 0AE4=1, directory_exist %1s%
    static OpcodeResult WINAPI Script_FS_DirectoryExists(CRunningScript* thread)
    {
        auto filename = ReadPathParam(thread);

        DWORD fAttr = GetFileAttributes(filename.c_str());
        bool exists = (fAttr != INVALID_FILE_ATTRIBUTES) && (fAttr & FILE_ATTRIBUTE_DIRECTORY);

        CLEO_SetThreadCondResult(thread, exists);
        return OR_CONTINUE;
    }

    // 0AE5=1, create_directory %1s% //IF and SET
    static OpcodeResult WINAPI Script_FS_CreateDirectory(CRunningScript* thread)
    {
        auto filename = ReadPathParam(thread);

        bool result = CreateDirectory(filename.c_str(), NULL) != 0;

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }

    // 0AE6=3, %2d% = find_first_file %1s% get_filename_to %3s% //IF and SET
    static OpcodeResult WINAPI Script_FS_FindFirstFile(CRunningScript* thread)
    {
        auto filename = ReadPathParam(thread);
        WIN32_FIND_DATA ffd = { 0 };
        HANDLE handle = FindFirstFile(filename.c_str(), &ffd);

        CLEO_SetIntOpcodeParam(thread, (DWORD)handle);

        if (handle != INVALID_HANDLE_VALUE)
        {
            m_hFileSearches.insert(handle);

            CLEO_WriteStringOpcodeParam(thread, ffd.cFileName);
            CLEO_SetThreadCondResult(thread, true);
        }
        else
        {
            CLEO_SkipOpcodeParams(thread, 1);
            CLEO_SetThreadCondResult(thread, false);
        }
        return OR_CONTINUE;
    }

    // 0AE7=2,%2s% = find_next_file %1d% //IF and SET
    static OpcodeResult WINAPI Script_FS_FindNextFile(CRunningScript* thread)
    {
        auto handle = (HANDLE)CLEO_GetIntOpcodeParam(thread);

        WIN32_FIND_DATA ffd = { 0 };
        if (FindNextFile(handle, &ffd))
        {
            CLEO_WriteStringOpcodeParam(thread, ffd.cFileName);
            CLEO_SetThreadCondResult(thread, true);
        }
        else
        {
            CLEO_SkipOpcodeParams(thread, 1);
            CLEO_SetThreadCondResult(thread, false);
        }
        return OR_CONTINUE;
    }

    // 0AE8=1, find_close %1d%
    static OpcodeResult WINAPI Script_FS_FindClose(CRunningScript* thread)
    {
        auto handle = (HANDLE)CLEO_GetIntOpcodeParam(thread);
        FindClose(handle);
        m_hFileSearches.erase(handle);
        return OR_CONTINUE;
    }

    // 0B00=1, delete_file %1s% //IF and SET
    static OpcodeResult WINAPI Script_FS_DeleteFile(CScriptThread* thread)
    {
        auto filename = ReadPathParam(thread);

        CLEO_SetThreadCondResult(thread, DeleteFile(filename.c_str()));

        return OR_CONTINUE;
    }

    static BOOL DeleteDir(const char *path)
    {
        char mask[MAX_PATH];
        HANDLE hSearch = NULL;
        WIN32_FIND_DATA wfd;
        char subPath[MAX_PATH];

        memset(&wfd, 0, sizeof(wfd));
        //search mask
        sprintf(mask, "%s\\*", path);

        //try to delete all inside first
        if ((hSearch = FindFirstFile(mask, &wfd)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //recursively delete subdirectories
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if ((strcmp(wfd.cFileName, "..") != 0) && (strcmp(wfd.cFileName, ".") != 0))
                    {
                        sprintf(subPath, "%s\\%s", path, wfd.cFileName);
                        if (!DeleteDir(subPath))
                            return FALSE;
                    }
                }
                else
                {
                    //remove read-only, system, hidden flags
                    sprintf(subPath, "%s\\%s", path, wfd.cFileName);
                    SetFileAttributes(subPath, FILE_ATTRIBUTE_NORMAL);
                    //delete file
                    if (!DeleteFile(subPath))
                        return FALSE;
                }


            } while (FindNextFile(hSearch, &wfd));
            FindClose(hSearch);
        }

        //delete empty directory
        return RemoveDirectory(path);
    }

    // 0B01=1, delete_directory %1s% with_all_files_and_subdirectories %2d% //IF and SET
    static OpcodeResult WINAPI Script_FS_DeleteDirectory(CScriptThread* thread)
    {
        auto dirpath = ReadPathParam(thread);
        int DeleteAllInsideFlag = CLEO_GetIntOpcodeParam(thread);

        BOOL result;
        if (DeleteAllInsideFlag)
        {
            //remove directory with all files and subdirectories
            result = DeleteDir(dirpath.c_str());
        }
        else
        {
            //try to remove as empty directory
            result = RemoveDirectory(dirpath.c_str());
        }

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }

    // 0B02=2, move_file %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_MoveFile(CScriptThread* thread)
    {
        auto filepath = ReadPathParam(thread);
        auto newFilepath = ReadPathParam(thread);

        BOOL result = GetFileAttributes(filepath.c_str()) & FILE_ATTRIBUTE_DIRECTORY;
        if (!result)
            result = MoveFile(filepath.c_str(), newFilepath.c_str());

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }

    // 0B03=2, move_directory %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_MoveDir(CScriptThread* thread)
    {
        auto filepath = ReadPathParam(thread);
        auto newFilepath = ReadPathParam(thread);

        BOOL result = GetFileAttributes(filepath.c_str()) & FILE_ATTRIBUTE_DIRECTORY;
        if (result)
            result = MoveFile(filepath.c_str(), newFilepath.c_str());

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }

    // 0B04=2, copy_file %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_CopyFile(CScriptThread* thread)
    {
        auto filepath = ReadPathParam(thread);
        auto newFilepath = ReadPathParam(thread);

        BOOL result = CopyFile(filepath.c_str(), newFilepath.c_str(), FALSE);
        if (result)
        {
            // copy file attributes
            DWORD fattr = GetFileAttributes(filepath.c_str());
            SetFileAttributes(newFilepath.c_str(), fattr);
        }

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }

    static BOOL CopyDir(const char *path, const char *newPath)
    {
        char mask[MAX_PATH];
        HANDLE hSearch = NULL;
        WIN32_FIND_DATA wfd;
        char subPath[MAX_PATH], newSubPath[MAX_PATH];
        DWORD fattr;

        //create parent directory
        if (!CreateDirectory(newPath, NULL))
            return FALSE;

        memset(&wfd, 0, sizeof(wfd));
        //search mask
        sprintf(mask, "%s\\*", path);

        //copy all files and folders into new directory
        if ((hSearch = FindFirstFile(mask, &wfd)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                sprintf(subPath, "%s\\%s", path, wfd.cFileName);
                sprintf(newSubPath, "%s\\%s", newPath, wfd.cFileName);
                //copy subdirectories
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if ((strcmp(wfd.cFileName, "..") != 0) && (strcmp(wfd.cFileName, ".") != 0))
                    {
                        if (!CopyDir(subPath, newSubPath))
                            return FALSE;
                    }
                }
                else
                {
                    //copy file into new directory
                    if (CopyFile(subPath, newSubPath, FALSE))
                    {
                        fattr = GetFileAttributes(subPath);
                        SetFileAttributes(newSubPath, fattr);
                    }
                    else	return FALSE;
                }


            } while (FindNextFile(hSearch, &wfd));
            FindClose(hSearch);
        }

        //return success
        return TRUE;
    }

    // 0B05=2, copy_directory %1d% to %2d% //IF and SET
    static OpcodeResult WINAPI Script_FS_CopyDir(CScriptThread* thread)
    {
        auto filepath = ReadPathParam(thread);
        auto newFilepath = ReadPathParam(thread);

        BOOL result = CopyDir(filepath.c_str(), newFilepath.c_str());

        CLEO_SetThreadCondResult(thread, result);
        return OR_CONTINUE;
    }
} fileSystemOperations;

std::set<DWORD> FileSystemOperations::m_hFiles;
std::set<HANDLE> FileSystemOperations::m_hFileSearches;
