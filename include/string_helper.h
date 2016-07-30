
#ifndef __STRING_HELPER_H__
#define __STRING_HELPER_H__

#include <string>

class STR
{
public:
    static std::string GetKeyValue(const std::string &source, const std::string left, const std::string right);
    static std::string GetRKeyValue(const std::string &source, const std::string left, const std::string right);
    static std::string Trim(const std::string &source, const std::string separators=" ");
    static std::string ToString(long long value);
    static size_t Str2UInt64(std::string value);
};

#endif
