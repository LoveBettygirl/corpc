#ifndef MEMORY_H
#define MEMORY_H

#include <memory>
#include <atomic>
#include <vector>
#include <mutex>

namespace corpc {

/** 协程使用的栈空间，来源于malloc分配的堆内存 */
class Memory {
public:
    typedef std::shared_ptr<Memory> ptr;

    Memory(int blockSize, int blockCount);
    ~Memory();
    int getRefCount();
    char *getStart();
    char *getEnd();
    char *getBlock(); // 从头开始线性扫描所有内存块，找第一个空闲块
    void backBlock(char *s); // 归还地址s对应的空闲块
    bool hasBlock(char *s); // 判断地址s对应的空闲块是否在start_开始end_结束的内存区域中

private:
    int blockSize_{0};
    int blockCount_{0};

    int size_{0};
    char *start_{nullptr};
    char *end_{nullptr};

    std::atomic<int> refCounts_{0};
    std::vector<uint8_t> blocks_;
    std::mutex mutex_;
};

}

#endif