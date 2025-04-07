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
inline void MemFill(U p, T v, int n) { memset((void*)p, v, n); }
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
        switch (MemRead<BYTE>(p))
        {
            case OP_JMP:
                *r = (T)(DWORD(p) + 5 + MemRead<signed int>(p + 1));
                break;

            case OP_JMPSHORT:
                *r = (T)(DWORD(p) + 2 + MemRead<signed char>(p + 1));
                break;

            default:
                *r = (T)nullptr;
                break;
        }
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
        if (MemRead<BYTE>(p) == OP_CALL)
            *r = (T)(DWORD(p) + 5 + MemRead<signed int>(p + 1));
        else
            *r = (T)nullptr;
    }

    MemWrite<BYTE>(p++, OP_CALL);
    MemWrite<DWORD>(p, (DWORD)v - (DWORD)p - 4);
}

// Read and convert a relative offset to full
template<typename T, typename U>
T MemReadOffsetPtr(U p)
{
    return (T)((size_t)MemRead<T>(p) + p + sizeof(T));
}

inline void MemGrantAccess(size_t address, size_t size)
{
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}
