#include <pthread.h>
#include <memory>
#include "mutex.h"
#include "eventloop.h"
#include "log.h"
#include "coroutine.h"
#include "coroutine_hook.h"

// this file copy form sylar

namespace corpc {

CoroutineMutex::CoroutineMutex() {}

CoroutineMutex::~CoroutineMutex()
{
    if (lock_) {
        unlock();
    }
}

void CoroutineMutex::lock()
{
    if (Coroutine::isMainCoroutine()) {
        LOG_ERROR << "main coroutine can't use coroutine mutex";
        return;
    }

    Coroutine *cor = Coroutine::getCurrentCoroutine();

    std::unique_lock<std::mutex> lock(mutex_);
    if (!lock_) {
        lock_ = true;
        LOG_DEBUG << "coroutine succ get coroutine mutex";
        lock.unlock();
    }
    else {
        sleepCors_.push(cor);
        auto temp = sleepCors_;
        lock.unlock();

        LOG_DEBUG << "coroutine yield, pending coroutine mutex, current sleep queue exist ["
                    << temp.size() << "] coroutines";

        Coroutine::yield();
    }
}

void CoroutineMutex::unlock()
{
    if (Coroutine::isMainCoroutine()) {
        LOG_ERROR << "main coroutine can't use coroutine mutex";
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (lock_) {
        lock_ = false;
        if (sleepCors_.empty()) {
            return;
        }

        Coroutine *cor = sleepCors_.front();
        sleepCors_.pop();
        lock.unlock();

        if (cor) {
            // wakeup the first cor in sleep queue
            LOG_DEBUG << "coroutine unlock, now to resume coroutine[" << cor->getCorId() << "]";

            corpc::EventLoop::getEventLoop()->addTask([cor]() { corpc::Coroutine::resume(cor); }, true);
        }
    }
}

}