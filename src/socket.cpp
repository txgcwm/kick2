
/*
*
*
*
*	Socket Class Source File		In Matrix
*
*	Created by Bonbon	2015.01.18
*
*	Updated by Bonbon	2015.01.18
*
*/

#include <string>
#include <cstring>
#include <iostream>
#include <errno.h>

#include <sys/ioctl.h>

#include "socket.h"
#include "log.h"

Socket::Socket()
    :m_sockfd(INVALID_SOCKET),
    unblocked(false)
{}

Socket::Socket(SOCKET sockfd)
    :m_sockfd(sockfd)
{}

Socket::~Socket()
{}

SOCKET Socket::Fd() const
{
    return m_sockfd;
}

int Socket::Init()
{
    int ret = 0;
#ifdef WIN32
    WSADATA wd;
    ret = WSAStartup(MAKEWORD(2, 0), &wd);
    if (ret < 0)
    {
        Log::Write(LOG_ERROR, "Winsock startup failed");
    }
#endif
    return ret;
}

void Socket::Uninit()
{
#ifdef WIN32
    WSACleanup();
#endif
}

int Socket::Create(int family, int type, int protocol)
{
    if (INVALID_SOCKET == (m_sockfd = socket(family, type, protocol)))
    {
        ERROR_LOG("Socket::Create error");
        return INVALID_SOCKET;
    }
    //DEBUG_LOG("Socket::Create(), create a new socket: " << m_sockfd);
    return m_sockfd;
}

int Socket::SetBlock()
{
    if (!unblocked)
    {
        return 0;
    }
    else
    {
        unblocked = false;
    }
    int ret;
#ifdef WIN32
    u_long arg = 0;
    ret = ioctlsocket(m_sockfd, FIONBIO, &arg);
    if (NO_ERROR != ret)
    {
        Log::Write(LOG_ERROR, "Socket::SetBlock error");
    }
#else
    int cflags = fcntl(m_sockfd, F_GETFL, 0);
    ret = fcntl(m_sockfd, F_SETFL, cflags & ~O_NONBLOCK);
    if (-1 == ret)
    {
        ERROR_LOG("Socket::SetBlock error");
    }
#endif
    return ret;
}

int Socket::SetNonBlock()
{
    if (unblocked)
    {
        return 0;
    }
    else
    {
        unblocked = true;
    }
    int ret;
#ifdef WIN32
    u_long arg = 1;
    ret = ioctlsocket(m_sockfd, FIONBIO, &arg);
    if (NO_ERROR != ret)
    {
        Log::Write(LOG_ERROR, "Socket::SetNonBlock error");
    }
#else
    int cflags = fcntl(m_sockfd, F_GETFL, 0);
    ret = fcntl(m_sockfd, F_SETFL, cflags | O_NONBLOCK);
    if (-1 == ret)
    {
        ERROR_LOG("Socket::SetNonBlock error");
    }
#endif
    return ret;
}

int Socket::SetOption(int level, int optname, const char *optval, int optlen)
{
    int n = setsockopt(m_sockfd, level, optname, optval, optlen);
    return n;
}

int Socket::Bind(const char *ip, unsigned short port)
{
    int n;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // it may be a bug
    /*if (INADDR_ANY == (unsigned long)ip || INADDR_LOOPBACK == (unsigned long)ip || INADDR_BROADCAST == (unsigned long)ip)
    {
        addr.sin_addr.s_addr = (unsigned long)ip;
    }
    else
    {
        inet_pton(AF_INET, ip, &addr.sin_addr);
    }*/
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if ((n = bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr))) < 0)
    {
        ERROR_LOG("Socket::Bind error");
        Close();
    }
    return n;
}

int Socket::Listen(int backlog)
{
    int n;
    if (-1 == (n = listen(m_sockfd, backlog)))
    {
        ERROR_LOG( "Socket::Listen error");
        Close();
    }
    return n;
}

SOCKET Socket::Accept(struct sockaddr * addr, socklen_t * len)
{
    SOCKET connfd;
    if (INVALID_SOCKET == (connfd = accept(m_sockfd, addr, len)) && !unblocked)
    {
        ERROR_LOG("Accept socket error");
        Close();
    }
    return connfd;
}

int Socket::Connect(const struct sockaddr *addr, int len)
{
    int n;
    if ((n = connect(m_sockfd, addr, len)) < 0 && errno != EINPROGRESS)
    {
        ERROR_LOG("Socket::Connect(), socket connect failed, fd: " << m_sockfd << ", errno: " << errno);
    }
    return n;
}

int Socket::Send(const char *buff, int len, int flags)
{
    int n;
    n = send(m_sockfd, buff, len, flags);
    return (n);
}

int Socket::Recv(char *buff, int len, int flags)
{
    int n;
    n = recv(m_sockfd, buff, len, flags);
    return (n);
}

int Socket::SendTo(const char *buff, int len, int flags, const sockaddr *to, int tolen)
{
    int n;
    n = sendto(m_sockfd, buff, len, flags, to, tolen);
    return (n);
}

int Socket::RecvFrom(char * buff, int len, int flags, struct sockaddr *from, socklen_t * fromlen)
{
    int n;
    n = recvfrom(m_sockfd, buff, len, flags, from, fromlen);
    return (n);
}

size_t Socket::IsReadable(SOCKET fd)
{
    int n, rc;
    rc = ioctl(fd, FIONREAD, &n);
    return rc == -1 ? 0 : n;
}

int Socket::Close()
{
    //DEBUG_LOG("Close fd:" << m_sockfd);
#ifdef WIN32
    return ::closesocket(m_sockfd);
#elif __linux__
    return ::close(m_sockfd);
#endif
}
