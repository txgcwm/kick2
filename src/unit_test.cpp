
#include <iostream>
#include <cstring>
#include <getopt.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "kick2.h"
#include "datetime.h"
#include "ae_engine.h"
#include "http_client.h"
#include "hls_handler.h"
#include "http_helper.h"
#include "string_helper.h"
#include "http_parser_ext.h"

const std::string Text =
"GET /CCTV8HD_6000.m3u8 HTTP/1.1\r\n"
"User-Agent: kick2\r\n"
"Host: 192.168.111.131:80\r\n"
"Connection : Keep-Alive\r\n"
"Host : 192.168.111.131\r\n\r\n";

void *thread_main(void *arg)
{
    aeEventLoop *eventLoop = (aeEventLoop *)arg;
    aeMain(eventLoop);
    return NULL;
}

void socket_connected(struct aeEventLoop *eventLoop, int fd, void *data, int mask);

static int reconnect_socket(struct aeEventLoop *eventLoop, int fd, void *data) {
    aeDeleteFileEvent(eventLoop, fd, AE_WRITABLE | AE_READABLE);
    close(fd);
    Socket socket;
    int ret = socket.Create(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ret)
    {
        return INVALID_SOCKET;
    }
    socket.SetNonBlock();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "192.168.111.131", &addr.sin_addr);
    ret = socket.Connect((sockaddr *)&addr, sizeof(addr));
    if (ret == -1 && errno != EINPROGRESS)
    {
        DEBUG_LOG("socket connect to 192.168.111.131:80 fail, errno: " << errno);
    }        
    DEBUG_LOG("socket connect to 192.168.111.131:80 successfully");

    if (aeCreateFileEvent(eventLoop, socket.Fd(), (AE_READABLE | AE_WRITABLE), socket_connected, data) != AE_OK)
        return 1;
    return 0;
}

void on_write(struct aeEventLoop *eventLoop, int fd, void *data, int mask);

void on_read(struct aeEventLoop *eventLoop, int fd, void *data, int mask)
{
    if (mask&AE_READABLE)
    {
        Connection *c = (Connection *)data;
        int nread = read(fd, c->Response->Buf->Data + c->Read, c->Response->Buf->Capacity-c->Read);
        c->Read += nread;
        HttpParser parser;
        parser.Initialize();
        parser.ParseMsg(c->Response->Buf->Data, c->Read, HTTP_RESPONSE, *c->Response);
        DEBUG_LOG("on_read(), receive " << nread << " data. c->Read: " << c->Read << ", "
            "msg.Length: " << c->Response->Length << ", msg.State: " << c->Response->State << ", buf: " << c->Response->Buf->Data);
        if (nread > 0 && (c->Read == c->Response->Length || c->Response->State == 1))
        {
            //aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
            aeCreateFileEvent(eventLoop, fd, AE_WRITABLE, on_write, data);
            c->Read = 0;
            memset(c->Response->Buf->Data, 0, c->Response->Buf->Capacity);
            c->Response->Length = 0;
            c->Response->State = 0;
            //if (c->Response->Headers["Connection"] != "keep-alive")
            if (c->Response->Headers["Connection"] == "close")
            {
                reconnect_socket(eventLoop, fd, data);
            }
            c->Response->Headers.clear();
        }
        if (nread <= 0)
        {
            DEBUG_LOG("socket read error, delete event");
            aeDeleteFileEvent(eventLoop, fd, AE_WRITABLE | AE_READABLE);
            close(fd);
        }
    }
}

void on_write(struct aeEventLoop *eventLoop, int fd, void *data, int mask)
{    
    if (mask&AE_WRITABLE)
    {                   
        std::string reqMsg = Text;// HttpHelper::BuildGetReq(std::string("http://192.168.111.131/CCTV8HD_6000.m3u8"));
        int ret = write(fd, reqMsg.c_str(), reqMsg.size());
        //int ret = Socket(fd).Send(reqMsg.c_str(), reqMsg.size(), 0);
        DEBUG_LOG("socket send, msg: \n" << reqMsg << ", result: " << ret << ", errno: " << errno);

        if (ret == (int)reqMsg.size())
        {
            aeDeleteFileEvent(eventLoop, fd, AE_WRITABLE);
           // aeCreateFileEvent(eventLoop, fd, AE_READABLE, on_read, data);
        }

        if (ret < 0)
        {
            DEBUG_LOG("socket write error, delete event");
            aeDeleteFileEvent(eventLoop, fd, AE_READABLE | AE_WRITABLE);
            close(fd);
        }
        //DEBUG_LOG("multicast_on_read(), AE_WRITABLE.");
    }
}

void socket_connected(struct aeEventLoop *eventLoop, int fd, void *data, int mask)
{
    aeCreateFileEvent(eventLoop, fd, AE_READABLE, on_read, data);
    aeCreateFileEvent(eventLoop, fd, AE_WRITABLE, on_write, data);
}

int main(int argc, char **argv)
{
    //Config config;
    //config.Duration = 0;
    //config.ThreadNum = 1;
    //config.LaunchInterval = 10;
    //config.Timeout = 1000;
    //config.Concurrency = 1;
    //config.Url = "http://172.21.11.140:8099/live/HNWS_2000.m3u8";
    //config.Url = "http://172.21.11.140:8099/live/HNWS_2000/HNWS_2000_146961928.ts";
    //config.Url = "http://192.168.111.131/CCTV8HD_6000.m3u8";
    //
    //std::list<std::string> playUrls;
    //if (!config.Url.empty())
    //{
    //    playUrls.push_back(config.Url);
    //}
    //
    //Kick2 *kick = new Kick2();
    //kick->Initialize(config);
    //kick->LaunchHlsLoad(playUrls);

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
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "192.168.111.131", &addr.sin_addr);
    

    aeEventLoop *eventLoop = aeCreateEventLoop(1000);
    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, eventLoop);

    ret = socket.Connect((sockaddr *)&addr, sizeof(addr));
    if (ret == -1 && errno != EINPROGRESS)
    {
        DEBUG_LOG("socket connect to 192.168.111.131:80 fail, errno: " << errno);
    }        
    DEBUG_LOG("socket connect to 192.168.111.131:80 successfully");

    HttpMsg request, response;
    char rxbuf[8096], txbuf[8096];
    request.Buf = new Block(txbuf, 8096);
    response.Buf = new Block(rxbuf, 8096);
    Connection connection("", 80, socket.Fd(), NULL, NULL);
    connection.Response = &response;

    if (aeCreateFileEvent(eventLoop, socket.Fd(), (AE_READABLE | AE_WRITABLE), socket_connected, &connection) != AE_OK)
        return 1;

    pthread_join(thread, NULL);

    while(1)
        usleep(200000000);
    return 0;
}
