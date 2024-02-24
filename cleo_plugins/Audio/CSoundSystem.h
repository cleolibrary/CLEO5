#pragma once
#include "CLEO.h"
#include "CLEO_Utils.h"
#include "plugin.h"
#include "bass.h"
#include <set>

#pragma comment(lib, "bass.lib")

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
        static float speed; // time scale
        static float volume; // global volume

    public:
        bool Init();
        inline bool Initialized() { return initialized; }

        CSoundSystem()
        {
            // TODO: give to user an ability to force a sound device to use (ini-file or cmd-line?)
        }

        ~CSoundSystem()
        {
            TRACE("Closing SoundSystem...");
            UnloadAllStreams();
            if (initialized)
            {
                TRACE("Freeing BASS library");
                BASS_Free();
                initialized = false;
            }
            TRACE("SoundSystem closed!");
        }

        CAudioStream* LoadStream(const char *filename, bool in3d = false);
        void PauseStreams();
        void ResumeStreams();
        void UnloadStream(CAudioStream *stream);
        void UnloadAllStreams();
        void Update(bool active = true);
        bool HasStream(CAudioStream* stream) { return streams.find(stream) != streams.end(); };
    };

    class CAudioStream
    {
        friend class CSoundSystem;

        CAudioStream(const CAudioStream&);

    protected:
        HSTREAM streamInternal = 0;

        enum eStreamState
        {
            no,
            playing,
            paused,
            stopped,
        } state = no;

        bool OK = false;
        float rate = 44100.0f; // file's sampling rate
        double speed = 1.0f;
        double volume = 1.0f;

        // transition
        double volumeTarget = 1.0f;
        double volumeTransitionStep = 1.0f;
        double speedTarget = 1.0f;
        double speedTransitionStep = 1.0f;

        CAudioStream();

    public:
        CAudioStream(const char *src);
        virtual ~CAudioStream();

        // actions on streams
        void Play();
        void Pause(bool change_state = true);
        void Stop();
        void Resume();
        float GetLength() const;
        DWORD GetState() const;

        void SetVolume(float val, float transitionTime = 0.0f);
        float GetVolume() const;

        void SetSpeed(float val, float transitionTime = 0.0f);
        float GetSpeed() const;

        void Loop(bool enable);
		HSTREAM GetInternal();

        // overloadable actions
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSize(float radius);
        virtual void Link(CPlaceable *placable = nullptr);
        virtual void Process(bool force = false);
    };

    class C3DAudioStream : public CAudioStream
    {
        friend class CSoundSystem;

        C3DAudioStream(const C3DAudioStream&);

    protected:
        CPlaceable* link = nullptr;
        BASS_3DVECTOR position = { 0.0f, 0.0f, 0.0f };
    public:
        C3DAudioStream(const char *src);
        virtual ~C3DAudioStream();

        // overloaded actions
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSize(float radius);
        virtual void Link(CPlaceable *placable = nullptr);
        virtual void Process(bool force = false);
    };
}
