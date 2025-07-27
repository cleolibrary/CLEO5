#include "ArenaAllocator.h"
#include "CLEO.h"
#include "plugin.h"
#include <set>

using namespace CLEO;

ArenaAllocator::ArenaAllocator(size_t size) 
    : m_memory(nullptr), m_size(size), m_usedMemory(0), m_autoGrow(size == 0)
{  
    if (size == 0)
    {
        m_size = 256; // default size if not specified, will grow as needed
    }
    m_memory = calloc(m_size, 1);
}

ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept
    : m_memory(other.m_memory), m_size(other.m_size), m_usedMemory(other.m_usedMemory), m_autoGrow(other.m_autoGrow)
{
    other.m_memory = nullptr; // transfer ownership
    other.m_size = 0;
    other.m_usedMemory = 0;
    other.m_autoGrow = false;
}

ArenaAllocator::~ArenaAllocator()
{
    if (m_memory)
    {
        free(m_memory);
        m_memory = nullptr;
    }
}
void* ArenaAllocator::allocate(size_t size)
{
    if (m_usedMemory + size <= m_size)
    {
        // enough memory: happy path
        void* ptr = static_cast<char*>(m_memory) + m_usedMemory;
        m_usedMemory += size;
        return ptr;
    }

    if (!m_autoGrow)
    {
        return nullptr; // out of arena memory
    }

    size_t newSize = m_size * 2;
    void* newMemory = realloc(m_memory, newSize);
    if (!newMemory)
    {
        return nullptr; // out of process memory
    }
    m_memory = newMemory;
    m_size = newSize;

    return allocate(size); // retry allocation with the new size
}

void ArenaAllocator::reset()
{
    m_usedMemory = 0; // reset used memory
}
