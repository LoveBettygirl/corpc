#include <cstdlib>
#include <cassert>
#include <cstring>
#include <atomic>
#include "coroutine.h"
#include "log.h"

namespace corpc {

// main coroutine, every io thread have a main_coroutine
static thread_local Coroutine *mainCoroutine = nullptr;

// current thread is runing which coroutine
static thread_local Coroutine *currCoroutine = nullptr;

static thread_local RunTime *currRuntime = nullptr;

static std::atomic<int> coroutineCount{0};
static std::atomic<int> currCoroutineId{1};

int getCoroutineIndex()
{
    return currCoroutineId;
}

RunTime* getCurrentRunTime()
{
    return currRuntime;
}

void setCurrentRunTime(RunTime* v)
{
    currRuntime = v;
}

void CoFunction(Coroutine *co)
{
    if (co != nullptr) {
        co->setIsInCoFunc(true);

        // 去执行协程回调函数
        co->callback_();

        co->setIsInCoFunc(false);
    }

    // here coroutine's callback function finished, that means coroutine's life is over. we should yiled main couroutine
    Coroutine::yield();
}

// 创建主协程
Coroutine::Coroutine()
{
    // main coroutine'id is 0
    corId_ = 0;
    coroutineCount++;
    memset(&coctx_, 0, sizeof(coctx_));
    currCoroutine = this;
}

Coroutine::Coroutine(int size, char *stack_ptr) : stackSize_(size), stackSp_(stack_ptr)
{
    assert(stackSp_);

    if (!mainCoroutine) {
        mainCoroutine = new Coroutine();
    }

    corId_ = currCoroutineId++;
    coroutineCount++;
}

Coroutine::Coroutine(int size, char *stack_ptr, std::function<void()> cb)
    : stackSize_(size), stackSp_(stack_ptr)
{
    assert(stackSp_);

    if (!mainCoroutine) {
        mainCoroutine = new Coroutine();
    }

    setCallBack(cb);
    corId_ = currCoroutineId++;
    coroutineCount++;
}

bool Coroutine::setCallBack(std::function<void()> cb)
{
    if (this == mainCoroutine) {
        LOG_ERROR << "main coroutine can't set callback";
        return false;
    }
    if (isInCofunc_) {
        LOG_ERROR << "this coroutine is in CoFunction";
        return false;
    }

    callback_ = cb;

    char *top = stackSp_ + stackSize_;

    top = reinterpret_cast<char *>((reinterpret_cast<unsigned long>(top)) & -16LL);

    memset(&coctx_, 0, sizeof(coctx_));

    coctx_.regs[kRSP] = top;
    coctx_.regs[kRBP] = top;
    coctx_.regs[kRETAddr] = reinterpret_cast<char *>(CoFunction);
    coctx_.regs[kRDI] = reinterpret_cast<char *>(this);

    canResume_ = true;

    return true;
}

Coroutine::~Coroutine()
{
    coroutineCount--;
}

Coroutine *Coroutine::getCurrentCoroutine()
{
    if (currCoroutine == nullptr) {
        mainCoroutine = new Coroutine();
        currCoroutine = mainCoroutine;
    }
    return currCoroutine;
}

Coroutine *Coroutine::getMainCoroutine()
{
    if (mainCoroutine) {
        return mainCoroutine;
    }
    mainCoroutine = new Coroutine();
    return mainCoroutine;
}

bool Coroutine::isMainCoroutine()
{
    return mainCoroutine == nullptr || currCoroutine == mainCoroutine;
}

void Coroutine::yield()
{
    if (mainCoroutine == nullptr) {
        LOG_ERROR << "main coroutine is nullptr";
        return;
    }

    if (currCoroutine == mainCoroutine) {
        LOG_ERROR << "current coroutine is main coroutine";
        return;
    }

    Coroutine *co = currCoroutine;
    currCoroutine = mainCoroutine;
    currCoroutine = nullptr;
    coctx_swap(&(co->coctx_), &(mainCoroutine->coctx_));
}

void Coroutine::resume(Coroutine *co)
{
    if (currCoroutine != mainCoroutine) {
        LOG_ERROR << "swap error, current coroutine must be main coroutine";
        return;
    }

    if (!mainCoroutine) {
        LOG_ERROR << "main coroutine is nullptr";
        return;
    }

    if (!co || !co->canResume_) {
        LOG_ERROR << "pending coroutine is nullptr or can_resume is false";
        return;
    }

    if (currCoroutine == co) {
        LOG_ERROR << "current coroutine is pending cor, need't swap";
        return;
    }

    currCoroutine = co;
    currRuntime = co->getRunTime();

    coctx_swap(&(mainCoroutine->coctx_), &(co->coctx_));
}

}
