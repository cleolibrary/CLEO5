#pragma once
#include "CCodeInjector.h"

namespace CLEO
{
    class CGameMenu
    {
    public:
        void Inject(CCodeInjector& inj);

    private:
        static void __fastcall HOOK_DrawMenuBackground(CSprite2d* texture, int dummy, CRect* rect, RwRGBA* color);
        void(__fastcall* DrawMenuBackground_Orig)(CSprite2d* texture, int dummy, CRect* rect, RwRGBA* color) = nullptr;
    };
}
