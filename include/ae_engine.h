

#ifndef __AE_ENGINE_H__
#define __AE_ENGINE_H__

#include <time.h>

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
    virtual void OnRead (int fd, ClientData *userData, int mask) = 0;
    virtual void OnWrite(int fd, ClientData *userData, int mask) = 0;
    virtual void OnError(int fd, ClientData *userData, int mask) = 0;
};

class ITimerEvent
{
public:
    virtual int  OnTimer(long long id, ClientData *userData)     = 0;
};

class AeEngine :public Thread
{
public:
    AeEngine();
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

void aeIoCallback(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
int aeTimeCallback(struct aeEventLoop *eventLoop, long long id, void *clientData);

#endif
