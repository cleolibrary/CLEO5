#include "CSoundSystem.h"
#include "CAEAudioHardware.h"
#include "CCamera.h"
#include <windows.h>

namespace CLEO
{
    bool CSoundSystem::useFloatAudio = false;
    BASS_3DVECTOR CSoundSystem::pos(0.0, 0.0, 0.0);
    BASS_3DVECTOR CSoundSystem::vel(0.0, 0.0, 0.0);
    BASS_3DVECTOR CSoundSystem::front(0.0, -1.0, 0.0);
    BASS_3DVECTOR CSoundSystem::top(0.0, 0.0, 1.0);
    float CSoundSystem::speed = 1.0f;
    float CSoundSystem::volume = 1.0f;

    void EnumerateBassDevices(int& total, int& enabled, int& default_device)
    {
        BASS_DEVICEINFO info;
        for (default_device = -1, enabled = 0, total = 0; BASS_GetDeviceInfo(total, &info); ++total)
        {
            if (info.flags & BASS_DEVICE_ENABLED) ++enabled;
            if (info.flags & BASS_DEVICE_DEFAULT) default_device = total;
            TRACE("Found sound device %d%s: %s", total, default_device == total ?
                " (default)" : "", info.name);
        }
    }

    bool CSoundSystem::Init()
    {
        if (initialized) return true; // already done

        int default_device, total_devices, enabled_devices;
        EnumerateBassDevices(total_devices, enabled_devices, default_device);

        BASS_DEVICEINFO info = { nullptr, nullptr, 0 };
        if (forceDevice != -1 && BASS_GetDeviceInfo(forceDevice, &info) &&
            info.flags & BASS_DEVICE_ENABLED)
            default_device = forceDevice;

        TRACE("On system found %d devices, %d enabled devices, assuming device to use: %d (%s)",
            total_devices, enabled_devices, default_device, BASS_GetDeviceInfo(default_device, &info) ?
            info.name : "Unknown device");

        if (BASS_Init(default_device, 44100, BASS_DEVICE_3D, RsGlobal.ps->window, nullptr) &&
            BASS_Set3DFactors(1.0f, 3.0f, 80.0f) &&
            BASS_Set3DPosition(&pos, &vel, &front, &top))
        {
            TRACE("SoundSystem initialized");

            // Can we use floating-point (HQ) audio streams?
            DWORD floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, NULL); // floating-point channel support? 0 = no, else yes
            if (floatable)
            {
                TRACE("Floating-point audio supported!");
                useFloatAudio = true;
                BASS_StreamFree(floatable);
            }
            else TRACE("Floating-point audio not supported!");

            if (BASS_GetInfo(&SoundDevice))
            {
                if (SoundDevice.flags & DSCAPS_EMULDRIVER)
                    TRACE("Audio drivers not installed - using DirectSound emulation");
                if (!SoundDevice.eax)
                    TRACE("Audio hardware acceleration disabled (no EAX)");
            }

            initialized = true;
            BASS_Apply3D();
            return true;
        }

