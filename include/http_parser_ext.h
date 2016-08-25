
#ifndef __HTTP_PARSER_EXT_H__
#define __HTTP_PARSER_EXT_H__

#include <string>

#include "http.h"
#include "http_parser.h"

using namespace std;

class HttpParser
{
public:
    void Initialize();
    static int ParseUrl(const string &url, Url &httpUrl);
    size_t ParseMsg(const char *buf, size_t len, enum http_parser_type type, HttpMsg &httpMsg);

private:
    http_parser_settings m_settings;
};

int request_uri(http_parser *parser, const char *at, size_t len);
int header_filed(http_parser *parser, const char *at, size_t len);
int header_value(http_parser *parser, const char *at, size_t len);
int header_complete(http_parser *parser);
int body(http_parser *parser, const char *at, size_t len);
int message_complete(http_parser *parser);

#endif
