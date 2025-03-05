#pragma once
#include "bass.h"
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

        static BASS_3DVECTOR pos;
        static BASS_3DVECTOR vel;
        static BASS_3DVECTOR front;
        static BASS_3DVECTOR top;
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

    bool isNetworkSource(const char* path);
}
