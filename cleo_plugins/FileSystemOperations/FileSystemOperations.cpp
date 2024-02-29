#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"
#include "FileUtils.h"

#include <set>

using namespace CLEO;
using namespace plugin;

#define READ_FILE_HANDLE_PARAM() CLEO_GetIntOpcodeParam(thread); \
    if((size_t)handle <= MinValidAddress) \
    { auto info = ScriptInfoStr(thread); SHOW_ERROR("Invalid '0x%X' file handle param in script %s \nScript suspended.", handle, info.c_str()); return thread->Suspend(); } \
    else if(m_hFiles.find(handle) == m_hFiles.end()) { auto info = ScriptInfoStr(thread); SHOW_ERROR("Invalid or already closed '0x%X' file handle param in script %s \nScript suspended.", handle, info.c_str()); return thread->Suspend(); }

class FileSystemOperations 
{
public:
    static std::set<DWORD> m_hFiles;
    static std::set<HANDLE> m_hFileSearches;

    static void WINAPI OnFinalizeScriptObjects()
    {
        // clean up opened files
        for (auto handle : m_hFiles) File::close(handle);
        m_hFiles.clear();

        // clean up file searches
        for (auto handle : m_hFileSearches) FindClose(handle);
        m_hFileSearches.clear();
    }

    FileSystemOperations() 
    {
        auto cleoVer = CLEO_GetVersion();
        if (cleoVer < CLEO_VERSION)
        {
            auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
            MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
            return;
        }

        File::initialize(CLEO_GetGameVersion()); // file utils

        //register opcodes
        CLEO_RegisterOpcode(0x0A99, opcode_0A99); // set_current_directory
        CLEO_RegisterOpcode(0x0A9A, opcode_0A9A); // openfile
        CLEO_RegisterOpcode(0x0A9B, opcode_0A9B); // closefile
        CLEO_RegisterOpcode(0x0A9C, opcode_0A9C); // get_file_size
        CLEO_RegisterOpcode(0x0A9D, opcode_0A9D); // read_from_file
        CLEO_RegisterOpcode(0x0A9E, opcode_0A9E); // write_to_file 

        CLEO_RegisterOpcode(0x0AAB, Script_FS_FileExists);
        CLEO_RegisterOpcode(0x0AE4, Script_FS_DirectoryExists);
        CLEO_RegisterOpcode(0x0AE5, Script_FS_CreateDirectory);
        CLEO_RegisterOpcode(0x0AE6, Script_FS_FindFirstFile);
        CLEO_RegisterOpcode(0x0AE7, Script_FS_FindNextFile);
        CLEO_RegisterOpcode(0x0AE8, Script_FS_FindClose);

        CLEO_RegisterOpcode(0x0AD5, opcode_0AD5); // file_seek
        CLEO_RegisterOpcode(0x0AD6, opcode_0AD6); // is_end_of_file_reached
        CLEO_RegisterOpcode(0x0AD7, opcode_0AD7); // read_string_from_file
        CLEO_RegisterOpcode(0x0AD8, opcode_0AD8); // write_string_to_file
        CLEO_RegisterOpcode(0x0AD9, opcode_0AD9); // write_formatted_string_to_file
        CLEO_RegisterOpcode(0x0ADA, opcode_0ADA); // scan_file

        CLEO_RegisterOpcode(0x0B00, Script_FS_DeleteFile);
        CLEO_RegisterOpcode(0x0B01, Script_FS_DeleteDirectory);
        CLEO_RegisterOpcode(0x0B02, Script_FS_MoveFile);
        CLEO_RegisterOpcode(0x0B03, Script_FS_MoveDir);
        CLEO_RegisterOpcode(0x0B04, Script_FS_CopyFile);
        CLEO_RegisterOpcode(0x0B05, Script_FS_CopyDir);

        CLEO_RegisterOpcode(0x2300, opcode_2300); // get_file_position
        CLEO_RegisterOpcode(0x2301, opcode_2301); // read_block_from_file
        CLEO_RegisterOpcode(0x2302, opcode_2302); // resolve_filepath
        CLEO_RegisterOpcode(0x2303, opcode_2303); // get_script_filename

        // register event callbacks
        CLEO_RegisterCallback(eCallbackId::ScriptsFinalize, OnFinalizeScriptObjects);
    }

