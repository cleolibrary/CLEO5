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
        None = 0,
        SoundEffect,
        Music,
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
    
    bool isNetworkSource(const char* path);
}
