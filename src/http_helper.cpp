
#include <sstream>

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
    std::stringstream buf;
    buf << "GET " << parsedUrl.ToUri() << " HTTP/1.0\r\n"
        << "User-Agent: kick2\r\n"
        << "\r\n";
    return buf.str();
}
std::string HttpHelper::BulidPostReq(const std::string &url, const std::string &data)
{
    return "";
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
    Socket::Init();
    struct hostent *hptr = NULL;
    struct sockaddr_in addr;
    hptr = gethostbyname(domain);
    Socket::Uninit();

    if (NULL == hptr)
    {
        return 0;
    }
    switch (hptr->h_addrtype)
    {
    case AF_INET:
    case AF_INET6:
        memcpy(&addr.sin_addr.s_addr, hptr->h_addr_list[0], hptr->h_length);
        //inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
        return addr.sin_addr.s_addr;

    default:
        return 0;
    }
}
