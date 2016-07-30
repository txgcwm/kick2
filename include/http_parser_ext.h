#pragma once

#include <string>
#include <cstring>

#include "common.h"
#include "http.h"
#include "http_parser.h"

using namespace std;

class HttpParser
{
public:
    static int ParseUrl(const string &url, Url &httpUrl);

private:
    static size_t ParseReq(const char *buf, size_t len, HttpMsg &httpMsg);
    static size_t ParseResp(const char *buf, size_t len, HttpMsg &httpMsg);

public:
    static size_t ParseReqHeader(const char *buf, size_t len, HttpMsg &httpMsg);
    static size_t ParseRespHeader(const char *buf, size_t len, HttpMsg &httpMsg);
};
