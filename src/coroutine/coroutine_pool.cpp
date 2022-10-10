#include <vector>
#include <sys/mman.h>
#include "config.h"
#include "log.h"
#include "coroutine_pool.h"
#include "coroutine.h"

namespace corpc {

extern corpc::Config::ptr gConfig;
static CoroutinePool *tCoroutineContainerPtr = nullptr;

CoroutinePool *getCoroutinePool()
{
    if (!tCoroutineContainerPtr) {
        tCoroutineContainerPtr = new CoroutinePool(gConfig->corPoolSize_, gConfig->corStackSize_);
    }
    return tCoroutineContainerPtr;
}

CoroutinePool::CoroutinePool(int poolSize, int stackSize /*= 1024 * 128 B*/) : poolSize_(poolSize), stackSize_(stackSize)
{
    // set main coroutine first
    Coroutine::getCurrentCoroutine();
    memoryPool_.push_back(std::make_shared<Memory>(stackSize, poolSize));
    Memory::ptr temp = memoryPool_[0];

    // 预先分配一部分堆内存，协程优先使用预分配的堆内存，如果用完了再额外申请/释放
    // 避免使用malloc反复申请释放内存（导致内存碎片过多，影响性能）
    for (int i = 0; i < poolSize; ++i) {
        Coroutine::ptr cor = std::make_shared<Coroutine>(stackSize, temp->getBlock());
        cor->setIndex(i);
        freeCors_.push_back(std::make_pair(cor, false));
    }
}

CoroutinePool::~CoroutinePool()
{
}

Coroutine::ptr CoroutinePool::getCoroutineInstanse()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < poolSize_; ++i) {
        if (!freeCors_[i].first->getIsInCoFunc() && !freeCors_[i].second) {
            freeCors_[i].second = true;
            Coroutine::ptr cor = freeCors_[i].first;
            lock.unlock();
            return cor;
        }
    }

    for (size_t i = 1; i < memoryPool_.size(); ++i) {
        char *temp = memoryPool_[i]->getBlock();
        if (temp) {
            Coroutine::ptr cor = std::make_shared<Coroutine>(stackSize_, temp);
            return cor;
        }
    }
    memoryPool_.push_back(std::make_shared<Memory>(stackSize_, poolSize_));
    return std::make_shared<Coroutine>(stackSize_, memoryPool_[memoryPool_.size() - 1]->getBlock());
}

void CoroutinePool::returnCoroutine(Coroutine::ptr cor)
{
    int i = cor->getIndex();
    if (i >= 0 && i < poolSize_) {
        freeCors_[i].second = false;
    }
    else {
        for (size_t i = 1; i < memoryPool_.size(); ++i) {
            if (memoryPool_[i]->hasBlock(cor->getStackPtr())) {
                memoryPool_[i]->backBlock(cor->getStackPtr());
            }
        }
    }
}

}