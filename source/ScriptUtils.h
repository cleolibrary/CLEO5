#pragma once
#include "..\cleo_sdk\CLEO.h"

namespace CLEO
{
    // check for extra SCM data at the end of script block
    static DWORD GetExtraInfoSize(BYTE* scriptData, DWORD size)
    {
        static constexpr char SB_Footer_Sig[] = { '_', '_', 'S', 'B', 'F', 'T', 'R', '\0' };

        if (size < (sizeof(SB_Footer_Sig) + sizeof(DWORD))) return 0; // not enough data for signature + size

        auto ptr = scriptData + size; // data end
        ptr -= sizeof(SB_Footer_Sig);
        if (memcmp(ptr, &SB_Footer_Sig, sizeof(SB_Footer_Sig))) return 0; // signature not present

        ptr -= sizeof(DWORD);
        auto codeSize = *reinterpret_cast<DWORD*>(ptr);
        if (codeSize > size) return 0; // error: reported size greater than script block

        return size - codeSize;
    }
}