    //0A99=1,set_current_directory %1b:userdir/rootdir%
    static OpcodeResult __stdcall opcode_0A99(CRunningScript* thread)
    {
        const char* path;

        auto paramType = CLEO_GetOperandType(thread);
        if (IsImmInteger(paramType) || IsVariable(paramType))
        {
            // numbered predefined paths
            auto idx = OPCODE_READ_PARAM_INT();
            switch (idx)
            {
                case 0: path = DIR_GAME; break;
                case 1: path = DIR_USER; break;
                case 2: path = DIR_SCRIPT; break;
                default:
                    LOG_WARNING(0, "Value (%d) not known by opcode [0A99] in script %s", idx, ScriptInfoStr(thread).c_str());
                    return OR_CONTINUE;
            }

            CLEO_SetScriptWorkDir(thread, path);
            return OR_CONTINUE;
        }
        else
        {
            OPCODE_READ_PARAM_STRING(str);
            path = str;
        }

        CLEO_SetScriptWorkDir(thread, path);
        return OR_CONTINUE;
    }

    //0A9A=3,%3d% = openfile %1d% mode %2d% // IF and SET
    static OpcodeResult WINAPI opcode_0A9A(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

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
            param.uParam = OPCODE_READ_PARAM_INT();
            strcpy(mode, param.strParam);
        }
        else
        {
            OPCODE_READ_PARAM_STRING_LEN(strMode, 15);
            strcpy(mode, strMode);
        }

        // either CLEO 3 or CLEO 4 made a big mistake! (they differ in one major unapparent preference)
        // lets try to resolve this with a legacy mode
        bool legacy = CLEO_GetScriptVersion(thread) < CLEO_VER_4_3;

        auto handle = File::open(filename, mode, legacy);
        if (!File::isOk(handle))
        {
            OPCODE_WRITE_PARAM_INT(0);
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        m_hFiles.insert(handle);

        OPCODE_WRITE_PARAM_INT(handle);
        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    //0A9B=1,closefile %1d%
    static OpcodeResult WINAPI opcode_0A9B(CRunningScript* thread)
    {
        DWORD handle = READ_FILE_HANDLE_PARAM();

        if (m_hFiles.find(handle) != m_hFiles.end())
        {
            File::close(handle);
            m_hFiles.erase(handle);
        }
        return OR_CONTINUE;
    }

    //0A9C=2,get_file_size %1d% store_to %2d%
    static OpcodeResult WINAPI opcode_0A9C(CRunningScript* thread)
    {
        DWORD handle = READ_FILE_HANDLE_PARAM();

        auto size = File::getSize(handle);

        OPCODE_WRITE_PARAM_INT(size);
        return OR_CONTINUE;
    }

    //0A9D=3,read_from_file %1d% size %2d% store_to %3d%
    static OpcodeResult WINAPI opcode_0A9D(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        auto size = OPCODE_READ_PARAM_INT();
        auto destination = OPCODE_READ_PARAM_OUTPUT_VAR();

        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        destination->dwParam = 0; // clear not overwritten bytes - https://github.com/cleolibrary/CLEO4/issues/91
        if (size > 0) File::read(handle, destination, size);
        return OR_CONTINUE;
    }

    //0A9E=3,write_to_file %1d% size %2d% from %3d%
    static OpcodeResult WINAPI opcode_0A9E(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        auto size = OPCODE_READ_PARAM_INT();

        if (size < 0)
        {
            SHOW_ERROR("Invalid '%d' size argument in script %s\nScript suspended.", size, ScriptInfoStr(thread).c_str());
            return thread->Suspend();
        }

        if (size == 0)
        {
            return OR_CONTINUE; // done
        }

        const void* source;
        auto paramType = thread->PeekDataType();
        if (IsVariable(paramType))
        {
            source = CLEO_GetPointerToScriptVariable(thread);
        }
        else if(IsImmString(paramType) || IsVarString(paramType))
        {
            static char buffer[MAX_STR_LEN];

            if (size > MAX_STR_LEN)
            {
                SHOW_ERROR("Size argument (%d) greater than supported (%d) in script %s\nScript suspended.", size, MAX_STR_LEN, ScriptInfoStr(thread).c_str());
                return thread->Suspend();
            }

            ZeroMemory(buffer, size); // padd with zeros if size > length
            source = CLEO_ReadStringOpcodeParam(thread, buffer, sizeof(buffer));
        }
        else
        {
            if (size > sizeof(SCRIPT_VAR))
            {
                SHOW_ERROR("Size argument (%d) greater than supported (%d) in script %s\nScript suspended.", size, sizeof(SCRIPT_VAR), ScriptInfoStr(thread).c_str());
                return thread->Suspend();
            }

            CLEO_RetrieveOpcodeParams(thread, 1);
            source = CLEO_GetOpcodeParamsArray();
        }

        File::write(handle, source, size);
        if (File::isOk(handle)) File::flush(handle);
        return OR_CONTINUE;
    }

    // 0AAB=1,  does_file_exist %1s%
    static OpcodeResult WINAPI Script_FS_FileExists(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

        DWORD fAttr = GetFileAttributes(filename);
        bool exists = (fAttr != INVALID_FILE_ATTRIBUTES) && !(fAttr & FILE_ATTRIBUTE_DIRECTORY);

        OPCODE_CONDITION_RESULT(exists);
        return OR_CONTINUE;
    }

    //0AD5=3,  file_seek %1d% offset %2d% origin %3d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD5(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        auto offset = OPCODE_READ_PARAM_INT();
        auto origin = OPCODE_READ_PARAM_UINT();

        auto ok = File::seek(handle, offset, origin);

        OPCODE_CONDITION_RESULT(ok);
        return OR_CONTINUE;
    }

