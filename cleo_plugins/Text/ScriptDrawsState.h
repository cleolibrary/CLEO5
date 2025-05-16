#pragma once
#include <CTheScripts.h>
#include <extensions/Screen.h>
#include <array>

struct ScriptDrawsState
{
    static constexpr size_t Script_Rectangles_Capacity = 0x80;
    static constexpr size_t Script_Sprites_Capacity = 0x80;
    static constexpr size_t Script_Texts_Capacity = 0x60;

    eUseTextCommandState useTextCommands = eUseTextCommandState::DISABLED;
    
    WORD rectanglesCount = 0;
    std::array<tScriptRectangle, Script_Rectangles_Capacity> rectangles;

    std::array<CSprite2d, Script_Sprites_Capacity> sprites;

    WORD textsCount = 0;
    std::array<tScriptText, Script_Texts_Capacity> texts;

    void Store()
    {
        useTextCommands = CTheScripts::UseTextCommands;

        rectanglesCount = CTheScripts::NumberOfIntroRectanglesThisFrame;
        if (useTextCommands != eUseTextCommandState::DISABLED)
        {
            std::memcpy(rectangles.data(), CTheScripts::IntroRectangles, rectangles.size());
        }

        std::memcpy(sprites.data(), CTheScripts::ScriptSprites, sprites.size());

        if (useTextCommands != eUseTextCommandState::DISABLED)
        {
            std::memcpy(texts.data(), CTheScripts::IntroTextLines, texts.size());
        }
    }

    void Apply(bool clearState = false)
    {
        CTheScripts::UseTextCommands = useTextCommands;

        // rectangles
        if (clearState)
        {
            CTheScripts::NumberOfIntroRectanglesThisFrame = 0;
            std::fill(
                CTheScripts::IntroRectangles,
                CTheScripts::IntroRectangles + Script_Rectangles_Capacity,
                tScriptRectangle{});
        }
        else
        {
            CTheScripts::NumberOfIntroRectanglesThisFrame = rectanglesCount;
            std::memcpy(CTheScripts::IntroRectangles, rectangles.data(), rectangles.size());
        }

        // loaded textures
        std::memcpy(CTheScripts::ScriptSprites, sprites.data(), sprites.size());

        // texts
        if (clearState)
        {
            CTheScripts::NumberOfIntroTextLinesThisFrame = 0;
            std::fill(
                CTheScripts::IntroTextLines,
                CTheScripts::IntroTextLines + Script_Texts_Capacity,
                tScriptText{});
        }
        else
        {
            CTheScripts::NumberOfIntroTextLinesThisFrame = textsCount;
            std::memcpy(CTheScripts::IntroTextLines, texts.data(), texts.size());
        }
    }

    bool IsEmpty() const
    {
        return useTextCommands == eUseTextCommandState::DISABLED || (!rectanglesCount && !textsCount);
    }
}; 
