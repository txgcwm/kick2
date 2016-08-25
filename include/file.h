
#ifndef __FILE_H__
#define __FILE_H__

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <fstream>

#include <fcntl.h>
#include <libaio.h>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <bits/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "ae_engine.h"

using namespace std;

#define LIBAIO_EVENTS_SIZE 512

struct AsyncIoInfo;

struct IAsyncIoEvent
{
    virtual void OnRead(AsyncIoInfo *asyncIoInfo, void *buffer, int64_t offset, int64_t requestBytes, int64_t completedBytes) = 0;
    virtual void OnWrite(AsyncIoInfo *asyncIoInfo, void *buffer, int64_t offset, int64_t requestBytes, int64_t completedBytes) = 0;
};

struct AsyncIoInfo
{
    void *UserData;
    IAsyncIoEvent *Callbacker;
    int32_t Fd;
    AsyncIoInfo()
        :UserData(NULL), Callbacker(NULL), Fd(0) {}
    AsyncIoInfo(void *userData, IAsyncIoEvent *callbacker)
        :UserData(userData), Callbacker(callbacker), Fd(0) {}
};

struct FileInfo
{
    string Path;
    string FileName;
    int64_t FileSize;
    int64_t LastModifyTime;
    string FullFileName()
    {
        return Path + FileName;
    }
    FileInfo()
        :Path(), FileName(), FileSize(-1), LastModifyTime(0) {}
    FileInfo(string path, string fileName, int64_t fileSize, int64_t lastModifyTime)
        :Path(path), FileName(fileName), FileSize(fileSize), LastModifyTime(lastModifyTime) {}        
};

class File :public IIoEvent
{
public:
    File(AeEngine *engine);
    ~File();

public:
    bool Initialize();
    virtual void OnRead(int fd, ClientData *userData, int mask);
    virtual void OnWrite(int fd, ClientData *userData, int mask);

    static bool Exist(const string &fileName);
    static int32_t Create(const string &fileName, int64_t size = 0);
    static int32_t CreateDirectory(const string &directoryName);
    // only support move a normal file
    //static int32_t Move(const string &srcName, const string &destName, bool canOverwrite = FALSE);
    //static int32_t Copy(const string &srcName, const string &destName, bool canOverwrite = FALSE);
    static int32_t Rename(const string &oldName, const string &newName, bool canOverwrite = false);
    static int64_t GetSize(const string &fileName);
    static int64_t GetSizeOnDisk(const string &fileName);
    static int32_t Delete(const string &fileName);
    static int32_t Scan(const string &directory, int layers, std::list<FileInfo> &fileInfo);
    static int32_t FastScan(const string &directory, int layers, std::list<FileInfo> &fileInfo);
    static int32_t GetFileInfo(const string &fileName, FileInfo &fileInfo);
    //static int32_t GetRealPath(const string &path, string &realPath);
    static string GetFileNameByFullName(const string &fullName);
    static int32_t GetCurrentPath(string &currentPath);
    static int32_t Truncate(const string &fullFileName, int64_t length);

    static bool ReadToStream(const string &fileName, stringstream &stream);
    static bool WriteFromStream(const string &fileName, const stringstream &stream);

    static int32_t ReadSync(const char *fileName, int64_t offset, int64_t length, void *buffer);
    static int32_t WriteSync(const char *filename, int64_t offset, int64_t length, void *data);

    int32_t ReadAsync(const string &fileName, int64_t offset, int64_t length, void *buffer, const AsyncIoInfo *asyncInfo);
    int32_t WriteAsync(const string &fileName, int64_t offset, int64_t length, void *data, const AsyncIoInfo *asyncInfo);
    //int32_t AsyncIoCancel(int32_t fd);

private:
    void OnAsyncIoComplete(struct io_event &aioEvent);
    void OnAsyncIoError(struct io_event &aioEvent);

private:
    int                  m_eventFd;
    io_context_t	     m_aioCtx;
    AeEngine            *m_engine;
};

int __FileScanFilter(const struct dirent *entry);

#endif
