#include "stdafx.h"
#include "CGameMenu.h"
#include "CleoBase.h"
#include "CDebug.h"
#include "CFont.h"
#include "plugin.h"
#include <sstream>

namespace CLEO
{
    CMenuManager* MenuManager;

    void __declspec(naked) CTexture_DrawInRect_Hook()
    {
        __asm
        {
            push ebp
            mov ebp, esp
            push ecx
            push dword ptr [ebp + 0x08]
            push dword ptr [ebp + 0x04]
            mov ecx, dword ptr [ebp + 0x0C]
            call CTexture__DrawInRect
            pop ebp
            ret 0x04
        }
    }

    void __fastcall OnDrawMenuBackground(void *texture, int dummy, RwRect2D *rect, RwRGBA *color)
    {
        _asm push ebp
        _asm mov ebp, esp

        // Call original CTexture_DrawInRect function
        _asm
        {
            push color
            push rect
            mov ecx, texture
            call CTexture__DrawInRect
        }

        // Set font properties
        CFont::SetBackground(false, false);
        CFont::SetWrapx(640.0f);
        CFont::SetFontStyle(FONT_MENU);
        CFont::SetProportional(true);
        CFont::SetOrientation(ALIGN_LEFT);

        CFont::SetColor({ 0xAD, 0xCE, 0xC4, 0xFF });
        CFont::SetEdge(1);
        CFont::SetDropColor({ 0x00, 0x00, 0x00, 0xFF });

        // Calculate font size and positioning
        const float fontSize = 0.5f / 448.0f;
        const float aspect = (float)RsGlobal.maximumWidth / RsGlobal.maximumHeight;
        const float subtextHeight = 0.75f; // factor of first line height
        float sizeX = fontSize * 0.5f / aspect * RsGlobal.maximumWidth;
        float sizeY = fontSize * RsGlobal.maximumHeight;

        float posX = 25.0f * sizeX; // left margin
        float posY = RsGlobal.maximumHeight - 15.0f * sizeY; // bottom margin

        // Check if any scripts or plugins are active
        auto cs_count = GetInstance().ScriptEngine.WorkingScriptsCount();
        auto plugin_count = GetInstance().PluginSystem.GetNumPlugins();
        if (cs_count || plugin_count)
        {
            posY -= 15.0f * sizeY; // add space for bottom text
        }

        // Draw CLEO version text
        std::ostringstream text;
        text &lt;&lt; "CLEO v" &lt;&lt; CLEO_VERSION_STR;
#ifdef _DEBUG
        text &lt;&lt; " ~r~~h~DEBUG";
#endif
        CFont::SetScale(sizeX, sizeY);
        CFont::PrintString(posX, posY - 15.0f * sizeY, text.str().c_str());

        if (cs_count || plugin_count)
        {
            text.str("");
            if (plugin_count) text &lt;&lt; plugin_count &lt;&lt; (plugin_count &gt; 1 ? " plugins" : " plugin");
            if (cs_count &amp;&amp; plugin_count) text &lt;&lt; " / ";
            if (cs_count) text &lt;&lt; cs_count &lt;&lt; (cs_count &gt; 1 ? " scripts" : " script");

            posY += 15.0f * sizeY; 
            sizeX *= subtextHeight;
            sizeY *= subtextHeight;
            CFont::SetScale(sizeX, sizeY);
            CFont::PrintString(posX, posY - 15.0f * sizeY, text.str().c_str());
        }

        _asm pop ebp
        _asm ret 0x04
    }

    void CGameMenu::Inject(CCodeInjector&amp; inj)
    {
        TRACE("Injecting MenuStatusNotifier...");
        CGameVersionManager&amp; gvm = GetInstance().VersionManager;
        MenuManager = gvm.TranslateMemoryAddress(MA_MENU_MANAGER);

        inj.MemoryNop(gvm.TranslateMemoryAddress(MA_CALL_CTEXTURE_DRAW_BG_RECT).address + 1, 5);
        inj.MemoryJump(gvm.TranslateMemoryAddress(MA_CALL_CTEXTURE_DRAW_BG_RECT).address + 1, (void*)CTexture_DrawInRect_Hook, true);

        inj.ReplaceFunction(OnDrawMenuBackground, gvm.TranslateMemoryAddress(MA_CALL_CTEXTURE_DRAW_BG_RECT));
    }
}
