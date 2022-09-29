#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cassert>
#include <mutex>
// #include "loop.h"
#include "log.h"
#include "coroutine.h"
#include "mutex.h"

namespace corpc {

class EventLoop;

enum IOEvent {
    READ = EPOLLIN,
    WRITE = EPOLLOUT,
    ETModel = EPOLLET,
};

class Channel : public std::enable_shared_from_this<Channel> {
public:
    typedef std::shared_ptr<Channel> ptr;

    Channel(corpc::EventLoop *loop, int fd = -1);

    Channel(int fd);

    virtual ~Channel();

    void handleEvent(IOEvent flag);

    void setCallBack(IOEvent flag, std::function<void()> cb);

    std::function<void()> getCallBack(IOEvent flag) const;

    void addListenEvents(IOEvent event);

    void delListenEvents(IOEvent event);

    void updateToEventLoop();

    void unregisterFromEventLoop();

    int getFd() const;

    void setFd(const int fd);

    int getListenEvents() const;

    EventLoop *getEventLoop() const;

    void setEventLoop(EventLoop *r);

    void setNonBlock();

    bool isNonBlock();

    void setCoroutine(Coroutine *cor);

    Coroutine *getCoroutine();

    void clearCoroutine();

public:
    std::mutex mutex_;

protected:
    int fd_{-1};
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;

    int listenEvents_{0};

    EventLoop *loop_{nullptr};

    Coroutine *coroutine_{nullptr};
};

class ChannelContainer {
public:
    ChannelContainer(int size);

    Channel::ptr getChannel(int fd);

public:
    static ChannelContainer *getChannelContainer();

private:
    RWMutex mutex_;
    std::vector<Channel::ptr> fds_;
};

}

#endif