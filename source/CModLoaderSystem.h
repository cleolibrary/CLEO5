namespace CLEO
{
    class CModLoaderSystem
    {
        bool initialized = false;
        bool active = false;

    public:
        static bool IsInstalled(); // ModLoader.asi is present in the game

        CModLoaderSystem() = default;
        CModLoaderSystem(const CModLoaderSystem&) = delete; // no copying
        ~CModLoaderSystem() = default;
        
        void Init();
        bool IsActive() const; // initialized and working

        // resolve path taking in consideration ModLoader and this script's POV. Input must be absolute path (CLEO's virtual paths are not supported there!)
        const BOOL (__cdecl* ResolvePath)(const char* scriptPath, char* path, DWORD pathMaxSize) = nullptr;

        // general purpose files search. Result will also contain ModLoader files visible from this script's POV
        const CLEO::StringList (__cdecl* ListFiles)(const char* scriptPath, const char* searchPattern, BOOL listDirs, BOOL listFiles) = nullptr;

        // query interesting CLEO files found by ModLoader. Collected file types: cleo, cm, cs, cs3, cs4, fxt
        const CLEO::StringList (__cdecl* ListCleoFiles)(const char* searchPattern) = nullptr;

        // get cs|cs3|cs4 files that ModLoader pretends to be in cleo\ directory
        const CLEO::StringList (__cdecl* ListCleoStartupScripts)() = nullptr;

        // release StringList container
        const void(__cdecl* StringListFree)(CLEO::StringList) = nullptr;
    };
}
