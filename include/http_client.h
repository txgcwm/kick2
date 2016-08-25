
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "ae_engine.h"

#include "http.h"
#include "socket.h"
#include "http_parser_ext.h"

#define RECVBUF 1024
#define MIME_DEFAULT "MimeDefault"

class MemPool;
class HttpClient;

struct IHttpHandler
{
    virtual void Handle(Connection *connection, HttpClient *client, void *userData) = 0;
};

class HttpRespHandler :public IHttpHandler
{
    virtual void Handle(Connection *connection, HttpClient *client, void *userData);
};

class ConnectionPool
{
public:
    ConnectionPool();
    SOCKET GetConnection(const char *host, uint16_t port);
    void Free(const char *host, uint16_t port, SOCKET);

private:
    std::map<std::string, std::list<SOCKET> > m_pool;
    pthread_mutex_t m_mutex;
};

class HttpClient :public IIoEvent
{
public:
    HttpClient(AeEngine *engine, MemPool *memPool, ConnectionPool *connectionPool);
    ~HttpClient();

public:
    void Initialize();
    int SendGetReq(const std::string &url, void *userData = NULL);
    int SendMsgAsync(const char *host, uint16_t port, const char *buf, uint64_t len, void *userData = NULL);
    int HandleResponse(Connection *connection);
    virtual void OnRead (int fd, ClientData *data, int mask);
    virtual void OnWrite(int fd, ClientData *data, int mask);

    int Abort(Connection *connection);
    int Close(Connection *connection, bool canReuse);

    void RegisterHandler(const string mime, IHttpHandler *handler);

public:
    static SOCKET Connect(const char *host, unsigned short port);

private:
    AeEngine *m_engine;
    MemPool *m_memPool;
    ConnectionPool *m_connectionPool;
    HttpParser *m_parser;
    HttpRespHandler *m_defaultHander;
    map<string, IHttpHandler *> m_respHandlers;
};

#endif
