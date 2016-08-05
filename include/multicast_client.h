
#ifndef __MULTICAST_CLIENT_H__
#define __MULTICAST_CLIENT_H__

#include <string>

#include "ae_engine.h"
#include "socket.h"

struct MulticastInfo
{
    std::string Source;
    std::string Multicast;
    std::string Interface;
    uint16_t Port;
};

class MulticastClient:public IIoEvent
{
public:
    MulticastClient(AeEngine *engine);
    int Receive(const std::string &addr, const std::string &nic);

private:
    SOCKET Create();
    int ParseAddr(const std::string &addr, MulticastInfo &multicastInfos);
    int JoinGroup(int fd, std::string multicast, std::string interface);    
    int LeaveGroup(int fd, std::string multicast, std::string interface);
    int JoinSourceGroup(int fd, std::string multicast, std::string source, std::string interface);
    int LeaveSourceGroup(int fd, std::string multicast, std::string source, std::string interface);

    int Bind(int fd, std::string local, uint16_t port);

public:
    virtual void OnRead(int fd, ClientData *userData, int mask);
    virtual void OnWrite(int fd, ClientData *userData, int mask);

private:
    AeEngine *m_engine;
};

#endif
