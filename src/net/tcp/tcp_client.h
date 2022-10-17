#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <memory>
#include <google/protobuf/service.h>
#include "coroutine.h"
#include "coroutine_hook.h"
#include "netaddress.h"
#include "eventloop.h"
#include "tcp_connection.h"

namespace corpc {

// You should use TcpClient in a coroutine(not main coroutine)
class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> ptr;

    TcpClient(NetAddress::ptr addr, ProtocalType type = TinyPb_Protocal);
    ~TcpClient();

    void resetFd();
    int sendAndRecvTinyPb(const std::string &msg_no, TinyPbStruct::pb_ptr &res);
    void stop();

    TcpConnection *getConnection();

    void setTimeout(const int v) { maxTimeout_ = v; }
    void setTryCounts(const int v) { tryCounts_ = v; }

    const std::string &getErrInfo() { return errInfo_; }
    NetAddress::ptr getPeerAddr() const { return peerAddr_; }
    NetAddress::ptr getLocalAddr() const { return localAddr_; }
    AbstractCodeC::ptr getCodeC() { return m_codec; }

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

    AbstractCodeC::ptr m_codec{nullptr};

    bool connectSucc_{false};
};

}

#endif