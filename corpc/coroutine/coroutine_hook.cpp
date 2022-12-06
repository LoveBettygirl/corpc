#include <cassert>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/net/channel.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/timer.h"
#include "corpc/common/log.h"
#include "corpc/common/config.h"

#define HOOK_SYS_FUNC(name) name##_fun_ptr_t g_sys_##name##_fun = (name##_fun_ptr_t)dlsym(RTLD_NEXT, #name);

HOOK_SYS_FUNC(accept);
HOOK_SYS_FUNC(read);
HOOK_SYS_FUNC(write);
HOOK_SYS_FUNC(connect);
HOOK_SYS_FUNC(sleep);


namespace corpc {

extern corpc::Config::ptr gConfig;
static bool gHook = true;

void setHook(bool value)
{
    gHook = value;
}

// 为channel（也就是fd）关联当前子协程，将读/写事件加入到事件循环对应的epoll中管理
void toEpoll(corpc::Channel::ptr channel, int events)
{
    corpc::Coroutine *curCor = corpc::Coroutine::getCurrentCoroutine(); // 获取到的一定是子协程
    if (events & corpc::IOEvent::READ) {
        LOG_DEBUG << "fd:[" << channel->getFd() << "], register read event to epoll";
        channel->setCoroutine(curCor);
        channel->addListenEvents(corpc::IOEvent::READ);
    }
    if (events & corpc::IOEvent::WRITE) {
        LOG_DEBUG << "fd:[" << channel->getFd() << "], register write event to epoll";
        channel->setCoroutine(curCor);
        channel->addListenEvents(corpc::IOEvent::WRITE);
    }
}

ssize_t read_hook(int fd, void *buf, size_t count)
{
    LOG_DEBUG << "this is hook read";
    if (corpc::Coroutine::isMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys read func";
        return g_sys_read_fun(fd, buf, count);
    }

    corpc::EventLoop::getEventLoop();

    corpc::Channel::ptr channel = corpc::ChannelContainer::getChannelContainer()->getChannel(fd);
    if (channel->getEventLoop() == nullptr) {
        channel->setEventLoop(corpc::EventLoop::getEventLoop());
    }

    channel->setNonBlock();

    // must first register read event on epoll
    // because loop should always care read event when a connection sockfd was created
    // so if first call sys read, and read return success, this fucntion will not register read event and return
    // for this connection sockfd, loop will never care read event
    ssize_t n = g_sys_read_fun(fd, buf, count);
    if (n > 0) {
        return n;
    }

    toEpoll(channel, corpc::IOEvent::READ);

    LOG_DEBUG << "read func to yield";
    corpc::Coroutine::yield();

    channel->delListenEvents(corpc::IOEvent::READ);
    channel->clearCoroutine();

    LOG_DEBUG << "read func yield back, now to call sys read";
    return g_sys_read_fun(fd, buf, count);
}

// sockfd: listenfd
int accept_hook(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    LOG_DEBUG << "this is hook accept";
    if (corpc::Coroutine::isMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys accept func";
        return g_sys_accept_fun(sockfd, addr, addrlen);
    }
    corpc::EventLoop::getEventLoop(); // 一定是main loop

    corpc::Channel::ptr channel = corpc::ChannelContainer::getChannelContainer()->getChannel(sockfd);
    if (channel->getEventLoop() == nullptr) {
        channel->setEventLoop(corpc::EventLoop::getEventLoop());
    }

    channel->setNonBlock();

    int n = g_sys_accept_fun(sockfd, addr, addrlen);
    if (n > 0) {
        return n;
    }

    toEpoll(channel, corpc::IOEvent::READ);

    LOG_DEBUG << "accept func to yield";
    corpc::Coroutine::yield();

    channel->delListenEvents(corpc::IOEvent::READ);

    LOG_DEBUG << "accept func yield back, now to call sys accept";
    return g_sys_accept_fun(sockfd, addr, addrlen);
}

ssize_t write_hook(int fd, const void *buf, size_t count)
{
    LOG_DEBUG << "this is hook write";
    if (corpc::Coroutine::isMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys write func";
        return g_sys_write_fun(fd, buf, count);
    }
    corpc::EventLoop::getEventLoop();

    corpc::Channel::ptr channel = corpc::ChannelContainer::getChannelContainer()->getChannel(fd);
    if (channel->getEventLoop() == nullptr) {
        channel->setEventLoop(corpc::EventLoop::getEventLoop());
    }

    channel->setNonBlock();

    ssize_t n = g_sys_write_fun(fd, buf, count);
    if (n > 0) {
        return n;
    }

    toEpoll(channel, corpc::IOEvent::WRITE);

    LOG_DEBUG << "write func to yield";
    corpc::Coroutine::yield();

    channel->delListenEvents(corpc::IOEvent::WRITE);
    channel->clearCoroutine();

    LOG_DEBUG << "write func yield back, now to call sys write";
    return g_sys_write_fun(fd, buf, count);
}

int connect_hook(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    LOG_DEBUG << "this is hook connect";
    if (corpc::Coroutine::isMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys connect func";
        return g_sys_connect_fun(sockfd, addr, addrlen);
    }
    corpc::EventLoop *loop = corpc::EventLoop::getEventLoop();

    corpc::Channel::ptr channel = corpc::ChannelContainer::getChannelContainer()->getChannel(sockfd);
    if (channel->getEventLoop() == nullptr) {
        channel->setEventLoop(loop);
    }
    corpc::Coroutine *curCor = corpc::Coroutine::getCurrentCoroutine();

    channel->setNonBlock();
    int n = g_sys_connect_fun(sockfd, addr, addrlen);
    if (n == 0) {
        LOG_DEBUG << "direct connect succ, return";
        return n;
    }
    else if (errno != EINPROGRESS) {
        LOG_DEBUG << "connect error and errno is't EINPROGRESS, errno=" << errno << ",error=" << strerror(errno);
        return n;
    }

    LOG_DEBUG << "errno == EINPROGRESS";

    toEpoll(channel, corpc::IOEvent::WRITE);

    bool isTimeout = false; // 是否超时

    // 超时函数句柄
    auto timeoutcb = [&isTimeout, curCor]() {
        // 设置超时标志，然后唤醒协程
        isTimeout = true;
        corpc::Coroutine::resume(curCor);
    };

    corpc::TimerEvent::ptr event = std::make_shared<corpc::TimerEvent>(gConfig->maxConnectTimeout, false, timeoutcb);

    corpc::Timer *timer = loop->getTimer();
    timer->addTimerEvent(event);

    corpc::Coroutine::yield();

    // write事件需要删除，因为连接成功后后面会重新监听该fd的写事件。
    channel->delListenEvents(corpc::IOEvent::WRITE);
    channel->clearCoroutine();

    // 定时器也需要删除
    timer->delTimerEvent(event);

    n = g_sys_connect_fun(sockfd, addr, addrlen);
    if ((n < 0 && errno == EISCONN) || n == 0) {
        LOG_DEBUG << "connect succ";
        return 0;
    }

    if (isTimeout) {
        LOG_ERROR << "connect error,  timeout[ " << gConfig->maxConnectTimeout << "ms]";
        errno = ETIMEDOUT;
    }

    LOG_DEBUG << "connect error and errno=" << errno << ", error=" << strerror(errno);
    return -1;
}

unsigned int sleep_hook(unsigned int seconds)
{
    LOG_DEBUG << "this is hook sleep";
    if (corpc::Coroutine::isMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys sleep func";
        return g_sys_sleep_fun(seconds);
    }

    corpc::Coroutine *curCor = corpc::Coroutine::getCurrentCoroutine();

    bool isTimeout = false;
    auto timeoutcb = [curCor, &isTimeout]() {
        LOG_DEBUG << "onTime, now resume sleep cor";
        isTimeout = true;
        // 设置超时标志，然后唤醒协程
        corpc::Coroutine::resume(curCor);
    };

    corpc::TimerEvent::ptr event = std::make_shared<corpc::TimerEvent>(1000 * seconds, false, timeoutcb);

    corpc::EventLoop::getEventLoop()->getTimer()->addTimerEvent(event);

    LOG_DEBUG << "now to yield sleep";
    // beacuse read or write maybe resume this coroutine, so when this cor be resumed, must check is timeout, otherwise should yield again
    while (!isTimeout) {
        corpc::Coroutine::yield();
    }

    return 0;
}

}

extern "C" {

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    if (!corpc::gHook) {
        return g_sys_accept_fun(sockfd, addr, addrlen);
    }
    else {
        return corpc::accept_hook(sockfd, addr, addrlen);
    }
}

ssize_t read(int fd, void *buf, size_t count)
{
    if (!corpc::gHook) {
        return g_sys_read_fun(fd, buf, count);
    }
    else {
        return corpc::read_hook(fd, buf, count);
    }
}

ssize_t write(int fd, const void *buf, size_t count)
{
    if (!corpc::gHook) {
        return g_sys_write_fun(fd, buf, count);
    }
    else {
        return corpc::write_hook(fd, buf, count);
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (!corpc::gHook) {
        return g_sys_connect_fun(sockfd, addr, addrlen);
    }
    else {
        return corpc::connect_hook(sockfd, addr, addrlen);
    }
}

unsigned int sleep(unsigned int seconds)
{
    if (!corpc::gHook) {
        return g_sys_sleep_fun(seconds);
    }
    else {
        return corpc::sleep_hook(seconds);
    }
}

}