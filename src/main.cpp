
#include <list>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>

#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "log.h"
#include "kick2.h"
#include "string_helper.h"

#define VERSION "0.1.1"

static struct option longopts[] = {
    { "duration",            required_argument, NULL, 'd' },
    { "threads",             required_argument, NULL, 't' },
    { "launch-interval",     required_argument, NULL, 'l' },
    { "script",              required_argument, NULL, 's' },
    { "interface",           required_argument, NULL, 'i' },
    { "debug",               no_argument,       NULL, 'D' },
    { "hls",                 no_argument,       NULL, 'H' },
    { "multicast",           no_argument,       NULL, 'm' },
    { "use-memory-max",      required_argument, NULL, 'u' },
    { "timeout",             required_argument, NULL, 'T' },
    { "help",                no_argument,       NULL, 'h' },
    { "version",             no_argument,       NULL, 'v' },
    { NULL,                  0,                 NULL,  0 }
};

static void usage() {
    printf("Usage: kick2 <options> <url>                                   \n"
        "  Options:                                                        \n"
        "    -c, --concurrency        Concurrencys to keep open            \n"
        //"    -d, --duration           Duration of test                     \n"
        "    -t, --threads            Number of threads to use             \n"
        //"                                                                  \n"
        "    -D, --debug              save debug log and response file     \n"
        "    -s, --script             Load script file with request urls   \n"
        "    -i, --interface          Specify the network interface card   \n"
        "    -l, --launch-interval    Launch a new task interval           \n"
        "    -H, --hls                Launch hls load test                 \n"
        "    -m, --multicast          Launch multicast receiver [debug]    \n"
        "    -u, --use-memory-max     Set use memory max [MB]              \n"
        //"    -T, --timeout            Socket/request timeout               \n"
        "    -v, --version            Print version details                \n");
}

int parseArgs(int argc, char **argv, Config &config)
{
    config.Timeout        = 1000;
    config.Duration       = 0;
    config.ThreadNum      = 1;
    config.LaunchInterval = 10;
    config.UseMemoryMax   = 1 * 1024 * 1024 * 1024;
    
    config.ReqType = HTTP_LOAD;
    config.IsDebug = false;
    config.Concurrency = 0;
    config.Interface = "any";

    int c;
    while ((c = getopt_long(argc, argv, "c:t:d:l:s:i:T:u:HDmhv?", longopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            if(!config.IsDebug)
                config.Concurrency = STR::Str2UInt64(optarg);
            break;
        case 'd':
            config.Duration = STR::Str2UInt64(optarg);
            break;
        case 't':
            config.ThreadNum = STR::Str2UInt64(optarg);
            break;
        case 'l':
            config.LaunchInterval = STR::Str2UInt64(optarg);
            break;
        case 'T':
            config.Timeout = STR::Str2UInt64(optarg);
            break;
        case 's':
            config.ScriptFile = optarg;
            break;
        case 'u':
            config.UseMemoryMax = STR::Str2UInt64(optarg) * 1024 * 1024;
            break;
        case 'i':
            config.Interface = optarg;
            break;
        case 'v':
            printf("kick2 %s [%s] ", VERSION, aeGetApiName());
            printf("Copyright (C) 2016 Bonbon\n");
            break;
        case 'H':
            config.ReqType = HLS_LOAD;
            break;
        case 'm':
            config.ReqType = MULTICAST;
            break;
        case 'D':
            config.IsDebug = true;
            config.Concurrency = 1;
            break;
        case 'h':
        case '?':
        case ':':
        default:
            break;
        }
    }
    if (optind != argc)
        config.Url = argv[optind];
    if (config.ScriptFile.empty() && config.Url.empty())
        return -1;
    if (!config.ScriptFile.empty() && !config.Url.empty())
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    Config config;
    if (parseArgs(argc, argv, config) == -1)
    {
        usage();
        return -1;
    }
    std::list<std::string> playUrls;
    if (!config.Url.empty())
    {
        playUrls.push_back(config.Url);
    }
    else
    {
        std::ifstream inFileStream(config.ScriptFile.c_str());
        if (!inFileStream)
        {
            ERROR_LOG("open script file: " << config.ScriptFile << " failed, errno" << errno);
            return -1;
        }
        std::stringstream buf;
        buf << inFileStream.rdbuf();
        inFileStream.close();
        std::string line;
        while (getline(buf, line))
        {
            playUrls.push_back(line);
            if (config.IsDebug)
                break;
        }        
    }

    Kick2 *kick = new Kick2();
    kick->Initialize(config);

    switch (config.ReqType)
    {
    case HTTP_LOAD:
        kick->LaunchHttpLoad(playUrls);
        break;

    case HLS_LOAD:
        kick->LaunchHlsLoad(playUrls);
        break;

    case MULTICAST:
        kick->LaunchMulticastTest(config.Url, config.Interface);
        break;
    }

    return 0;
}
