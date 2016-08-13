
#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <errno.h>

#include "zmalloc.h"
#include "config.h"

#include "log.h"
#include "mem_pool.h"
#include "http_client.h"
#include "http_helper.h"
#include "string_helper.h"

void HttpRespHandler::Handle(Connection *connection, HttpClient *client, void *userData)
{
    HttpMsg &response = *connection->Response;
    DEBUG_LOG("HttpRespHandler::Handle(), "
        "Content-Type: " << response.Headers.at(string(HTTP_HEADER_CONTENT_TYPE)) << 
        " buf: " << (void *)response.Buf->Data << ", "
        "Content-Length:" << response.Headers.at(string(HTTP_HEADER_CONTENT_LENGTH)));
}

ConnectionPool::ConnectionPool()
{
    memset(&m_mutex, 0, sizeof(m_mutex));
}

SOCKET ConnectionPool::GetConnection(const char *host, uint16_t port)
{
    std::string addr = std::string(host) + ":" + STR::ToString(port);
    SOCKET socket;
    pthread_mutex_lock(&m_mutex);
    if (m_pool.find(addr) == m_pool.end() || m_pool[addr].empty())
        socket = HttpClient::Connect(host, port);
    else
    {
        socket = m_pool[addr].front();
        m_pool[addr].pop_front();
    }
    pthread_mutex_unlock(&m_mutex);
    return socket;
}
void ConnectionPool::Free(const char *host, uint16_t port, SOCKET socket)
{
    std::string addr = std::string(host) + STR::ToString(port);
    pthread_mutex_lock(&m_mutex);
    if (m_pool.find(addr) == m_pool.end())
        m_pool[addr] = std::list<SOCKET>(1, socket);
    else
    {
        m_pool[addr].push_back(socket);
    }
    pthread_mutex_unlock(&m_mutex);
}

HttpClient::HttpClient(AeEngine *engine, MemPool *memPool, ConnectionPool *connectionPool)
    :m_engine(engine), m_memPool(memPool), m_connectionPool(connectionPool), m_parser(NULL), m_defaultHander(NULL)
{
    m_parser = new HttpParser();
    m_defaultHander = new HttpRespHandler();
}
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
    m_parser->Initialize();    
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
    return SendMsgAsync(host.c_str(), hostPort, reqMsg.c_str(), reqMsg.size(), userData);
}

int HttpClient::SendMsgAsync(const char *host, uint16_t port, const char *buf, uint64_t len, void *userData)
{
    HttpMsg *request = new HttpMsg();
    m_parser->ParseMsg(buf, len, HTTP_REQUEST, *request);
    SOCKET fd = m_connectionPool->GetConnection(host, port);    
    if (fd >= 0)
    {
        Connection *connection = new Connection(host, port, fd, request, userData);
        ClientData *data = new ClientData(this, connection);
        m_engine->AddIoEvent(fd, AE_WRITABLE, data);
        m_engine->AddIoEvent(fd, AE_READABLE, data);
    }
    else
    {
        ERROR_LOG("HttpClient::SendMsg(), connect to " << host << " failed, errno: " << errno);
    }    
    return fd;
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
    socket.SetNonBlock();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    ret = socket.Connect((sockaddr *)&addr, sizeof(addr));
    if (ret == -1 && errno != EINPROGRESS)
    {
        socket.Close();
        return INVALID_SOCKET;
    }        
    return socket.Fd();
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
            response->Buf = m_memPool->Allocate(300);
            connection->Response = response;
        }
        if (response->Length == 0)
        {
            int nread = ReadToBuffer(fd, *response->Buf, connection->Read, response->Buf->Capacity - connection->Read);
            if (nread > 0 || errno == EINTR)
                connection->Read += nread;
            else
            {
                Close(connection, false);
                delete data;
                break;
            }
            m_parser->ParseMsg(response->Buf->Data, connection->Read, HTTP_RESPONSE, *response);
            if (response->Length > RECVBUF)
            {
                DEBUG_LOG("HttpClient::OnRead(), shall create bigger than " << RECVBUF << " new buffer");
                response->Buf->Next = m_memPool->Allocate(response->Length - RECVBUF);
            }
        }
        if (response->Length > connection->Read&&Socket::IsReadable(fd))
        {
            int nread = ReadToBuffer(fd, *response->Buf, connection->Read, response->Length - connection->Read);
            if (nread >= 0 || errno == EINTR)
                connection->Read += nread;
            else
            {
                Close(connection, false);
                delete data;
                break;
            }
        }
        DEBUG_LOG("HttpClient::OnRead(), fd: " << fd << ", "
            "response: "<<(void *)response<<", read: " << connection->Read << ", length: " << response->Length);
        if (connection->Read == response->Length || response->State == 1)
        {
            bool canReuseFd = true;
            if (response->Headers["Connection"] == "close")
            {
                canReuseFd = false;
            }
            DEBUG_LOG("HttpClient::OnRead(), receive response: \n" 
                << response->Buf->Data << (response->Buf->Next ? response->Buf->Next->Data : ""));
            HandleResponse(connection);

            Close(connection, canReuseFd);
            delete data;
        }

    } while (0);
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
            SendGetReq(response.Headers["Location"], connection->Data);
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
        handler->Handle(connection, this, connection->Data);

    } while (0);
    return 0;
}

int HttpClient::Close(Connection *connection, bool canReuse)
{
    assert(connection);
    if (canReuse)
        m_connectionPool->Free(connection->Host.c_str(), connection->Port, connection->Fd);
    else
        close(connection->Fd);
    m_engine->DeleteIoEvent(connection->Fd, AE_READABLE | AE_WRITABLE);
    if(connection->Request)
        delete connection->Request;
    if (connection->Response)
    {
        m_memPool->Free(connection->Response->Buf);
        delete connection->Response;
    }
    delete connection;
    return 0;
}

void HttpClient::OnWrite(int fd, ClientData *data, int mask)
{
    Connection *connection = (Connection *)data->UserData;
    char buf[HEADER_MAX_LENGTH] = { 0 };
    std::string host = connection->Host;
    std::string uri = connection->Request->Uri;
    HttpHelper::BuildGetReq(buf, sizeof(buf), host.c_str(), uri.c_str());
    int result = write(fd, buf + connection->Written, strlen(buf) - connection->Written);
    if (result > 0)
        connection->Written += result;
    if (connection->Written == strlen(buf))
    {
        DEBUG_LOG("HttpClient::SendMsg(), send request to: " << connection->Host << ":" << connection->Port << " successfully, "
            "fd: " << fd << ", result: " << result << ", msg: \n" << buf);
        m_engine->DeleteIoEvent(fd, AE_WRITABLE);
    }
    else if (result < 0 && errno != EINTR)
    {
        ERROR_LOG("HttpClient::SendMsg(), send request to: " << connection->Host << ":" << connection->Port << " failed, "
            "fd: " << fd << ", result: " << result << ", errno: " << errno << ", msg: \n" << buf);
        Close(connection, false);
        delete data;
    }
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
