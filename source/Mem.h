#pragma once

#define OP_NOP			0x90
#define OP_RET			0xC3
#define OP_CALL			0xE8
#define OP_JMP			0xE9
#define OP_JMPSHORT		0xEB

template<typename T, typename U>
inline void MemWrite(U p, const T v) { *(T*)p = v; }
template<typename T, typename U>
inline void MemWrite(U p, const T v, int n) { memcpy((void*)p, &v, n); }
template<typename T, typename U>
inline T MemRead(U p) { return *(T*)p; }
template<typename T, typename U>
inline void MemFill(U p, T v, int n) { memset((void*)p, (int)v, n); }
template<typename T, typename U>
inline void MemCopy(U p, const T v) { memcpy((void*)p, &v, sizeof(T)); }
template<typename T, typename U>
inline void MemCopy(U p, const T v, int n) { memcpy((void*)p, &v, n); }
template<typename T, typename U>
inline void MemCopy(U p, const T* v) { memcpy((void*)p, v, sizeof(T)); }
template<typename T, typename U>
inline void MemCopy(U p, const T* v, int n) { memcpy((void*)p, v, n); }

// Write a jump to v to the address at p and copy the replaced jump address to r
template<typename T, typename U>
inline void MemJump(U p, const T v, T *r = nullptr)
{
    if (r != nullptr)
    {
        *r = MemReadInstrucionDestination<std::remove_pointer<T>::type>(p);
    }

    MemWrite<BYTE>(p++, OP_JMP);
    MemWrite<DWORD>(p, ((DWORD)v - (DWORD)p) - 4);
}

// Write a call to v to the address at p and copy the replaced call address to r
template<typename T, typename U>
inline void MemCall(U p, const T v, T *r = nullptr)
{
    if (r != nullptr)
    {
        *r = MemReadInstrucionDestination<std::remove_pointer<T>::type>(p);
    }

    MemWrite<BYTE>(p++, OP_CALL);
    MemWrite<DWORD>(p, (DWORD)v - (DWORD)p - 4);
}

// Read and convert a relative offset to absolute address
template<typename T, typename U>
T MemReadOffsetPtr(U p)
{
    return (T)((size_t)MemRead<T>(p) + (size_t)p + sizeof(T));
}

// Read absolute target address of jump or call instruction
template<typename T, typename U>
T MemReadInstrucionDestination(U p)
{
    auto ptr = (BYTE*)p;
    BYTE opcode = *ptr;
    ptr++;

    T dest = (T)nullptr;
    switch (opcode)
    {
    case OP_CALL:
        dest = MemReadOffsetPtr<DWORD>(ptr);
        break;

    case OP_JMP:
        dest = MemReadOffsetPtr<DWORD>(ptr);
        break;

    case OP_JMPSHORT:
        dest = MemReadOffsetPtr<BYTE>(ptr);
        break;
    }

    return dest;
}

inline void MemGrantAccess(size_t address, size_t size)
{
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}
