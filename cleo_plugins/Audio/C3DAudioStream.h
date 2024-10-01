#pragma once
#include "CAudioStream.h"

namespace CLEO
{
    class C3DAudioStream : public CAudioStream
    {
    public:
        const float Volume_3D_Adjust = 0.25f; // to match other ingame 3d sounds

        C3DAudioStream(const char* filepath);

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
        float sourceRadius = 5.0f;

        C3DAudioStream(const C3DAudioStream&) = delete; // no copying!
        void UpdatePosition();
    };
}

