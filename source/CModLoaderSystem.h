namespace CLEO
{
    class CModLoaderSystem
    {
        bool initialized = false;
        bool active = false; // ModLoader present?

    public:
        CModLoaderSystem() = default;
        CModLoaderSystem(const CModLoaderSystem&) = delete; // no copying
        ~CModLoaderSystem() = default;
        void Init();

        bool IsActive() const;

        const void (__cdecl* StringListFree)(CLEO::StringList) = nullptr;

        const BOOL (__cdecl* ResolvePath)(const char* scriptPath, char* path, DWORD pathMaxSize) = nullptr;

        const CLEO::StringList (__cdecl* ListFiles)(const char* scriptPath, const char* searchPattern, BOOL listDirs, BOOL listFiles) = nullptr;
        const CLEO::StringList (__cdecl* ListCleoFiles)(const char* searchPattern) = nullptr;
        const CLEO::StringList(__cdecl* ListCleoStartupScripts)() = nullptr;
    };
}
