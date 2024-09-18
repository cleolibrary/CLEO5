#include "C3DAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"

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
    ok = true;
}

C3DAudioStream::~C3DAudioStream()
{
    if (streamInternal) BASS_StreamFree(streamInternal);
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

    // calculate distance based volume
    float distance = CSoundSystem::GetDistance(&position);
    distance = max(distance - sourceRadius, 0.0f);
    distance /= max(GetParam(StreamParam::Volume), 0.0001f); // louder sounds reach further

    // hand fitted to match game's decay curve
    float decay = 1.0f / (1.0f + 0.32f * distance);
    decay = max(decay - 0.03948f, 0.0f);

    return CAudioStream::CalculateVolume() * Volume_3D_Adjust * decay;
}

void C3DAudioStream::UpdatePosition()
{
    if (host == nullptr) 
    {
        if (!placed && !CSoundSystem::skipFrame)
        {
            const auto direction = CVector(0.0f, 0.0f, 1.0f); // up TODO: allow to be set
            const auto velocity = CVector(0.0f, 0.0f, 0.0f);

            BASS_ChannelSet3DPosition(streamInternal, 
                (BASS_3DVECTOR*)&position, 
                (BASS_3DVECTOR*)&direction, 
                (BASS_3DVECTOR*)&velocity);
            placed = true;
        }
    }
    else // attached to some entity
    {
        // TODO: relative offset, attaching to bones/car components
        CVector velocity = position; // store previous
        position = host->GetPosition(); // get new

        if (!CSoundSystem::skipFrame)
        {
            velocity = position - velocity;
            velocity /= CSoundSystem::timeStep; // meters peer second

            // for some reason required for BASS to produce correct results
            velocity.x = sqrtf(abs(velocity.x)) * (velocity.x > 0.0f ? 1.0f : -1.0f);
            velocity.y = sqrtf(abs(velocity.y)) * (velocity.y > 0.0f ? 1.0f : -1.0f);
            velocity.z = sqrtf(abs(velocity.z)) * (velocity.z > 0.0f ? 1.0f : -1.0f);

            BASS_ChannelSet3DPosition(streamInternal, 
                (BASS_3DVECTOR*)&position, 
                (BASS_3DVECTOR*)&host->GetForward(), 
                (BASS_3DVECTOR*)&velocity);
            placed = true;
        }
    }
}

