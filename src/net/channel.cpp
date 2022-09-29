#include <fcntl.h>
#include <unistd.h>
#include "channel.h"

namespace corpc {

static ChannelContainer* gChannelContainer = nullptr;

Channel::Channel(corpc::EventLoop *loop, int fd/*=-1*/) : fd_(fd), loop_(loop)
{
    if (loop_ == nullptr) {
        LOG_ERROR << "create loop first";
    }
}

Channel::Channel(int fd) : fd_(fd)
{
}

Channel::~Channel() {}

void Channel::handleEvent(IOEvent flag)
{
    if (flag == READ) {
        readCallback_();
    } else if (flag == WRITE) {
        writeCallback_();
    } else {
        LOG_ERROR << "error flag";
    }
}

void Channel::setCallBack(IOEvent flag, std::function<void()> cb)
{
    if (flag == READ) {
        readCallback_ = cb;
    } else if (flag == WRITE) {
        writeCallback_ = cb;
    } else {
        LOG_ERROR << "error flag";
    }
}

std::function<void()> Channel::getCallBack(IOEvent flag) const
{
    if (flag == READ) {
        return readCallback_;
    } else if (flag == WRITE) {
        return writeCallback_;
    }
    return nullptr;
}

void Channel::addListenEvents(IOEvent event)
{
    if (listenEvents_ & event) {
        LOG_DEBUG << "already has this event, skip";
        return;
    }
    listenEvents_ |= event;
    updateToEventLoop();
}

void Channel::delListenEvents(IOEvent event)
{
    if (listenEvents_ & event) {
        LOG_DEBUG << "delete succ";
        listenEvents_ &= ~event;
        updateToEventLoop();
        return;
    }
    LOG_DEBUG << "this event not exist, skip";
}

void Channel::updateToEventLoop()
{
    epoll_event event;
    event.events = listenEvents_;
    event.data.ptr = this;
    if (!loop_) {
        loop_ = corpc::EventLoop::getEventLoop();
    }

    loop_->addEvent(fd_, event);
}

void Channel::unregisterFromEventLoop()
{
    if (!loop_) {
        loop_ = corpc::EventLoop::GetEventLoop();
    }
    loop_->delEvent(fd_);
    listenEvents_ = 0;
    readCallback_ = nullptr;
    writeCallback_ = nullptr;
}

int Channel::getFd() const
{
    return fd_;
}

void Channel::setFd(const int fd)
{
    fd_ = fd;
}

int Channel::getListenEvents() const
{
    return listenEvents_;
}

EventLoop* Channel::getEventLoop() const
{
    return loop_;
}

void Channel::setEventLoop(EventLoop* loop)
{
    loop_ = loop;
}

void Channel::setNonBlock() {
    if (fd_ == -1) {
        LOG_ERROR << "error, fd=-1";
        return;
    }

    int flag = fcntl(fd_, F_GETFL, 0); 
    if (flag & O_NONBLOCK) {
        LOG_DEBUG << "fd already set o_nonblock";
        return;
    }

    fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
    flag = fcntl(fd_, F_GETFL, 0); 
    if (flag & O_NONBLOCK) {
        LOG_DEBUG << "succ set o_nonblock";
    } else {
        LOG_ERROR << "set o_nonblock error";
    }
}

bool Channel::isNonBlock() {
    if (fd_ == -1) {
        LOG_ERROR << "error, fd=-1";
        return false;
    }
    int flag = fcntl(fd_, F_GETFL, 0); 
    return (flag & O_NONBLOCK);
}

void Channel::setCoroutine(Coroutine* cor)
{
    coroutine_ = cor;
}

void Channel::clearCoroutine()
{
    coroutine_ = nullptr;
}

Coroutine* Channel::getCoroutine()
{
    return coroutine_;
}

Channel::ptr ChannelContainer::getChannel(int fd) {
    RWMutex::ReadLock rlock(mutex_);
    if (fd < static_cast<int>(fds_.size())) {
        corpc::Channel::ptr re = fds_[fd]; 
        rlock.unlock();
        return re;
    }
    rlock.unlock();

    RWMutex::WriteLock wlock(mutex_);
    int n = (int)(fd * 1.5);
    for (int i = fds_.size(); i < n; ++i) {
        fds_.push_back(std::make_shared<Channel>(i));
    }
    corpc::Channel::ptr re = fds_[fd]; 
    wlock.unlock();
    return re;
}

ChannelContainer::ChannelContainer(int size) {
    for(int i = 0; i < size; ++i) {
        fds_.push_back(std::make_shared<Channel>(i));
    }
}

ChannelContainer* ChannelContainer::getChannelContainer() {
    if (gChannelContainer == nullptr) {
        gChannelContainer = new ChannelContainer(1000); 
    }
    return gChannelContainer;
}


}