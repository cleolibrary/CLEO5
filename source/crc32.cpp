#include "crc32.h"
#include <array>
#include <ctype.h>

// CRC-32 lookup table.
static constexpr std::array<unsigned long, 256> MakeCrc32Table()
{
    std::array<unsigned long, 256> table{};
    for (unsigned long i = 0; i < 256; i++)
    {
        unsigned long crc = i;
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? (0xEDB88320UL ^ (crc >> 1)) : (crc >> 1);
        table[i] = crc;
    }
    return table;
}

constexpr auto crcTable = MakeCrc32Table();

unsigned long crc32FromUpcaseString(const char* str)
{
    unsigned long crc = 0xFFFFFFFF;
    while (*str)
        crc = crcTable[(crc ^ toupper(*str++)) & 0xff] ^ (crc >> 8);
    return crc;
}

unsigned long crc32FromUpcaseStdString(const std::string& str)
{
    return crc32FromUpcaseString(str.c_str());
}

unsigned long crc32FromString(const char* str)
{
    unsigned long crc = 0xFFFFFFFF;
    while (*str)
        crc = crcTable[(crc ^ (*str++)) & 0xff] ^ (crc >> 8);
    return crc;
}

unsigned long crc32FromStdString(const std::string& str)
{
    return crc32FromString(str.c_str());
}

unsigned long crc32(const unsigned char* buf, unsigned long len)
{
    unsigned long crc = 0xFFFFFFFF;
    for (unsigned i = 0; i < len; i++)
        crc = crcTable[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}
