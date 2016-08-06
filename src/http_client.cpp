
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
    Socket(socket).Close();
    //pthread_mutex_lock(&m_mutex);
    //if (m_pool.find(addr) == m_pool.end())
    //    m_pool[addr] = std::list<SOCKET>(1, socket);
    //else
    //{
    //    m_pool[addr].push_back(socket);
    //}
    //pthread_mutex_unlock(&m_mutex);
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
    return SendMsg(host.c_str(), hostPort, reqMsg.c_str(), reqMsg.size(), userData);
}

int HttpClient::SendMsg(const char *host, uint16_t port, const char *buf, uint64_t len, void *userData)
{
    HttpMsg *request = new HttpMsg();
    m_parser->ParseMsg(buf, len, HTTP_REQUEST, *request);
    Connection *connection = new Connection(host, port, request, userData);

    int result = 0;
    SOCKET fd = m_connectionPool->GetConnection(host, port);
    if (fd >= 0)
    {
        result = Socket(fd).Send(buf, len, 0);
        if (result > 0)
        {
            DEBUG_LOG("HttpClient::SendMsg(), send request to: " << host << ":" << port << " successfully, "
                "fd: " << fd << ", result: " << result << ", msg: \n" << buf);
        }            
        else
        {
            ERROR_LOG("HttpClient::SendMsg(), send request to: " << host << ":" << port << " failed, "
                "fd: " << fd << ", result: " << result << ", errno: " << errno << ", msg: \n" << buf);
        }            
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
            char buf[RECVBUF + 1] = { 0 };
            Block *block = new Block(buf, RECVBUF);
            response->Buf = block;
            connection->Response = response;
        }
        if (response->Length == 0)
        {
            connection->Read += ReadToBuffer(*response->Buf, connection->Read, fd, RECVBUF - connection->Read);
            m_parser->ParseMsg(response->Buf->Data, connection->Read, HTTP_RESPONSE, *response);
            if (response->Length > RECVBUF)
            {
                DEBUG_LOG("HttpClient::OnRead(), shall create bigger than " << RECVBUF << " new buffer");
                response->Buf->Next = m_memPool->Allocate(response->Length - RECVBUF);
            }
        }
        if (response->Length > connection->Read&&Socket::IsReadable(fd))
            connection->Read += ReadToBuffer(*response->Buf, connection->Read, fd, response->Length - connection->Read);
        DEBUG_LOG("HttpClient::OnRead(), fd: " << fd << ", "
            "response: "<<(void *)response<<", read: " << connection->Read << ", length: " << response->Length);
        if (connection->Read == response->Length || response->State == 1)
        {
            m_engine->DeleteIoEvent(fd, AE_READABLE);
            m_connectionPool->Free(connection->Host.c_str(), connection->Port, fd);

            DEBUG_LOG("HttpClient::OnRead(), receive response: \n" << response->Buf->Data);
            HandleResponse(connection);

            m_memPool->Free(response->Buf->Next);
            delete response->Buf;
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
