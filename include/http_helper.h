#pragma once

#include <string>

class HttpHelper
{
public:
    static std::string BuildGetReq(const std::string &uri);
    static std::string BulidPostReq(const std::string &uri, const std::string &data);

    static bool IsDomain(const char *host);
    static unsigned long Resolve(const char * domain);
};
