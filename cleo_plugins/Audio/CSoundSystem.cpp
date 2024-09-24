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

    CVector CSoundSystem::position(0.0, 0.0, 0.0);
    CVector CSoundSystem::forward(0.0, 1.0, 0.0);
    CVector CSoundSystem::up(0.0, 0.0, 1.0);
    CVector CSoundSystem::velocity(0.0, 0.0, 0.0);
    bool CSoundSystem::skipFrame = true;
    float CSoundSystem::timeStep = 0.02f;
    float CSoundSystem::masterSpeed = 1.0f;

    eStreamType CSoundSystem::LegacyModeDefaultStreamType = eStreamType::None;

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

    bool isNetworkSource(const char* path)
    {
        return _strnicmp("http:", path, 5) == 0 ||
            _strnicmp("https:", path, 6) == 0;
    }

    CSoundSystem::~CSoundSystem()
    {
        TRACE(""); // seaprator
        TRACE("Finalizing SoundSystem...");
        Clear();

        if (initialized)
        {
            TRACE("Freeing BASS library");
            BASS_Free();
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

        if (BASS_Init(deviceIndex, 44100, BASS_DEVICE_3D, RsGlobal.ps->window, nullptr))
        {
            TRACE("BASS sound system initialized");

            // Can we use floating-point (HQ) audio streams?
            DWORD floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, NULL); // floating-point channel support? 0 = no, else yes
            if (floatable)
            {
                TRACE("Floating-point audio supported");
                useFloatAudio = true;
                BASS_StreamFree(floatable);
            }
            else TRACE("Floating-point audio not supported");

            if (BASS_GetInfo(&SoundDevice))
            {
                if (SoundDevice.flags & DSCAPS_EMULDRIVER)
                    TRACE("Audio drivers not installed - using DirectSound emulation");

                if (!SoundDevice.eax)
                    TRACE("Audio hardware acceleration disabled (no EAX)");
            }

            BASS_Set3DFactors(1.0f, 0.0f, 1.0f); // rollf emulated by us in 3D streams

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
            // update globals
            skipFrame = TheCamera.m_bJust_Switched || TheCamera.m_bCameraJustRestored || CPad::GetPad(0)->JustOutOfFrontEnd; // avoid camera change/jump cut velocity glitches
            timeStep = (float)(CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped) / 1000.0f; // time delta in seconds
            masterSpeed = CTimer::ms_fTimeScale;

            CVector prevPos = position;
            position = TheCamera.GetPosition(); // get new

            if (!skipFrame)
            {
                if (paused) Resume();

                // camera velocity
                velocity = position - prevPos;
                velocity /= timeStep; // meters peer second

                // make Doppler effect less dramatic
                velocity.x = sqrtf(abs(velocity.x)) * (velocity.x > 0.0f ? 1.0f : -1.0f);
                velocity.y = sqrtf(abs(velocity.y)) * (velocity.y > 0.0f ? 1.0f : -1.0f);
                velocity.z = sqrtf(abs(velocity.z)) * (velocity.z > 0.0f ? 1.0f : -1.0f);

                forward = TheCamera.GetForward();
                up = TheCamera.GetUp();

                BASS_Set3DPosition(
                    &toBass(position),
                    &toBass(velocity),
                    &toBass(forward),
                    &toBass(up)
                );
            }

            // process streams
            for (auto stream : streams) stream->Process();

            // apply changes
            if (!skipFrame) BASS_Apply3D();
        }
    }

    float CSoundSystem::GetDistance(const CVector* position)
    {
        auto v = CSoundSystem::position;
        v -= *position;
        return v.Magnitude();
    }

    float CSoundSystem::GetMasterVolume(eStreamType type)
    {
        // global volume settings
        float volume;
        switch (type)
        {
            case SoundEffect: 
                volume = AEAudioHardware.m_fEffectMasterScalingFactor* AEAudioHardware.m_fEffectsFaderScalingFactor * 0.5f; // fitted to game's sfx volume
                break;

            case Music: 
                volume = AEAudioHardware.m_fMusicMasterScalingFactor * AEAudioHardware.m_fMusicFaderScalingFactor * 0.5f;
                break;

            default: 
                volume = 1.0f; 
                break;
        }

        // screen fading
        volume = 1.0f - TheCamera.m_fFadeAlpha / 255.0f; // TODO: handle TheCamera.m_bIgnoreFadingStuffForMusic if neccessary

        // TODO: muscic volume lowering when in cutscenes, when mission or ped speach is acttive etc.

        return volume;
    }
}
