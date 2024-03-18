#include "CTextManager.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include "CFileMgr.h"
#include "CText.h"
#include <wtypes.h>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace FS = std::filesystem;

namespace CLEO
{
    CTextManager::CTextManager() : fxts(1, crc32FromUpcaseStdString)
    {
    }

    const char* CTextManager::Get(const char* key)
    {
        return TheText.Get(key);
    }

    bool CTextManager::AddFxt(const char *key, const char *value, bool dynamic)
    {
        // TODO: replace this part with in-place construction of FxtEntry,
        // when it will be implemented in libstdc++
        // ^^ me iz noob and doesnt can use GNU

        auto fxt = fxts.find(key);

        if (fxt != fxts.end())
        {
            if (!dynamic || fxt->second->is_static)
            {
                LOG_WARNING(0, "Attempting to add FXT \'%s\' - FAILED (GXT conflict)", key, value);
                return false;
            }

            fxt->second->text = value;
        }
        else
        {
            std::string str = key;
            std::transform(str.begin(), str.end(), str.begin(), ::toupper);
            fxts[str.c_str()] = new FxtEntry(value, !dynamic);
        }
        return true;
    }

    bool CTextManager::RemoveFxt(const char *key)
    {
        return fxts.erase(key) != 0;
    }

    const char *CTextManager::LocateFxt(const char *key)
    {
        std::string str = key;
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        const_fxt_iterator found = fxts.find(str);
        if (found == fxts.end()) return nullptr;
        return found->second->text.c_str();
    }

    void CTextManager::ClearDynamicFxts()
    {
        TRACE("Deleting dynamic fxts...");
        for (auto it = fxts.begin(); it != fxts.end();)
        {
            if (!it->second->is_static)
            {
                delete it->second;
                fxts.erase(it++);
            }
            else ++it;
        }
    }

    CTextManager::~CTextManager()
    {
        TRACE("Deleting FXTs...");
        size_t count = 0;
        for (auto it = fxts.begin(); it != fxts.end();)
        {
            delete it->second;
            fxts.erase(it++);
            ++count;
        }
    }

    void CTextManager::LoadFxts()
    {
        // create FXT directory if not present yet
        FS::create_directory(FS::path(CFileMgr::ms_rootDirName).append("cleo\\cleo_text"));

        // load whole FXT files directory
        auto list = CLEO::CLEO_ListDirectory(nullptr, "cleo\\cleo_text\\*.fxt", false, true);
        for (DWORD i = 0; i < list.count; i++)
        {
            try
            {
                std::ifstream stream(list.paths[i]);
                auto result = ParseFxtFile(stream);
                TRACE("Added %d new FXT entries from file %s", result, list.paths[i]);
            }
            catch (std::exception& ex)
            {
                LOG_WARNING(0, "Loading of FXT file '%s' failed: \n%s", list.paths[i], ex.what());
            }
        }
        CLEO::CLEO_ListDirectoryFree(list);
    }

    void CTextManager::Clear()
    {
        fxts.clear();
    }

    CTextManager::FxtEntry::FxtEntry(const char *_text, bool _static) : text(_text), is_static(_static)
    {
    }

    size_t CTextManager::ParseFxtFile(std::istream& stream)
    {
        static char buf[0x100];
        char *key_iterator, *value_iterator, *value_start, *key_start;
        stream.exceptions(std::ios::badbit);

        size_t addedCount = 0;
        while (true)
        {
            if (stream.eof()) break;

            stream.getline(buf, sizeof(buf));
            if (stream.fail()) break;

            // parse extracted line	
            key_start = key_iterator = buf;
            while (*key_iterator)
            {
                if (*key_iterator == '#')	// start of comment
                    break;
                if (*key_iterator == '/' && key_iterator[1] == '/')
                    break;
                if (isspace(*key_iterator))
                {
                    *key_iterator = '\0';
                    // while (isspace(*++key_iterator)) ; // skip leading spaces
                    value_start = value_iterator = key_iterator + 1;
                    while (*value_iterator)
                    {
                        // start of comment
                        if (*value_iterator == '#' || (*key_iterator == '/' && key_iterator[1] == '/'))
                        {
                            *value_iterator = '\0';
                            break;
                        }
                        break;
                        value_iterator++;
                    }

                    // register found fxt entry
                    if (AddFxt(key_start, value_start, false))
                    {
                        addedCount++;
                    }

                    break;
                }
                key_iterator++;
            }
        }

        return addedCount;
    }
}
