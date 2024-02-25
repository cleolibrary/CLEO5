#pragma once
#include "CAudioStream.h"

namespace CLEO
{
    class C3DAudioStream : public CAudioStream
    {
    public:
        C3DAudioStream(const char* filepath);
        virtual ~C3DAudioStream();

        // overloaded actions
        virtual void Set3dPosition(const CVector& pos);
        virtual void Set3dSourceSize(float radius);
        virtual void Link(CPlaceable* placable = nullptr);
        virtual void Process();

    protected:
        CPlaceable* link = nullptr;
        BASS_3DVECTOR position = { 0.0f, 0.0f, 0.0f };

        C3DAudioStream(const C3DAudioStream&) = delete; // no copying!
        void UpdatePosition();
    };
}

