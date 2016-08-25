
#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <map>
#include <list>
#include <stdint.h>

#include "zmalloc.h"

#define INITIAL_BLOCK_SIZE 64*1024*1024

struct Block
{
    char *Data;
    uint32_t Size;
    uint32_t Capacity;
    Block *Next;

    Block(uint32_t capacity)
        :Data(NULL), Size(0), Capacity(capacity), Next(NULL) 
    {
        Data = (char *)zmalloc(capacity);
    }

    Block(char *data, uint32_t capacity)
        :Data(data), Size(0), Capacity(capacity), Next(NULL) {}

    ~Block()
    {
        zfree(Data);
    }
};

class MemPool
{
public:
    MemPool();

public:
    int Initialize(uint64_t initialSize);
    Block *Allocate(uint32_t size);
    void Free(Block *buffer);

private:
    std::map<uint32_t /*offset*/, Block *> m_basePool;
    std::list<Block *> m_pool;
    std::map<uint32_t,std::list<Block *> > m_pool2;
    uint64_t m_maxMemory;
};

int ReadToBuffer(int fd, Block &buffer, uint32_t offset, uint32_t len);

#endif

