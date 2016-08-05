
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "log.h"
#include "string_helper.h"
#include "network_helper.h"
#include "multicast_client.h"

MulticastClient::MulticastClient(AeEngine *engine)
    :m_engine(engine)
{}

int MulticastClient::Receive(const std::string &addr, const std::string &nic)
{
    int result = -1;
    do
    {
        MulticastInfo *multicastInfo = new MulticastInfo();
        if (ParseAddr(addr, *multicastInfo) == -1)
            break;
        int sockFd = Create();
        if (sockFd == INVALID_SOCKET)
        {
            ERROR_LOG("MulticastClient::Receive(), create new socket failed!");
            break;
        }
        multicastInfo->Interface = NetworkHelper::GetIpAddrByNic(nic);
        if (multicastInfo->Source.empty())
            result = JoinGroup(sockFd, multicastInfo->Multicast, multicastInfo->Interface);
        else
            result = JoinSourceGroup(sockFd, multicastInfo->Multicast, multicastInfo->Source, multicastInfo->Interface);
        if (result == -1)
        {
            ERROR_LOG("MulticastClient::Receive(), join group failed! multicast addr: " << addr);
            break;
        }
        result = Bind(sockFd, multicastInfo->Interface, multicastInfo->Port);
        if (result == -1)
        {
            ERROR_LOG("MulticastClient::Receive(), socket bind failed! "
                "bind ip: "<< multicastInfo->Interface<<", bind port: "<< multicastInfo->Port);
            break;
        }            
        ClientData *clientData = new ClientData(this, multicastInfo);
        result = m_engine->AddIoEvent(sockFd, AE_READABLE, clientData);
    } while (0);
    
    return result;
}

SOCKET MulticastClient::Create()
{
    Socket socket;
    if (socket.Create(AF_INET, SOCK_DGRAM, 0) == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
    }
    const char on = 1, off = 0;
    socket.SetOption(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    socket.SetOption(IPPROTO_IP, IP_MULTICAST_LOOP, &off, sizeof(off));
    DEBUG_LOG("MulticastClient::Create(), create new multicast client socket, fd: "<<socket.Fd());
    return socket.Fd();
}

int MulticastClient::ParseAddr(const std::string &addr, MulticastInfo &multicastInfo)
{
    const char *multicastAddr = addr.c_str();
    if (strncmp(multicastAddr, "udp://", 6) || !strchr(multicastAddr + 6, ':'))
    {
        ERROR_LOG("MulticastClient::ParseUrl(), multicast addr is not invalid, multicast addr: " << multicastAddr);
        return -1;
    }

    const char *pos = multicastAddr + 6;
    const char *index = strchr(multicastAddr, '@');
    if (index)
    {
        multicastInfo.Source = std::string(pos, index - pos);
        pos = index + 1;
    }
    index = strchr(pos, ':');
    multicastInfo.Multicast = std::string(pos, index - pos);
    multicastInfo.Port = atoi(index + 1);
    return 0;
}

int MulticastClient::JoinGroup(int fd, std::string multicast, std::string interface)
{
    struct ip_mreq imr;
    uint32_t addr = 0;
    inet_pton(AF_INET, multicast.c_str(), &addr);
    imr.imr_multiaddr.s_addr = addr;
    inet_pton(AF_INET, interface.c_str(), &addr);
    imr.imr_interface.s_addr = addr;
    DEBUG_LOG("MulticastClient::JoinGroup(), it shall join igmp v1/v2 multicast group.");
    return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr));
}

int MulticastClient::LeaveGroup(int fd, std::string multicast, std::string interface)
{
    struct ip_mreq imr;
    uint32_t addr = 0;
    inet_pton(AF_INET, multicast.c_str(), &addr);
    imr.imr_multiaddr.s_addr = addr;
    inet_pton(AF_INET, interface.c_str(), &addr);
    imr.imr_interface.s_addr = addr;
    DEBUG_LOG("MulticastClient::LeaveGroup(), it shall leave igmp v1/v2 multicast group.");
    return setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &imr, sizeof(imr));
}

int MulticastClient::JoinSourceGroup(int fd, std::string multicast, std::string source, std::string interface)
{
    struct ip_mreq_source imr;
    uint32_t addr = 0;
    inet_pton(AF_INET, multicast.c_str(), &addr);
    imr.imr_multiaddr.s_addr = addr;
    inet_pton(AF_INET, source.c_str(), &addr);
    imr.imr_sourceaddr.s_addr = addr;
    inet_pton(AF_INET, interface.c_str(), &addr);
    imr.imr_interface.s_addr = addr;
    DEBUG_LOG("MulticastClient::JoinSourceGroup(), it shall join igmp v3 multicast group.");
    return setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &imr, sizeof(imr));
}

int MulticastClient::LeaveSourceGroup(int fd, std::string multicast, std::string source, std::string interface)
{
    struct ip_mreq_source imr;
    uint32_t addr = 0;
    inet_pton(AF_INET, multicast.c_str(), &addr);
    imr.imr_multiaddr.s_addr = addr;
    inet_pton(AF_INET, source.c_str(), &addr);
    imr.imr_sourceaddr.s_addr = addr;
    inet_pton(AF_INET, interface.c_str(), &addr);
    imr.imr_interface.s_addr = addr;
    DEBUG_LOG("MulticastClient::LeaveSourceGroup(), it shall leave igmp v3 multicast group.");
    return setsockopt(fd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, &imr, sizeof(imr));
}

int MulticastClient::Bind(int fd, std::string local, uint16_t port)
{
    return Socket(fd).Bind(local.c_str(), port);
}

void MulticastClient::OnRead(int fd, ClientData *clientData, int mask)
{
    MulticastInfo *info = (MulticastInfo *)clientData->UserData;
    char buf[8096] = { 0 };
    size_t completed = read(fd, buf, sizeof(buf));
    std::string multicastAddr;
    if (info->Source.empty())
        multicastAddr = std::string("udp://") + info->Multicast + ":" + STR::ToString(info->Port);
    else
        multicastAddr = std::string("udp://") + info->Source + "@" + info->Multicast + ":" + STR::ToString(info->Port);
    DEBUG_LOG("MulticastClient::OnRead(), receive " << completed << " data from " << multicastAddr);
}

void MulticastClient::OnWrite(int fd, ClientData *userData, int mask)
{}
