#include <memory>
#include <map>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include "eventloop.h"
#include "iothread.h"
#include "timewheel.h"
#include "coroutine.h"
#include "config.h"
#include "coroutine_pool.h"
#include "tcp_connection.h"

namespace corpc {

extern corpc::Config::ptr gConfig;
static thread_local EventLoop *tLoopPtr = nullptr;
static thread_local IOThread *tCurrIoThread = nullptr;

IOThread::IOThread()
{
    int ret = sem_init(&initSemaphore_, 0, 0);
    assert(ret == 0);

    ret = sem_init(&startSemaphore_, 0, 0);
    assert(ret == 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread(std::bind(&IOThread::mainFunc, this)));

    LOG_DEBUG << "semaphore begin to wait until new thread frinish IOThread::mainFunc() to init";
    // wait until new thread finish IOThread::mainFunc() func to init
    ret = sem_wait(&initSemaphore_);
    assert(ret == 0);
    LOG_DEBUG << "semaphore wait end, finish create io thread";

    sem_destroy(&initSemaphore_);
}

IOThread::~IOThread()
{
    loop_->stop();
    thread_->join();

    if (loop_ != nullptr) {
        delete loop_;
        loop_ = nullptr;
    }
}

IOThread *IOThread::getCurrentIOThread()
{
    return tCurrIoThread;
}

sem_t *IOThread::getStartSemaphore()
{
    return &startSemaphore_;
}

EventLoop *IOThread::getEventLoop()
{
    return loop_;
}

std::shared_ptr<std::thread> IOThread::getThreadId()
{
    return thread_;
}

void IOThread::setThreadIndex(const int index)
{
    index_ = index;
}

int IOThread::getThreadIndex()
{
    return index_;
}

void IOThread::mainFunc()
{
    tLoopPtr = new EventLoop();
    assert(tLoopPtr != nullptr);

    tCurrIoThread = this;
    loop_ = tLoopPtr;
    loop_->setEventLoopType(SubLoop);
    tid_ = gettid();

    Coroutine::getCurrentCoroutine(); // io线程的主协程

    LOG_DEBUG << "finish iothread init, now post semaphore";
    sem_post(&initSemaphore_);

    // wait for main thread post startSemaphore_ to start iothread loop
    sem_wait(&startSemaphore_);

    sem_destroy(&startSemaphore_);

    LOG_DEBUG << "IOThread " << tid_ << " begin to loop";
    tLoopPtr->loop();
}

void IOThread::addClient(TcpConnection *tcpConn)
{
    tcpConn->registerToTimeWheel();
    tcpConn->setUpServer();
    return;
}

IOThreadPool::IOThreadPool(int size) : size_(size)
{
    ioThreads_.resize(size);
    for (int i = 0; i < size; ++i) {
        ioThreads_[i] = std::make_shared<IOThread>();
        ioThreads_[i]->setThreadIndex(i);
    }
}

void IOThreadPool::start()
{
    for (int i = 0; i < size_; ++i) {
        int ret = sem_post(ioThreads_[i]->getStartSemaphore());
        assert(ret == 0);
    }
}

// 轮询选择I/O线程
IOThread *IOThreadPool::getIOThread()
{
    index_ = (index_ + 1) % size_;
    return ioThreads_[index_].get();
}

int IOThreadPool::getIOThreadPoolSize()
{
    return size_;
}

void IOThreadPool::broadcastTask(std::function<void()> cb)
{
    for (auto i : ioThreads_) {
        i->getEventLoop()->addTask(cb, true);
    }
}

void IOThreadPool::addTaskByIndex(int index, std::function<void()> cb)
{
    if (index >= 0 && index < size_) {
        ioThreads_[index]->getEventLoop()->addTask(cb, true);
    }
}

void IOThreadPool::addCoroutineToRandomThread(Coroutine::ptr cor, bool self /* = false*/)
{
    if (size_ == 1) {
        ioThreads_[0]->getEventLoop()->addCoroutine(cor, true);
        return;
    }
    srand(time(0));
    int i = 0;
    while (true) {
        i = rand() % (size_);
        if (!self && ioThreads_[i]->getThreadId() == tCurrIoThread->getThreadId()) {
            i++;
            if (i == size_) {
                i -= 2; // 此时下标为size_ - 1的线程为当前线程，-2是为了跳过当前线程
            }
        }
        break;
    }
    ioThreads_[i]->getEventLoop()->addCoroutine(cor, true);
}

Coroutine::ptr IOThreadPool::addCoroutineToRandomThread(std::function<void()> cb, bool self /* = false*/)
{
    Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
    cor->setCallBack(cb);
    addCoroutineToRandomThread(cor, self);
    return cor;
}

Coroutine::ptr IOThreadPool::addCoroutineToThreadByIndex(int index, std::function<void()> cb, bool self /* = false*/)
{
    if (index >= (int)ioThreads_.size() || index < 0) {
        LOG_ERROR << "addCoroutineToThreadByIndex error, invalid iothread index[" << index << "]";
        return nullptr;
    }
    Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
    cor->setCallBack(cb);
    ioThreads_[index]->getEventLoop()->addCoroutine(cor, true);
    return cor;
}

void IOThreadPool::addCoroutineToEachThread(std::function<void()> cb)
{
    for (auto i : ioThreads_) {
        Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
        cor->setCallBack(cb);
        i->getEventLoop()->addCoroutine(cor, true);
    }
}

}