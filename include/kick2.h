
#ifndef __KICK2_H__
#define __KICK2_H__

#include <list>
#include <string>
#include <inttypes.h>

#include "ae_engine.h"

struct Config
{
    uint64_t LaunchInterval;
    uint64_t Duration;
    uint64_t ThreadNum;
    uint64_t Timeout;
    std::string ScriptFile;
    std::string Url;
    uint64_t Concurrency;

    Config();
};

class Kick2
{
public:
    Kick2();
    void Initialize(const Config &config);
    void LaunchHlsLoad(const std::list<std::string> &playUrls);

private:
    AeEngine *m_engine;
    Config    m_config;
};

#endif
