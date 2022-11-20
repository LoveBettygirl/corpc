#ifndef CORPC_NET_TCP_TIMEWHEEL_H
#define CORPC_NET_TCP_TIMEWHEEL_H

#include <queue>
#include <vector>
#include "corpc/net/tcp/abstract_slot.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/timer.h"

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