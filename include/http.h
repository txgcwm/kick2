
#ifndef __HTTP_H__
#define __HTTP_H__

#include <map>
#include <string>
#include <inttypes.h>

#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"

struct Url
{
    std::string Scheme;
    std::string Host;
    std::string Port;
    std::string Path;
    std::string Args;
    std::string Fragment;
    std::string UserInfo;
    std::string ToFullUrl()
    {
        std::string url = Scheme + "://";
        if (!UserInfo.empty())
            url += UserInfo + "@";
        url += Host + ":" + Port + ToUri();
        return url;
    }
    std::string ToUri()
    {
        std::string uri = Path;
        if (!Args.empty())
            uri += std::string("?") + Args;
        if (!Fragment.empty())
            uri += std::string("#") + Fragment;
        return uri;
    }
};

struct HttpBody
{
    uint64_t Offset;
    uint64_t Length;
    HttpBody()
        :Offset(0), Length(0) {}
};

class Block;
struct HttpMsg
{
    std::string Method;
    std::string Uri;
    std::string HttpVersion;
    uint32_t StatusCode;
    
    unsigned int State : 8;
    std::string HeaderFiled;
    std::map<std::string, std::string> Headers;
    HttpBody Body;
    Block *Buf;
    uint64_t ContentLength;
    uint64_t Length;
    HttpMsg()
        :Method(), Uri(), HttpVersion(), StatusCode(), State(0), Body(), 
        ContentLength(0), Length(0) {}
};

struct Connection
{
    std::string Host;
    uint16_t Port;
    HttpMsg *Request;
    HttpMsg *Response;
    uint64_t Read;
    uint64_t Written;
    void *UserData;

    Connection(const char *host, uint16_t port, HttpMsg *request, void *userData)
        :Host(host), Port(port), Request(request), Response(NULL), Read(0), Written(0), UserData(userData) {}
};

#endif
