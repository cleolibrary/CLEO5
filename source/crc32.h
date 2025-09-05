#pragma once
#include <string>

unsigned long crc32(const unsigned char* buf, unsigned long len);
unsigned long crc32FromUpcaseString(const char* str);
unsigned long crc32FromUpcaseStdString(const std::string& str);
unsigned long crc32FromString(const char* str);
unsigned long crc32FromStdString(const std::string& str);