    //0AD6=1,  is_end_of_file_reached %1d%
    static OpcodeResult WINAPI opcode_0AD6(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();

        bool end = !File::isOk(handle) || File::isEndOfFile(handle);

        OPCODE_CONDITION_RESULT(end);
        return OR_CONTINUE;
    }

    //0AD7=3,  read_string_from_file %1d% to %2d% size %3d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD7(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();

        char* buffer = nullptr;
        int bufferSize = 0;
        DWORD needsTerminator = TRUE;
        CLEO_ReadStringParamWriteBuffer(thread, &buffer, &bufferSize, &needsTerminator);

        auto size = OPCODE_READ_PARAM_INT();

        if (size < 0)
        {
            auto info = ScriptInfoStr(thread);
            SHOW_ERROR("Invalid size argument (%d) in script %s\nScript suspended.", size, info.c_str());
            return thread->Suspend();
        }

        if (size == 0)
        {
            if (bufferSize > 0) buffer[0] = '\0';
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        std::vector<char> tmpBuff;
        tmpBuff.resize(size);
        auto data = tmpBuff.data();

        bool ok = File::readString(handle, data, size) != nullptr;
        if(!ok)
        {
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        // copy into result param
        int len = strlen(data);
        int resultSize = min(len, bufferSize - (int)needsTerminator);

        memcpy(buffer, data, resultSize);
        if(resultSize < bufferSize) buffer[resultSize] = '\0'; // terminate string whenever possible

        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    //0AD8=2,  write_string_to_file %1d% from %2d% //IF and SET
    static OpcodeResult WINAPI opcode_0AD8(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        OPCODE_READ_PARAM_STRING(text);

        auto ok = File::writeString(handle, text);
        if (!ok)
        {
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        File::flush(handle);
        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    //0AD9=-1,write_formated_text %2d% to_file %1d%
    static OpcodeResult WINAPI opcode_0AD9(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        OPCODE_READ_PARAM_STRING(format);
        static char text[4 * MAX_STR_LEN]; CLEO_ReadParamsFormatted(thread, format, text, MAX_STR_LEN);
        
        auto ok = File::writeString(handle, text);
        if (!ok)
        {
            return OR_CONTINUE;
        }

        File::flush(handle);
        return OR_CONTINUE;
    }

    //0ADA=-1,  %3d% = scan_file %1d% format %2d% //IF and SET
    static OpcodeResult WINAPI opcode_0ADA(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        OPCODE_READ_PARAM_STRING(format);
        auto result = OPCODE_READ_PARAM_OUTPUT_VAR();

        size_t paramCount = 0;
        SCRIPT_VAR* outputParams[35];
        while (CLEO_GetOperandType(thread) != eDataType::DT_END)
        {
            // TODO: if target param is string variable it should be handled correctly
            outputParams[paramCount++] = CLEO_GetPointerToScriptVariable(thread);
        }
        CLEO_SkipUnusedVarArgs(thread); // var arg terminator

        result->dwParam = File::scan(handle, format, (void**)&outputParams);

        //OPCODE_CONDITION_RESULT(paramCount == result->dwParam);
        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    // 0AE4=1,  directory_exist %1s%
    static OpcodeResult WINAPI Script_FS_DirectoryExists(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

        DWORD fAttr = GetFileAttributes(filename);
        bool exists = (fAttr != INVALID_FILE_ATTRIBUTES) && (fAttr & FILE_ATTRIBUTE_DIRECTORY);

        OPCODE_CONDITION_RESULT(exists);
        return OR_CONTINUE;
    }

    // 0AE5=1,  create_directory %1s% //IF and SET
    static OpcodeResult WINAPI Script_FS_CreateDirectory(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

        bool result = CreateDirectory(filename, NULL) != 0;

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    // 0AE6=3,  %2d% = find_first_file %1s% get_filename_to %3s% //IF and SET
    static OpcodeResult WINAPI Script_FS_FindFirstFile(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

        WIN32_FIND_DATA ffd = { 0 };
        HANDLE handle = FindFirstFile(filename, &ffd);

        if (handle == INVALID_HANDLE_VALUE) // -1
        {
            OPCODE_WRITE_PARAM_INT(-1); // invalid handle
            OPCODE_SKIP_PARAMS(1); // filename
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        m_hFileSearches.insert(handle);

        OPCODE_WRITE_PARAM_INT(handle);
        OPCODE_WRITE_PARAM_STRING(ffd.cFileName);
        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    // 0AE7=2,%2s% = find_next_file %1d% //IF and SET
    static OpcodeResult WINAPI Script_FS_FindNextFile(CRunningScript* thread)
    {
        auto handle = (HANDLE)OPCODE_READ_PARAM_INT();

        if (m_hFileSearches.find(handle) == m_hFileSearches.end())
        {
            LOG_WARNING(thread, "Invalid or already closed file search handle (0x%X) in script %s", handle, ScriptInfoStr(thread).c_str());
            OPCODE_SKIP_PARAMS(1);
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        WIN32_FIND_DATA ffd = { 0 };
        if (!FindNextFile(handle, &ffd))
        {
            OPCODE_SKIP_PARAMS(1);
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        OPCODE_WRITE_PARAM_STRING(ffd.cFileName);
        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    // 0AE8=1,find_close %1d%
    static OpcodeResult WINAPI Script_FS_FindClose(CRunningScript* thread)
    {
        auto handle = (HANDLE)OPCODE_READ_PARAM_INT();

        if (m_hFileSearches.find(handle) == m_hFileSearches.end())
        {
            LOG_WARNING(thread, "Invalid or already closed file search handle (0x%X) in script %s", handle, ScriptInfoStr(thread).c_str());
            return OR_CONTINUE;
        }

        FindClose(handle);
        m_hFileSearches.erase(handle);
        return OR_CONTINUE;
    }

    // 0B00=1,  delete_file %1s% //IF and SET
    static OpcodeResult WINAPI Script_FS_DeleteFile(CScriptThread* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filename);

        auto success = DeleteFile(filename);

        OPCODE_CONDITION_RESULT(success);
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
        OPCODE_READ_PARAM_FILEPATH(filename);
        auto deleteContents = OPCODE_READ_PARAM_BOOL();

        BOOL result;
        if (deleteContents)
        {
            // remove directory with all files and subdirectories
            result = DeleteDir(filename);
        }
        else
        {
            // try to remove as empty directory
            result = RemoveDirectory(filename);
        }

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    // 0B02=2, move_file %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_MoveFile(CScriptThread* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filepath);
        OPCODE_READ_PARAM_FILEPATH(newFilepath);

        BOOL result = GetFileAttributes(filepath) & FILE_ATTRIBUTE_DIRECTORY;
        if (!result)
            result = MoveFile(filepath, newFilepath);

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    // 0B03=2, move_directory %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_MoveDir(CScriptThread* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filepath);
        OPCODE_READ_PARAM_FILEPATH(newFilepath);

        BOOL result = GetFileAttributes(filepath) & FILE_ATTRIBUTE_DIRECTORY;
        if (result)
            result = MoveFile(filepath, newFilepath);

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    // 0B04=2, copy_file %1s% to %2s% //IF and SET
    static OpcodeResult WINAPI Script_FS_CopyFile(CScriptThread* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filepath);
        OPCODE_READ_PARAM_FILEPATH(newFilepath);

        BOOL result = CopyFile(filepath, newFilepath, FALSE);
        if (result)
        {
            // copy file attributes
            DWORD fattr = GetFileAttributes(filepath);
            SetFileAttributes(newFilepath, fattr);
        }

        OPCODE_CONDITION_RESULT(result);
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

        return TRUE;
    }

    // 0B05=2,  copy_directory %1d% to %2d% //IF and SET
    static OpcodeResult WINAPI Script_FS_CopyDir(CScriptThread* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(filepath);
        OPCODE_READ_PARAM_FILEPATH(newFilepath);

        BOOL result = CopyDir(filepath, newFilepath);

        OPCODE_CONDITION_RESULT(result);
        return OR_CONTINUE;
    }

    //2300=2,get_file_position %1d% store_to %2d%
    static OpcodeResult WINAPI opcode_2300(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();

        auto pos = File::getPos(handle);

        OPCODE_WRITE_PARAM_INT(pos);
        return OR_CONTINUE;
    }

    //2301=3,read_block_from_file %1d% size %2d% buffer %3d% // IF and SET
    static OpcodeResult WINAPI opcode_2301(CRunningScript* thread)
    {
        auto handle = READ_FILE_HANDLE_PARAM();
        auto size = OPCODE_READ_PARAM_INT();
        auto destination = OPCODE_READ_PARAM_PTR();

        if (size < 0)
        {
            auto info = ScriptInfoStr(thread);
            SHOW_ERROR("Invalid size argument (%d) in script %s\nScript suspended.", size, info.c_str());
            return thread->Suspend();
        }

        if (size == 0)
        {
            OPCODE_CONDITION_RESULT(true); // done
            return OR_CONTINUE;
        }

        auto readCount = File::read(handle, destination, size);
        if (readCount != size)
        {
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }

    //2302=2,%2s% = resolve_filepath %1s%
    static OpcodeResult __stdcall opcode_2302(CRunningScript* thread)
    {
        OPCODE_READ_PARAM_FILEPATH(path); // it also resolves the path to absolute form

        OPCODE_WRITE_PARAM_STRING(path);
        return OR_CONTINUE;
    }

    //2303=3,%3s% = get_script_filename %1d% full_path %2d% // IF and SET
    static OpcodeResult __stdcall opcode_2303(CRunningScript* thread)
    {
        auto script = OPCODE_READ_PARAM_INT();
        auto fullPath = OPCODE_READ_PARAM_BOOL();

        if (script == -1) // special case: current script
        {
            script = (int)thread;
        }
        else
        {
            OPCODE_VALIDATE_POINTER(script);
        }

        const char* filename = CLEO_GetScriptFilename((CRunningScript*)script);
        if (filename == nullptr)
        {
            OPCODE_SKIP_PARAMS(1);
            OPCODE_CONDITION_RESULT(false);
            return OR_CONTINUE;
        }

        if (!fullPath)
        {
            OPCODE_WRITE_PARAM_STRING(filename);
        }
        else
        {
            std::string absolute = ".\\";
            absolute += filename;
            absolute.resize(MAX_STR_LEN);
            CLEO_ResolvePath((CRunningScript*)script, absolute.data(), MAX_STR_LEN);
            OPCODE_WRITE_PARAM_STRING(absolute.c_str());
        }

        OPCODE_CONDITION_RESULT(true);
        return OR_CONTINUE;
    }
} fileSystemOperations;

std::set<DWORD> FileSystemOperations::m_hFiles;
std::set<HANDLE> FileSystemOperations::m_hFileSearches;
