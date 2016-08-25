
#include <iostream>
#include <assert.h>
#include <errno.h>
#include <cstdlib>

#include "log.h"
#include "file.h"

File::File(AeEngine *engine) :m_eventFd(-1), m_engine(engine)
{}

File::~File()
{}

bool File::Initialize()
{
    bool isOk = true;
    do
    {
        m_eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (m_eventFd < 0)    // -1, errno
        {
            ERROR_LOG("File::Initialize(), create event fd failed, errno: " << errno);
            isOk = false;
            break;
        }
        DEBUG_LOG("File::Initialize(), create event fd: " << m_eventFd);

        memset(&m_aioCtx, 0, sizeof(m_aioCtx));
        int result = io_setup(LIBAIO_EVENTS_SIZE, &m_aioCtx);
        if (result != 0)
        {
            ERROR_LOG("File::Initialize(), libaio initialize failed, errno: " << errno);
            isOk = false;
            break;
        }
        DEBUG_LOG("File::Initialize(), libaio initialize successfully");

        m_engine->AddIoEvent(m_eventFd, AE_READABLE, NULL);
    } while (0);

    return isOk;
}

void File::OnRead(int fd, ClientData *data, int mask)
{
    do
    {
        int eventFd = fd;
        uint64_t finishedNum;
        int result = read(eventFd, &finishedNum, sizeof(finishedNum));
        if (result != sizeof(finishedNum))
        {
            perror("read");
            ERROR_LOG("File::OnRead(), read event fd failed. result: " << result << ", errno: " << errno);
            break;
        }
        while (finishedNum)
        {
            struct io_event aioEvents[LIBAIO_EVENTS_SIZE];
            struct timespec timeout = { 0, 0 };
            int ready = io_getevents(m_aioCtx, 1, LIBAIO_EVENTS_SIZE, aioEvents, &timeout);
            if (ready < 0)
            {
                perror("io_getevent");
                ERROR_LOG("io_getevents() failed. errno: " << ready);
                break;
            }
            for (int i = 0; i < ready; ++i)
            {
                if (aioEvents[i].res < aioEvents[i].obj->u.c.nbytes || aioEvents[i].res2 != 0)
                {
                    OnAsyncIoError(aioEvents[i]);
                }
                else
                {
                    OnAsyncIoComplete(aioEvents[i]);
                }
            }
            finishedNum -= ready;
        }
    } while (0);
}

void File::OnWrite(int fd, ClientData *data, int mask)
{}

