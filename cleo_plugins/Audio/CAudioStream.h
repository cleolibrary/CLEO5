#pragma once
#include "CSoundSystem.h"
#include "plugin.h"
#include "bass.h"

namespace CLEO
{
    class CAudioStream
    {
    public:
        enum eStreamState
        {
            Stopped = -1,
            PlayingInactive, // internal: playing, but not processed yet or the sound system is silenced right now
            Playing,
            Paused,
        };

        CAudioStream(const char* filepath); // filesystem or URL
        virtual ~CAudioStream();

        bool IsOk() const;
        HSTREAM GetInternal(); // get BASS stream

        eStreamState GetState() const;
        void Play();
        void Pause(bool changeState = true);
        void Stop();
        void Resume();

        void SetLooping(bool enable);
        bool GetLooping() const;

        float GetLength() const;

        void SetProgress(float value);
        float GetProgress() const;

        void SetSpeed(float value, float transitionTime = 0.0f);
        float GetSpeed() const;

        void SetType(eStreamType value);
        eStreamType GetType() const;

        void SetVolume(float value, float transitionTime = 0.0f);
        float GetVolume() const;

        // 3d
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSourceSize(float radius);
        virtual void Link(CPlaceable* placable = nullptr);

        virtual void Process();

    protected:
        HSTREAM streamInternal = 0;
        eStreamState state = Paused;
        eStreamType type = eStreamType::SoundEffect;
        bool ok = false;
        float rate = 44100.0f; // file's sampling rate
        float speed = 1.0f;
        float volume = 1.0f;

        // transitions
        float volumeTarget = 1.0f;
        float volumeTransitionStep = 1.0f;
        float speedTarget = 1.0f;
        float speedTransitionStep = 1.0f;

        CAudioStream() = default;
        CAudioStream(const CAudioStream&) = delete; // no copying!

        void UpdateVolume();
        void UpdateSpeed();
    };
}
