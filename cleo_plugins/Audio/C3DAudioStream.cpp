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
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, 0.0f); // muted until processed
    ok = true;
}

void C3DAudioStream::Set3dPosition(const CVector& pos)
{
    host = nullptr;
    hostType = ENTITY_TYPE_NOTHING;
    offset = pos;
}

void C3DAudioStream::Set3dSourceSize(float radius)
{
    BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, radius, 1E+12f, -1, -1, -1.0f);
}

void C3DAudioStream::SetHost(CEntity* host, const CVector& offset)
{
    if (host != nullptr)
    {
        this->host = host;
        hostType = (eEntityType)host->m_nType;
    }
    else
    {
        this->host = nullptr;
        hostType = ENTITY_TYPE_NOTHING;
    }

    this->offset = offset;
}

void C3DAudioStream::Process()
{
    CAudioStream::Process();
    UpdatePosition();
}

void C3DAudioStream::UpdatePosition()
{
    auto prevPos = position;

    if (host != nullptr)
    {
        if (hostType == ENTITY_TYPE_NOTHING) return;

        // host despawned?
        bool hostValid = false;
        switch (hostType)
        {
            case ENTITY_TYPE_OBJECT:
                hostValid = CPools::ms_pObjectPool->IsObjectValid((CObject*)host);
                break;

            case ENTITY_TYPE_PED:
                hostValid = CPools::ms_pPedPool->IsObjectValid((CPed*)host);
                break;

            case ENTITY_TYPE_VEHICLE:
                hostValid = CPools::ms_pVehiclePool->IsObjectValid((CVehicle*)host);
                break;
        }
        if (!hostValid)
        {
            hostType = ENTITY_TYPE_NOTHING;
            Stop();
            return;
        }

        RwV3dTransformPoint((RwV3d*)&position, (RwV3d*)&offset, (RwMatrix*)host->GetMatrix());
    }
    else // world offset
    {
        position = offset;
    }

    if (prevPos.Magnitude() > 0.0f) // not equal to 0,0,0
    {
        CVector velocity = position - prevPos;
        velocity /= CSoundSystem::timeStep;
        BASS_ChannelSet3DPosition(streamInternal, &toBass(position), nullptr, &toBass(velocity));
    }
    else
    {
        BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, 0.0f); // muted until next update
    }
}

