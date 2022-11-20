#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "corpc/common/log.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/mutex.h"
#include "corpc/net/channel.h"
#include "corpc/net/timer.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/coroutine/coroutine_hook.h"

extern read_fun_ptr_t g_sys_read_fun;   // sys read func
extern write_fun_ptr_t g_sys_write_fun; // sys write func

namespace corpc {

static thread_local EventLoop *tLoopPtr = nullptr; // 当前线程对应的事件循环
static thread_local int tMaxEpollTimeout = 10000; // ms

// 全局协程队列，让其他线程也能执行当前线程中的协程（n-m模型，n个线程执行m个协程）
// 不采用n-m模型，本质上跟muduo 的one loop per thread模式一样（如果改成协程的实现就是n-1模型），还多了协程切换的代价
static CoroutineTaskQueue *tCouroutineTaskQueue = nullptr;

EventLoop::EventLoop()
{
    if (tLoopPtr != nullptr) {
        LOG_FATAL << "this thread has already create a reactor";
    }

    tid_ = gettid();

    LOG_DEBUG << "thread[" << tid_ << "] succ create a reactor object";
    tLoopPtr = this;

    if ((epfd_ = epoll_create(1)) <= 0) {
        LOG_FATAL << "start server error. epoll_create error, sys error=" << strerror(errno);
    }
    else {
        LOG_DEBUG << "epfd_ = " << epfd_;
    }
    // assert(epfd_ > 0);

    if ((wakefd_ = eventfd(0, EFD_NONBLOCK)) <= 0) {
        LOG_FATAL << "start server error. event_fd error, sys error=" << strerror(errno);
    }
    LOG_DEBUG << "wakefd = " << wakefd_;
    // assert(wakefd_ > 0);
    addWakeupFd();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "~EventLoop";
    close(epfd_);
    if (timer_ != nullptr) {
        delete timer_;
        timer_ = nullptr;
    }
    tLoopPtr = nullptr;
}

EventLoop *EventLoop::getEventLoop()
{
    if (tLoopPtr == nullptr) {
        LOG_DEBUG << "Create new EventLoop";
        tLoopPtr = new EventLoop();
    }
    return tLoopPtr;
}

// call by other threads, need lock
void EventLoop::addEvent(int fd, epoll_event event, bool isWakeup /*=true*/)
{
    if (fd == -1) {
        LOG_ERROR << "add error. fd invalid, fd = -1";
        return;
    }
    if (isLoopThread()) {
        addEventInLoopThread(fd, event);
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingAddFds_.insert(std::pair<int, epoll_event>(fd, event));
    }
    if (isWakeup) {
        wakeup();
    }
}

// call by other threads, need lock
void EventLoop::delEvent(int fd, bool isWakeup /*=true*/)
{
    if (fd == -1) {
        LOG_ERROR << "add error. fd invalid, fd = -1";
        return;
    }

    // 如果该loop所在的线程正在运行，那就直接删除fd的事件
    if (isLoopThread()) {
        delEventInLoopThread(fd);
        return;
    }

    // 如果该loop所在的线程未唤醒，先将fd加入到pendingDelFds_数组中，再唤醒线程
    // 唤醒后统一将pendingDelFds_中的所有fd的事件
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingDelFds_.push_back(fd);
    }
    if (isWakeup) {
        wakeup();
    }
}

// 唤醒：向eventfd写8个字节，对应的线程会因eventfd的读事件从epoll_wait中唤醒
void EventLoop::wakeup()
{
    if (!isLooping_) {
        return;
    }

    uint64_t temp = 1;
    if (g_sys_write_fun(wakefd_, &temp, 8) != 8) {
        LOG_ERROR << "write wakeupfd[" << wakefd_ << "] error";
    }
}

bool EventLoop::isLoopThread() const
{
    if (tid_ == gettid()) {
        return true;
    }
    return false;
}

void EventLoop::addWakeupFd()
{
    int op = EPOLL_CTL_ADD;
    epoll_event event;
    event.data.fd = wakefd_;
    event.events = EPOLLIN;
    if ((epoll_ctl(epfd_, op, wakefd_, &event)) != 0) {
        LOG_ERROR << "epoo_ctl error, fd[" << wakefd_ << "], errno=" << errno << ", err=" << strerror(errno);
    }
    fds_.push_back(wakefd_);
}