        LOG_WARNING(0, "Could not initialize BASS sound system. Error code: %d", BASS_ErrorGetCode());
        return false;
    }

    CAudioStream *CSoundSystem::LoadStream(const char *filename, bool in3d)
    {
        CAudioStream *result = in3d ? new C3DAudioStream(filename) : new CAudioStream(filename);
        if (result->OK)
        {
            streams.insert(result);
            return result;
        }
        delete result;
        return nullptr;
    }

    void CSoundSystem::UnloadStream(CAudioStream *stream)
    {
        if (streams.erase(stream))
            delete stream;
        else
            TRACE("Unloading of stream that is not in list of loaded streams");
    }

    void CSoundSystem::UnloadAllStreams()
    {
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream)
        {
            delete stream;
        });
        streams.clear();
    }

    void CSoundSystem::ResumeStreams()
    {
        paused = false;
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
            if (stream->state == CAudioStream::playing) stream->Resume();
        });
    }

    void CSoundSystem::PauseStreams()
    {
        paused = true;
        std::for_each(streams.begin(), streams.end(), [](CAudioStream *stream) {
            if (stream->state == CAudioStream::playing) stream->Pause(false);
        });
    }

    void CSoundSystem::Update(bool active)
    {
        if (!active || CTimer::m_UserPause || CTimer::m_CodePause) // covers menu pausing, no disc in drive pausing, etc.
        {
            if (!paused) PauseStreams();
        }
        else // not in menu
        {
            if (paused) ResumeStreams();

            // get game globals
            speed = CTimer::ms_fTimeScale;
            volume = AEAudioHardware.m_fEffectMasterScalingFactor * 0.5f; // fit to game's sfx volume

            // camera movements
            CMatrixLink * pMatrix = nullptr;
            CVector * pVec = nullptr;
            if (TheCamera.m_matrix)
            {
                pMatrix = TheCamera.m_matrix;
                pVec = &pMatrix->pos;
            }
            else pVec = &TheCamera.m_placement.m_vPosn;

            BASS_3DVECTOR prevPos = pos;
            pos = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

            // calculate velocity
            vel = prevPos;
            vel.x -= pos.x;
            vel.y -= pos.y;
            vel.z -= pos.z;
            auto timeDelta = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);
            vel.x *= timeDelta;
            vel.y *= timeDelta;
            vel.z *= timeDelta;

            // setup the ears
            if (!TheCamera.m_bJust_Switched && !TheCamera.m_bCameraJustRestored) // avoid jump cut velocity glitches
            {
                BASS_Set3DPosition(
                    &pos,
                    &vel, // keep previous velocity on jump cuts
                    pMatrix ? &BASS_3DVECTOR(pMatrix->at.y, pMatrix->at.z, pMatrix->at.x) : nullptr,
                    pMatrix ? &BASS_3DVECTOR(pMatrix->up.y, pMatrix->up.z, pMatrix->up.x) : nullptr
                );
            }

            // process all streams
            for(auto stream : streams)
            {
                stream->Process();
            };

            // apply above changes
            BASS_Apply3D();
        }
    }

    CAudioStream::CAudioStream()
    {
    }

    CAudioStream::CAudioStream(const char *src)
    {
        unsigned flags = BASS_SAMPLE_SOFTWARE;
        if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

        if (!(streamInternal = BASS_StreamCreateFile(FALSE, src, 0, 0, flags)) &&
            !(streamInternal = BASS_StreamCreateURL(src, 0, flags, 0, nullptr)))
        {
            LOG_WARNING(0, "Loading audiostream %s failed. Error code: %d", src, BASS_ErrorGetCode());
            return;
        }
        
        BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
        OK = true;
    }

    CAudioStream::~CAudioStream()
    {
        if (streamInternal) BASS_StreamFree(streamInternal);
    }

    C3DAudioStream::C3DAudioStream(const char *src) : CAudioStream()
    {
        unsigned flags = BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_SOFTWARE;
        if (CSoundSystem::useFloatAudio) flags |= BASS_SAMPLE_FLOAT;

        if (!(streamInternal = BASS_StreamCreateFile(FALSE, src, 0, 0, flags)) &&
            !(streamInternal = BASS_StreamCreateURL(src, 0, flags, nullptr, nullptr)))
        {
            LOG_WARNING(0, "Loading 3d-audiostream %s failed. Error code: %d", src, BASS_ErrorGetCode());
            return;
        }

        BASS_ChannelGetAttribute(streamInternal, BASS_ATTRIB_FREQ, &rate);
        BASS_ChannelSet3DAttributes(streamInternal, BASS_3DMODE_NORMAL, 5.0f, 1E+12f, -1, -1, -1.0f);
        OK = true;
    }

    C3DAudioStream::~C3DAudioStream()
    {
        if (streamInternal) BASS_StreamFree(streamInternal);
    }

    void CAudioStream::Play()
    {
        Process(true); // update settings
        BASS_ChannelPlay(streamInternal, TRUE);
        state = playing;
    }

    void CAudioStream::Pause(bool change_state)
    {
        BASS_ChannelPause(streamInternal);
        if (change_state) state = paused;
    }

    void CAudioStream::Stop()
    {
        BASS_ChannelPause(streamInternal);
        BASS_ChannelSetPosition(streamInternal, 0, BASS_POS_BYTE);
        state = paused;
    }

    void CAudioStream::Resume()
    {
        BASS_ChannelPlay(streamInternal, FALSE);
        state = playing;
    }

    float CAudioStream::GetLength() const
    {
        return (float)BASS_ChannelBytes2Seconds(streamInternal, BASS_ChannelGetLength(streamInternal, BASS_POS_BYTE));
    }

    DWORD CAudioStream::GetState() const
    {
        if (state == stopped) return -1; // dont do this in case we changed state by pausing

        switch (BASS_ChannelIsActive(streamInternal))
        {
            case BASS_ACTIVE_STOPPED:
            default:
                return -1;
            case BASS_ACTIVE_PLAYING:
            case BASS_ACTIVE_STALLED:
                return 1;
            case BASS_ACTIVE_PAUSED:
                return 2;
        };
    }

    void CAudioStream::SetVolume(float val, float transitionTime)
    {
        if (val < 0.0f) // just update
        {
            if (volume != volumeTarget)
            {
                auto timeDelta = CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped;
                volume += volumeTransitionStep * (double)timeDelta; // animate the transition

                // check progress
                auto remaining = volumeTarget - volume;
                remaining *= (volumeTransitionStep > 0.0) ? 1.0 : -1.0;
                if (remaining < 0.0) volume = volumeTarget; // overshoot
            }
        }
        else
        {
            volumeTarget = val;
            if (transitionTime <= 0.0) volume = val; // instant
            else volumeTransitionStep = (volumeTarget - volume) / (1000.0 * transitionTime);
        }

        BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_VOL, (float)volume * CSoundSystem::volume);
    }

    float CAudioStream::GetVolume() const
    {
        return (float)volume;
    }

    void CAudioStream::SetSpeed(float val, float transitionTime)
    {
        if (val < 0.0f) // just update
        {
            if (speed != speedTarget)
            {
                auto timeDelta = CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped;
                speed += speedTransitionStep * (double)timeDelta; // animate the transition

                // check progress
                auto remaining = speedTarget - speed;
                remaining *= (speedTransitionStep > 0.0) ? 1.0 : -1.0;
                if (remaining < 0.0) // overshoot
                {
                    speed = speedTarget; // done
                    if (speed <= 0.0f) Pause();
                }
            }
        }
        else
        {
            if (val > 0.0f) Resume();

            speedTarget = val;
            if (transitionTime <= 0.0) speed = val; // instant
            else speedTransitionStep = (speedTarget - speed) / (1000.0 * transitionTime);
        }

        float freq = rate * (float)speed * CSoundSystem::speed;
        freq = max(freq, 0.000001f); // 0 results in original speed
        BASS_ChannelSetAttribute(streamInternal, BASS_ATTRIB_FREQ, freq);
    }

    float CAudioStream::GetSpeed() const
    {
        return (float)speed;
    }

    void CAudioStream::Loop(bool enable)
    {
        BASS_ChannelFlags(streamInternal, enable ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
    }

	HSTREAM CAudioStream::GetInternal()
	{
		return streamInternal;
	}

    void CAudioStream::Process(bool force)
    {
        switch (BASS_ChannelIsActive(streamInternal))
        {
            case BASS_ACTIVE_PAUSED:
                state = paused;
                break;

            case BASS_ACTIVE_PLAYING:
            case BASS_ACTIVE_STALLED:
                state = playing;
                break;

            case BASS_ACTIVE_STOPPED:
                state = stopped;
                break;
        }

        if (state != playing && !force) return; // done

        SetSpeed(-1.0f);
        SetVolume(-1.0f);
    }

    void CAudioStream::Set3dPosition(const CVector& pos)
    {
        // not applicable for 2d audio
    }

    void CAudioStream::Link(CPlaceable *placable)
    {
        // not applicable for 2d audio
    }

    void C3DAudioStream::Set3dPosition(const CVector& pos)
    {
        link = nullptr;

        position.x = pos.y;
        position.y = pos.z;
        position.z = pos.x;
        BASS_3DVECTOR vel = { 0.0f, 0.0f, 0.0f };
        BASS_ChannelSet3DPosition(streamInternal, &position, nullptr, &vel);
    }

    void C3DAudioStream::Link(CPlaceable *placable)
    {
        link = placable;
        Process(true);
    }

    void C3DAudioStream::Process(bool force)
    {
        switch (BASS_ChannelIsActive(streamInternal))
        {
            case BASS_ACTIVE_PAUSED:
                state = paused;
                break;

            case BASS_ACTIVE_PLAYING:
            case BASS_ACTIVE_STALLED:
                state = playing;
                break;

            case BASS_ACTIVE_STOPPED:
                state = stopped;
                break;
        }

        if (state != playing && !force) return; // done

        if (link) // attached to entity
        {
            auto prevPos = position;
            CVector* pVec = link->m_matrix ? &link->m_matrix->pos : &link->m_placement.m_vPosn;
            position = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

            // calculate velocity
            BASS_3DVECTOR vel = position;
            vel.x -= prevPos.x;
            vel.y -= prevPos.y;
            vel.z -= prevPos.z;
            auto timeDelta = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);
            vel.x *= timeDelta;
            vel.y *= timeDelta;
            vel.z *= timeDelta;

            BASS_ChannelSet3DPosition(streamInternal, &position, nullptr, &vel);
        }

        SetSpeed(-1.0f);
        SetVolume(-1.0f);

        if (force) BASS_Apply3D(); // apply changes now
    }
}
