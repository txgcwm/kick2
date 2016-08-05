
#ifndef __KICK2_H__
#define __KICK2_H__

#include <list>
#include <string>
#include <inttypes.h>

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

private:
    static uint64_t Counter;
};

class MemPool;

class Kick2
{
public:
    Kick2();
    void Initialize(const Config &config);
    void LaunchHttpLoad(const std::list<std::string> &playUrls);
    void LaunchHlsLoad(const std::list<std::string> &playUrls);
    void LaunchMulticastTest(const std::string &addr, const std::string &interface);

private:
    AeEngine *m_engine;
    Config    m_config;
    MemPool *m_memPool;
};

#endif