// need't mutex, only this thread call
void EventLoop::addEventInLoopThread(int fd, epoll_event event)
{
    assert(isLoopThread());

    int op = EPOLL_CTL_ADD;
    bool isAdd = true;
    auto it = find(fds_.begin(), fds_.end(), fd);
    if (it != fds_.end()) {
        isAdd = false;
        op = EPOLL_CTL_MOD;
    }

    if (epoll_ctl(epfd_, op, fd, &event) != 0) {
        LOG_ERROR << "epoll_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
        return;
    }
    if (isAdd) {
        fds_.push_back(fd);
    }
    LOG_DEBUG << "epoll_ctl add succ, fd[" << fd << "]";
}

// need't mutex, only this thread call
void EventLoop::delEventInLoopThread(int fd)
{
    assert(isLoopThread());

    auto it = find(fds_.begin(), fds_.end(), fd);
    if (it == fds_.end()) {
        LOG_DEBUG << "fd[" << fd << "] not in this loop";
        return;
    }
    int op = EPOLL_CTL_DEL;

    if ((epoll_ctl(epfd_, op, fd, nullptr)) != 0) {
        LOG_ERROR << "epoll_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
    }

    fds_.erase(it);
    LOG_DEBUG << "del succ, fd[" << fd << "]";
}

void EventLoop::loop()
{
    assert(isLoopThread());
    if (isLooping_) {
        LOG_DEBUG << "this reactor is looping!";
        return;
    }

    isLooping_ = true;
    stopFlag_ = false;

    // firstCoroutine的作用：第一个协程就让当前线程执行，如果还有其他的协程，再把它放到全局的协程任务队列中
    // 以起到尽量减少访问全局协程队列，起到一些性能提升的作用
    Coroutine *firstCoroutine = nullptr;

    while (!stopFlag_) {
        const int MAX_EVENTS = 10;
        epoll_event reEvents[MAX_EVENTS + 1];

        if (firstCoroutine) {
            corpc::Coroutine::resume(firstCoroutine);
            firstCoroutine = nullptr;
        }

        // main reactor need't to resume coroutine in global CoroutineTaskQueue, only io thread do this work
        if (loopType_ != MainLoop) {
            Channel *ptr = nullptr;
            while (true) {
                ptr = CoroutineTaskQueue::getCoroutineTaskQueue()->pop();
                if (ptr) {
                    ptr->setEventLoop(this);
                    corpc::Coroutine::resume(ptr->getCoroutine());
                }
                else {
                    break;
                }
            }
        }

        std::unique_lock<std::mutex> lock(mutex_);
        std::vector<std::function<void()>> tempTasks;
        tempTasks.swap(pendingTasks_);
        lock.unlock();

        for (size_t i = 0; i < tempTasks.size(); ++i) {
            if (tempTasks[i]) {
                tempTasks[i]();
            }
        }

        int ret = epoll_wait(epfd_, reEvents, MAX_EVENTS, tMaxEpollTimeout);

        if (ret < 0) {
            LOG_ERROR << "epoll_wait error, skip, errno=" << strerror(errno);
        }
        else {
            for (int i = 0; i < ret; ++i) {
                epoll_event oneEvent = reEvents[i];

                // 如果是eventfd的读事件，那就是其他线程唤醒了当前线程
                if (oneEvent.data.fd == wakefd_ && (oneEvent.events & READ)) {
                    // wakeup
                    char buf[8] = {0};
                    while (true) {
                        if ((g_sys_read_fun(wakefd_, buf, 8) == -1) && errno == EAGAIN) {
                            break;
                        }
                    }
                }
                else {
                    Channel *ptr = (Channel*)oneEvent.data.ptr;
                    if (ptr != nullptr) {
                        int fd = ptr->getFd();

                        if ((!(oneEvent.events & EPOLLIN)) && (!(oneEvent.events & EPOLLOUT))) {
                            LOG_ERROR << "socket [" << fd << "] occur other unknow event:[" << oneEvent.events << "], need unregister this socket";
                            delEventInLoopThread(fd);
                        }
                        else {
                            // if register coroutine, pending coroutine to common coroutine_tasks
                            if (ptr->getCoroutine()) {
                                // the first one coroutine when epoll_wait back, just directly resume by this thread, not add to global CoroutineTaskQueue
                                // because every operate CoroutineTaskQueue should add mutex lock
                                if (!firstCoroutine) {
                                    firstCoroutine = ptr->getCoroutine();
                                    continue;
                                }
                                if (loopType_ == SubLoop) {
                                    delEventInLoopThread(fd);
                                    ptr->setEventLoop(nullptr);
                                    CoroutineTaskQueue::getCoroutineTaskQueue()->push(ptr);
                                }
                                else {
                                    // main loop, just resume this coroutine. it is accept coroutine. and main loop only have this coroutine
                                    // main loop只负责接受连接，对应的子协程只有一个accept协程
                                    corpc::Coroutine::resume(ptr->getCoroutine());
                                    if (firstCoroutine) {
                                        firstCoroutine = nullptr;
                                    }
                                }
                            }
                            else {
                                // 该fd没有对应的协程，跟muduo的方式一样
                                std::function<void()> readcb;
                                std::function<void()> writecb;
                                readcb = ptr->getCallBack(READ);
                                writecb = ptr->getCallBack(WRITE);
                                // if timer event, direct excute
                                if (fd == timerfd_) {
                                    readcb();
                                    continue;
                                }
                                if (oneEvent.events & EPOLLIN) {
                                    std::unique_lock<std::mutex> lock(mutex_);
                                    pendingTasks_.push_back(readcb);
                                }
                                if (oneEvent.events & EPOLLOUT) {
                                    std::unique_lock<std::mutex> lock(mutex_);
                                    pendingTasks_.push_back(writecb);
                                }
                            }
                        }
                    }
                }
            }

            std::map<int, epoll_event> tempAdd;
            std::vector<int> tempDel;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                tempAdd.swap(pendingAddFds_);
                pendingAddFds_.clear();

                tempDel.swap(pendingDelFds_);
                pendingDelFds_.clear();
            }
            for (auto i = tempAdd.begin(); i != tempAdd.end(); ++i) {
                addEventInLoopThread((*i).first, (*i).second);
            }
            for (auto i = tempDel.begin(); i != tempDel.end(); ++i) {
                delEventInLoopThread((*i));
            }
        }
    }
    LOG_DEBUG << "loop end";
    isLooping_ = false;
}

