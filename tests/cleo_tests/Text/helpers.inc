function toggle_subtitles(flag: int)
    const menuManager = 0xBA6748
    const bShowSubtitles = 0x44
    write_memory_with_offset {address} menuManager {offset} bShowSubtitles {size} 1 {value} flag
end

function assert_subtitle(str: string)
    int str1 = getCurrSubtitleText()
    if str == 0
    then
        assert_eq(str1, 0)
    else
        assert_eqs(str1, str)
    end
end

function assert_brief_flag(expected: int)
    int actual = read_memory {address} 0x00A4448B {size} 1 {vp} false // CTheScripts::bAddNextMessageToPreviousBriefs
    assert_eq(actual, expected) // flag must be set to false
end

function assert_brief_at(index: int, expected: string)
    int str = getBriefTextAt(index)
    assert_eqs(str, expected)
end


function CMessages_ClearPreviousBriefArray<cdecl, 0x69DE70>()

// get pointer to current subtitle text
function getCurrSubtitleText(): int
    int txt = read_memory {address} 0x00C1A7F0 {size} 4 {vp} false // GTA SA 1.0: CMessages::BriefMessages[0].text
    return txt
end

// get pointer to previous brief
function getBriefTextAt(index: int): int
    int offset = index * 0x20 // sizeof(PreviousBrief) = 0x20, m_pText at offset 0
    int txt = read_memory_with_offset {address} 0x00C1A570 {offset} offset {size} 4 // GTA SA 1.0: CMessages::PreviousBriefs[index].m_pText
    return txt
end