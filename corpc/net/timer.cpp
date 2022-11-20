#include <sys/timerfd.h>
#include <cassert>
#include <cstring>
#include <vector>
#include <sys/time.h>
#include <functional>
#include <map>
#include "corpc/net/timer.h"
#include "corpc/coroutine/coroutine_hook.h"

extern read_fun_ptr_t g_sys_read_fun; // sys read func

namespace corpc {

int64_t getNowMs()
{
    timeval val;
    gettimeofday(&val, nullptr);
    int64_t re = val.tv_sec * 1000 + val.tv_usec / 1000;
    return re;
}

// timefd 是 Linux 新增的一个特性。他可以让我们把定时器当做套接字 fd 来处理。
// 只需要把 timefd 的可读事件也注册到 epoll 上。当时间到时，timefd发生可读事件，epoll_wait 返回，然后去执行其上绑定的定时任务。
// 可以达到 ms 级别精度
Timer::Timer(EventLoop *loop) : Channel(loop)
{
    fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    LOG_DEBUG << "timer fd = " << fd_;
    if (fd_ == -1) {
        LOG_DEBUG << "timerfd_create error";
    }
    readCallback_ = std::bind(&Timer::onTimer, this);
    addListenEvents(READ);
}

Timer::~Timer()
{
    unregisterFromEventLoop();
    close(fd_);
}

void Timer::addTimerEvent(TimerEvent::ptr event, bool needReset /*=true*/)
{
    RWMutex::WriteLock lock(eventMutex_);
    bool isReset = false;
    // 如果当前事件是第一个事件，需要更新触发时间
    if (pendingEvents_.empty()) {
        isReset = true;
    }
    else {
        auto it = pendingEvents_.begin();
        // 有比当前触发时间更早触发的事件，需要更新触发时间
        if (event->arriveTime_ < it->second->arriveTime_) {
            isReset = true;
        }
    }
    pendingEvents_.emplace(event->arriveTime_, event);
    lock.unlock();

    if (isReset && needReset) {
        LOG_DEBUG << "need reset timer";
        resetArriveTime();
    }
}

void Timer::delTimerEvent(TimerEvent::ptr event)
{
    event->isCanceled_ = true;

    RWMutex::WriteLock lock(eventMutex_);
    auto begin = pendingEvents_.lower_bound(event->arriveTime_);
    auto end = pendingEvents_.upper_bound(event->arriveTime_);
    auto it = begin;
    for (it = begin; it != end; it++) {
        if (it->second == event) {
            LOG_DEBUG << "find timer event, now delete it. src arrive time=" << event->arriveTime_;
            break;
        }
    }
    if (it != pendingEvents_.end()) {
        pendingEvents_.erase(it);
    }
    lock.unlock();
    LOG_DEBUG << "del timer event succ, origin arrive time=" << event->arriveTime_;
}

void Timer::resetArriveTime()
{
    RWMutex::ReadLock lock(eventMutex_);
    std::multimap<int64_t, TimerEvent::ptr> temp = pendingEvents_;
    lock.unlock();

    if (temp.size() == 0) {
        LOG_DEBUG << "no timerevent pending, size = 0";
        return;
    }

    int64_t now = getNowMs();
    auto it = temp.begin();
    if (it->first < now) {
        LOG_DEBUG << "all timer events has already expire";
        return;
    }
    int64_t interval = it->first - now;

    itimerspec newValue;
    memset(&newValue, 0, sizeof(newValue));

    timespec ts;
    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;
    newValue.it_value = ts;

    int ret = timerfd_settime(fd_, 0, &newValue, nullptr);

    if (ret != 0) {
        LOG_ERROR << "tiemr_settime error, interval=" << interval;
    }
}

void Timer::onTimer()
{
    char buf[8] = {0};
    while (true) {
        if ((g_sys_read_fun(fd_, buf, 8) == -1) && errno == EAGAIN) {
            break;
        }
    }

    int64_t now = getNowMs();
    RWMutex::WriteLock lock(eventMutex_);
    auto it = pendingEvents_.begin();
    std::vector<TimerEvent::ptr> tmps;
    std::vector<std::pair<int64_t, std::function<void()>>> tasks;
    for (it = pendingEvents_.begin(); it != pendingEvents_.end(); ++it) {
        if (it->first <= now && !(it->second->isCanceled_)) {
            tmps.push_back(it->second);
            tasks.push_back(std::make_pair(it->second->arriveTime_, it->second->task_));
        }
        else {
            break;
        }
    }

    pendingEvents_.erase(pendingEvents_.begin(), it);
    lock.unlock();

    for (auto i = tmps.begin(); i != tmps.end(); ++i) {
        // 非一次性的定时事件
        if ((*i)->isRepeated_) {
            (*i)->resetTime(); // 根据当前时间和计时间隔（interval_）计算下一次执行事件的时间
            addTimerEvent(*i, false);
        }
    }

    // 定时器触发之后，需要更新下次触发时间
    resetArriveTime();

    for (auto i : tasks) {
        i.second();
    }
}

}
