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

    protected:
        CEntity* host = nullptr;
        eEntityType hostType = ENTITY_TYPE_NOTHING;
        CVector offset = { 0.0f, 0.0f, 0.0f }; // offset in relation to host

        CVector position = { 0.0f, 0.0f, 0.0f }; // last world position

        C3DAudioStream(const C3DAudioStream&) = delete; // no copying!
        void UpdatePosition();
    };
}

