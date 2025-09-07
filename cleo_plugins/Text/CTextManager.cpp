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
    CTextManager::CTextManager() : fxts(1, crc32FromUpcaseStdString) {}

    const char* CTextManager::Get(const char* key)
    {
        return TheText.Get(key);
    }

    bool CTextManager::AddFxt(const char* key, const char* value, bool dynamic)
    {
        auto fxt = fxts.find(key);

        if (fxt != fxts.end())
        {
            if (fxt->second->text.compare(value) == 0) return true; // already present

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

    bool CTextManager::RemoveFxt(const char* key)
    {
        return fxts.erase(key) != 0;
    }

    const char* CTextManager::LocateFxt(const char* key)
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
            else
                ++it;
        }
    }

    CTextManager::~CTextManager()
    {
        TRACE(""); // separator
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
        TRACE(""); // separator
        TRACE("Loading CLEO text files...");

        // create FXT directory if not present yet
        FS::create_directory(std::string(CLEO_GetGameDirectory()) + "\\cleo\\cleo_text");

        // load whole FXT files directory
        auto list = CLEO::CLEO_ListDirectory(nullptr, "cleo\\cleo_text\\*.fxt", false, true);
        for (DWORD i = 0; i < list.count; i++)
        {
            try
            {
                std::ifstream stream(list.strings[i]);
                auto result = ParseFxtFile(stream);
                TRACE(" Added %d new FXT entries from file '%s'", result, list.strings[i]);
            }
            catch (std::exception& ex)
            {
                LOG_WARNING(0, " Loading of FXT file '%s' failed: \n%s", list.strings[i], ex.what());
            }
        }
        CLEO::CLEO_StringListFree(list);

        TRACE(""); // separator
    }

    void CTextManager::Clear()
    {
        fxts.clear();
    }

    CTextManager::FxtEntry::FxtEntry(const char* _text, bool _static) : text(_text), is_static(_static) {}

    size_t CTextManager::ParseFxtFile(std::istream& stream, bool dynamic, bool remove)
    {
        static char buf[0x100];
        char *key_iterator, *value_iterator, *value_start, *key_start;
        stream.exceptions(std::ios::badbit);

        size_t keyCount = 0;
        while (true)
        {
            if (stream.eof()) break;

            stream.getline(buf, sizeof(buf));
            if (stream.fail()) break;

            // parse extracted line
            key_start = key_iterator = buf;
            while (*key_iterator)
            {
                if (*key_iterator == '#') // start of comment
                    break;
                if (*key_iterator == '/' && key_iterator[1] == '/') // comment
                    break;

                if (isspace(*key_iterator))
                {
                    *key_iterator = '\0'; // terminate key string

                    if (remove)
                    {
                        if (RemoveFxt(key_start))
                        {
                            keyCount++;
                        }
                        break;
                    }

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
                    if (AddFxt(key_start, value_start, dynamic))
                    {
                        keyCount++;
                    }

                    break;
                }
                key_iterator++;
            }
        }

        return keyCount;
    }
} // namespace CLEO
