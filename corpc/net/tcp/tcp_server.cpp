#include <sys/socket.h>
#include <cassert>
#include <fcntl.h>
#include <cstring>
#include "corpc/net/tcp/tcp_server.h"
#include "corpc/net/tcp/io_thread.h"
#include "corpc/net/tcp/timewheel.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/common/config.h"
#include "corpc/net/tcp/tcp_connection.h"
#include "corpc/net/http/http_codec.h"
#include "corpc/net/pb/pb_rpc_dispatcher.h"
#include "corpc/net/pb/pb_codec.h"

namespace corpc {

extern corpc::Config::ptr gConfig;

TcpAcceptor::TcpAcceptor(NetAddress::ptr netAddr) : localAddr_(netAddr)
{
    family_ = localAddr_->getFamily();
}

void TcpAcceptor::init()
{
    listenfd_ = socket(localAddr_->getFamily(), SOCK_STREAM, 0);
    if (listenfd_ < 0) {
        LOG_FATAL << "start server error. socket error, sys error=" << strerror(errno);
    }
    LOG_DEBUG << "create listenfd succ, listenfd=" << listenfd_;

    int val = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        LOG_FATAL << "set REUSEADDR error";
    }

    socklen_t len = localAddr_->getSockLen();
    int ret = bind(listenfd_, localAddr_->getSockAddr(), len);
    if (ret != 0) {
        LOG_FATAL << "start server error. bind error, errno=" << errno << ", error=" << strerror(errno);
    }

    LOG_DEBUG << "set REUSEADDR succ";
    ret = listen(listenfd_, 10);
    if (ret != 0) {
        LOG_FATAL << "start server error. listen error, fd= " << listenfd_ << ", errno=" << errno << ", error=" << strerror(errno);
    }
}

TcpAcceptor::~TcpAcceptor()
{
    Channel::ptr channel = ChannelContainer::getChannelContainer()->getChannel(listenfd_);
    channel->unregisterFromEventLoop();
    if (listenfd_ != -1) {
        close(listenfd_);
    }
}

int TcpAcceptor::toAccept()
{
    socklen_t len = 0;
    int ret = 0;

    if (family_ == AF_INET) {
        sockaddr_in cliAddr;
        memset(&cliAddr, 0, sizeof(cliAddr));
        len = sizeof(cliAddr);
        // call hook accept
        ret = accept_hook(listenfd_, reinterpret_cast<sockaddr *>(&cliAddr), &len);
        if (ret == -1) {
            LOG_DEBUG << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
            return -1;
        }
        LOG_INFO << "New client accepted succ! port:[" << ntohs(cliAddr.sin_port) << "]";
        peerAddr_ = std::make_shared<IPAddress>(cliAddr);
    }
    else if (family_ == AF_UNIX) {
        sockaddr_un cliAddr;
        len = sizeof(cliAddr);
        memset(&cliAddr, 0, sizeof(cliAddr));
        // call hook accept
        ret = accept_hook(listenfd_, reinterpret_cast<sockaddr *>(&cliAddr), &len);
        if (ret == -1) {
            LOG_DEBUG << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
            return -1;
        }
        peerAddr_ = std::make_shared<UnixDomainAddress>(cliAddr);
    }
    else {
        LOG_ERROR << "unknown protocol family type!";
        close(ret);
        return -1;
    }

    LOG_INFO << "New client accepted succ! fd:[" << ret << "], addr:[" << peerAddr_->toString() << "]";
    return ret;
}

TcpServer::TcpServer(NetAddress::ptr addr, ProtocolType type /*= Pb_Protocol*/) : addr_(addr)
{
    ioPool_ = std::make_shared<IOThreadPool>(gConfig->iothreadNum);
    if (type == Http_Protocol) {
        dispatcher_ = std::make_shared<HttpDispacther>();
        codec_ = std::make_shared<HttpCodeC>();
        protocolType_ = Http_Protocol;
    }
    else if (type == Pb_Protocol) {
        dispatcher_ = std::make_shared<PbRpcDispacther>();
        codec_ = std::make_shared<PbCodeC>();
        protocolType_ = Pb_Protocol;
    }

    // main loop对应主线程
    mainLoop_ = corpc::EventLoop::getEventLoop();
    mainLoop_->setEventLoopType(MainLoop);

    // 创建时间轮对象，添加定时事件到main loop的epoll，用于定时关闭非活跃连接
    timeWheel_ = std::make_shared<TcpTimeWheel>(mainLoop_, gConfig->timewheelBucketNum, gConfig->timewheelInterval);

    // 定时清理维护的所有客户端clients_中已关闭的连接，减少资源占用
    clearClientTimerEvent_ = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::clearClientTimerFunc, this));
    mainLoop_->getTimer()->addTimerEvent(clearClientTimerEvent_);

    LOG_INFO << "TcpServer setup on [" << addr_->toString() << "]";
}

