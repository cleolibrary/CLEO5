#pragma once
#include "CAudioStream.h"

namespace CLEO
{
    class C3DAudioStream : public CAudioStream
    {
    public:
        const float Volume_3D_Adjust = 0.333f; // to match other ingame sound sources

        C3DAudioStream(const char* filepath);
        virtual ~C3DAudioStream();

        // overloaded actions
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSourceSize(float radius);
        virtual void Link(CEntity* entity = nullptr);
        virtual void Process();
        virtual float CalculateVolume();

    protected:
        CEntity* host = nullptr;
        CVector position = { 0.0f, 0.0f, 0.0f };
        bool placed = false; // position calcualted?
        float sourceRadius = 10.0f; // same as in play_mission_audio opcode

        C3DAudioStream(const C3DAudioStream&) = delete; // no copying!
        void UpdatePosition();
    };
}

