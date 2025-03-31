#pragma once
#include "CAudioStream.h"

namespace CLEO
{
    class C3DAudioStream : public CAudioStream
    {
    public:
        C3DAudioStream(const char* filepath);

        // overloaded actions
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSourceSize(float radius);
        virtual void SetHost(CEntity* host, const CVector& offset);
        virtual void Process();
        virtual float CalculateVolume();
        virtual float CalculateSpeed();

    protected:
        const float Volume_3D_Adjust = 0.5f; // match other ingame sound sources
        static double CalculateDistanceDecay(float radius, float distance);
        static float CalculateDirectionDecay(const CVector& listenerDir, const CVector& relativePos);

        CEntity* host = nullptr;
        eEntityType hostType = ENTITY_TYPE_NOTHING;
        CVector offset = { 0.0f, 0.0f, 0.0f }; // offset in relation to host
        float radius = 0.5f; // size of sound source

        bool placed = false;
        CVector position = { 0.0f, 0.0f, 0.0f }; // last world position
        CVector velocity = { 0.0f, 0.0f, 0.0f };

        C3DAudioStream(const C3DAudioStream&) = delete; // no copying!
        void UpdatePosition();
    };
}

