#include "C3DAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"

using namespace CLEO;

C3DAudioStream::C3DAudioStream(const char* filepath) : CAudioStream()
{
    // see https://github.com/cleolibrary/CLEO5/pull/230
    static_assert(offsetof(C3DAudioStream, streamInternal) == 4 && alignof(C3DAudioStream) == 4, "C3DAudioStream compatibility with CLEO4 broken!");

    if (isNetworkSource(filepath) && !CSoundSystem::allowNetworkSources)
    {
        TRACE("Loading of 3d-audiostream '%s' failed. Support of network sources was disabled in SA.Audio.ini", filepath);
        return;
    }

    unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE;
    if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

    if (!(streamInternal = BASS_StreamCreateFile(FALSE, filepath, 0, 0, flags)) &&
        !(streamInternal = BASS_StreamCreateURL(filepath, 0, flags, nullptr, nullptr)))
    {
        LOG_WARNING(0, "Loading of 3d-audiostream '%s' failed. Error code: %d", filepath, BASS_ErrorGetCode());
        return;
    }

    BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
    BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, 3.0f, 1E+12f, -1, -1, -1.0f);
    ok = true;
}

void C3DAudioStream::Set3dPosition(const CVector& pos)
{
    link = nullptr;
    position = pos;
    BASS_3DVECTOR vel = { 0.0f, 0.0f, 0.0f };

    BASS_ChannelSet3DPosition(streamInternal, &toBass(position), nullptr, &vel);
}

void C3DAudioStream::Set3dSourceSize(float radius)
{
    BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, radius, 1E+12f, -1, -1, -1.0f);
}

void C3DAudioStream::Link(CPlaceable* placable)
{
    link = placable;
}

void C3DAudioStream::Process()
{
    CAudioStream::Process();

    if (state != Playing) return; // done

    UpdatePosition();
}

void C3DAudioStream::UpdatePosition()
{
    if (link) // attached to entity
    {
        auto prevPos = position;
        position = link->GetPosition();

        // calculate velocity
        auto timeDelta = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);
        auto vel = position - prevPos;
        vel /= CSoundSystem::timeStep;

        BASS_ChannelSet3DPosition(streamInternal, &toBass(position), nullptr, &toBass(vel));
    }
}

