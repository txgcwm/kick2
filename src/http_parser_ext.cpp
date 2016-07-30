
#include <sstream>

#include "common.h"
#include "string_helper.h"
#include "http_parser_ext.h"

using namespace std;

int HttpParser::ParseUrl(const string &url, Url &httpUrl)
{
    struct http_parser_url parts;
    const char *curl = url.c_str();
    int result = http_parser_parse_url(curl, strlen(curl), 0, &parts);
    if (result ||
        !(parts.field_set & (1 << UF_SCHEMA)) ||
        !(parts.field_set & (1 << UF_HOST)))
    {
        return -1;
    }
    httpUrl.Scheme = string(curl + parts.field_data[UF_SCHEMA].off, parts.field_data[UF_SCHEMA].len);
    httpUrl.Host = string(curl + parts.field_data[UF_HOST].off, parts.field_data[UF_HOST].len);
    httpUrl.Path = "/";
    if (parts.field_set & (1 << UF_PORT))
        httpUrl.Port = string(curl + parts.field_data[UF_PORT].off, parts.field_data[UF_PORT].len);
    if (httpUrl.Port.empty())
        httpUrl.Port = "80";
    if (parts.field_set & (1 << UF_PATH))
        httpUrl.Path = string(curl + parts.field_data[UF_PATH].off, parts.field_data[UF_PATH].len);
    if (parts.field_set & (1 << UF_QUERY))
        httpUrl.Args = string(curl + parts.field_data[UF_QUERY].off, parts.field_data[UF_QUERY].len);
    if (parts.field_set & (1 << UF_FRAGMENT))
        httpUrl.Fragment = string(curl + parts.field_data[UF_FRAGMENT].off, parts.field_data[UF_FRAGMENT].len);
    if (parts.field_set & (1 << UF_USERINFO))
        httpUrl.UserInfo = string(curl + parts.field_data[UF_USERINFO].off, parts.field_data[UF_USERINFO].len);
    return 0;
}

size_t HttpParser::ParseReq(const char *buf, size_t len, HttpMsg &httpMsg)
{
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = NULL;
    size_t parsed = http_parser_execute(&parser, &settings, buf, len);
    httpMsg.Method = http_method_str((http_method)parser.method);
    stringstream stringBuf;
    stringBuf << "http/" << parser.http_major << "." << parser.http_minor;
    httpMsg.HttpVersion = stringBuf.str();
    //httpMsg.Body.Length = parser.content_length;
    return parsed;
}

size_t HttpParser::ParseResp(const char *buf, size_t len, HttpMsg &httpMsg)
{
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = NULL;
    size_t parsed = http_parser_execute(&parser, &settings, buf, len);
    stringstream stringBuf;
    stringBuf << "http/" << parser.http_major << "." << parser.http_minor;
    httpMsg.HttpVersion = stringBuf.str();
    httpMsg.StatusCode = parser.status_code;
    //httpMsg.Body.Length = parser.content_length;
    return parsed;
}

size_t HttpParser::ParseReqHeader(const char *buf, size_t len, HttpMsg &httpMsg)
{
    size_t parsed = ParseReq(buf, len, httpMsg);
    const char *pos = strstr(buf, "\r\n") + 2;
    string line = string(buf, pos - buf - 2);
    httpMsg.Uri = STR::GetKeyValue(line, " ", " ");
    while (strncmp(pos,"\r\n",2))
    {
        const char *next = strstr(pos, "\r\n");
        string line = string(pos, next - pos);
        string key = STR::GetKeyValue(line, "", ":");
        string value = STR::Trim(STR::GetKeyValue(line, ":", "")," \r\n");
        httpMsg.Headers.insert(pair<string, string>(key, value));
        if (key == HTTP_HEADER_CONTENT_LENGTH)
        {
            httpMsg.Body.Length = STR::Str2UInt64(value);
        }
        pos = next + 2;
    }
    httpMsg.Body.Offset = pos - buf + 2;
    return parsed;
}

size_t HttpParser::ParseRespHeader(const char *buf, size_t len, HttpMsg &httpMsg)
{
    size_t parsed = ParseResp(buf, len, httpMsg);
    const char *pos = strstr(buf, "\r\n") + 2;
    while (strncmp(pos, "\r\n", 2))
    {
        const char *next = strstr(pos, "\r\n");
        string line = string(pos, next - pos);
        string key = STR::GetKeyValue(line, "", ":");
        string value = STR::Trim(STR::GetKeyValue(line, ":", ""), " \r\n");
        httpMsg.Headers.insert(pair<string, string>(key, value));
        if (key == HTTP_HEADER_CONTENT_LENGTH)
        {
            httpMsg.Body.Length = STR::Str2UInt64(value);
        }
        pos = next + 2;
    }
    httpMsg.Body.Offset = pos - buf + 2;
    return parsed;
}
