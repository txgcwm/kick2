
#include <assert.h>

#include "ae_engine.h"
#include "zmalloc.h"

AeEngine::AeEngine()
    :m_eventLoop(NULL)
{
    m_eventLoop = aeCreateEventLoop(1000);
}
AeEngine::~AeEngine()
{
    assert(m_eventLoop);
    aeDeleteEventLoop(m_eventLoop);
}

int AeEngine::Initialize()
{
    return RunTask();
}

int AeEngine::Main(void *arg)
{
    aeMain(m_eventLoop);
    return 0;
}

int AeEngine::AddIoEvent(int fd, int mask, ClientData *data)
{
    return aeCreateFileEvent(m_eventLoop, fd, mask, aeIoCallback, data);
}

void aeIoCallback(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    ClientData *data = (ClientData *)clientData;
    IIoEvent *ioCallbacker = (IIoEvent *)data->Callbacker;
    if (mask&AE_ERR)
    {
        ioCallbacker->OnError(fd, data, mask);
    }
    if (mask&AE_READABLE)
    {
        ioCallbacker->OnRead(fd, data, mask);
    }
    if (mask&AE_WRITABLE)
    {
        ioCallbacker->OnWrite(fd, data, mask);
    }
}

void AeEngine::DeleteIoEvent(int fd, int mask)
{
    aeDeleteFileEvent(m_eventLoop, fd, mask);
}

long long AeEngine::AddTimerEvent(long long milliseconds, ClientData *data)
{
    return aeCreateTimeEvent(m_eventLoop, milliseconds, aeTimeCallback, data, NULL);
}

int aeTimeCallback(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    ClientData *data = (ClientData *)clientData;
    ITimerEvent *timeCallbacker = (ITimerEvent *)data->Callbacker;
    return timeCallbacker->OnTimer(id, data);
}

int AeEngine::DeleteTimeEvent(long long id)
{
    return aeDeleteTimeEvent(m_eventLoop, id);
}

void AeEngine::StopEngine()
{
    aeStop(m_eventLoop);
}
