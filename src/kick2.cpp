
#include <assert.h>

#include "kick2.h"
#include "datetime.h"
#include "http_client.h"
#include "hls_handler.h"

Config::Config()
    :LaunchInterval(0), Duration(0), ThreadNum(0), Timeout(0), Concurrency(0)
{}


Kick2::Kick2()
    :m_engine(NULL)
{
    m_engine = new AeEngine();
}

void Kick2::Initialize(const Config &config)
{
    m_engine->Initialize();
    m_config = config;
}

void Kick2::LaunchHlsLoad(const std::list<std::string> &playUrls)
{
    assert(playUrls.size());
    HttpClient client(m_engine);
    client.Initialize();
    HlsHandler *hlsHandler = new HlsHandler(m_engine);
    client.RegisterHandler(string(MIME_DEFAULT), hlsHandler);

    uint64_t moment = DateTime::UnixTimeMs();
    uint64_t completedConcurrency = 0;
    std::list<std::string>::const_iterator url = playUrls.begin();
    for (; url != playUrls.end();)
    {
        moment = DateTime::UnixTimeMs();
        HlsTask *task = new HlsTask(*url, moment, &client);
        if (url->find("save=true") || url->find("save=1"))
            task->NeedSave = true;

        client.SendGetReq(task->PlayUrl, task);
        uint64_t sleeps = m_config.LaunchInterval - DateTime::UnixTimeMs() + moment;
        if (sleeps > 0)
            usleep(sleeps * 1000);
        if (m_config.Concurrency > 0)
        {
            ++url;
            if (++completedConcurrency == m_config.Concurrency)
                break;
        }
        else if(task->TaskId % 1000 == 0)
        {
            task->NeedSave = true;
        }
    }
    m_engine->Wait();
}
