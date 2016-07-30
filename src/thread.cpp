
#include "thread.h"

int Thread::RunTask()
{
    return pthread_create(&m_thread, NULL, ThreadMain, this);
}

void *ThreadMain(void *arg)
{
    Thread *self = (Thread *)arg;
    self->Main(arg);
    return NULL;
}

void Thread::Wait()
{
    pthread_join(m_thread, NULL);
}
