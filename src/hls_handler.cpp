
#include <iostream>

#include <assert.h>

#include "log.h"
#include "datetime.h"
#include "string_helper.h"
#include "http_helper.h"
#include "hls_handler.h"

uint64_t HlsTask::Concurrency = 0;

HlsTask::HlsTask(std::string playUrl, uint64_t now, HttpClient *client)
    :TaskId(0),PlayUrl(playUrl), StartMoment(0), NextMoment(0),
    Client(client), IsEndlist(false), NeedSave(false)
{
    StartMoment = DateTime::UnixTimeMs();
    NextMoment = StartMoment;
    ++Concurrency;
}

HlsTask::~HlsTask()
{
    --Concurrency;
}

HlsHandler::HlsHandler(AeEngine *engine)
    :m_engine(engine)
{}

void HlsHandler::Handle(Connection *connection, HttpClient *client, void *userData)
{
    HttpMsg &response = *connection->Response;
    HlsTask *task = (HlsTask *)userData;
    assert(task);

    if (response.StatusCode == 404)
    {
        return;
    }

    if (response.Headers.at(string(HTTP_HEADER_CONTENT_TYPE)) == "application/vnd.apple.mpegurl" ||
        response.Headers.at(string(HTTP_HEADER_CONTENT_TYPE)) == "application/x-www-form-urlencoded")
    {
        HandleM3u8Resp(connection, client, task);
    }
    else
    {
        HandleSegmentResp(connection, client, task);
    }
}


int HlsHandler::HandleM3u8Resp(Connection *connection, HttpClient *client, HlsTask *task)
{
    HttpMsg &response = *connection->Response;
    M3u8Info m3u8Info;
    ParseM3u8(*response.Buf, m3u8Info);
    task->IsEndlist = m3u8Info.IsEndlist;

    list<SegmentInfo>::iterator item = m3u8Info.Sources.begin();
    for (; item != m3u8Info.Sources.end(); ++item)
    {
        string requestUrl;
        if (item->Uri.find("://") != string::npos)
        {
            requestUrl = item->Uri;
        }
        else if (item->Uri.at(0) == '/')
        {
            requestUrl = string("http://") + connection->Host + ":" + STR::ToString(connection->Port)
                + item->Uri;
        }
        else
        {
            std::string basePath = STR::GetRKeyValue(connection->Request->Uri, "", "/") + "/";
            requestUrl = string("http://") + connection->Host + ":" + STR::ToString(connection->Port)
                + basePath + item->Uri;
        }

        uint64_t now = DateTime::UnixTimeMs();
        if (task->NextMoment <= now)
        {
            task->NextMoment += item->Duration;
            ClientData *data = new ClientData(this, task);
            m_engine->AddTimerEvent(task->NextMoment - now, data);
            client->SendGetReq(requestUrl, task);
        }
        else
        {
            item->FullUrl = requestUrl;
            task->TaskQueue.push_back(*item);
            DEBUG_LOG("[TaskId:" << task->TaskId << "] HlsHandler::Handle(), "
                "queue add segment: " << requestUrl);
        }
    }
    return 0;
}
int HlsHandler::ParseM3u8(const Block &buffer, M3u8Info &m3u8Info)
{
    const char *found = strstr(buffer.Data, "#EXTM3U");
    if (found == NULL)
    {
        return -1;
    }
    string content = string(buffer.Data, buffer.Size);
    string strBuf = STR::GetKeyValue(content, "#EXT-X-VERSION:", "\n");
    m3u8Info.Version = STR::Str2UInt64(strBuf);
    strBuf = STR::GetKeyValue(content, "#EXT-X-MEDIA-SEQUENCE:", "\n");
    m3u8Info.Sequence = STR::Str2UInt64(strBuf);

    const Block *block = &buffer;
    const char *pos = found;
    size_t readLen = 0;
    while (*pos != 0x0)
    {
        string line;
        const char *next = strchr(pos, '\n');
        if (next)
        {
            line = string(pos, next - pos);
        }
        else
        {
            line = string(pos);
        }

        SegmentInfo segmentInfo;
        if (strncmp(pos, "#", 1))
        {
            segmentInfo.FileType = M3u8Type;
            segmentInfo.Uri = STR::Trim(line, " \r\n");
            m3u8Info.Sources.push_back(segmentInfo);
        }
        else if (strncmp(pos, "#EXTINF", 7) == 0)
        {
            strBuf = STR::GetKeyValue(line, "#EXTINF:", ",");
            segmentInfo.Duration = STR::Str2UInt64(strBuf) * 1000;
            segmentInfo.FileType = TsType;
            if (!next || *(next + 1) == 0x0)
                break;
            pos = next + 1;
            next = strchr(pos, '\n');
            if (next)
            {
                segmentInfo.Uri = string(pos, next - pos);
                m3u8Info.Sources.push_back(segmentInfo);
            }
        }
        else if (strncmp(pos, "#EXT-X-ENDLIST", 14) == 0)
        {
            m3u8Info.IsEndlist = true;
        }
        if (!next || next - block->Data == block->Size - 1)
        {
            if (block->Next)
            {
                readLen += block->Size;
                block = block->Next;
                next = block->Data;
            }
            else
                break;
        }
        while (*next == '\r' || *next == '\n')
            ++next;
        pos = next;
    }
    return 0;
}

int HlsHandler::HandleSegmentResp(Connection *connection, HttpClient *client, HlsTask *task)
{
    DEBUG_LOG("HlsHandler::HandleSegmentResp(), uri: "<<connection->Request->Uri);
    std::string resourceName = STR::GetRKeyValue(connection->Request->Uri, "/", "?");
    std::string fileName = std::string("./") + resourceName;
    DEBUG_LOG("[TaskId:" << task->TaskId << "] HlsHandler::Handle(), "
        "the segment has got, filename: " << fileName);
    return 0;
}

int HlsHandler::OnTimer(long long id, ClientData *data)
{
    HlsTask *task = (HlsTask *)data->UserData;
    ResumeTask(task);
    delete data;
    return AE_NOMORE;
}

void HlsHandler::ResumeTask(HlsTask *task)
{
    std::string fullUrl;
    if (task->TaskQueue.empty() && task->IsEndlist == false)
    {
        fullUrl = task->PlayUrl;
    }
    else if (task->TaskQueue.empty() && task->IsEndlist == true)
    {
        OnTaskComplete(task);
        return;
    }
    else
    {
        SegmentInfo segment = *task->TaskQueue.begin();
        task->TaskQueue.pop_front();
        fullUrl = segment.FullUrl;
        task->NextMoment += segment.Duration;
        ClientData *data = new ClientData(this, task);
        m_engine->AddTimerEvent(task->NextMoment - DateTime::UnixTimeMs(), data);
    }
    task->Client->SendGetReq(fullUrl, task);
}

void HlsHandler::OnTaskComplete(HlsTask *task)
{
    DEBUG_LOG("[TaskId:" << task->TaskId << "] HlsHandler::OnTaskComplete(), "
        "task has finished, original url: " << task->PlayUrl);
    delete task;
}
