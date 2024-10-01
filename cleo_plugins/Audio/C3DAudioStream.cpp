#include "C3DAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"
#include "CCamera.h"

using namespace CLEO;

C3DAudioStream::C3DAudioStream(const char* filepath) : CAudioStream()
{
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
    BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, -1.0f, -1.0f, -1, -1, -1.0f);
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, 0.0f); // muted until processed
    ok = true;
}

void C3DAudioStream::Set3dPosition(const CVector& pos)
{
    host = nullptr;
    position = pos;
    placed = false;
}

void C3DAudioStream::Set3dSourceSize(float radius)
{
    sourceRadius = radius;
}

void C3DAudioStream::Link(CEntity* entity)
{
    host = entity;
    placed = false;
}

void C3DAudioStream::Process()
{
    UpdatePosition();
    CAudioStream::Process();
}

float C3DAudioStream::CalculateVolume()
{
    if (!placed) return 0.0f; // muted until position calculated

    float volumeDb = 0.0; // default volume for game's mission audio

    // calculate distance based volume
    float distance = CSoundSystem::GetDistance(&position);
    distance = max(distance - sourceRadius, 0.0f);
    //distance /= max(powf(sourceRadius, 0.333f), 0.001f); // bigger sources reach further

    //float decay = 1.0f / (1.0f + 0.32f * distance); // hand fitted to match game's decay curve
    auto decay = (float)CAEAudioEnvironment__GetDistanceAttenuation(distance);
    //decay = dbToLinear(decay);

    //float volume = GetParam(Volume) * Volume_3D_Adjust * decay;
    //volume = max(volume - 0.025f, 0.0f); // BASS tends to make even very low volumes still well audible. Fight it with small offset

    volumeDb += decay;

    float volume = dbToLinear(volumeDb);

    return volume * Volume_3D_Adjust * CSoundSystem::GetMasterVolume(type);
}

void C3DAudioStream::UpdatePosition()
{
    CVector direction;
    CVector velocity;

    if (CSoundSystem::skipFrame) placed = false; // needs recalculation

    if (host != nullptr)  // attached to some entity
    {
        // TODO: relative offset, attaching to bones/car components
        velocity = position; // store previous
        position = host->GetPosition(); // get new

        direction = host->GetForward();

        if (!CSoundSystem::skipFrame)
        {
            velocity = position - velocity; // curr - prev
            velocity /= CSoundSystem::timeStep; // meters peer second
        }
    }
    else
    {
        direction = { 0.0f, 0.0f, 1.0f, }; // up
        velocity = { 0.0f, 0.0f, 0.0f, };
    }

    // apply
    if (!CSoundSystem::skipFrame)
    {
        float outsideness = min(CSoundSystem::GetDistance(&position) / (2.0f * sourceRadius), 1.0f); // factor
        if (outsideness >= 1.0f)
        {
            BASS_ChannelSet3DPosition(streamInternal,
                &toBass(position),
                &toBass(direction),
                &toBass(velocity));
        }
        else // listener inside sound soruce, no lef-right panning or Doppler effects then
        {
            auto fakeVel = velocity * outsideness + CSoundSystem::velocity * (1.0f - outsideness); // blend

            // modify blending curve for stereo panning
            outsideness = max(2.0f * outsideness - 1.0f, 0.0f); 
            outsideness *= outsideness;

            auto fakePos = CSoundSystem::position + CSoundSystem::forward; // 1 meter in front of the listener, dead center
            fakePos = position * outsideness + fakePos * (1.0f - outsideness); // blend

            BASS_ChannelSet3DPosition(streamInternal,
                &toBass(fakePos),
                &toBass(direction),
                &toBass(fakeVel));
        }

        placed = true;
    }
}

