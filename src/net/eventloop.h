#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <atomic>
#include <map>
#include <functional>
#include <queue>
#include <mutex>
#include "coroutine.h"
#include "channel.h"

namespace corpc {

enum EventLoopType {
    MainLoop = 1, // event loop of main thread
    SubLoop = 2   // event loop of io thread
};

class Channel;
class Timer;

class EventLoop {
public:
    typedef std::shared_ptr<EventLoop> ptr;
    explicit EventLoop();
    ~EventLoop();
    void addEvent(int fd, epoll_event event, bool isWakeup = true);
    void delEvent(int fd, bool isWakeup = true);
    void addTask(std::function<void()> task, bool isWakeup = true);
    void addTask(std::vector<std::function<void()>> task, bool isWakeup = true);
    void addCoroutine(corpc::Coroutine::ptr cor, bool isWakeup = true);
    void wakeup();
    void loop();
    void stop();
    Timer *getTimer();
    pid_t getTid();
    void setEventLoopType(EventLoopType type);

public:
    static EventLoop *getEventLoop();

private:
    void addWakeupFd();
    bool isLoopThread() const;
    void addEventInLoopThread(int fd, epoll_event event);
    void delEventInLoopThread(int fd);

private:
    int epfd_{-1};
    int wakefd_{-1};
    int timerfd_{-1};
    bool stopFlag_{false};
    bool isLooping_{false};
    bool isInitTimer_{false};
    pid_t tid_{0};

    std::mutex mutex_;

    std::vector<int> fds_;
    std::atomic<int> fdSize_;

    // fds that wait for operate
    // 1 -- to add to loop
    // 2 -- to del from loop
    std::map<int, epoll_event> pendingAddFds_;
    std::vector<int> pendingDelFds_;
    std::vector<std::function<void()>> pendingTasks_;

    Timer *timer_{nullptr};

    EventLoopType loopType_{SubLoop};
};

class CoroutineTaskQueue {
public:
    static CoroutineTaskQueue *getCoroutineTaskQueue();
    void push(Channel *fd);
    Channel *pop();

private:
    std::queue<Channel*> task_;
    std::mutex mutex_;
};

}

#endif