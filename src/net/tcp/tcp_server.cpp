#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include "tcp_server.h"
#include "iothread.h"
#include "timewheel.h"
#include "coroutine.h"
#include "coroutine_hook.h"
#include "coroutine_pool.h"
#include "config.h"

namespace corpc {

extern corpc::Config::ptr gConfig;

TcpAcceptor::TcpAcceptor(NetAddress::ptr netAddr) : localAddr_(netAddr)
{
    family_ = localAddr_->getFamily();
}

void TcpAcceptor::init()
{
    fd_ = socket(localAddr_->getFamily(), SOCK_STREAM, 0);
    if (fd_ < 0) {
        LOG_FATAL << "start server error. socket error, sys error=" << strerror(errno);
    }
    LOG_DEBUG << "create listenfd succ, listenfd=" << fd_;

    int val = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        LOG_FATAL << "set REUSEADDR error";
    }

    socklen_t len = localAddr_->getSockLen();
    int ret = bind(fd_, localAddr_->getSockAddr(), len);
    if (ret != 0) {
        LOG_FATAL << "start server error. bind error, errno=" << errno << ", error=" << strerror(errno);
    }

    LOG_DEBUG << "set REUSEADDR succ";
    ret = listen(fd_, 10);
    if (ret != 0) {
        LOG_FATAL << "start server error. listen error, fd= " << fd_ << ", errno=" << errno << ", error=" << strerror(errno);
    }
}

TcpAcceptor::~TcpAcceptor()
{
    Channel::ptr channel = ChannelContainer::getChannelContainer()->getChannel(fd_);
    channel->unregisterFromEventLoop();
    if (fd_ != -1) {
        close(fd_);
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
        ret = accept_hook(fd_, reinterpret_cast<sockaddr *>(&cliAddr), &len);
        if (ret == -1) {
            LOG_DEBUG << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
            return -1;
        }
        LOG_INFO << "New client accepted succ! port:[" << cliAddr.sin_port;
        peerAddr_ = std::make_shared<IPAddress>(cliAddr);
    }
    else if (family_ == AF_UNIX) {
        sockaddr_un cliAddr;
        len = sizeof(cliAddr);
        memset(&cliAddr, 0, sizeof(cliAddr));
        // call hook accept
        ret = accept_hook(fd_, reinterpret_cast<sockaddr *>(&cliAddr), &len);
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

    LOG_INFO << "New client accepted succ! fd:[" << ret << ", addr:[" << peerAddr_->toString() << "]";
    return ret;
}

TcpServer::TcpServer(NetAddress::ptr addr, ProtocalType type /*= TinyPb_Protocal*/) : addr_(addr)
{
    ioPool_ = std::make_shared<IOThreadPool>(gConfig->iothreadNum_);
    if (type == Http_Protocal) {
        m_dispatcher = std::make_shared<HttpDispacther>();
        m_codec = std::make_shared<HttpCodeC>();
        m_protocal_type = Http_Protocal;
    }
    else {
        m_dispatcher = std::make_shared<TinyPbRpcDispacther>();
        m_codec = std::make_shared<TinyPbCodeC>();
        m_protocal_type = TinyPb_Protocal;
    }

    mainLoop_ = corpc::EventLoop::getEventLoop();
    mainLoop_->setEventLoopType(MainLoop);

    timeWheel_ = std::make_shared<TcpTimeWheel>(mainLoop_, gConfig->timewheelBucketNum_, gConfig->timewheelInterval_);

    clearClientTimerEvent_ = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::ClearClientTimerFunc, this));
    mainLoop_->getTimer()->addTimerEvent(clearClientTimerEvent_);

    LOG_INFO << "TcpServer setup on [" << addr_->toString() << "]";
}

void TcpServer::start()
{
    acceptor_.reset(new TcpAcceptor(addr_));
    acceptor_->init();
    acceptCor_ = getCoroutinePool()->getCoroutineInstanse();
    acceptCor_->setCallBack(std::bind(&TcpServer::MainAcceptCorFunc, this));

    LOG_INFO << "resume accept coroutine";
    corpc::Coroutine::resume(acceptCor_.get());

    ioPool_->start();
    mainLoop_->loop();
}

TcpServer::~TcpServer()
{
    getCoroutinePool()->returnCoroutine(acceptCor_);
    LOG_DEBUG << "~TcpServer";
}

void TcpServer::MainAcceptCorFunc()
{
    while (!isStopAccept_) {
        int fd = acceptor_->toAccept();
        if (fd == -1) {
            LOG_ERROR << "accept ret -1 error, return, to yield";
            Coroutine::yield();
            continue;
        }
        IOThread *ioThread = ioPool_->getIOThread();
        TcpConnection::ptr conn = addClient(ioThread, fd);
        conn->initServer();
        LOG_DEBUG << "tcpconnection address is " << conn.get() << ", and fd is" << fd;

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
    if (m_protocal_type == TinyPb_Protocal)
    {
        if (service)
        {
            dynamic_cast<TinyPbRpcDispacther *>(m_dispatcher.get())->registerService(service);
        }
        else
        {
            ErrorLog << "register service error, service ptr is nullptr";
            return false;
        }
    }
    else
    {
        ErrorLog << "register service error. Just TinyPB protocal server need to resgister Service";
        return false;
    }
    return true;
}

bool TcpServer::registerHttpServlet(const std::string &url_path, HttpServlet::ptr servlet)
{
    if (m_protocal_type == Http_Protocal)
    {
        if (servlet)
        {
            dynamic_cast<HttpDispacther *>(m_dispatcher.get())->registerServlet(url_path, servlet);
        }
        else
        {
            ErrorLog << "register http servlet error, servlet ptr is nullptr";
            return false;
        }
    }
    else
    {
        ErrorLog << "register http servlet error. Just Http protocal server need to resgister HttpServlet";
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

void TcpServer::ClearClientTimerFunc()
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
    return m_dispatcher;
}

AbstractCodeC::ptr TcpServer::getCodec()
{
    return m_codec;
}

}
