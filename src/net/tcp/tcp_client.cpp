#include <sys/socket.h>
#include <arpa/inet.h>
#include "log.h"
#include "coroutine.h"
#include "coroutine_hook.h"
#include "coroutine_pool.h"
#include "tcp_client.h"
#include "error_code.h"
#include "timer.h"
#include "channel.h"

namespace corpc {

TcpClient::TcpClient(NetAddress::ptr addr, ProtocolType type /*= Pb_Protocol*/) : peerAddr_(addr)
{
    family_ = peerAddr_->getFamily();
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        LOG_ERROR << "call socket error, fd=-1, sys error=" << strerror(errno);
    }
    LOG_DEBUG << "TcpClient() create fd = " << fd_;
    localAddr_ = std::make_shared<corpc::IPAddress>("127.0.0.1", 0);
    loop_ = EventLoop::getEventLoop();

    if (type == Http_Protocol) {
        codec_ = std::make_shared<HttpCodeC>();
    }
    else if (type == Pb_Protocol) {
        codec_ = std::make_shared<PbCodeC>();
    }

    connection_ = std::make_shared<TcpConnection>(this, loop_, fd_, 128, peerAddr_);
}

TcpClient::~TcpClient()
{
    if (fd_ > 0) {
        ChannelContainer::getChannelContainer()->getChannel(fd_)->unregisterFromEventLoop();
        close(fd_);
        LOG_DEBUG << "~TcpClient() close fd = " << fd_;
    }
}

TcpConnection *TcpClient::getConnection()
{
    if (!connection_.get()) {
        connection_ = std::make_shared<TcpConnection>(this, loop_, fd_, 128, peerAddr_);
    }
    return connection_.get();
}

void TcpClient::resetFd()
{
    corpc::Channel::ptr channel = corpc::ChannelContainer::getChannelContainer()->getChannel(fd_);
    channel->unregisterFromEventLoop();
    close(fd_);
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        LOG_ERROR << "call socket error, fd=-1, sys error=" << strerror(errno);
    }
}

int TcpClient::sendAndRecvPb(const std::string &msgNo, PbStruct::pb_ptr &res)
{
    bool isTimeout = false;
    corpc::Coroutine *curCor = corpc::Coroutine::getCurrentCoroutine();
    auto timercb = [this, &isTimeout, curCor]() {
        LOG_INFO << "TcpClient timer out event occur";
        isTimeout = true;
        this->connection_->setOverTimeFlag(true);
        corpc::Coroutine::resume(curCor);
    };
    TimerEvent::ptr event = std::make_shared<TimerEvent>(maxTimeout_, false, timercb);
    loop_->getTimer()->addTimerEvent(event);

    LOG_DEBUG << "add rpc timer event, timeout on " << event->arriveTime_;

    while (!isTimeout) {
        LOG_DEBUG << "begin to connect";
        if (connection_->getState() != Connected) {
            int ret = connect_hook(fd_, reinterpret_cast<sockaddr *>(peerAddr_->getSockAddr()), peerAddr_->getSockLen());
            if (ret == 0) {
                LOG_DEBUG << "connect [" << peerAddr_->toString() << "] succ!";
                connection_->setUpClient();
                break;
            }
            resetFd();
            if (isTimeout) {
                LOG_INFO << "connect timeout, break";
                goto ERR_DEAL;
            }
            if (errno == ECONNREFUSED) {
                std::stringstream ss;
                ss << "connect error, peer[ " << peerAddr_->toString() << " ] closed.";
                errInfo_ = ss.str();
                LOG_ERROR << "cancel overtime event, err info=" << errInfo_;
                loop_->getTimer()->delTimerEvent(event);
                return ERROR_PEER_CLOSED;
            }
            if (errno == EAFNOSUPPORT) {
                std::stringstream ss;
                ss << "connect cur sys ror, err info is " << std::string(strerror(errno)) << " ] closed.";
                errInfo_ = ss.str();
                LOG_ERROR << "cancle overtime event, err info=" << errInfo_;
                loop_->getTimer()->delTimerEvent(event);
                return ERROR_CONNECT_SYS_ERR;
            }
        }
        else {
            break;
        }
    }

    if (connection_->getState() != Connected) {
        std::stringstream ss;
        ss << "connect peer addr[" << peerAddr_->toString() << "] error. sys error=" << strerror(errno);
        errInfo_ = ss.str();
        loop_->getTimer()->delTimerEvent(event);
        return ERROR_FAILED_CONNECT;
    }

    connection_->setUpClient();
    connection_->output();
    if (connection_->getOverTimerFlag()) {
        LOG_INFO << "send data over time";
        isTimeout = true;
        goto ERR_DEAL;
    }

    while (!connection_->getResPackageData(msgNo, res)) {
        LOG_DEBUG << "redo getResPackageData";
        connection_->input();

        if (connection_->getOverTimerFlag()) {
            LOG_INFO << "read data over time";
            isTimeout = true;
            goto ERR_DEAL;
        }
        if (connection_->getState() == Closed) {
            LOG_INFO << "peer close";
            goto ERR_DEAL;
        }

        connection_->execute();
    }

    loop_->getTimer()->delTimerEvent(event);
    errInfo_ = "";
    return 0;

ERR_DEAL:
    // connect error should close fd and reopen new one
    ChannelContainer::getChannelContainer()->getChannel(fd_)->unregisterFromEventLoop();
    close(fd_);
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    std::stringstream ss;
    if (isTimeout) {
        ss << "call rpc falied, over " << maxTimeout_ << " ms";
        errInfo_ = ss.str();

        connection_->setOverTimeFlag(false);
        return ERROR_RPC_CALL_TIMEOUT;
    }
    else {
        ss << "call rpc falied, peer closed [" << peerAddr_->toString() << "]";
        errInfo_ = ss.str();
        return ERROR_PEER_CLOSED;
    }
}

void TcpClient::stop()
{
    if (!isStop_) {
        isStop_ = true;
        loop_->stop();
    }
}

}
