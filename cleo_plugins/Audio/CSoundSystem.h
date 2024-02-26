#pragma once
#include "bass.h"
#include <set>

namespace CLEO
{
    class CAudioStream;
    class C3DAudioStream;

    class CSoundSystem
    {
        friend class CAudioStream;
        friend class C3DAudioStream;

        std::set<CAudioStream*> streams;
        BASS_INFO SoundDevice = { 0 };
        bool initialized = false;
        int forceDevice = -1;
        bool paused = false;

        static bool useFloatAudio;

        static BASS_3DVECTOR pos;
        static BASS_3DVECTOR vel;
        static BASS_3DVECTOR front;
        static BASS_3DVECTOR top;
        static float masterSpeed; // game simulation speed
        static float masterVolume;

    public:
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
}
