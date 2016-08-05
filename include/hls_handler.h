
#ifndef __HLS_HANDLER_H__
#define __HLS_HANDLER_H__

#include <list>
#include <inttypes.h>

#include "mem_pool.h"
#include "ae_engine.h"
#include "http_client.h"

enum HlsFileType
{
    M3u8Type = 1,
    TsType = 2,
    OtherType = 3
};

struct SegmentInfo
{
    HlsFileType FileType;
    int Duration;
    std::string Uri;
    std::string FullUrl;
    SegmentInfo()
        :Duration(0) {}
};

struct M3u8Info
{
    short Version;
    size_t Sequence;
    bool IsEndlist;
    list<SegmentInfo> Sources;
    M3u8Info()
        :Version(0), Sequence(0), IsEndlist(false) {}
};

struct HlsTask
{
    uint64_t TaskId;
    std::string PlayUrl;
    uint64_t StartMoment;
    uint64_t NextMoment;
    std::list<SegmentInfo> TaskQueue;
    HttpClient *Client;
    bool IsEndlist;
    bool NeedSave;
    static uint64_t Concurrency;
    HlsTask(std::string playUrl, uint64_t now, HttpClient *client);
    ~HlsTask();
};

class HlsHandler : public ITimerEvent, public IHttpHandler
{
public:
    HlsHandler(AeEngine *engine);

public:
    virtual void Handle(Connection *connection, HttpClient *client, void *userData);
    virtual int OnTimer(long long id, ClientData *data);
    void ResumeTask(HlsTask *task);
    void OnTaskComplete(HlsTask *task);

private:
    int HandleM3u8Resp(Connection *connection, HttpClient *client, HlsTask *task);
    int HandleSegmentResp(Connection *connection, HttpClient *client, HlsTask *task);
    int ParseM3u8(const Block &buffer, M3u8Info &m3u8Info);

private:
    AeEngine *m_engine;
};

#endif
