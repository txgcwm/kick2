
#include <iostream>
#include <getopt.h>

#include "log.h"
#include "datetime.h"
#include "ae_engine.h"
#include "http_client.h"
#include "hls_handler.h"
#include "http_helper.h"
#include "string_helper.h"
#include "http_parser_ext.h"




int main(int argc, char **argv)
{
    string url = "http://hls.yun.gehua.net.cn:8088/live/CCTV5HD_6000.m3u8?coship_debug=vlc";
    url = "http://192.168.111.131/CCTV8HD_6000.m3u8";
    //url = ccurl;
    AeEngine *engine = new AeEngine();
    engine->Initialize();
    HttpClient client(engine);
    client.Initialize();
    HlsHandler *hlsHandler = new HlsHandler(engine);
    client.RegisterHandler(string(MIME_DEFAULT), hlsHandler);
    client.RegisterHandler("application/x-www-form-urlencoded", hlsHandler);
    client.RegisterHandler("application/vnd.apple.mpegurl", hlsHandler);
    client.RegisterHandler("video/mp2t", hlsHandler);
    client.RegisterHandler("application/octet-stream", hlsHandler);
    
    //while (0)
    {
        HlsTask *task = new HlsTask(url, DateTime::UnixTimeMs(), &client);
        client.SendGetReq(task->PlayUrl, task);
        usleep(100000000);
    }
    
    //string tmp = "#abc?|def#|ghi?|jkl|mno?";
    //
    //DEBUG_LOG("tmp string: " << tmp);
    //DEBUG_LOG("STR::GetKeyValue(tmp, \"abc\", \"|\"): " << STR::GetKeyValue(tmp, "abc", "|"));
    //DEBUG_LOG("STR::GetKeyValue(tmp, \"jkl\", \"def\"): " << STR::GetKeyValue(tmp, "jkl", "def"));
    //DEBUG_LOG("STR::GetKeyValue(tmp, \"?\", \"?\"): " << STR::GetKeyValue(tmp, "?", "?"));
    //
    //DEBUG_LOG("STR::GetRKeyValue(tmp, \"abc\", \"|\"): " << STR::GetRKeyValue(tmp, "abc", "|"));
    //DEBUG_LOG("STR::GetRKeyValue(tmp, \"jkl\", \"def\"): " << STR::GetRKeyValue(tmp, "jkl", "def"));
    //DEBUG_LOG("STR::GetRKeyValue(tmp, \"?\", \"?\"): " << STR::GetRKeyValue(tmp, "?", "?"));

    while(1)
        usleep(200000000);
    return 0;
}
