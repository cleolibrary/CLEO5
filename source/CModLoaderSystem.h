
namespace CLEO
{
    class CModLoaderSystem
    {
        bool active = false; // ModLoader present?

    public:
        CModLoaderSystem() = default;
        ~CModLoaderSystem() = default;

        void Init();
        void ResolvePath(char* buff, size_t buffSize) const;
    };
}
