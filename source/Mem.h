#pragma once
#include <cstring> 

using BYTE = unsigned char;
using DWORD = unsigned long;

constexpr BYTE OP_JMP = 0xE9;
constexpr BYTE OP_CALL = 0xE8;

template<typename T, typename U>
inline void MemWrite(U p, const T& v) { *(T*)p = v; } 

template<typename T, typename U>
inline void MemWrite(U p, const T& v, size_t n) { memcpy((void*)p, &v, n); } 

template<typename T, typename U>
inline T MemRead(U p) { return *(T*)p; }

template<typename T, typename U>
inline void MemFill(U p, T v, size_t n) { memset((void*)p, v, n); }

template<typename T, typename U>
inline void MemCopy(U dest, const T* src, size_t n) { memcpy((void*)dest, src, n); } 

template<typename T, typename U>
inline void MemJump(U p, const T v, T *r = nullptr)
{
    MemWrite<BYTE>(p, OP_JMP);
    p += sizeof(BYTE); 
    if (r) *r = (T)(MemRead<DWORD>((DWORD)p) + (DWORD)p + sizeof(DWORD));
    MemWrite<DWORD>((DWORD)p, ((DWORD)v - (DWORD)p) - sizeof(DWORD));
}

template<typename T, typename U>
inline void MemCall(U p, const T v, T *r = nullptr)
{
    MemWrite<BYTE>(p, OP_CALL);
    p += sizeof(BYTE); 
    if (r) *r = (T)(MemRead<DWORD>((DWORD)p) + (DWORD)p + sizeof(DWORD));
    MemWrite<DWORD>((DWORD)p, ((DWORD)v - (DWORD)p) - sizeof(DWORD));
}

template<typename T, typename U>
T MemReadOffsetPtr(U p)
{
    return (T)((size_t)MemRead<DWORD>((DWORD)p) + (DWORD)p + sizeof(DWORD));
}
