#pragma once
#include "CDebug.h"
#include "Mem.h"

namespace CLEO
{
    // a pointer automatically convertible to integral types
    union memory_pointer
    {
        size_t address;
        void *pointer;

        inline memory_pointer(void *p) : pointer(p) { }
        inline memory_pointer(size_t a) : address(a) { }
        inline operator void *() { return pointer; }
        inline operator size_t() { return address; }
        inline memory_pointer& operator=(void* p) { address = (size_t)p; return *this; }
        inline memory_pointer& operator=(size_t p) { address = (size_t)p; return *this; }

        inline bool operator==(const void* p) const { return address == (size_t)p; }
        inline bool operator!=(const void* p) const { return address != (size_t)p; }
        inline bool operator==(const memory_pointer& p) const { return address == p.address; }
        inline bool operator!=(const memory_pointer& p) const { return address != p.address; }

        // conversion to/from any-type pointer
        template<typename T>
        inline memory_pointer(T *p) : pointer(reinterpret_cast<void *>(p)) {}
        template<typename T>
        inline operator T *() { return reinterpret_cast<T *>(pointer); }
        template<typename T>
        inline memory_pointer& operator=(T *p) { return *this = reinterpret_cast<void *>(p); }
    };

    VALIDATE_SIZE(memory_pointer, 4);

    const memory_pointer memory_und = nullptr;

    // Implements dirty tricks for low-level memory managemant
    class CCodeInjector
    {
        bool bAccessOpen;

    public:
        CCodeInjector() 
        {
            bAccessOpen = false;
            OpenReadWriteAccess(); // moved here so that access is open for plugins
        }

        ~CCodeInjector() = default;

        void OpenReadWriteAccess();
        void CloseReadWriteAccess();

        template<typename T>
        void ReplaceFunction(T *funcPtr, memory_pointer position, T** origFuncPtr = nullptr)
        {
            MemCall((size_t)position, (size_t)funcPtr, (size_t*)origFuncPtr);

            if (origFuncPtr == nullptr) { TRACE("Replaced call at: 0x%08X", (DWORD)position); }
            else { TRACE("Replaced call at: 0x%08X, original function was: 0x%08X", (DWORD)position, (DWORD)*origFuncPtr); }
        }

        void ReplaceJump(memory_pointer newJumpDst, memory_pointer position, memory_pointer* origJumpDest = nullptr)
        {
            MemJump((size_t)position, (size_t)newJumpDst, (size_t*)origJumpDest);

            if (origJumpDest == nullptr) { TRACE("Replaced jump at: 0x%08X", (DWORD)position); }
            else { TRACE("Replaced jump at: 0x%08X, original destination was: 0x%08X", (DWORD)position, (DWORD)origJumpDest->address); }
        }

        template<typename T>
        void InjectFunction(T *funcPtr, memory_pointer position)
        {
            TRACE("Injecting function at: 0x%08X", (DWORD)position);
            MemJump((size_t)position, (size_t)funcPtr);
        }

        void Nop(memory_pointer addr, size_t size)
        {
            MemFill(addr, OP_NOP, size);
        }

        template<typename T> void MemoryReadOffset(memory_pointer addr, T& result, bool force_vp = false)
        {
            DWORD oldProtect;
            bool vp = force_vp;
            if (vp) VirtualProtect(addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
            result = MemReadOffsetPtr<T>(addr.address);
            //if (vp) VirtualProtect(addr, sizeof(T), oldProtect, &oldProtect);
        }

        // copies object given by memory address @addr to the @result object
        template<typename T> void MemoryRead(memory_pointer addr, T& result, bool force_vp = false)
        {
            DWORD oldProtect;
            bool vp = force_vp;
            if (vp) VirtualProtect(addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
            result = MemRead<T>(addr);
            //if (vp) VirtualProtect(addr, sizeof(T), oldProtect, &oldProtect);
        }

        // copies array of objects given by memory address @addr to the @result array of length @cnt
        template<typename T> void MemoryRead(memory_pointer addr, T *result, size_t n, bool force_vp = false)
        {
            DWORD oldProtect;
            bool vp = force_vp;
            if (vp) VirtualProtect(addr, sizeof(T) * n, PAGE_EXECUTE_READWRITE, &oldProtect);
            MemCopy(result, addr, n);
            //if (vp) VirtualProtect(addr, sizeof(T) * n, oldProtect, &oldProtect);
        }

        // copies @proto object to the object given by memory address @addr
        template<typename T> void MemoryWrite(memory_pointer addr, const T& proto, bool force_vp = false, size_t n = 1)
        {
            DWORD oldProtect;
            if (force_vp) VirtualProtect(addr, sizeof(T) * n, PAGE_EXECUTE_READWRITE, &oldProtect);
            if (n != 1) MemFill(addr, proto, n);
            else MemWrite(addr, proto);
            //if(force_vp) VirtualProtect(addr, sizeof(T) * n, oldProtect, &oldProtect);
        }

        // copies array of objects @proto of length @cnt to array of objects given by memory address @addr
        template<typename T> void MemoryWrite(memory_pointer addr, const T *proto, size_t n, bool force_vp = false)
        {
            DWORD oldProtect;
            if (force_vp) VirtualProtect(addr, sizeof(T) * n, PAGE_EXECUTE_READWRITE, &oldProtect);
            MemCopy(addr, proto, n);
            //if(force_vp) VirtualProtect(addr, sizeof(T) * n, oldProtect, &oldProtect);
        }
    };
}
