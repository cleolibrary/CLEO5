#pragma once

#include <set>
class ArenaAllocator
{
    void* m_memory;
    size_t m_size;
    size_t m_usedMemory;
    bool m_autoGrow;
public:

    ArenaAllocator(size_t size);
    ~ArenaAllocator();

    ArenaAllocator(ArenaAllocator&& other) noexcept;
    // Delete copy constructor and assignment to force move-only
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    void* allocate(size_t size);
    void reset();

    size_t getUsedMemory() const { return m_usedMemory; }
    size_t getTotalMemory() const { return m_size; }
};

