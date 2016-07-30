
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "ae_engine.h"

#include "http.h"
#include "socket.h"
#include "http_parser_ext.h"

#define RECVBUF 8192
#define MIME_DEFAULT "MimeDefault"

class HttpClient;

struct IHttpHandler
{
    virtual void Handle(Connection *connection, HttpClient *client, void *userData) = 0;
};

class HttpRespHandler :public IHttpHandler
{
    virtual void Handle(Connection *connection, HttpClient *client, void *userData);
};

class HttpClient :public IIoEvent
{
public:
    HttpClient(AeEngine *engine);
    ~HttpClient();

public:
    void Initialize();
    int SendGetReq(const std::string &url, void *userData = NULL);
    int SendMsg(const char *host, uint16_t port, const char *buf, uint64_t len, void *userData = NULL);
    int HandleResponse(Connection *connection);
    virtual void OnRead (int fd, ClientData *data, int mask);
    virtual void OnWrite(int fd, ClientData *data, int mask);
    virtual void OnError(int fd, ClientData *data, int mask);

    void RegisterHandler(const string mime, IHttpHandler *handler);

private:
    SOCKET Connect(const char *host, unsigned short port);

private:
    AeEngine *m_engine;
    map<string, IHttpHandler *> m_respHandlers;
    HttpRespHandler *m_defaultHander;
};

#endif
