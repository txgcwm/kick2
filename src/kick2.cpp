
#include <stdlib.h>
#include <assert.h>

#include "kick2.h"
#include "mem_pool.h"
#include "datetime.h"
#include "http_client.h"
#include "hls_handler.h"

uint64_t HttpTask::Counter = 10000;

HttpTask::HttpTask(std::string url, uint64_t now, HttpClient *client)
    : TaskId(Counter++), Url(url), StartMoment(0), Client(client), NeedSave(false)
{
    StartMoment = DateTime::UnixTimeMs();
}

Kick2::Kick2()
    :m_engine(NULL), m_memPool(NULL)
{
    m_engine = new AeEngine();
    m_memPool = new MemPool();
}

void Kick2::Initialize(const Config &config)
{
    m_engine->Initialize();
    m_memPool->Initialize(512000000);
    m_config = config;
}

void Kick2::LaunchHlsLoad(const std::list<std::string> &playUrls)
{
    assert(playUrls.size());
    ConnectionPool *connectionPool = new ConnectionPool();
    HttpClient client(m_engine, m_memPool, connectionPool);
    client.Initialize();
    HlsHandler *hlsHandler = new HlsHandler(m_engine);
    client.RegisterHandler(string(MIME_DEFAULT), hlsHandler);

    uint64_t moment = DateTime::UnixTimeMs();
    std::list<std::string>::const_iterator url = playUrls.begin();
    while (m_config.Concurrency > 0 && m_config.Concurrency > HlsTask::Concurrency)
    {
        moment = DateTime::UnixTimeMs();
        HlsTask *task = new HlsTask(*url, moment, &client);
        srand(moment);
        task->TaskId = rand();
        if (url->find("save=true") || url->find("save=1") || m_config.IsDebug)
            task->NeedSave = true;
        client.SendGetReq(task->PlayUrl, task);
        if (m_config.IsDebug)
            break;

        int64_t sleeps = m_config.LaunchInterval - DateTime::UnixTimeMs() + moment;
        if (sleeps > 0)
            usleep(sleeps * 1000);
        if (++url == playUrls.end())
            url = playUrls.begin();
    }
    m_engine->Wait();
}

void Kick2::LaunchHttpLoad(const std::list<std::string> &playUrls)
{
    assert(playUrls.size());
    ConnectionPool *connectionPool = new ConnectionPool();
    HttpClient client(m_engine, m_memPool, connectionPool);
    client.Initialize();

    uint64_t moment = DateTime::UnixTimeMs();
    std::list<std::string>::const_iterator url = playUrls.begin();
    while (1)
    {
        moment = DateTime::UnixTimeMs();
        HttpTask *task = new HttpTask(*url, moment, &client);
        if (url->find("save=true") || url->find("save=1") || m_config.IsDebug)
            task->NeedSave = true;
        client.SendGetReq(*url, task);
        if (m_config.IsDebug)
            break;
        int64_t sleeps = m_config.LaunchInterval - DateTime::UnixTimeMs() + moment;
        if (sleeps > 0)
            usleep(sleeps * 1000);
        if (++url == playUrls.end())
            url = playUrls.begin();
    }
    m_engine->Wait();
}
