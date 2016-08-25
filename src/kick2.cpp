
#include <stdlib.h>
#include <assert.h>

#include "kick2.h"
#include "mem_pool.h"
#include "datetime.h"
#include "http_client.h"
#include "hls_handler.h"
#include "multicast_client.h"

uint64_t HttpTask::Counter = 10000;

HttpTask::HttpTask(std::string url, uint64_t now, HttpClient *client)
    : TaskId(Counter++), Url(url), StartMoment(0), Client(client), NeedSave(false)
{
    StartMoment = DateTime::UnixTimeMs();
}

int HttpTask::Launch()
{
    return Client->SendGetReq(Url); 
}

Kick2::Kick2()
    :m_memPool(NULL)
{
    m_memPool = new MemPool();
}

void Kick2::Initialize(const Config &config)
{
    m_memPool->Initialize(config.UseMemoryMax);
    m_config = config;
}

void Kick2::LaunchHlsLoad(const std::list<std::string> &playUrls)
{
    assert(playUrls.size());
    assert(m_config.ThreadNum);
    ConnectionPool *connectionPool = new ConnectionPool();
    for (uint32_t i = 0; i < m_config.ThreadNum; ++i)
    {
        AeEngine *engine = new AeEngine(20000);
        engine->Initialize();

        HttpClient *client = new HttpClient(engine, m_memPool, connectionPool);
        client->Initialize();
        HlsHandler *hlsHandler = new HlsHandler(engine);
        client->RegisterHandler(string(MIME_DEFAULT), hlsHandler);
        
        ThreadInfo *thread = new ThreadInfo();
        thread->Engine = engine;
        thread->Client = client;
        m_threads.push_back(thread);
    }

    uint64_t moment = DateTime::UnixTimeMs();
    uint32_t load = m_config.Concurrency / m_config.ThreadNum;
    std::list<std::string>::const_iterator url = playUrls.begin();
    while (m_config.Concurrency > 0 && m_config.Concurrency > HlsTask::Concurrency)
    {
        moment = DateTime::UnixTimeMs();
        uint32_t index = (HlsTask::Concurrency / load) % m_config.ThreadNum;
        HlsTask *task = new HlsTask(*url, moment, m_threads[index]->Client);
        srand(moment);
        task->TaskId = rand();
        if (url->find("save=true") || url->find("save=1") || m_config.IsDebug)
            task->NeedSave = true;
        task->Launch();
        if (m_config.IsDebug)
            break;

        int64_t sleeps = m_config.LaunchInterval - DateTime::UnixTimeMs() + moment;
        if (sleeps > 0)
            usleep(sleeps * 1000);
        if (++url == playUrls.end())
            url = playUrls.begin();
    }
    for (uint32_t i = 0; i < m_config.ThreadNum; ++i)
    {
        m_threads[i]->Engine->Wait();
    }
}

void Kick2::LaunchHttpLoad(const std::list<std::string> &urls)
{
    assert(urls.size());
    assert(m_config.ThreadNum);
    ConnectionPool *connectionPool = new ConnectionPool();
    for (uint32_t i = 0; i < m_config.ThreadNum; ++i)
    {
        AeEngine *engine = new AeEngine(20000);
        engine->Initialize();

        HttpClient *client = new HttpClient(engine, m_memPool, connectionPool);
        client->Initialize();

        ThreadInfo *thread = new ThreadInfo();
        thread->Engine = engine;
        thread->Client = client;
        m_threads.push_back(thread);
    }

    uint64_t moment = DateTime::UnixTimeMs();
    uint32_t load = m_config.Concurrency / m_config.ThreadNum;
    std::list<std::string>::const_iterator url = urls.begin();
    while (1)
    {
        moment = DateTime::UnixTimeMs();
        uint32_t index = (HlsTask::Concurrency / load) % m_config.ThreadNum;
        HttpTask *task = new HttpTask(*url, moment, m_threads[index]->Client);
        if (url->find("save=true") || url->find("save=1") || m_config.IsDebug)
            task->NeedSave = true;
        task->Launch();
        if (m_config.IsDebug)
            break;
        int64_t sleeps = m_config.LaunchInterval - DateTime::UnixTimeMs() + moment;
        if (sleeps > 0)
            usleep(sleeps * 1000);
        if (++url == urls.end())
            url = urls.begin();
    }
    for (uint32_t i = 0; i < m_config.ThreadNum; ++i)
    {
        m_threads[i]->Engine->Wait();
    }
}

void Kick2::LaunchMulticastTest(const std::string &addr, const std::string &interface)
{
    assert(!addr.empty());
    AeEngine engine(20000);
    engine.Initialize();
    MulticastClient client(&engine);
    if (client.Receive(addr, interface) != -1)
        engine.Wait();
}
