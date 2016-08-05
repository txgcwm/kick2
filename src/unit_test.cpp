
#include <iostream>
#include <getopt.h>

#include "log.h"
#include "kick2.h"
#include "datetime.h"
#include "ae_engine.h"
#include "http_client.h"
#include "hls_handler.h"
#include "http_helper.h"
#include "string_helper.h"
#include "http_parser_ext.h"

int main(int argc, char **argv)
{
    Config config;
    config.Duration = 0;
    config.ThreadNum = 1;
    config.LaunchInterval = 10;
    config.Timeout = 1000;
    config.Concurrency = 1;
    config.Url = "http://172.21.11.140:8099/live/HNWS_2000.m3u8";
    config.Url = "http://172.21.11.140:8099/live/HNWS_2000/HNWS_2000_146961928.ts";
    config.Url = "http://192.168.111.131/CCTV8HD_6000.m3u8";

    std::list<std::string> playUrls;
    if (!config.Url.empty())
    {
        playUrls.push_back(config.Url);
    }

    Kick2 *kick = new Kick2();
    kick->Initialize(config);
    kick->LaunchHlsLoad(playUrls);

    while(1)
        usleep(200000000);
    return 0;
}
