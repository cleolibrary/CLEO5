#pragma once
#include <CTheScripts.h>
#include <CTxdStore.h>
#include <CLEO_Utils.h>
#include <array>

struct ScriptDrawsState
{
    eUseTextCommandState useTextCommands = eUseTextCommandState::DISABLED;

    WORD textsCount = 0;
    std::array<tScriptText, _countof(CTheScripts::IntroTextLines)> texts;

    WORD rectanglesCount = 0;
    std::array<tScriptRectangle, _countof(CTheScripts::IntroRectangles)> rectangles;

    std::array<CSprite2d, _countof(CTheScripts::ScriptSprites)> sprites;

    TxdDef scriptTxd;                                   // current script.txd
    std::unordered_map<std::string, TxdDef> loadedTxds; // texture dictionaries loaded by script

    ScriptDrawsState()                        = default;
    ScriptDrawsState(const ScriptDrawsState&) = delete; // no copying!
    ~ScriptDrawsState()                       = default;

    void Store()
    {
        useTextCommands = CTheScripts::UseTextCommands;

        textsCount = CTheScripts::NumberOfIntroTextLinesThisFrame;
        std::memcpy(texts.data(), CTheScripts::IntroTextLines, texts.size() * sizeof(tScriptText));

        rectanglesCount = CTheScripts::NumberOfIntroRectanglesThisFrame;
        std::memcpy(rectangles.data(), CTheScripts::IntroRectangles, rectangles.size() * sizeof(tScriptRectangle));

        std::memcpy(sprites.data(), CTheScripts::ScriptSprites, sprites.size() * sizeof(CSprite2d));

        // store script.txd
        scriptTxd = *FindScriptTxd();
    }

    void Reset()
    {
        CTheScripts::UseTextCommands = (useTextCommands == eUseTextCommandState::DISABLE_NEXT_FRAME)
                                           ? eUseTextCommandState::DISABLED
                                           : useTextCommands;

        // texts
        CTheScripts::NumberOfIntroTextLinesThisFrame = 0;
        std::fill(
            CTheScripts::IntroTextLines, CTheScripts::IntroTextLines + _countof(CTheScripts::IntroTextLines),
            tScriptText()
        );

        // rectangles
        CTheScripts::NumberOfIntroRectanglesThisFrame = 0;
        std::fill(
            CTheScripts::IntroRectangles, CTheScripts::IntroRectangles + _countof(CTheScripts::IntroRectangles),
            tScriptRectangle()
        );

        // loaded textures
        RestoreScriptTextures();
    }

    void Apply()
    {
        CTheScripts::UseTextCommands = useTextCommands;

        // texts
        CTheScripts::NumberOfIntroTextLinesThisFrame = textsCount;
        std::memcpy(CTheScripts::IntroTextLines, texts.data(), texts.size() * sizeof(tScriptText));

        // rectangles
        CTheScripts::NumberOfIntroRectanglesThisFrame = rectanglesCount;
        std::memcpy(CTheScripts::IntroRectangles, rectangles.data(), rectangles.size() * sizeof(tScriptRectangle));

        // loaded textures
        RestoreScriptTextures();
    }

    void RestoreScriptTextures()
    {
        // restore script.txd
        *FindScriptTxd() = scriptTxd;
        std::memcpy(CTheScripts::ScriptSprites, sprites.data(), sprites.size() * sizeof(CSprite2d));
    }

    bool IsEmpty() const { return !rectanglesCount && !textsCount; }

    TxdDef* FindScriptTxd()
    {
        TRACE(
            "ScriptDrawsState::FindScriptTxd: TxdPool size = %d, free = %d",
            CTxdStore::ms_pTxdPool->GetNoOfUsedSpaces(), CTxdStore::ms_pTxdPool->GetNoOfFreeSpaces()
        );

        auto slot = CTxdStore::FindTxdSlot("script");
        TRACE("ScriptDrawsState::FindScriptTxd: script txd slot = %d", slot);
        if (slot == -1)
        {
            slot = CTxdStore::AddTxdSlot("script");
            TRACE("ScriptDrawsState::FindScriptTxd: added script txd slot = %d", slot);
        }
        auto pTxd = CTxdStore::ms_pTxdPool->GetAt(slot);
        TRACE("ScriptDrawsState::FindScriptTxd: script txd = %p", pTxd);
        return pTxd;
    }
};
