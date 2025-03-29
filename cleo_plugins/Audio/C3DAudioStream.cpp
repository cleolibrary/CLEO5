#include "C3DAudioStream.h"
#include "CSoundSystem.h"
#include "CLEO_Utils.h"
#include "CCamera.h"
#include "CWaterLevel.h"
#include "CWeather.h"

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

    unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE | BASS_STREAM_PRESCAN;
    if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

    if (!(streamInternal = BASS_StreamCreateFile(FALSE, filepath, 0, 0, flags)) &&
        !(streamInternal = BASS_StreamCreateURL(filepath, 0, flags, nullptr, nullptr)))
    {
        LOG_WARNING(0, "Loading of 3d-audiostream '%s' failed. Error code: %d", filepath, BASS_ErrorGetCode());
        return;
    }

    BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
    BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, -1.0f, -1.0f, -1, -1, -1.0f);
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_BUFFER, 0.0f); // no delay for effects
    BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, 0.0f); // muted until processed

    effectMuffle = BASS_ChannelSetFX(streamInternal, BASS_FX_BFX_PEAKEQ, 0);

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
    this->radius = std::max<float>(radius, 0.1f);
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
    UpdatePosition();
    CAudioStream::Process();

    // position and velocity
    CVector relPos = position - CSoundSystem::position;
    float distance = relPos.NormaliseAndMag();
    float inFactor = std::clamp(2.0f - distance / radius, 0.0f, 1.0f); // sound source size simulation

    // stereo panning
    float sign = dot(CSoundSystem::direction, relPos) > 0.0f ? 1.0f : -1.0f;
    CVector centerPos = CSoundSystem::position + CSoundSystem::direction * distance * sign;
    CVector percPos = lerp(position, centerPos, inFactor);

    CVector percVel = lerp(velocity, CSoundSystem::velocity, inFactor);

    BASS_ChannelSet3DPosition(streamInternal, &toBass(percPos), nullptr, &toBass(percVel));

    // under water effect
    if (effectMuffle != 0)
    {
        float muffle = 0.0f;
        
        // in water
        float inWater= std::clamp((CWeather::UnderWaterness - 0.333f) * 4.0f, 0.0f, 1.0f); // listener submerged
        if (inWater < 1.0f)
        {
            // TODO: source under water
            //float level;
            //CWaterLevel::GetWaterLevel(position.x, position.y, position.z, &level, true, nullptr);
        }
        TRACE("water %f", inWater);

        muffle = max(muffle, inWater);

        BASS_BFX_PEAKEQ eqparam;
        eqparam.lChannel = BASS_BFX_CHANALL;

        eqparam.lBand = 0;
        eqparam.fCenter = 3000.0f;
        eqparam.fQ = 0.2f;
        eqparam.fGain = inWater * -120.0f;
        BASS_FXSetParameters(effectMuffle, &eqparam);

        eqparam.lBand = 1;
        eqparam.fCenter = 40.0f;
        eqparam.fQ = 0.22f;
        eqparam.fGain = inWater * 22.0f;
        BASS_FXSetParameters(effectMuffle, &eqparam);

        eqparam.lBand = 2;
        eqparam.fCenter = 200.0f;
        eqparam.fQ = 0.32f;
        eqparam.fGain = inWater * 35.0f;
        BASS_FXSetParameters(effectMuffle, &eqparam);
    }
}

float C3DAudioStream::CalculateVolume()
{
    if (!placed)
    {
        return 0.0f; // mute until processed
    }

    CVector relPos = position - CSoundSystem::position;
    float distance = relPos.NormaliseAndMag();
    float inFactor = std::clamp(2.0f - distance / radius, 0.0f, 1.0f); // sound source size simulation

    float vol = Volume_3D_Adjust;

    switch (type)
    {
        case SoundEffect: vol *= CSoundSystem::masterVolumeSfx; break;
        case Music: vol *= CSoundSystem::masterVolumeMusic; break;
        case UserInterface: vol *= CSoundSystem::masterVolumeSfx; break;
        default: vol *= 1.0f; break;
    }

    // distance decay
    vol *= CalculateDistanceDecay(radius, distance / max(volume.value(), 1.0f));

    // "look" direction decay
    vol *= lerp(CalculateDirectionDecay(CSoundSystem::direction, relPos), 1.0f, inFactor);

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

float C3DAudioStream::CalculateSpeed()
{
    float masterSpeed;
    switch (type)
    {
        case SoundEffect: masterSpeed = CSoundSystem::masterSpeed; break;
        case Music: masterSpeed = CSoundSystem::masterSpeed; break;
        case UserInterface: masterSpeed = 1.0f; break;
        default: masterSpeed = 1.0f;
    }

    return masterSpeed * speed.value();
}

float C3DAudioStream::CalculateDistanceDecay(float radius, float distance)
{
    distance = max(distance - radius, 0.0f);

    // exact match to ingame sounds
    /*float factor = 1.0f / powf(1.0f + distance, 2); // inverse square
    factor -= 0.00006f;*/

    // more natural feeling
    float factor = 1.0f / (1.0f + 0.01f * distance * distance);
    factor -= 0.015f;

    factor = std::clamp(factor, 0.0f, 1.0f);
    return factor;
}

float C3DAudioStream::CalculateDirectionDecay(const CVector& listenerDir, const CVector& relativePos)
{
    float factor = dot(listenerDir, relativePos);
    factor = 0.6f + 0.4f * factor; // 0.2 to 1.0
    return factor;
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
            placed = false;
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
        velocity = position - prevPos;
        velocity /= CSoundSystem::timeStep;
        placed = true;
    }
    else
    {
        placed = false;
    }
}