void TcpServer::start()
{
    acceptor_.reset(new TcpAcceptor(addr_));
    acceptor_->init();
    // 调用getCoroutinePool()会自动设置主线程的主协程
    acceptCor_ = getCoroutinePool()->getCoroutineInstanse(); // acceptCor_：主线程的子协程，主线程只有这一个子协程，用于接受新连接
    acceptCor_->setCallBack(std::bind(&TcpServer::mainAcceptCorFunc, this));

    LOG_INFO << "resume accept coroutine";
    corpc::Coroutine::resume(acceptCor_.get());

    ioPool_->start();
    mainLoop_->loop();
}

TcpServer::~TcpServer()
{
    if (acceptCor_) {
        getCoroutinePool()->returnCoroutine(acceptCor_);
    }
    LOG_DEBUG << "~TcpServer";
}

void TcpServer::mainAcceptCorFunc()
{
    while (!isStopAccept_) {
        int fd = acceptor_->toAccept();
        if (fd == -1) {
            LOG_ERROR << "accept ret -1 error, return, to yield";
            Coroutine::yield();
            continue;
        }
        IOThread *ioThread = ioPool_->getIOThread(); // 为新连接选择一个io线程，轮询方式
        // 将新连接的fd生成新的tcp连接对象，并加入已连接的客户端列表中
        // 生成新的tcp连接对象的过程中，要将fd封装成channel对象，为channel对象设置好对应的事件循环，初始化缓冲区长度，为tcp连接分配新的子协程，设置状态为已连接（Connected）
        TcpConnection::ptr conn = addClient(ioThread, fd);
        // 为刚才给tcp连接分配的新（子）协程，设置子协程执行的主函数（主函数中需要读连接的数据，处理读到的数据，生成要写的数据，将数据发送出去）
        conn->initServer();
        LOG_DEBUG << "tcpconnection address is " << conn.get() << ", and fd is" << fd;

        // 先在分配的io线程对应loop中开始执行子协程的主函数（如果这个io线程未唤醒，需要先唤醒再执行子协程），以执行到read_hook或write_hook
        ioThread->getEventLoop()->addCoroutine(conn->getCoroutine());
        tcpCounts_++;
        LOG_DEBUG << "current tcp connection count is [" << tcpCounts_ << "]";
    }
}

void TcpServer::addCoroutine(Coroutine::ptr cor)
{
    mainLoop_->addCoroutine(cor);
}

bool TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service)
{
    if (protocolType_ == Pb_Protocol) {
        if (service) {
            dynamic_cast<PbRpcDispacther *>(dispatcher_.get())->registerService(service);
        }
        else {
            LOG_ERROR << "register service error, service ptr is nullptr";
            return false;
        }
    }
    else {
        LOG_ERROR << "register service error. Just PB protocol server need to register Service";
        return false;
    }
    return true;
}

bool TcpServer::registerHttpServlet(const std::string &urlPath, HttpServlet::ptr servlet)
{
    if (protocolType_ == Http_Protocol) {
        if (servlet) {
            dynamic_cast<HttpDispacther *>(dispatcher_.get())->registerServlet(urlPath, servlet);
        }
        else {
            LOG_ERROR << "register http servlet error, servlet ptr is nullptr";
            return false;
        }
    }
    else {
        LOG_ERROR << "register http servlet error. Just Http protocol server need to resgister HttpServlet";
        return false;
    }
    return true;
}

TcpConnection::ptr TcpServer::addClient(IOThread *ioThread, int fd)
{
    auto it = clients_.find(fd);
    if (it != clients_.end()) {
        it->second.reset();
        // set new Tcpconnection
        LOG_DEBUG << "fd " << fd << "have exist, reset it";
        it->second = std::make_shared<TcpConnection>(this, ioThread, fd, 128, getPeerAddr());
        return it->second;
    }
    else {
        LOG_DEBUG << "fd " << fd << "did't exist, new it";
        TcpConnection::ptr conn = std::make_shared<TcpConnection>(this, ioThread, fd, 128, getPeerAddr());
        clients_.insert(std::make_pair(fd, conn));
        return conn;
    }
}

void TcpServer::freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot)
{
    auto cb = [slot, this]() mutable {
        this->getTimeWheel()->fresh(slot);
        slot.reset();
    };
    mainLoop_->addTask(cb);
}

void TcpServer::clearClientTimerFunc()
{
    // delete Closed TcpConnection per loop
    // for free memory
    for (auto &i : clients_) {
        if (i.second && i.second.use_count() > 0 && i.second->getState() == Closed) {
            // need to delete TcpConnection
            LOG_DEBUG << "TcpConection [fd:" << i.first << "] will delete, state=" << i.second->getState();
            (i.second).reset();
        }
    }
}

NetAddress::ptr TcpServer::getPeerAddr()
{
    return acceptor_->getPeerAddr();
}

NetAddress::ptr TcpServer::getLocalAddr()
{
    return addr_;
}

TcpTimeWheel::ptr TcpServer::getTimeWheel()
{
    return timeWheel_;
}

IOThreadPool::ptr TcpServer::getIOThreadPool()
{
    return ioPool_;
}

AbstractDispatcher::ptr TcpServer::getDispatcher()
{
    return dispatcher_;
}

AbstractCodeC::ptr TcpServer::getCodec()
{
    return codec_;
}

}
