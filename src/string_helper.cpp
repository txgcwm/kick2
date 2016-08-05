
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "common.h"
#include "string_helper.h"

std::string STR::GetKeyValue(const std::string &source, const std::string left, const std::string right)
{
    const char * left_pos = source.c_str();
    const char * right_pos = left_pos + source.size();

    if (!left.empty())
        left_pos = strstr(left_pos, left.c_str());

    if (left_pos != NULL)
        left_pos += left.size();
    else
        left_pos = source.c_str();

    if (!right.empty())
        right_pos = strstr(left_pos + 1, right.c_str());

    if (right_pos == NULL)
        right_pos = source.c_str() + source.size();
    return std::string(left_pos, right_pos - left_pos);
}

std::string STR::GetRKeyValue(const std::string &source, const std::string left, const std::string right)
{
    const char * left_pos = source.c_str();
    const char * right_pos = left_pos + source.size();

    if (!right.empty())
    {
        size_t found = source.rfind(right);
        right_pos = found == std::string::npos ? right_pos : left_pos + found;
    }       

    if (!left.empty())
    {
        size_t found = std::string(left_pos, right_pos - left_pos).rfind(left);
        left_pos = found == std::string::npos ? source.c_str() : left_pos + found + left.size();
    }
    return std::string(left_pos, right_pos - left_pos);
}

std::string STR::Trim(const std::string &source, const std::string separators)
{
    if (source.empty())
    {
        return source;
    }
    std::string dest = source;
    size_t i = 0;
    for (; i < source.length() && separators.find(source.at(i)) != std::string::npos; ++i);
    dest = source.substr(i);
    for (i = dest.length(); i > 0 && separators.find(dest.at(i - 1)) != std::string::npos; --i);
    dest = dest.substr(0, i);
    return dest;
}

std::string STR::ToString(long long value)
{
    char buf[21] = { 0 };
    snprintf(buf, sizeof(buf), "%lld", value);
    return std::string(buf);
}

uint64_t STR::Str2UInt64(std::string value)
{
    return atoll(value.c_str());
}

