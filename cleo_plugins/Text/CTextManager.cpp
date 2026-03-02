#include "CTextManager.h"
#include "..\cleo_sdk\CLEO_Utils.h"
#include "crc32.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace FS = std::filesystem;

namespace CLEO
{
    bool CTextManager::Add(const char* gxtKey, const char* text, bool dynamic)
    {
        auto hash = GetHash(gxtKey);

        if (dynamic && Contains(hash) && !entries[hash].dynamic)
            return false; // can not overwrite static entry with dynamic one

        auto& entry = entries[hash]; // get or create
        entry.key = gxtKey;
        entry.dynamic = dynamic;
        entry.text = text;

        return true;
    }

    bool CTextManager::Remove(const char* gxtKey, bool force)
    {
        auto hash = GetHash(gxtKey);

        if (!Contains(hash))
            return true; // already not present

        if (!force && !entries[hash].dynamic)
            return false; // protected key

        entries.erase(hash);
        return true;
    }

    void CTextManager::Clear()
    {
        entries.clear();
    }

    void CTextManager::LoadFxts()
    {
        TRACE(""); // separator
        TRACE("Loading CLEO text files...");

        // create FXT directory if not present yet
        FS::create_directory(string(CLEO_GetGameDirectory()) + "\\cleo\\cleo_text");

        // load whole FXT files directory
        auto list = CLEO::CLEO_ListDirectory(nullptr, "cleo\\cleo_text\\*.fxt", false, true);
        for (DWORD i = 0; i < list.count; i++)
        {
            try
            {
                auto result = ParseFxtFile(list.strings[i]);
                TRACE(" Added %d new FXT entries from file '%s'", result, list.strings[i]);
            }
            catch (exception& ex)
            {
                LOG_WARNING(0, " Loading of FXT file '%s' failed: \n%s", list.strings[i], ex.what());
            }
        }
        CLEO::CLEO_StringListFree(list);

        TRACE(""); // separator
    }

    size_t CTextManager::ParseFxtFile(const char* filename, bool remove)
    {
        array<char, 512> buff;

        char *key_iterator, *value_iterator, *value_start, *key_start;

        ifstream stream(filename);
        stream.exceptions(ios::badbit);

        size_t keyCount = 0;
        while (true)
        {
            if (stream.eof()) break;

            stream.getline(buff.data(), buff.size());
            if (stream.fail()) break;

            // parse extracted line
            key_start = key_iterator = buff.data();
            while (*key_iterator)
            {
                if (*key_iterator == '#') // start of comment
                    break;
                if (*key_iterator == '/' && key_iterator[1] == '/') // comment
                    break;

                if (isspace(*key_iterator))
                {
                    *key_iterator = '\0'; // terminate key string

                    // report collisions
                    TestKeyCollision(key_start);

                    if (remove)
                    {
                        if (Contains(key_start) && Remove(key_start, true))
                        {
                            keyCount++;
                        }
                        break;
                    }

                    value_start = value_iterator = key_iterator + 1;
                    while (*value_iterator)
                    {
                        value_iterator++;
                    }

                    // remove trailing spaces as the game fails to render a string with them
                    while (value_iterator > value_start && isspace((unsigned char)*(value_iterator - 1)))
                    {
                        value_iterator--;
                    }
                    *value_iterator = '\0';

                    // register found fxt entry
                    Add(key_start, value_start, false);
                    keyCount++;

                    break;
                }
                key_iterator++;
            }
        }

        return keyCount;
    }

    bool CTextManager::Contains(const char* gxtKey) const
    {
        return Contains(GetHash(gxtKey));
    }

    const char* CTextManager::Get(const char* gxtKey) const
    {
        auto entry = entries.find(GetHash(gxtKey));

        if (entry == entries.end())
            return nullptr; // not found

        return entry->second.text.c_str();
    }

    // example collision: 'UXDLX' and 'ALADAAB'
    void CTextManager::TestKeyCollision(const char* gxtKey, CLEO::CRunningScript* script) const
    {
        if (!IsStrictValidation(script))
            return;

        auto entry = entries.find(GetHash(gxtKey));
        if (entry == entries.end())
            return; // not found

        if (_stricmp(gxtKey, entry->second.key.c_str()))
        {
            LOG_WARNING(script,
                "GXT key collision detected! Hash of key '%s' matches with already present '%s'%s%s",
                gxtKey,
                entry->second.key.c_str(),
                script ? " in script " : "",
                script ? ScriptInfoStr(script).c_str() : "");
        }
    }

    CTextManager::GxtHash CTextManager::GetHash(const char* gxtKey)
    {
        return crc32FromUpcaseString(gxtKey);
    }

    bool CLEO::CTextManager::Contains(GxtHash hash) const
    {
        return entries.find(hash) != entries.end();
    }
}