bool File::Exist(const string &fileName)
{
    return access(fileName.c_str(), F_OK) == 0;
}
int32_t File::Create(const string &fileName, int64_t size)
{
    int fd = open(fileName.c_str(), O_CREAT | O_EXCL | O_RDWR | O_DIRECT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (fd == -1)     // -1, errno
    {
        if (errno == EEXIST)
        {
            ERROR_LOG("File::Create(), the file: " << fileName << " exists, "
                "or O_EXCL is not supported in the environment, errno: " << errno);
        }
        else
        {
            ERROR_LOG("File::Create(), create file failed, file name: " << fileName << ", errno: " << errno);
        }
        return -1;
    }
    if (size > 0)
    {
        if (fallocate(fd, 0, 0, size) == -1)    // -1
        {
            ERROR_LOG("File::Create(), fallocate() failed, errno: " << errno);
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}

int32_t File::CreateDirectory(const string &directoryName)
{
    int result = mkdir(directoryName.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (result < 0)    // -1
    {
        ERROR_LOG("File::CreateDirectory(), mkdir() failed, directory name: " << directoryName << ", errno: " << errno);
        return -1;
    }
    return 0;
}

int32_t File::Rename(const string &oldName, const string &newName, bool canOverwrite)
{
    int32_t result = 0;
    do
    {
        bool isExist = Exist(oldName);
        if (isExist == false)
        {
            ERROR_LOG("File::Rename(), the original file: " << oldName << " do not exist");
            return -1;
        }

        isExist = Exist(newName);
        if ((isExist == true) && (canOverwrite == false))
        {
            ERROR_LOG("File::Rename(), the destination file: " << newName << " already exists, so move file failed");
            result = -1;
            break;
        }

        struct stat buf;
        int result = stat(oldName.c_str(), &buf);
        if (result < 0)     // -1, errno
        {
            ERROR_LOG("File::Rename(), require file: " << newName << " stat info failed, errno" << errno);
            result = -1;
            break;
        }

        result = rename(oldName.c_str(), newName.c_str());
        if (result < 0)
        {
            ERROR_LOG("File::Rename(), reanme file: " << newName << " failed, errno" << errno);
            result = -1;
            break;
        }

    } while (0);
    return result;
}
int64_t File::GetSize(const string &fileName)
{
    struct stat buf;
    int result = stat(fileName.c_str(), &buf);
    if (result < 0)     // -1, errno
    {
        ERROR_LOG("File::GetSize(), require file: " << fileName << " stat info failed, errno" << errno);
        return -1;
    }
    return buf.st_size;
}
int64_t File::GetSizeOnDisk(const string &fileName)
{
    struct stat buf;
    int result = stat(fileName.c_str(), &buf);
    if (result < 0)     // -1, errno
    {
        ERROR_LOG("File::GetSizeOnDisk(), require file: " << fileName << " stat info failed, errno" << errno);
        return -1;
    }
    return buf.st_blksize / 8 * buf.st_blocks;
}
int32_t File::Delete(const string &fileName)
{
    int result = remove(fileName.c_str());
    if (result < 0)     // -1, errno
    {
        ERROR_LOG("File::Delete(), remove file: " << fileName << " failed, errno" << errno);
        return -1;
    }
    return 0;
}
int32_t File::Scan(const string &path, int layers, list<FileInfo> &fileInfos)
{
    int32_t result = 0;
    DIR *dirFd = NULL;
    do
    {
        if (layers <= 0)
        {
            break;
        }

        assert(path.at(path.size() - 1) == '/');

        dirFd = opendir(path.c_str());
        if (!dirFd)
        {
            ERROR_LOG("File::Scan(), open dir: " << path << " failed, errno" << errno);
            result = -1;
            break;
        }
        struct stat buf;
        struct dirent entry;
        struct dirent *item = NULL;
        result = readdir_r(dirFd, &entry, &item);
        for (; (item != NULL) && (result == 0); result = readdir_r(dirFd, &entry, &item))
        {
            if (strcmp(item->d_name, ".") == 0 ||
                strcmp(item->d_name, "..") == 0)
                continue;
            string fullName = path + item->d_name;
            result = stat(fullName.c_str(), &buf);
            if (result < 0)
            {
                ERROR_LOG("File::Scan(), require file: " << fullName << " stat info failed, "
                    "errno" << errno);
                result = -1;
                continue;
            }
            if (S_ISDIR(buf.st_mode))
            {
                string subPath = item->d_name;
                result = Scan(path + subPath + "/", layers - 1, fileInfos);
                continue;
            }
            FileInfo fileInfo;
            fileInfo.Path = path;
            fileInfo.FileName = item->d_name;
            fileInfo.FileSize = buf.st_size;
            fileInfo.LastModifyTime = buf.st_mtim.tv_sec;
            fileInfos.push_back(fileInfo);
        }

    } while (0);
    if (dirFd != NULL)
    {
        closedir(dirFd);
    }
    return 0;
}

int32_t File::FastScan(const string &directory, int layers, std::list<FileInfo> &fileInfos)
{
    int result = 0;
    do
    {
        std::string path = directory;
        struct dirent **namelist;
        result = scandir(path.c_str(), &namelist, __FileScanFilter, alphasort);
        if (result == -1)
        {
            perror("result");
            DEBUG_LOG("File::FastScan(), execute scandir() failed, errno: " << errno);
            break;
        }
        else
        {
            struct stat buf;
            for (int i = 0; i < result; ++i)
            {
                std::string fullName = path + namelist[i]->d_name;
                if (stat(fullName.c_str(), &buf) < 0)
                {
                    ERROR_LOG("File::FastScan(), require file: " << fullName << " stat info failed, "
                        "errno" << errno);
                    continue;
                }
                if (S_ISDIR(buf.st_mode))
                {
                    continue;
                }
                FileInfo fileInfo;
                fileInfo.Path = path;
                fileInfo.FileName = namelist[i]->d_name;
                fileInfo.FileSize = buf.st_size;
                fileInfo.LastModifyTime = buf.st_mtim.tv_sec;
                fileInfos.push_back(fileInfo);
                free(namelist[i]);
            }
            free(namelist);
        }
    } while (0);

    return result != -1 ? 0 : -1;
}

int __FileScanFilter(const struct dirent *entry)
{
    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0)
        return 0;
    else
        return 1;
};

int32_t File::GetFileInfo(const string &fileName, FileInfo &fileInfo)
{
    int32_t result = 0;
    do
    {
        size_t found = fileName.rfind('/');
        if (found == string::npos || found == fileName.size() - 1)
        {
            ERROR_LOG("File::GetFileInfo(), fileName do not include path. "
                "file name: " << fileName);
            result = EINVAL;
        }

        struct stat buf;
        int result = stat(fileName.c_str(), &buf);
        if (result < 0)
        {
            ERROR_LOG("File::GetFileInfo(), require file: " << fileName << " stat info failed, "
                "errno: " << errno);
            result = -1;
        }

        fileInfo.Path = fileName.substr(0, found + 1);
        fileInfo.FileName = fileName.substr(found + 1);
        fileInfo.FileSize = buf.st_size;
        fileInfo.LastModifyTime = buf.st_mtim.tv_sec;
    } while (0);
    return result;
}

string File::GetFileNameByFullName(const string &fullName)
{
    size_t found = fullName.rfind('/');
    if (found == string::npos || found == fullName.size() - 1)
    {
        ERROR_LOG("File::GetFileInfo(), fileName do not include path");
    }
    return fullName.substr(found + 1);
}

int32_t File::GetCurrentPath(string &currentPath)
{
    int32_t result = 0;
    do
    {
        char buf[PATH_MAX];
        int result = readlink("/proc/self/exe", buf, PATH_MAX);
        if (result < 0 || result >= PATH_MAX)
        {
            perror("readlink");
            ERROR_LOG("File::GetCurrentPath(), readlink() failed");
            result = -1;
            break;
        }

        for (int i = result; i >= 0; --i)
        {
            if (buf[i] == '/')
            {
                buf[i + 1] = '\0';
                break;
            }
        }
        currentPath = buf;
    } while (0);
    return result;
}

int32_t File::Truncate(const string &fullFileName, int64_t length)
{
    int32_t result = truncate(fullFileName.c_str(), length);
    if (result != 0)
    {
        ERROR_LOG("File::Truncate(), truncate file failed, "
            "file: " << fullFileName << ", length: " << length << ", errno: " << errno);
    }
    return result;
}

bool File::ReadToStream(const string &fileName, stringstream &stream)
{
    std::ifstream inFileStream(fileName.c_str());
    if (!inFileStream)
    {
        ERROR_LOG("File::ReadToStream(), open file: " << fileName << " failed, errno" << errno);
        return false;
    }
    stream << inFileStream.rdbuf();
    inFileStream.close();

    return true;
}
bool File::WriteFromStream(const string &fileName, const stringstream &stream)
{
    std::ofstream outFileStream(fileName.c_str());
    if (!outFileStream)
    {
        ERROR_LOG("File::WriteFromStream(), open file: " << fileName << " failed, errno" << errno);
        return false;
    }
    outFileStream << stream.str();
    outFileStream.close();
    return true;
}

int32_t File::ReadSync(const char *fileName, int64_t offset, int64_t length, void *buffer)
{
    int32_t result;
    do
    {
        int fd = open(fileName, O_LARGEFILE | O_RDONLY | O_NOATIME);
        if (fd == -1)     // -1, errno
        {
            perror("open");
            ERROR_LOG("File::ReadSync(), open file: " << fileName << " failed, errno: " << errno);
            result = -1;
            break;
        }
        //struct stat info;
        //if (fstat(fd, &info) == -1)
        //{
        //    result = -1;
        //    ERROR_LOG("File::ReadSync(), fstat file fd: " << fd << " failed, errno: " << errno);
        //    break;
        //}
        if (lseek(fd, offset, SEEK_SET) == -1)
        {
            result = -1;
            break;
        }
        result = read(fd, buffer, length);
    } while (0);   

    return result;
}
int32_t File::WriteSync(const char *fileName, int64_t offset, int64_t length, void *data)
{
    int32_t result;
    do
    {
        int fd = open(fileName, O_LARGEFILE | O_WRONLY);
        if (fd == -1)     // -1, errno
        {
            perror("open");
            ERROR_LOG("File::WriteSync(), open file: " << fileName << " failed, errno: " << errno);
            result = -1;
            break;
        }
        //struct stat info;
        //if (fstat(fd, &info) == -1)
        //{
        //    result = -1;
        //    ERROR_LOG("File::ReadSync(), fstat file fd: " << fd << " failed, errno: " << errno);
        //    break;
        //}
        if (lseek(fd, offset, SEEK_SET) == -1)
        {
            result = -1;
            break;
        }
        result = write(fd, data, length);
    } while (0);

    return result;
}

int32_t File::ReadAsync(const string &fileName, int64_t offset, int64_t length, void *buffer, const AsyncIoInfo *asyncIoInfo)
{
    assert(asyncIoInfo->Callbacker);
    int32_t result = 0;
    do
    {
        int fd = open(fileName.c_str(), O_LARGEFILE | O_DIRECT | O_RDONLY | O_NOATIME);
        if (fd == -1)     // -1, errno
        {
            perror("open");
            ERROR_LOG("File::ReadAsync(), open file: " << fileName << " failed, errno: " << errno);
            result = -1;
            break;
        }
        struct iocb *ptrIocb = new iocb();
        io_prep_pread(ptrIocb, fd, buffer, (size_t)length, offset);
        io_set_eventfd(ptrIocb, m_eventFd);
        ptrIocb->data = (void *)asyncIoInfo;

        result = io_submit(m_aioCtx, 1, &ptrIocb);
        if (result < 0)
        {
            close(fd);
            perror("io_submit");
            ERROR_LOG("File::ReadAsync(), io_submit() failed, errno: " << errno);
            result = -1;
            break;
        }
        result = fd;
    } while (0);
    return result;
}

int32_t File::WriteAsync(const string &fileName, int64_t offset, int64_t length, void *data, const AsyncIoInfo *asyncIoInfo)
{
    assert(asyncIoInfo->Callbacker);
    int32_t result = 0;
    do
    {
        int fd = open(fileName.c_str(), O_LARGEFILE | O_DIRECT | O_WRONLY);
        if (fd < 0)     // -1, errno
        {
            perror("open");
            ERROR_LOG("File::WriteAsnyc(), open file: " << fileName << " failed, errno: " << errno);
            result = -1;
            break;
        }
        struct iocb *ptrIocb = new iocb();
        io_prep_pwrite(ptrIocb, fd, data, length, offset);
        io_set_eventfd(ptrIocb, m_eventFd);
        ptrIocb->data = (void *)asyncIoInfo;

        result = io_submit(m_aioCtx, 1, &ptrIocb);
        if (result < 0)
        {
            close(fd);
            perror("io_submit");
            ERROR_LOG("File::WriteAsnyc(), io_submit() failed, errno: " << errno);
            result = -1;
            break;
        }
        result = fd;
    } while (0);

    return result;
}

void File::OnAsyncIoComplete(struct io_event &aioEvent)
{
    iocb *ptrIocb = aioEvent.obj;
    assert(ptrIocb);
    close(ptrIocb->aio_fildes);
    IAsyncIoEvent *callbacker = ((AsyncIoInfo *)ptrIocb->data)->Callbacker;
    switch (ptrIocb->aio_lio_opcode)
    {
    case IO_CMD_PREAD:
        callbacker->OnRead((AsyncIoInfo *)ptrIocb->data, ptrIocb->u.c.buf,
            ptrIocb->u.c.offset, ptrIocb->u.c.nbytes, aioEvent.res);
        break;

    case IO_CMD_PWRITE:
        callbacker->OnWrite((AsyncIoInfo *)ptrIocb->data, ptrIocb->u.c.buf,
            ptrIocb->u.c.offset, ptrIocb->u.c.nbytes, aioEvent.res);
        break;
    }
}

void File::OnAsyncIoError(struct io_event &aioEvent)
{
    iocb *ptrIocb = aioEvent.obj;
    assert(ptrIocb);
    if (aioEvent.res2 != 0)
    {
        ERROR_LOG("File::OnAsyncIoError(), async io complete failed, res2" << aioEvent.res2);
        close(ptrIocb->aio_fildes);
        IAsyncIoEvent *callbacker = ((AsyncIoInfo *)ptrIocb->data)->Callbacker;
        switch (ptrIocb->aio_lio_opcode)
        {
        case IO_CMD_PREAD:
            callbacker->OnRead((AsyncIoInfo *)ptrIocb->data, ptrIocb->u.c.buf,
                ptrIocb->u.c.offset, ptrIocb->u.c.nbytes, -1);
            break;

        case IO_CMD_PWRITE:
            callbacker->OnWrite((AsyncIoInfo *)ptrIocb->data, ptrIocb->u.c.buf,
                ptrIocb->u.c.offset, ptrIocb->u.c.nbytes, -1);
            break;
        }
    }
    else if (aioEvent.res < ptrIocb->u.c.nbytes)
    {
        ERROR_LOG("The libaio complete bytes is less than request.");
        OnAsyncIoComplete(aioEvent);
    }
}
