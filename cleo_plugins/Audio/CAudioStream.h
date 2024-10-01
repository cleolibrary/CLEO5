#pragma once
#include "CSoundSystem.h"
#include "plugin.h"
#include "bass.h"

namespace CLEO
{
    class CAudioStream
    {
    public:
        enum StreamState
        {
            Stopped = -1,
            PlayingInactive, // internal: playing, but not processed yet or the sound system is silenced right now
            Playing,
            Paused,
        };

        enum StreamParam : size_t
        {
            Speed,
            Volume,
            StreamParamMax
        };

        CAudioStream(const char* filepath); // filesystem or URL
        virtual ~CAudioStream();

        bool IsOk() const;
        HSTREAM GetInternal(); // get BASS stream

        StreamState GetState() const;
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
        virtual void Link(CEntity* entity = nullptr);

        virtual void Process();

    protected:
        HSTREAM streamInternal = 0;
        StreamState state = Paused;
        eStreamType type = eStreamType::SoundEffect;
        bool ok = false;
        float rate = 44100.0f; // file's sampling rate

        float paramCurrent[StreamParam::StreamParamMax];
        float paramTarget[StreamParam::StreamParamMax];
        float paramTransitionStep[StreamParam::StreamParamMax];

        CAudioStream();
        CAudioStream(const CAudioStream&) = delete; // no copying!

        void SetParam(StreamParam param, float value, float transitionTime = 0.0);
        float GetParam(StreamParam param);
        void ResetParams(); // set defaults

        // processing
        virtual float CalculateVolume();
        void ProcessTransitions(bool instant = false); // process param transitions
        void ApplyParams(); // send params to BASS
    };
}
