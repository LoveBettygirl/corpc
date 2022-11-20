#ifndef CORPC_NET_TCP_TCP_CLIENT_H
#define CORPC_NET_TCP_TCP_CLIENT_H

#include <memory>
#include "corpc/coroutine/coroutine.h"
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/net/net_address.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/tcp/tcp_connection.h"

namespace corpc {

// You should use TcpClient in a coroutine (not main coroutine)
class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> ptr;

    TcpClient(NetAddress::ptr addr, ProtocolType type = Pb_Protocol);
    ~TcpClient();

    void resetFd();
    int sendAndRecvPb(const std::string &msgNo, PbStruct::ptr &res);
    void stop();

    TcpConnection *getConnection();

    void setTimeout(const int v) { maxTimeout_ = v; }
    void setTryCounts(const int v) { tryCounts_ = v; }

    const std::string &getErrInfo() { return errInfo_; }
    NetAddress::ptr getPeerAddr() const { return peerAddr_; }
    NetAddress::ptr getLocalAddr() const { return localAddr_; }
    AbstractCodeC::ptr getCodeC() { return codec_; }

private:
    int family_{0};
    int fd_{-1};
    int tryCounts_{3};      // max try reconnect times
    int maxTimeout_{10000}; // max connect timeout, ms
    bool isStop_{false};
    std::string errInfo_; // error info of client

    NetAddress::ptr localAddr_{nullptr};
    NetAddress::ptr peerAddr_{nullptr};
    EventLoop *loop_{nullptr};
    TcpConnection::ptr connection_{nullptr};

    AbstractCodeC::ptr codec_{nullptr};

    bool connectSucc_{false};
};

}

#endif