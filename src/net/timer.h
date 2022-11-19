#ifndef CORPC_NET_TIMER_H
#define CORPC_NET_TIMER_H

#include <ctime>
#include <memory>
#include <map>
#include <functional>
#include "mutex.h"
#include "event_loop.h"
#include "channel.h"
#include "log.h"

namespace corpc {

int64_t getNowMs();

class TimerEvent {
public:
    typedef std::shared_ptr<TimerEvent> ptr;
    TimerEvent(int64_t interval, bool isRepeated, std::function<void()> task)
        : interval_(interval), isRepeated_(isRepeated), task_(task) {
        arriveTime_ = getNowMs() + interval_;
        LOG_DEBUG << "timeevent will occur at " << arriveTime_;
    }

    void resetTime() {
        arriveTime_ = getNowMs() + interval_;
        isCanceled_ = false;
    }

    void wake() {
        isCanceled_ = false;
    }

    void cancel() {
        isCanceled_ = true;
    }

    void cancleRepeated() {
        isRepeated_ = false;
    }

public:
    int64_t arriveTime_;  // when to execute task, ms
    int64_t interval_;    // interval between two tasks, ms
    bool isRepeated_{false};
    bool isCanceled_{false};
    std::function<void()> task_;
};

class Chennel;

class Timer : public Channel {
public:
    typedef std::shared_ptr<Timer> ptr;
    Timer(EventLoop *loop);
    ~Timer();
    void addTimerEvent(TimerEvent::ptr event, bool needReset = true);
    void delTimerEvent(TimerEvent::ptr event);
    void resetArriveTime();
    void onTimer();

private:
    std::multimap<int64_t, TimerEvent::ptr> pendingEvents_;
    RWMutex eventMutex_;
};

}

#endif