#ifndef CORPC_NET_TCP_TCP_CLIENT_H
#define CORPC_NET_TCP_TCP_CLIENT_H

#include <memory>
#include <functional>
#include "corpc/coroutine/coroutine.h"
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/net/net_address.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/tcp/tcp_connection.h"
#include "corpc/net/custom/custom_data.h"
#include "corpc/net/custom/custom_codec.h"

namespace corpc {

// You should use TcpClient in a coroutine (not main coroutine)
class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> ptr;

    TcpClient(NetAddress::ptr addr, ProtocolType type = Pb_Protocol);
    ~TcpClient();

    void resetFd();
    int sendAndRecvPb(const std::string &msgNo, PbStruct::ptr &res);
    int sendAndRecvData(CustomStruct::ptr &res);
    int sendData();
    int recvData(CustomStruct::ptr &res);
    void stop();

    TcpConnection *getConnection();

    void setTimeout(const int64_t v) { maxTimeout_ = v; }

    const std::string &getErrInfo() { return errInfo_; }
    NetAddress::ptr getPeerAddr() const { return peerAddr_; }
    NetAddress::ptr getLocalAddr() const { return localAddr_; }
    AbstractCodeC::ptr getCodeC() { return codec_; }
    void setCustomCodeC(CustomCodeC::ptr codec);
    void setCustomData(std::function<CustomStruct::ptr()> func);
    std::function<CustomStruct::ptr()> getCustomData() { return getCustomData_; }

private:
    int family_{0};
    int fd_{-1};
    int64_t maxTimeout_{5000}; // max rpc timeout, ms
    bool isStop_{false};
    std::string errInfo_; // error info of client

    NetAddress::ptr localAddr_{nullptr};
    NetAddress::ptr peerAddr_{nullptr};
    EventLoop *loop_{nullptr};
    TcpConnection::ptr connection_{nullptr};

    AbstractCodeC::ptr codec_{nullptr};

    bool connectSucc_{false};

    std::function<CustomStruct::ptr()> getCustomData_;
};

}

#endif