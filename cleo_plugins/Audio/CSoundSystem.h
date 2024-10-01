#pragma once
#include "CLEO_Utils.h"
#include "bass.h"
#include "CVector.h"
#include <set>

namespace CLEO
{
    class CAudioStream;
    class C3DAudioStream;

    enum eStreamType
    {
        None = 0,
        SoundEffect,
        Music,
    };

    // functions inside game executable
    static long double(__cdecl* CAEAudioUtility__AudioLog10)(float value) = memory_pointer(0x004D9E50);
    static double(__cdecl* CAEAudioEnvironment__GetDistanceAttenuation)(float distance) = memory_pointer(0x004D7F20);

    class CSoundSystem
    {
        friend class CAudioStream;
        friend class C3DAudioStream;

        std::set<CAudioStream*> streams;
        BASS_INFO SoundDevice = { 0 };
        bool initialized = false;
        bool paused = false;

        static bool useFloatAudio;
        static bool CSoundSystem::allowNetworkSources;

        static CVector position;
        static CVector forward;
        static CVector up;
        static CVector velocity;
        static bool skipFrame; // do not apply changes during this frame
        static float timeStep; // delta time for current frame
        static float masterSpeed; // game simulation speed

    public:
        static eStreamType LegacyModeDefaultStreamType;

        CSoundSystem() = default; // TODO: give to user an ability to force a sound device to use (ini-file or cmd-line?)
        ~CSoundSystem();

        bool Init();
        bool Initialized();

        CAudioStream* CreateStream(const char *filename, bool in3d = false);
        void DestroyStream(CAudioStream *stream);

        bool HasStream(CAudioStream* stream);
        void Clear(); // destroy all created streams

        void Pause();
        void Resume();
        void Process();

        static float GetDistance(const CVector* position); // distance from coords to the camera
        static float GetMasterVolume(eStreamType type);
    };

    // convert GTA to BASS coordinate system
    inline BASS_3DVECTOR toBass(const CVector& v) { return BASS_3DVECTOR(v.x, v.z, v.y); }
    inline BASS_3DVECTOR toBass(const CVector* v) { return toBass(*v); }

    // convert Decibel gain (-100.0 - 0.0) to linear factor (0.0 - 1.0)
    inline float dbToLinear(float value) { return powf(10.0f, value / 20.0f); }
    
    bool isNetworkSource(const char* path);
}
