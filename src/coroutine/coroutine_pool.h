#ifndef CORPC_COROUTINE_COUROUTINE_POOL_H
#define CORPC_COROUTINE_COUROUTINE_POOL_H

#include <vector>
#include <mutex>
#include "coroutine.h"
#include "memory.h"

namespace corpc {

class CoroutinePool {

public:
    CoroutinePool(int poolSize, int stackSize = 1024 * 128);
    ~CoroutinePool();

    Coroutine::ptr getCoroutineInstanse();
    void returnCoroutine(Coroutine::ptr cor);

private:
    int poolSize_{0};
    int stackSize_{0};

    // first--ptr of cor
    // second
    //    false -- can be dispatched
    //    true -- can't be dispatched
    std::vector<std::pair<Coroutine::ptr, bool>> freeCors_;

    std::mutex mutex_;

    std::vector<Memory::ptr> memoryPool_;
};

CoroutinePool *getCoroutinePool();

}

#endif