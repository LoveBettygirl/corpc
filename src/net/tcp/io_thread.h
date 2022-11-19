#ifndef CORPC_NET_TCP_IO_THREAD_H
#define CORPC_NET_TCP_IO_THREAD_H

#include <memory>
#include <map>
#include <atomic>
#include <functional>
#include <semaphore.h>
#include <thread>
#include "event_loop.h"
#include "timewheel.h"
#include "coroutine.h"

namespace corpc {

class TcpServer;

class IOThread {
public:
    typedef std::shared_ptr<IOThread> ptr;
    IOThread();
    ~IOThread();
    EventLoop *getEventLoop();
    void addClient(TcpConnection *tcpConn);
    std::shared_ptr<std::thread> getThreadId();
    void setThreadIndex(const int index);
    int getThreadIndex();
    sem_t *getStartSemaphore();

public:
    static IOThread *getCurrentIOThread();

private:
    void mainFunc();

private:
    EventLoop *loop_{nullptr};
    // pthread_t thread{0};
    std::shared_ptr<std::thread> thread_;
    pid_t tid_{-1};
    TimerEvent::ptr timerEvent_{nullptr};
    int index_{-1};

    sem_t initSemaphore_;
    sem_t startSemaphore_;
};

class IOThreadPool {
public:
    typedef std::shared_ptr<IOThreadPool> ptr;

    IOThreadPool(int size);
    void start();
    IOThread *getIOThread();
    int getIOThreadPoolSize();
    void broadcastTask(std::function<void()> cb);
    void addTaskByIndex(int index, std::function<void()> cb);
    void addCoroutineToRandomThread(Coroutine::ptr cor, bool self = false);

    // add a coroutine to random thread in io thread pool
    // self = false, means random thread cann't be current thread
    // please free cor, or causes memory leak
    // call returnCoroutine(cor) to free coroutine
    Coroutine::ptr addCoroutineToRandomThread(std::function<void()> cb, bool self = false);
    Coroutine::ptr addCoroutineToThreadByIndex(int index, std::function<void()> cb, bool self = false);
    void addCoroutineToEachThread(std::function<void()> cb);

private:
    int size_{0};
    std::atomic<int> index_{-1};
    std::vector<IOThread::ptr> ioThreads_;
};

}

#endif