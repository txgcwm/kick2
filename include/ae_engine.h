

#ifndef __AE_ENGINE_H__
#define __AE_ENGINE_H__

#include "ae.h"
#include "thread.h"

struct ClientData
{
    void *Callbacker;
    void *UserData;

    ClientData(void *callbacker, void *userData)
        :Callbacker(callbacker), UserData(userData)
    {}
};

struct IIoEvent
{
    virtual void OnRead (int fd, ClientData *data, int mask) = 0;
    virtual void OnWrite(int fd, ClientData *data, int mask) = 0;
};

class ITimerEvent
{
public:
    virtual int  OnTimer(long long id, ClientData *data)     = 0;
};

class AeEngine :public Thread
{
public:
    AeEngine(int size);
    ~AeEngine();

private:
    AeEngine(const AeEngine &) {}
    AeEngine &operator=(const AeEngine &) { return *this; }

public:
    virtual int Initialize();
    virtual int Main(void *arg);

public:
    int AddIoEvent(int fd, int mask, ClientData *data);
    void DeleteIoEvent(int fd, int mask);

    long long AddTimerEvent(long long milliseconds, ClientData *data);
    int DeleteTimeEvent(long long id);

    void StopEngine();

private:
    aeEventLoop *m_eventLoop;
};

void aeIoReadCallback (struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
void aeIoWriteCallback(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
int  aeTimeCallback(struct aeEventLoop *eventLoop, long long id, void *clientData);

#endif
