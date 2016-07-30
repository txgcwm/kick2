
#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

void *ThreadMain(void *arg);

class Thread
{
public:
    virtual int Initialize() = 0;
protected:
    int RunTask();
public:
    virtual int Main(void *arg) = 0;
    void Wait();

protected:
    pthread_t m_thread;
};

#endif
