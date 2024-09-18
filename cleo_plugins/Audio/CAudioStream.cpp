#include "CAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"
#include "CCamera.h"

using namespace CLEO;

CAudioStream::CAudioStream()
{
    ResetParams();
}

CAudioStream::CAudioStream(const char* filepath) : CAudioStream()
{
    if (isNetworkSource(filepath) && !CSoundSystem::allowNetworkSources)
    {
        TRACE("Loading of audiostream '%s' failed. Support of network sources was disabled in SA.Audio.ini", filepath);
        return;
    }

    unsigned flags = BASS_SAMPLE_SOFTWARE;
    if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

    if (!(streamInternal = BASS_StreamCreateFile(FALSE, filepath, 0, 0, flags)) &&
        !(streamInternal = BASS_StreamCreateURL(filepath, 0, flags, 0, nullptr)))
    {
        LOG_WARNING(0, "Loading of audiostream '%s' failed. Error code: %d", filepath, BASS_ErrorGetCode());
        return;
    }

    BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, 0.0f); // muted until processed
    ok = true;
}

CAudioStream::~CAudioStream()
{
    if (streamInternal) BASS_StreamFree(streamInternal);
}

void CAudioStream::Play()
{
    if (state == Stopped) BASS_ChannelSetPosition(streamInternal, 0, BASS_POS_BYTE); // rewind
    state = PlayingInactive; // needs to be processed
}

void CAudioStream::Pause(bool changeState)
{
    if (GetState() == Playing)
    {
        BASS_ChannelPause(streamInternal);
        state = changeState ? Paused : PlayingInactive;
    }
}

void CAudioStream::Stop()
{
    BASS_ChannelPause(streamInternal);
    state = Stopped;

    ProcessTransitions(true); // finish all active transitions
}

void CAudioStream::Resume()
{
    Play();
}

float CAudioStream::GetLength() const
{
    return (float)BASS_ChannelBytes2Seconds(streamInternal, BASS_ChannelGetLength(streamInternal, BASS_POS_BYTE));
}

void CAudioStream::SetProgress(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    auto total = BASS_ChannelGetLength(streamInternal, BASS_POS_BYTE);
    auto bytePos = QWORD(value * total);
    BASS_ChannelSetPosition(streamInternal, bytePos, BASS_POS_BYTE);
}

float CAudioStream::GetProgress() const
{
    auto total = BASS_ChannelGetLength(streamInternal, BASS_POS_BYTE); // returns -1 on error
    auto bytePos = BASS_ChannelGetPosition(streamInternal, BASS_POS_BYTE); // returns -1 on error

    if (bytePos == -1) bytePos = 0; // error or not available yet

    float progress = (float)bytePos / total;
    progress = std::clamp(progress, 0.0f, 1.0f);
    return progress;
}

CAudioStream::StreamState CAudioStream::GetState() const
{
    return (state == PlayingInactive) ? Playing : state;
}

void CAudioStream::SetLooping(bool enable)
{
    BASS_ChannelFlags(streamInternal, enable ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

bool CLEO::CAudioStream::GetLooping() const
{
    return (BASS_ChannelFlags(streamInternal, 0, 0) & BASS_SAMPLE_LOOP) != 0;
}

void CAudioStream::SetVolume(float value, float transitionTime)
{
    if (transitionTime > 0.0f) Resume();

    value = max(value, 0.0f);
    SetParam(StreamParam::Volume, value, transitionTime);
}

float CAudioStream::GetVolume() const
{
    return paramCurrent[StreamParam::Volume];
}

void CAudioStream::SetSpeed(float value, float transitionTime)
{
    if (transitionTime > 0.0f) Resume();

    value = max(value, 0.0f);
    SetParam(StreamParam::Speed, value, transitionTime);
}

float CAudioStream::GetSpeed() const
{
    return paramCurrent[StreamParam::Speed];
}

void CLEO::CAudioStream::SetType(eStreamType value)
{
    switch(value)
    {
        case eStreamType::SoundEffect:
        case eStreamType::Music:
            type = value;
            break;

        default:
            type = None;
    }
}

eStreamType CLEO::CAudioStream::GetType() const
{
    return type;
}

void CAudioStream::SetParam(StreamParam param, float value, float transitionTime)
{
    value = max(value, 0.0f);
    paramTarget[param] = value;

    if (transitionTime > 0.0f)
        paramTransitionStep[param] = (paramTarget[param] - paramCurrent[param]) / (1000.0f * transitionTime);
    else
        paramCurrent[param] = value; // instant
}

float CAudioStream::GetParam(StreamParam param)
{
    return paramCurrent[param];
}

void CAudioStream::ResetParams()
{
    // defaults
    SetParam(StreamParam::Speed, 1.0f);
    SetParam(StreamParam::Volume, 1.0f);
}

float CAudioStream::CalculateVolume()
{
    float masterVolume;
    switch(type)
    {
        case SoundEffect: masterVolume = CSoundSystem::masterVolumeSfx; break;
        case Music: masterVolume = CSoundSystem::masterVolumeMusic; break;
        default: masterVolume = 1.0f; break;
    }

    float fadeVolume = 1.0f - TheCamera.m_fFadeAlpha / 255.0f; // TODO: handle TheCamera.m_bIgnoreFadingStuffForMusic if neccessary

    return GetParam(Volume) * fadeVolume * masterVolume;
}

void CAudioStream::ProcessTransitions(bool instant)
{
    auto timeDelta = float(CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);

    for (size_t p = 0; p < StreamParam::StreamParamMax; p++)
    {
        if (paramCurrent[p] == paramTarget[p]) continue;

        if (instant)
            paramCurrent[p] = paramTarget[p];
        else
        {
            paramCurrent[p] += paramTransitionStep[p] * timeDelta;

            bool done = false;
            if (paramTransitionStep[p] > 0.0f)
            {
                if (paramCurrent[p] >= paramTarget[p]) done = true;
            }
            else
            {
                if (paramCurrent[p] <= paramTarget[p]) done = true;
            }

            if (done) paramCurrent[p] = paramTarget[p]; // correct if overshoot
        }
    }
}

void CAudioStream::ApplyParams()
{
    // volume
    float volume = CalculateVolume();
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, volume);

    // speed
    float freq = rate * GetParam(StreamParam::Speed) * CSoundSystem::masterSpeed;
    freq = max(freq, 0.000001f); // 0.0 restores original
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_FREQ, freq);
}

bool CAudioStream::IsOk() const
{
    return ok;
}

HSTREAM CAudioStream::GetInternal()
{
    return streamInternal;
}

void CAudioStream::Process()
{
    if (state == PlayingInactive)
    {
        BASS_ChannelPlay(streamInternal, FALSE);
        state = Playing;
    }

    if (!GetLooping() && GetProgress() >= 1.0f) // end reached
    {
        state = Stopped;
    }

    if (state != Playing) return; // done

    ProcessTransitions();

    if (GetParam(StreamParam::Volume) <= 0.0f) Pause();

    ApplyParams();
}

void CAudioStream::Set3dPosition(const CVector& pos)
{
    // not applicable for 2d audio
}

void CAudioStream::Set3dSourceSize(float radius)
{
    // not applicable for 2d audio
}

void CAudioStream::Link(CEntity* entity)
{
    // not applicable for 2d audio
}
