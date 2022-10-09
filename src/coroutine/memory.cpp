#include <memory>
#include <sys/mman.h>
#include <cassert>
#include <cstdlib>
#include "log.h"
#include "memory.h"

namespace corpc {

Memory::Memory(int blockSize, int blockCount) : blockSize_(blockSize), blockCount_(blockCount)
{
    size_ = blockCount_ * blockSize_;
    start_ = (char *)malloc(size_);
    assert(start_ != (void *)-1);
    LOG_INFO << "succ mmap " << size_ << " bytes memory";
    end_ = start_ + size_;
    blocks_.resize(blockCount_);
    for (size_t i = 0; i < blocks_.size(); ++i) {
        blocks_[i] = false;
    }
    refCounts_ = 0;
}

Memory::~Memory()
{
    if (!start_ || start_ == (void *)-1) {
        return;
    }
    free(start_);
    LOG_INFO << "~succ free munmap " << size_ << " bytes memory";
    start_ = end_ = nullptr;
    refCounts_ = 0;
}

char *Memory::getStart()
{
    return start_;
}

char *Memory::getEnd()
{
    return end_;
}

int Memory::getRefCount()
{
    return refCounts_;
}

char *Memory::getBlock()
{
    int t = -1;
    std::unique_lock<std::mutex> lock(mutex_);
    for (size_t i = 0; i < blocks_.size(); ++i) {
        if (blocks_[i] == false) {
            blocks_[i] = true;
            t = i;
            break;
        }
    }
    lock.unlock();
    if (t == -1) {
        return nullptr;
    }
    refCounts_++;
    return start_ + (t * blockSize_);
}

void Memory::backBlock(char *s)
{
    if (s > end_ || s < start_) {
        LOG_ERROR << "error, this block is not belong to this Memory";
        return;
    }
    int i = (s - start_) / blockSize_;
    std::unique_lock<std::mutex> lock(mutex_);
    blocks_[i] = false;
    lock.unlock();
    refCounts_--;
}

bool Memory::hasBlock(char *s)
{
    return ((s >= start_) && (s < end_));
}

}