
#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <errno.h>

#include "zmalloc.h"
#include "config.h"

#include "log.h"
#include "http_client.h"
#include "http_helper.h"
#include "string_helper.h"

void HttpRespHandler::Handle(Connection *connection, HttpClient *client, void *userData)
{
    HttpMsg &response = *connection->Response;
    DEBUG_LOG("HttpRespHandler::Handle(), "
        "Content-Type: " << response.Headers.at(string(HTTP_HEADER_CONTENT_TYPE)) << 
        " buf: " << (void *)response.Buf << ", "
        "Content-Length:" << response.Headers.at(string(HTTP_HEADER_CONTENT_LENGTH)));
}

HttpClient::HttpClient(AeEngine *engine)
    :m_engine(engine), m_defaultHander(NULL)
{}
HttpClient::~HttpClient()
{
    if (m_defaultHander != NULL)
    {
        delete m_defaultHander;
        m_defaultHander = NULL;
    }
}

void HttpClient::Initialize()
{
    m_defaultHander = new HttpRespHandler();
    RegisterHandler(string(MIME_DEFAULT), m_defaultHander);
}

int HttpClient::SendGetReq(const std::string &fullUrl, void *userData)
{
    Url parsedUrl;
    HttpParser::ParseUrl(fullUrl, parsedUrl);
    std::string reqMsg = HttpHelper::BuildGetReq(fullUrl);
    std::string host = parsedUrl.Host;
    if (HttpHelper::IsDomain(host.c_str()))
    {
        char addr[256];
        in_addr s;
        s.s_addr = HttpHelper::Resolve(host.c_str());
        inet_ntop(AF_INET, (void *)&s, addr, sizeof(addr));
        host = addr;
    }
    uint16_t hostPort = STR::Str2UInt64(parsedUrl.Port);
    return SendMsg(host.c_str(), hostPort, reqMsg.c_str(), reqMsg.size(), userData);
}

int HttpClient::SendMsg(const char *host, uint16_t port, const char *buf, uint64_t len, void *userData)
{
    HttpMsg *request = new HttpMsg();
    HttpParser parser;
    parser.ParseReqHeader(buf, len, *request);
    Connection *connection = new Connection(host, port, request, userData);

    int result = 0;
    SOCKET fd = Connect(host, port);
    if (fd >= 0)
    {
        result = Socket(fd).Send(buf, len, 0);
        DEBUG_LOG("HttpClient::SendMsg(), send request to: " << host << ":" << port << " successfully, msg: \n" << buf);
        ClientData *data = new ClientData(this, connection);
        m_engine->AddIoEvent(fd, AE_READABLE, data);
    }
    else
    {
        result = fd;
        ERROR_LOG("HttpClient::SendMsg(), connect to " << host << " failed, errno: " << errno);
    }    
    return result;
}

SOCKET HttpClient::Connect(const char *host, unsigned short port)
{
    Socket socket;
    socket.Init();
    int ret = socket.Create(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ret)
    {
        return INVALID_SOCKET;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    ret = socket.Connect((sockaddr *)&addr, sizeof(addr));
    return ret == 0 ? socket.Fd() : ret;
}

int HttpClient::HandleResponse(Connection *connection)
{
    do 
    {
        HttpMsg &response = *connection->Response;
        bool needHandle = true;
        switch (response.StatusCode)
        {
        case 301:
        case 302:
        case 303:
        case 307:
            needHandle = false;
            SendGetReq(response.Headers["Location"], connection->UserData);
            break;

        case 404:
            DEBUG_LOG("HttpClient::HandleResponse(), http server response 404 Not Found.");
            break;
        }
        if (needHandle == false)
            break;

        IHttpHandler *handler = NULL;
        if (m_respHandlers.find(response.Headers[HTTP_HEADER_CONTENT_TYPE]) == m_respHandlers.end())
        {
            handler = m_respHandlers[string(MIME_DEFAULT)];
        }
        else
        {
            handler = m_respHandlers[response.Headers[HTTP_HEADER_CONTENT_TYPE]];
        }
        assert(handler);
        handler->Handle(connection, this, connection->UserData);

    } while (0);
    return 0;
}

void HttpClient::OnRead(int fd, ClientData *data, int mask)
{
    do
    {
        Connection *connection = (Connection *)data->UserData;
        HttpMsg *response = connection->Response;
        if (response == NULL)
        {
            response = new HttpMsg();
            response->Buf = (char *)zmalloc(RECVBUF + 1);
            response->BufCapacity = RECVBUF;
            memset(response->Buf, 0, response->BufCapacity);
            connection->Response = response;
        }
        char *buf = response->Buf + connection->Read;
        size_t len = response->BufCapacity - connection->Read;
        ssize_t readSize = read(fd, buf, len);
        connection->Read += readSize;
        HttpParser::ParseRespHeader(response->Buf, connection->Read, *response);
        if (connection->Read == RECVBUF && Socket::IsReadable(fd) > 0)
        {
            DEBUG_LOG("create bigger than " << RECVBUF << " new buffer");
            char *bigBuf = (char *)zmalloc(response->Body.Offset + response->Body.Length + 1);
            memset(bigBuf, 0, response->Body.Offset + response->Body.Length + 1);
            memcpy(bigBuf, response->Buf, RECVBUF);
            zfree(response->Buf);
            response->Buf = bigBuf;
            response->BufCapacity = response->Body.Offset + response->Body.Length;            
            connection->Read += read(fd, response->Buf + connection->Read, response->BufCapacity - connection->Read);
        }

        if (connection->Read == response->Body.Offset + response->Body.Length)
        {
            m_engine->DeleteIoEvent(fd, AE_READABLE);
            Socket(fd).Uninit();
            Socket(fd).Close();

            DEBUG_LOG("HttpClient::OnRead(), receive response: \n" << response->Buf);
            HandleResponse(connection);

            zfree(response->Buf);
            delete connection->Request;
            delete connection->Response;
            delete connection;
            delete data;
        }
        
    } while (0);

    
}

void HttpClient::OnWrite(int fd, ClientData *data, int mask)
{

}
void HttpClient::OnError(int fd, ClientData *data, int mask)
{

}

void HttpClient::RegisterHandler(const string mime, IHttpHandler *handler)
{
    if (m_respHandlers.find(mime) == m_respHandlers.end())
    {
        m_respHandlers.insert(pair<string, IHttpHandler *>(mime, handler));
    }
    else
    {
        m_respHandlers[mime] = handler;
    }
}
