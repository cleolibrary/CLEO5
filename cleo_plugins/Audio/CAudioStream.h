#pragma once
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

        void Loop(bool enable); // enable looping

        float GetLength() const;

        void SetVolume(float val, float transitionTime = 0.0f);
        float GetVolume() const;

        void SetSpeed(float val, float transitionTime = 0.0f);
        float GetSpeed() const;

        // 3d
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSize(float radius);
        virtual void Link(CPlaceable* placable = nullptr);

        virtual void Process();

    protected:
        HSTREAM streamInternal = 0;
        eStreamState state = Stopped;
        bool ok = false;
        float rate = 44100.0f; // file's sampling rate
        double speed = 1.0f;
        double volume = 1.0f;

        // transitions
        double volumeTarget = 1.0f;
        double volumeTransitionStep = 1.0f;
        double speedTarget = 1.0f;
        double speedTransitionStep = 1.0f;

        CAudioStream() = default;
        CAudioStream(const CAudioStream&) = delete; // no copying!

        void UpdateVolume();
        void UpdateSpeed();
    };
}
