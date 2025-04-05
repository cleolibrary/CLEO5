#pragma once
#include "CCodeInjector.h"

namespace CLEO
{
    class CGameMenu : public VInjectible
    {
    public:
        virtual void Inject(CCodeInjector& inj);
    };
}
