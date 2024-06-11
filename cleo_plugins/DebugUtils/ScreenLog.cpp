#include "ScreenLog.h"
#include "CLEO_Utils.h"
#include "CFont.h"
#include "CTimer.h"

DWORD ScreenLog::timeDisplay = 1000;

ScreenLog::ScreenLog()
{
    Init();
}

void ScreenLog::Init()
{
    // load settings from ini file
    auto config = GetConfigFilename();

    level = (eLogLevel)GetPrivateProfileInt("ScreenLog", "Level", (UINT)eLogLevel::None, config.c_str());
    maxMessages = GetPrivateProfileInt("ScreenLog", "MessagesMax", 40, config.c_str());
    timeDisplay = GetPrivateProfileInt("ScreenLog", "MessageTime", 6000, config.c_str());
    timeFadeout = 200;
    fontSize = 0.01f * GetPrivateProfileInt("ScreenLog", "FontSize", 60, config.c_str());
}

void ScreenLog::Add(eLogLevel level, const char* msg)
{
    if (level > this->level)
    {
        return;
    }

    Entry entry(level, msg);
    if(!entries.empty() && entries.front() == entry)
    {
        entries.front().Repeat(); // duplicated
    }
    else
    {
        entries.push_front(std::move(entry));

        // remove older messages if necessary
        auto lines = CountLines(entry.msg);
        for (size_t i = 1; i < entries.size(); i++)
        {
            lines += CountLines(entries[i].msg);

            if (lines > maxMessages)
            {
                entries.resize(i);
                break;
            }
        }
    }
}

void ScreenLog::Draw()
{
    // remove expired messages
    while (!entries.empty() && entries.back().timeLeft < (-0.001f * timeFadeout))
    {
        entries.pop_back();
    }

    if (entries.empty())
    {
        return; // nothing to print
    }

    CFont::SetBackground(false, false);
    CFont::SetWrapx(99999999.0f); // no line wrap
    CFont::SetFontStyle(FONT_SUBTITLES);
    CFont::SetEdge(1);
    CFont::SetProportional(true);

    const float aspect = (float)RsGlobal.maximumWidth / RsGlobal.maximumHeight;
    float sizeX = fontSize * 0.55f * RsGlobal.maximumWidth / 640.0f / aspect;
    float sizeY = fontSize * RsGlobal.maximumHeight / 448.0f;
    CFont::SetScale(sizeX, sizeY);
    const float Row_Height = 18.0f;

    // calculate animation scroll pos
    float scroll = 0.0f;
    auto last = entries.back();
    if (last.timeLeft < 0.0f)
    {
        float progress = -last.timeLeft / (0.001f * timeFadeout);
        scroll = progress * CountLines(last.msg) * Row_Height * sizeY;
    }

    CFont::SetOrientation(ALIGN_LEFT);
    float posX = 15.0f * sizeX;
    float posY = 7.0f * sizeY - scroll;

    int lines = 0;
    float elapsed = 0.001f * (CTimer::m_snTimeInMilliseconds - CTimer::m_snPreviousTimeInMilliseconds);
    float elapsedAlt = elapsed;
    float rowTime = -0.001f * timeFadeout;
    for(auto it = entries.rbegin(); it != entries.rend(); it++) // draw oldest on top
    {
        auto& entry = *it;

        if(entry.timeLeft > 0.0f)
        {
            if(entry.timeLeft < elapsedAlt)
                entry.timeLeft = 0.0f;
            else
                entry.timeLeft -= elapsedAlt;
        }
        else
        {
            if (it == entries.rbegin()) entry.timeLeft -= elapsed; // fade out only oldest
        }

        elapsedAlt *= 0.9f; // keep every next line longer

        rowTime = max(rowTime, entry.timeLeft); // carred on from older entries
        
        BYTE alpha = 255;
        /*if (rowTime < 0)
        {
            float fadeProgress = -rowTime / (0.001f * timeFadeout);
            fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);
            fadeProgress = 1.0f - fadeProgress; // fade out
            fadeProgress = sqrtf(fadeProgress);
            alpha = (BYTE)(fadeProgress * 0xFF);
        };*/

        auto color = fontColor[(size_t)entry.level];
        alpha = min(alpha, color.a);
        color.a = alpha;

        CFont::SetColor(color);

        alpha = std::clamp(int(alpha * alpha) / 255, 0, 255); // corrected for fadeout
        CFont::SetDropColor(CRGBA(0, 0, 0, alpha));

        float y = posY + Row_Height * sizeY * lines;
        lines += CountLines(entry.msg);

        CFont::PrintString(posX, y, entry.GetMsg());
    }

    // for some reason last string on print list is always drawn incorrectly
    // Walkaround: add one extra dummy line then
    CFont::PrintString(0.0f, -500.0f, "_~n~_~n~_"); 
}

void ScreenLog::DrawLine(const char* msg, size_t row)
{
    CFont::SetBackground(false, false);
    CFont::SetWrapx(99999999.0f); // no line wrap
    CFont::SetFontStyle(FONT_SUBTITLES);
    CFont::SetEdge(1);
    CFont::SetProportional(true);

    const float aspect = (float)RsGlobal.maximumWidth / RsGlobal.maximumHeight;
    float sizeX = fontSize * 0.55f * RsGlobal.maximumWidth / 640.0f / aspect;
    float sizeY = fontSize * RsGlobal.maximumHeight / 448.0f;
    CFont::SetScale(sizeX, sizeY);

    CFont::SetOrientation(ALIGN_RIGHT);
    float posX = (float)RsGlobal.maximumWidth - 15.0f * sizeX;
    
    //if(FrontEndMenuManager.m_bHudOn)
    float posY = 0.25f * RsGlobal.maximumHeight;
    posY += 18.0f * sizeY * row;

    auto color = fontColor[(size_t)eLogLevel::Error];
    CFont::SetColor(color);
    CFont::SetDropColor(CRGBA(0, 0, 0, color.a));

    CFont::PrintString(posX, posY, msg);
}

size_t ScreenLog::CountLines(std::string& msg)
{
    size_t lines = 1;

    size_t pos = 0;
    while ((pos = msg.find("~n~", pos)) != std::string::npos)
    {
        lines++;
        pos += 3; // pattern length
    }

    lines += std::count(msg.begin(), msg.end(), '\n');

    return lines;
}

DWORD ScreenLog::GetTime()
{
    //return GetTickCount();
    return CTimer::m_snPreviousTimeInMillisecondsNonClipped;
}

