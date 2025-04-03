#include "plugin.h"
#include "CLEO.h"
#include "CLEO_Utils.h"

using namespace CLEO;
using namespace plugin;
using namespace std;

class GameEntities
{
public:
    GameEntities()
    {
        auto cleoVer = CLEO_GetVersion();
        if (cleoVer < CLEO_VERSION)
        {
            auto err = StringPrintf("This plugin requires version %X or later! \nCurrent version of CLEO is %X.", CLEO_VERSION >> 8, cleoVer >> 8);
            MessageBox(HWND_DESKTOP, err.c_str(), TARGET_NAME, MB_SYSTEMMODAL | MB_ICONERROR);
            return;
        }

        // register opcodes
        CLEO_RegisterOpcode(0x0A8E, opcode_0A8E); // x = a + b (int)
    }

    //0A8E=3,%3d% = %1d% + %2d% ; int
    static OpcodeResult WINAPI opcode_0A8E(CRunningScript* thread)
    {
        auto a = OPCODE_READ_PARAM_INT();
        auto b = OPCODE_READ_PARAM_INT();

        auto result = a + b;

        OPCODE_WRITE_PARAM_INT(result);
        return OR_CONTINUE;
    }
} Instance;
