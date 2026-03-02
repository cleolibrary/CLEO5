#pragma once
#include <wtypes.h>
#include <map>
#include <string>

namespace CLEO
{
    class CRunningScript;

    class CTextManager
    {
      public:
        bool Add(const char* gxtKey, const char* text, bool dynamic = true);
        void LoadFxts(); // load all cleo\cleo_text\*.fxt

        bool Remove(const char* gxtKey, bool force = false); // force to remove non-dynamic
        void Clear();                                        // remove all

        size_t ParseFxtFile(const char* filename, bool remove = false);

        bool Contains(const char* gxtKey) const;
        const char* Get(const char* gxtKey) const; // get from this dictionary only

        // print warning if there is already entry with same hash but different GXT key
        void TestKeyCollision(const char* gxtKey, CLEO::CRunningScript* thread = nullptr) const;

      private:
        typedef DWORD GxtHash;
        static GxtHash GetHash(const char* gxtKey);

        struct FxtEntry
        {
            std::string key;
            bool dynamic; // created by script command
            std::string text;
        };
        std::map<GxtHash, FxtEntry> entries;

        bool Contains(GxtHash hash) const;
    };
} // namespace CLEO
