#pragma once
#include "..\cleo_sdk\CLEO.h"
#include "CDebug.h"

namespace CLEO
{
    class ScmFunction
    {
      public:
        static const WORD Id_None = 0;
        static ScmFunction* Get(WORD id);
        static void Clear(); // clear storage

        BYTE callArgCount = 0;       // args provided to cleo_call
        BYTE* callIP      = nullptr; // address of 0AB1 for error messages

        // saved nesting context state
        void* savedBaseIP                                                        = nullptr;
        size_t savedCodeSize                                                     = 0; // Custom Scripts only
        BYTE* retnAddress                                                        = nullptr;
        std::array<BYTE*, _countof(CRunningScript::Stack)> savedStack            = {}; // gosub stack
        WORD savedSP                                                             = 0;
        std::array<SCRIPT_VAR, _countof(CRunningScript::LocalVar)> savedLocalVar = {};
        std::list<std::string> stringParams; // texts with this scope lifetime
        bool savedCondResult             = false;
        eLogicalOperation savedLogicalOp = {};
        bool savedNotFlag                = false;
        std::string savedScriptFileDir;  // modules switching
        std::string savedScriptFileName; // modules switching

        void* operator new(size_t size);
        void operator delete(void* mem);
        ScmFunction(CRunningScript* thread, BYTE* callIP);

        void Return(CRunningScript* thread);

        size_t GetCallStackSize() const;

      private:
        static const WORD Store_Size = 0x400;
        static ScmFunction* store[Store_Size];
        static WORD lastAllocIdx;

        WORD prevScmFunctionId = Id_None;
        WORD thisScmFunctionId = Id_None;
    };
} // namespace CLEO
