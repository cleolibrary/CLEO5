#include "CSoundSystem.h"
#include "CAudioStream.h"
#include "C3dAudioStream.h"
#include "CLEO_Utils.h"
#include "CAEAudioHardware.h"
#include "CCamera.h"
#include "CPad.h"
#include "CVector.h"

namespace CLEO
{
    bool CSoundSystem::useFloatAudio = false;
    bool CSoundSystem::allowNetworkSources = true;
    eStreamType CSoundSystem::LegacyModeDefaultStreamType = eStreamType::None;
    CVector CSoundSystem::position(0.0f, 0.0f, 0.0f);
    CVector CSoundSystem::direction(0.0f, 1.0f, 0.0f);
    CVector CSoundSystem::velocity(0.0f, 0.0f, 0.0f);
    bool CSoundSystem::skipFrame = true;
    float CSoundSystem::timeStep = 0.02f;
    float CSoundSystem::masterSpeed = 1.0f;
    float CSoundSystem::masterVolumeSfx = 1.0f;
    float CSoundSystem::masterVolumeMusic = 1.0f;

    void EnumerateBassDevices(int& total, int& enabled, int& default_device)
    {
        TRACE("Listing audio devices:");

        BASS_DEVICEINFO info;
        enabled = 0;
        default_device = -1;
        for (total = 0; BASS_GetDeviceInfo(total, &info); ++total)
        {
            if (info.flags & BASS_DEVICE_DEFAULT) default_device = total;

            bool isEnabled = info.flags & BASS_DEVICE_ENABLED;
            if (isEnabled) ++enabled;

            TRACE(" %d: %s%s", total, info.name, isEnabled ? "" : " (disabled)");
        }
        TRACE(" Default device index: %d", default_device);
    }

    CSoundSystem::~CSoundSystem()
    {
        TRACE(""); // seaprator
        TRACE("Finalizing SoundSystem...");
        Clear();

        if (initialized)
        {
            //TRACE("Freeing BASS library");
            //std::thread(BASS_Free); // causes deadlock with ModLoader
            initialized = false;
        }
        TRACE("SoundSystem finalized");
    }

    bool CSoundSystem::Init()
    {
        if (initialized) return true; // already done

        TRACE(""); // separator
        TRACE("Initializing SoundSystem...");

        auto ver = HIWORD(BASS_GetVersion());
        TRACE("BASS library version is %d (required %d or newer)", ver, BASSVERSION);
        if (ver < BASSVERSION)
        {
            SHOW_ERROR("Invalid BASS library version! Expected at least %d, found %d.", BASSVERSION, ver);
        }

        auto config = GetConfigFilename();
        LegacyModeDefaultStreamType = (eStreamType)GetPrivateProfileInt("General", "LegacyModeDefaultStreamType", 0, config.c_str());
        allowNetworkSources = GetPrivateProfileInt("General", "AllowNetworkSources", 1, config.c_str()) != 0;

        int deviceIndex, total_devices, enabled_devices;
        EnumerateBassDevices(total_devices, enabled_devices, deviceIndex);

        BASS_DEVICEINFO info = { "Unknown device", nullptr, 0 };
        BASS_GetDeviceInfo(deviceIndex, &info);

        int forceIndex = GetPrivateProfileInt("General", "AudioDevice", -1, config.c_str());
        if (forceIndex != -1)
        {
            BASS_DEVICEINFO forceInfo = { "Unknown device", nullptr, 0 };
            if (BASS_GetDeviceInfo(forceIndex, &forceInfo) && forceInfo.flags & BASS_DEVICE_ENABLED)
            {
                TRACE("Force selecting audio device #%d: %s", forceIndex, forceInfo.name);
                deviceIndex = forceIndex;
            }
            else
            {
                LOG_WARNING(0, "Failed to force select device #%d! Selecting default audio device #%d: %s", forceIndex, deviceIndex, info.name);
            }
        }
        else
        {
            TRACE("Selecting default audio device #%d: %s", deviceIndex, info.name);
        }

        BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);

        if (BASS_Init(deviceIndex, 44100, BASS_DEVICE_3D, RsGlobal.ps->window, nullptr) &&
            BASS_Set3DFactors(1.0f, 0.0f, 1.0f))
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
            return true;
        }

        LOG_WARNING(0, "Could not initialize BASS sound system. Error code: %d", BASS_ErrorGetCode());
        return false;
    }

    bool CSoundSystem::Initialized()
    {
        return initialized;
    }

    CAudioStream* CSoundSystem::CreateStream(const char *filename, bool in3d)
    {
        CAudioStream* result = in3d ? new C3DAudioStream(filename) : new CAudioStream(filename);
        if (!result->IsOk())
        {
            delete result;
            return nullptr;
        }

        streams.insert(result);
        return result;
    }

    void CSoundSystem::DestroyStream(CAudioStream *stream)
    {
        if (streams.erase(stream))
            delete stream;
        else
            TRACE("Unloading of stream that is not in list of loaded streams");
    }

    bool CSoundSystem::HasStream(CAudioStream* stream)
    {
        return streams.find(stream) != streams.end();
    }

    void CSoundSystem::Clear()
    {
        for (auto stream : streams)
        {
            delete stream;
        };
        streams.clear();
    }

    void CSoundSystem::Resume()
    {
        paused = false;
        for (auto stream : streams)
        {
            if(stream->GetState() == CAudioStream::Playing) stream->Resume();
        }
    }

    void CSoundSystem::Pause()
    {
        paused = true;
        for (auto stream : streams)
        {
            stream->Pause(false);
        };
    }

    void CSoundSystem::Process()
    {
        if (CTimer::m_UserPause || CTimer::m_CodePause) // covers menu pausing, no disc in drive pausing, etc.
        {
            if (!paused) Pause();
        }
        else // not in menu
        {
            if (paused) Resume();

            // update globals
            timeStep = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped); // delta in seconds
            masterSpeed = CTimer::ms_fTimeScale;
            masterVolumeSfx = AEAudioHardware.m_fEffectMasterScalingFactor * AEAudioHardware.m_fEffectsFaderScalingFactor * 0.5f; // fit to game's sfx volume
            masterVolumeMusic = AEAudioHardware.m_fMusicMasterScalingFactor * AEAudioHardware.m_fMusicFaderScalingFactor * 0.5f;

            // prevent camera jump-cut glitches
            int skipFramePrev = skipFrame;
            skipFrame = TheCamera.m_bJust_Switched || TheCamera.m_bCameraJustRestored || CPad::GetPad(0)->JustOutOfFrontEnd;

            CVector prevPos = position;
            position = TheCamera.GetPosition();
            direction = TheCamera.GetForward();

            // new camera velocity
            if (!skipFrame)
            {
                CVector vel = position - prevPos;
                vel /= timeStep; // meters per second

                if (!skipFramePrev)
                {
                    // averaging to smooth artifacts caused by GTA's janky mouse camera control
                    velocity = (velocity * 2.0f) + vel;
                    velocity /= 3.0f;
                }
                else
                {
                    velocity = vel;
                }

                BASS_Set3DPosition(
                    &toBass(position),
                    &toBass(velocity),
                    &toBass(direction),
                    &toBass(TheCamera.GetUp())
                );
            }

            // process streams
            for (auto stream : streams) stream->Process();

            // apply changes
            if (!skipFrame) BASS_Apply3D();
        }
    }
}
