
#include <cstdio>
#include <cstdlib>

#include <assert.h>
#include <netdb.h>

#include "socket.h"
#include "http_parser_ext.h"
#include "http_helper.h"

std::string HttpHelper::BuildGetReq(const std::string &url)
{
    HttpParser parser;
    Url parsedUrl;
    parser.ParseUrl(url, parsedUrl);    
    char buf[HEADER_MAX_LENGTH] = { 0 };
    snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\n"
        "User-Agent: kick2\r\n"
        "Connection: close\r\n"
        "Host: %s\r\n"
        "\r\n", parsedUrl.ToUri().c_str(), parsedUrl.Host.c_str());
    return std::string(buf);
}

int HttpHelper::BuildGetReq(char *buf, uint32_t len, const char *host, const char *uri)
{
    return snprintf(buf, len, "GET %s HTTP/1.0\r\n"
        "User-Agent: kick2\r\n"
        "Connection: close\r\n"
        "Host: %s\r\n"
        "\r\n", uri, host);
}

std::string HttpHelper::BulidPostReq(const std::string &url, const std::string &data)
{
    HttpParser parser;
    Url parsedUrl;
    parser.ParseUrl(url, parsedUrl);
    char buf[HEADER_MAX_LENGTH] = { 0 };
    snprintf(buf, sizeof(buf), "POST %s HTTP/1.1\r\n"
        "User-Agent: kick2\r\n"
        "Connection: keep-alive\r\n"
        "Host: %s\r\n"
        "\r\n", parsedUrl.ToUri().c_str(), parsedUrl.Host.c_str());
    return std::string(buf) + data;
}

bool HttpHelper::IsDomain(const char *host)
{
    assert(host);
    int isDomain = false;    
    const char* p = host;
    for (; *p != '\0'; p++)
    {
        if ((isalpha(*p)) && (*p != '.'))
        {
            isDomain = true;
            break;
        }
    }
    return isDomain;
}

unsigned long HttpHelper::Resolve(const char * domain)
{
    struct hostent *hptr = NULL;
    struct sockaddr_in addr;
    struct hostent hostinfo, *host;
    char    buf[1024];
    int ret;
    int result = gethostbyname_r(domain, &hostinfo, buf, sizeof(buf), &host, &ret);
    if (-1 == result)
    {
        return 0;
    }
    switch (hptr->h_addrtype)
    {
    case AF_INET:
    case AF_INET6:
        memcpy(&addr.sin_addr.s_addr, hptr->h_addr_list[0], hptr->h_length);
        return addr.sin_addr.s_addr;

    default:
        return 0;
    }
}
