#ifndef CORPC_NET_TCP_TCP_SERVER_H
#define CORPC_NET_TCP_TCP_SERVER_H

#include <map>
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

    TcpServer(NetAddress::ptr addr, ProtocolType type = Pb_Protocol);
    ~TcpServer();
    void start();
    void stop();
    void addCoroutine(corpc::Coroutine::ptr cor);
    bool registerService(std::shared_ptr<google::protobuf::Service> service);
    bool registerHttpServlet(const std::string &urlPath, HttpServlet::ptr servlet);
    TcpConnection::ptr addClient(IOThread *ioThread, int fd);
    void freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot);

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
};

}

#endif