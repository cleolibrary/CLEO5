#include "stdafx.h"
#include "ScmFunction.h"
#include "CCustomOpcodeSystem.h"
#include "CScriptEngine.h"

namespace CLEO
{
    ScmFunction* ScmFunction::store[Store_Size] = {0};
    WORD ScmFunction::lastAllocIdx              = 0;

    ScmFunction* ScmFunction::Get(WORD id)
    {
        if (id == Id_None) return nullptr;

        auto idx = id - 1; // skip Id_None

        if (idx >= Store_Size || store[idx] == nullptr)
        {
            SHOW_ERROR("CLEO function with id %d not found in the storage!", id);
            return nullptr;
        }

        return store[idx];
    }

    void ScmFunction::Clear()
    {
        for (auto scmFunc : store)
        {
            if (scmFunc != nullptr) delete scmFunc;
        }
        lastAllocIdx = 0;
    }

    void* ScmFunction::operator new(size_t size)
    {
        auto searchStart = lastAllocIdx;
        while (store[lastAllocIdx] != nullptr) // find first unused position in store
        {
            lastAllocIdx++;
            if (lastAllocIdx >= Store_Size) lastAllocIdx = 0; // warp around

            if (lastAllocIdx == searchStart)
            {
                SHOW_ERROR("CLEO function storage stack overfllow!");
                throw std::bad_alloc(); // the store is filled up
            }
        }

        auto func           = reinterpret_cast<ScmFunction*>(::operator new(size));
        store[lastAllocIdx] = func;
        return func;
    }

    void ScmFunction::operator delete(void* mem)
    {
        auto idx   = reinterpret_cast<ScmFunction*>(mem)->thisScmFunctionId - 1;
        store[idx] = nullptr;
        ::operator delete(mem);
    }

    ScmFunction::ScmFunction(CLEO::CRunningScript* thread, BYTE* callIP)
    {
        auto cs = reinterpret_cast<CCustomScript*>(thread);

        thisScmFunctionId = ScmFunction::lastAllocIdx + 1; // skip Id_None
        prevScmFunctionId = cs->GetScmFunction();
        this->callIP      = callIP;

        // create snapshot of current scope
        savedBaseIP   = cs->BaseIP;
        savedCodeSize = cs->IsCustom() ? cs->GetCodeSize() : -1;

        std::copy(std::begin(cs->Stack), std::end(cs->Stack), std::begin(savedStack));
        savedSP = cs->SP;

        auto scope = cs->IsMission() ? missionLocals : cs->LocalVar;
        std::copy(scope, scope + _countof(CRunningScript::LocalVar), savedLocalVar.begin());

        savedCondResult = cs->bCondResult;
        savedLogicalOp  = cs->LogicalOp;
        savedNotFlag    = cs->NotFlag;

        savedScriptFileDir  = cs->GetScriptFileDir();
        savedScriptFileName = cs->GetScriptFileName();

        // init new scope
        std::fill(std::begin(cs->Stack), std::end(cs->Stack), nullptr);
        cs->SP          = 0;
        cs->bCondResult = false;
        cs->LogicalOp   = eLogicalOperation::NONE;
        cs->NotFlag     = false;

        cs->SetScmFunction(thisScmFunctionId);
    }

    void ScmFunction::Return(CRunningScript* thread)
    {
        auto cs = reinterpret_cast<CCustomScript*>(thread);

        cs->BaseIP = savedBaseIP;
        if (cs->IsCustom()) cs->SetCodeSize(savedCodeSize);

        // restore parent scope's gosub call stack
        std::copy(savedStack.begin(), savedStack.end(), std::begin(cs->Stack));
        cs->SP = savedSP;

        // restore parent scope's local variables
        std::copy(savedLocalVar.begin(), savedLocalVar.end(), cs->IsMission() ? missionLocals : cs->LocalVar);

        // process conditional result of just ended function in parent scope
        bool condResult = cs->bCondResult;
        if (savedNotFlag) condResult = !condResult;

        if (savedLogicalOp >= eLogicalOperation::ANDS_1 && savedLogicalOp < eLogicalOperation::AND_END)
        {
            cs->bCondResult = savedCondResult && condResult;
            cs->LogicalOp   = --savedLogicalOp;
        }
        else if (savedLogicalOp >= eLogicalOperation::ORS_1 && savedLogicalOp < eLogicalOperation::OR_END)
        {
            cs->bCondResult = savedCondResult || condResult;
            cs->LogicalOp   = --savedLogicalOp;
        }
        else // eLogicalOperation::NONE
        {
            cs->bCondResult = condResult;
            cs->LogicalOp   = savedLogicalOp;
        }

        cs->SetScriptFileDir(savedScriptFileDir.c_str());
        cs->SetScriptFileName(savedScriptFileName.c_str());

        cs->SetIp(retnAddress);
        cs->SetScmFunction(prevScmFunctionId);
    }

    size_t ScmFunction::GetCallStackSize() const
    {
        if (prevScmFunctionId == Id_None) return 0;
        return 1 + Get(prevScmFunctionId)->GetCallStackSize();
    }
}; // namespace CLEO
