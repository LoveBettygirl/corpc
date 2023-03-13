#ifndef CORPC_NET_TCP_TCP_SERVER_H
#define CORPC_NET_TCP_TCP_SERVER_H

#include <map>
#include <functional>
#include <google/protobuf/service.h>
#include "corpc/net/event_loop.h"
#include "corpc/net/channel.h"
#include "corpc/net/timer.h"
#include "corpc/net/net_address.h"
#include "corpc/net/tcp/io_thread.h"
#include "corpc/net/tcp/timewheel.h"
#include "corpc/net/abstract_codec.h"
#include "corpc/net/abstract_dispatcher.h"
#include "corpc/net/http/http_dispatcher.h"
#include "corpc/net/http/http_servlet.h"
#include "corpc/net/tcp/tcp_connection.h"
#include "corpc/net/abstract_service_register.h"
#include "corpc/net/custom/custom_service.h"
#include "corpc/net/custom/custom_codec.h"
#include "corpc/net/custom/custom_dispatcher.h"

namespace corpc {

class TcpAcceptor {
public:
    typedef std::shared_ptr<TcpAcceptor> ptr;

    TcpAcceptor(NetAddress::ptr netAddr);
    void init();
    int toAccept();
    ~TcpAcceptor();

    NetAddress::ptr getPeerAddr() { return peerAddr_; }
    NetAddress::ptr geLocalAddr() { return localAddr_; }

private:
    int family_{-1};
    int listenfd_{-1};

    NetAddress::ptr localAddr_{nullptr};
    NetAddress::ptr peerAddr_{nullptr};
};

class TcpServer {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    using ConnectionCallback = std::function<void(const TcpConnection::ptr&)>;

    TcpServer(NetAddress::ptr addr, ProtocolType protocolType = Pb_Protocol);
    ~TcpServer();
    void start();
    void stop();
    void addCoroutine(corpc::Coroutine::ptr cor);
    bool registerService(std::shared_ptr<google::protobuf::Service> service);
    bool registerHttpServlet(const std::string &urlPath, HttpServlet::ptr servlet);
    bool registerService(std::shared_ptr<CustomService> service);
    TcpConnection::ptr addClient(IOThread *ioThread, int fd);
    void freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot);
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setCustomCodeC(CustomCodeC::ptr codec);
    void setCustomDispatcher(CustomDispatcher::ptr dispatcher);
    void setCustomData(std::function<CustomStruct::ptr()> func) { getCustomData_ = func; }
    std::function<CustomStruct::ptr()> getCustomData() { return getCustomData_; }

public:
    AbstractDispatcher::ptr getDispatcher();
    AbstractCodeC::ptr getCodec();
    NetAddress::ptr getPeerAddr();
    NetAddress::ptr getLocalAddr();
    IOThreadPool::ptr getIOThreadPool();
    TcpTimeWheel::ptr getTimeWheel();

private:
    void mainAcceptCorFunc();
    void clearClientTimerFunc();

private:
    NetAddress::ptr addr_;
    TcpAcceptor::ptr acceptor_;
    int tcpCounts_{0};
    EventLoop *mainLoop_{nullptr};
    bool isStopAccept_{false};
    Coroutine::ptr acceptCor_;
    AbstractDispatcher::ptr dispatcher_;
    AbstractCodeC::ptr codec_;
    AbstractServiceRegister::ptr register_;
    IOThreadPool::ptr ioPool_;
    ProtocolType protocolType_{Pb_Protocol};
    TcpTimeWheel::ptr timeWheel_;
    std::map<int, std::shared_ptr<TcpConnection>> clients_;
    TimerEvent::ptr clearClientTimerEvent_{nullptr};
    ConnectionCallback connectionCallback_; // 有新连接时的回调
    std::function<CustomStruct::ptr()> getCustomData_;
};

}

#endif