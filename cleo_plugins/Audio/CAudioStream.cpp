#include "CAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"
#include "CCamera.h"

using namespace CLEO;

CAudioStream::CAudioStream(const char* filepath)
{
    // see https://github.com/cleolibrary/CLEO5/pull/230
    static_assert(offsetof(CAudioStream, streamInternal) == 4 && alignof(CAudioStream) == 4, "CAudioStream compatibility with CLEO4 broken!");

    if (isNetworkSource(filepath) && !CSoundSystem::allowNetworkSources)
    {
        TRACE("Loading of audiostream '%s' failed. Support of network sources was disabled in SA.Audio.ini", filepath);
        return;
    }

    unsigned flags = BASS_SAMPLE_SOFTWARE | BASS_STREAM_PRESCAN;
    if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

    if (!(streamInternal = BASS_StreamCreateFile(FALSE, filepath, 0, 0, flags)) &&
        !(streamInternal = BASS_StreamCreateURL(filepath, 0, flags, 0, nullptr)))
    {
        LOG_WARNING(0, "Loading of audiostream '%s' failed. Error code: %d", filepath, BASS_ErrorGetCode());
        return;
    }

    BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
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

    // cancel ongoing transitions
    speed.finish();
    volume.finish();
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
    if (GetState() == Stopped)
    {
        state = Paused; // resume from set progress
    }

    value = std::clamp(value, 0.0f, 1.0f);
    auto bytePos = BASS_ChannelSeconds2Bytes(streamInternal, GetLength() * value);

    BASS_ChannelSetPosition(streamInternal, bytePos, BASS_POS_BYTE);
}

float CAudioStream::GetProgress() const
{
    auto bytePos = BASS_ChannelGetPosition(streamInternal, BASS_POS_BYTE);
    if (bytePos == -1) bytePos = 0; // error or not available yet
    auto pos = BASS_ChannelBytes2Seconds(streamInternal, bytePos);

    auto byteTotal = BASS_ChannelGetLength(streamInternal, BASS_POS_BYTE);
    auto total = BASS_ChannelBytes2Seconds(streamInternal, byteTotal);

    return (float)(pos / total);
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
    volume.setValue(max(value, 0.0f), transitionTime);
}

float CAudioStream::GetVolume() const
{
    return volume.value();
}

void CAudioStream::SetSpeed(float value, float transitionTime)
{
    if (value > 0.0f && transitionTime > 0.0f) Resume();
    speed.setValue(max(value, 0.0f), transitionTime);
}

float CAudioStream::GetSpeed() const
{
    return speed.value();
}

void CLEO::CAudioStream::SetType(eStreamType value)
{
    switch(value)
    {
        case eStreamType::SoundEffect:
        case eStreamType::Music:
        case eStreamType::UserInterface:
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

float CAudioStream::CalculateVolume()
{
    float vol = 1.0f;

    switch(type)
    {
        case SoundEffect: vol *= CSoundSystem::masterVolumeSfx; break;
        case Music: vol *= CSoundSystem::masterVolumeMusic; break;
        case UserInterface: vol *= CSoundSystem::masterVolumeSfx; break;
        default: vol *= 1.0f; break;
    }

    // screen black fade
    if (type != UserInterface && !TheCamera.m_bIgnoreFadingStuffForMusic)
    {
        vol *= 1.0f - TheCamera.m_fFadeAlpha / 255.0f;
    }

    // music volume lowering in cutscenes, when characters talk, mission sounds are played etc.
    if (type == Music)
    {
        if (TheCamera.m_bWideScreenOn) vol *= 0.25f;
    }

    // stream's volume
    vol *= volume.value();

    return vol;
}

float CAudioStream::CalculateSpeed()
{
    float masterSpeed;
    switch (type)
    {
        case SoundEffect: masterSpeed = CSoundSystem::masterSpeed; break;
        case Music: masterSpeed = TheCamera.m_bWideScreenOn ? 1.0f : CSoundSystem::masterSpeed; break;
        case UserInterface: masterSpeed = 1.0f; break;
        default: masterSpeed = 1.0f;
    }

    return speed.value() * masterSpeed;
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
        if (BASS_ChannelPlay(streamInternal, FALSE))
        {
            state = Playing;
        }
    }
    else
    {
        if (state == Playing && BASS_ChannelIsActive(streamInternal) == BASS_ACTIVE_STOPPED) // end reached
        {
            state = Stopped;
        }
    }

    float prevSpeed = speed.value();

    // update animated params
    speed.update(CSoundSystem::timeStep);
    volume.update(CSoundSystem::timeStep);

    // transitioning speed to 0 pauses playback
    if (prevSpeed > 0.0f && speed.value() <= 0.0f)
    {
        Pause();
    }

    if (state != Playing) return; // done

    float volume = CalculateVolume();
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, volume);

    float freq = rate * CalculateSpeed();
    freq = max(freq, 0.000001f); // 0 results in original speed
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_FREQ, freq);
}

void CAudioStream::Set3dPosition(const CVector& pos)
{
    // not applicable for 2d audio
}

void CAudioStream::Set3dSourceSize(float radius)
{
    // not applicable for 2d audio
}

void CAudioStream::SetHost(CEntity* placable, const CVector& offset)
{
    // not applicable for 2d audio
}
