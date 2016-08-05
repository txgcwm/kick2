
#include <cstdio>

#include "http_parser_ext.h"
#include "mem_pool.h"

using namespace std;

void HttpParser::Initialize()
{
    memset(&m_settings, 0, sizeof(m_settings));
    m_settings.on_url = request_uri;
    m_settings.on_header_field = header_filed;
    m_settings.on_header_value = header_value;
    m_settings.on_headers_complete = header_complete;
    m_settings.on_body = body;
    m_settings.on_message_complete = message_complete;
}

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

size_t HttpParser::ParseMsg(const char *buf, size_t len, enum http_parser_type type, HttpMsg &httpMsg)
{
    http_parser parser;
    http_parser_init(&parser, type);
    parser.data = &httpMsg;
    size_t parsed = http_parser_execute(&parser, &m_settings, buf, len);
    httpMsg.State = parser.state;
    return parsed;
}

int request_uri(http_parser *parser, const char *at, size_t len) {
    HttpMsg *msg = (HttpMsg *)parser->data;
    msg->Uri = std::string(at, len);
    return 0;
}

int header_filed(http_parser *parser, const char *at, size_t len) {    
    HttpMsg *httpMsg = (HttpMsg *)parser->data;
    httpMsg->HeaderFiled = std::string(at, len);
    return 0;
}

int header_value(http_parser *parser, const char *at, size_t len) {
    HttpMsg *httpMsg = (HttpMsg *)parser->data;
    std::string key = httpMsg->HeaderFiled;
    std::string value = std::string(at, len);
    httpMsg->Headers.insert(pair<string, string>(key, value));
    return 0;
}

int header_complete(http_parser *parser) {
    HttpMsg *httpMsg = (HttpMsg *)parser->data;
    httpMsg->ContentLength = parser->content_length;
    char buf[9] = { 0 };
    snprintf(buf, 9, "http/%d.%d", parser->http_major, parser->http_minor);
    httpMsg->HttpVersion = buf;
    if (parser->type == HTTP_REQUEST)
        httpMsg->Method = http_method_str((http_method)parser->method);
    else if (parser->type == HTTP_RESPONSE)
        httpMsg->StatusCode = parser->status_code;
    return 0;
}

int body(http_parser *parser, const char *at, size_t len) {
    HttpMsg *msg = (HttpMsg *)parser->data;
    msg->Body.Offset = at - msg->Buf->Data;
    msg->Body.Length = len;
    msg->Length = msg->Body.Offset + msg->ContentLength;
    return 0;
}

int message_complete(http_parser *parser) {
    return 0;
}
