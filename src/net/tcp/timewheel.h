#ifndef TIMEWHEEL_H
#define TIMEWHEEL_H

#include <queue>
#include <vector>
#include "abstract_slot.h"
#include "event_loop.h"
#include "timer.h"

namespace corpc {

class TcpConnection;

class TcpTimeWheel {

public:
    typedef std::shared_ptr<TcpTimeWheel> ptr;
    typedef AbstractSlot<TcpConnection> TcpConnectionSlot;
    TcpTimeWheel(EventLoop *loop, int bucketCount, int invertal = 10);
    ~TcpTimeWheel();

    void fresh(TcpConnectionSlot::ptr slot);
    void loopFunc();

private:
    EventLoop *loop_{nullptr};
    int bucketCount_{0};
    int interval_{0}; // second

    TimerEvent::ptr event_;
    std::queue<std::vector<TcpConnectionSlot::ptr>> wheel_;
};

}

#endif