void EventLoop::stop()
{
    if (!stopFlag_ && isLooping_) {
        stopFlag_ = true;
        wakeup();
    }
}

void EventLoop::addTask(std::function<void()> task, bool isWakeup /*=true*/)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingTasks_.push_back(task);
    }
    if (isWakeup) {
        wakeup();
    }
}

void EventLoop::addTask(std::vector<std::function<void()>> task, bool isWakeup /* =true*/)
{
    if (task.size() == 0) {
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingTasks_.insert(pendingTasks_.end(), task.begin(), task.end());
    }
    if (isWakeup) {
        wakeup();
    }
}

void EventLoop::addCoroutine(corpc::Coroutine::ptr cor, bool isWakeup /*=true*/)
{
    auto func = [cor]() {
        corpc::Coroutine::resume(cor.get());
    };
    addTask(func, isWakeup);
}

Timer *EventLoop::getTimer()
{
    if (!timer_) {
        timer_ = new Timer(this);
        timerfd_ = timer_->getFd();
    }
    return timer_;
}

pid_t EventLoop::getTid()
{
    return tid_;
}

void EventLoop::setEventLoopType(EventLoopType type)
{
    loopType_ = type;
}

CoroutineTaskQueue *CoroutineTaskQueue::getCoroutineTaskQueue()
{
    if (tCouroutineTaskQueue) {
        return tCouroutineTaskQueue;
    }
    tCouroutineTaskQueue = new CoroutineTaskQueue();
    return tCouroutineTaskQueue;
}

void CoroutineTaskQueue::push(Channel *cor)
{
    std::unique_lock<std::mutex> lock(mutex_);
    task_.push(cor);
    lock.unlock();
}

Channel *CoroutineTaskQueue::pop()
{
    Channel *re = nullptr;
    std::unique_lock<std::mutex> lock(mutex_);
    if (task_.size() >= 1) {
        re = task_.front();
        task_.pop();
    }
    lock.unlock();
    return re;
}

}
