#pragma once
#include "bass.h"
#include "CVector.h"
#include <set>

namespace CLEO
{
    class CAudioStream;
    class C3DAudioStream;

    enum eStreamType
    {
        None = 0, // user controlled
        SoundEffect, // conforms to global SFX volume and in-game speed
        Music, // conforms to global music volume, muted if game speed is not 1.0
        UserInterface // conforms to global SFX volume
    };

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
        static CVector direction;
        static CVector velocity;
        static bool skipFrame; // do not apply changes during this frame
        static float timeStep; // delta time for current frame
        static float masterSpeed; // game simulation speed
        static float masterVolumeSfx;
        static float masterVolumeMusic;

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
    };

    static bool isNetworkSource(const char* path) { return _strnicmp("http:", path, 5) == 0 || _strnicmp("https:", path, 6) == 0; }
    static float dot(CVector a, CVector b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static float lerp(float a, float b, float progress) { return a * (1.0f - progress) + b * progress; }
    static CVector lerp(CVector a, CVector b, float progress) { return a * (1.0f - progress) + b * progress; }
    static BASS_3DVECTOR toBass(const CVector& v) { return BASS_3DVECTOR(v.x, v.z, v.y); } // convert GTA to BASS coordinate system
}
