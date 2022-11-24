#include <sys/socket.h>
#include <arpa/inet.h>
#include "corpc/common/log.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/net/tcp/tcp_client.h"
#include "corpc/common/error_code.h"
#include "corpc/net/timer.h"
#include "corpc/net/channel.h"
#include "corpc/net/http/http_codec.h"
#include "corpc/net/pb/pb_codec.h"

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

int TcpClient::sendAndRecvPb(const std::string &msgNo, PbStruct::ptr &res)
{
    bool isTimeout = false;
    corpc::Coroutine *curCor = corpc::Coroutine::getCurrentCoroutine(); // 子协程
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
                connection_->setUpClient(); // 设置状态为已连接
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
    connection_->output(); // 发送请求
    if (connection_->getOverTimerFlag()) { // 数据发送超时
        LOG_INFO << "send data over time";
        isTimeout = true;
        goto ERR_DEAL;
    }

    // 接收服务端响应
    // 然后根据消息序列号，找到对应的响应
    while (!connection_->getResPackageData(msgNo, res)) {
        LOG_DEBUG << "redo getResPackageData";
        connection_->input(); // 接收响应

        if (connection_->getOverTimerFlag()) {
            LOG_INFO << "read data over time";
            isTimeout = true;
            goto ERR_DEAL;
        }
        if (connection_->getState() == Closed) {
            LOG_INFO << "peer close";
            goto ERR_DEAL;
        }

        connection_->execute(); // 解码缓冲区中服务端响应，并将序列号和解码后的响应保存起来，避免乱序
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
        ss << "call rpc failed, over " << maxTimeout_ << " ms";
        errInfo_ = ss.str();

        connection_->setOverTimeFlag(false);
        return ERROR_RPC_CALL_TIMEOUT;
    }
    else {
        ss << "call rpc failed, peer closed [" << peerAddr_->toString() << "]";
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
