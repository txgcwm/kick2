
#ifndef __HTTP_HELPER_H__
#define __HTTP_HELPER_H__

#include <string>

#include "http.h"

#define HEADER_MAX_LENGTH 1024

class HttpHelper
{
public:
    static std::string BuildGetReq(const std::string &uri);
    static int BuildGetReq(char *buf, uint32_t len, const char *host, const char *uri);
    static std::string BulidPostReq(const std::string &uri, const std::string &data);

    static bool IsDomain(const char *host);
    static unsigned long Resolve(const char * domain);
};

#endif
