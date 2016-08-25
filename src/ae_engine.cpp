
#include <time.h>
#include <assert.h>

#include "ae_engine.h"
#include "zmalloc.h"

AeEngine::AeEngine(int size)
    :m_eventLoop(NULL)
{
    m_eventLoop = aeCreateEventLoop(size);
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
    int result = 0;
    switch (mask)
    {
    case AE_READABLE:
        result = aeCreateFileEvent(m_eventLoop, fd, AE_READABLE, aeIoReadCallback, data);
        break;

    case AE_WRITABLE:
        result = aeCreateFileEvent(m_eventLoop, fd, AE_WRITABLE, aeIoWriteCallback, data);
        break;

    case AE_READABLE | AE_WRITABLE:
        result = aeCreateFileEvent(m_eventLoop, fd, AE_READABLE, aeIoReadCallback, data);
        if (result == AE_OK)
            result = aeCreateFileEvent(m_eventLoop, fd, AE_WRITABLE, aeIoWriteCallback, data);
        break;
    }
    return result;
}

void aeIoReadCallback(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    ClientData *data = (ClientData *)clientData;
    IIoEvent *ioCallbacker = (IIoEvent *)data->Callbacker;
    ioCallbacker->OnRead(fd, data, mask);
}

void aeIoWriteCallback(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    ClientData *data = (ClientData *)clientData;
    IIoEvent *ioCallbacker = (IIoEvent *)data->Callbacker;
    ioCallbacker->OnWrite(fd, data, mask);
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
