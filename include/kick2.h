
#ifndef __KICK2_H__
#define __KICK2_H__

#include <list>
#include <vector>
#include <string>
#include <stdint.h>

#include "ae_engine.h"

enum LodeType
{
    HTTP_LOAD = 1,
    HLS_LOAD = 2,
    MULTICAST = 3
};

struct Config
{
    uint64_t Timeout;
    uint64_t Duration;
    uint64_t ThreadNum;
    uint64_t LaunchInterval;
    uint64_t UseMemoryMax;
    enum LodeType ReqType;
    std::string ScriptFile;
    std::string Url;
    std::string Interface;
    bool IsDebug;
    uint64_t Concurrency;
};

class HttpClient;

struct HttpTask
{
public:
    uint64_t TaskId;
    std::string Url;
    uint64_t StartMoment;
    HttpClient *Client;
    bool NeedSave;
    HttpTask(std::string url, uint64_t now, HttpClient *client);
    inline int Launch();

private:
    static uint64_t Counter;
};

class MemPool;

struct ThreadInfo
{
    AeEngine *Engine;
    HttpClient *Client;
};

class Kick2
{
public:
    Kick2();
    void Initialize(const Config &config);
    void LaunchHttpLoad(const std::list<std::string> &urls);
    void LaunchHlsLoad(const std::list<std::string> &playUrls);
    void LaunchMulticastTest(const std::string &addr, const std::string &interface);

private:
    std::vector<ThreadInfo *> m_threads;
    Config    m_config;
    MemPool *m_memPool;
};

#endif
