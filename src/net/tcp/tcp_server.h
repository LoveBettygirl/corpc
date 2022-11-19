#ifndef CORPC_NET_TCP_TCP_SERVER_H
#define CORPC_NET_TCP_TCP_SERVER_H

#include <map>
#include <google/protobuf/service.h>
#include "event_loop.h"
#include "channel.h"
#include "timer.h"
#include "net_address.h"
#include "io_thread.h"
#include "timewheel.h"
#include "abstract_codec.h"
#include "abstract_dispatcher.h"
#include "http_dispatcher.h"
#include "http_servlet.h"
#include "tcp_connection.h"

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
    IOThreadPool::ptr ioPool_;
    ProtocolType protocolType_{Pb_Protocol};
    TcpTimeWheel::ptr timeWheel_;
    std::map<int, std::shared_ptr<TcpConnection>> clients_;
    TimerEvent::ptr clearClientTimerEvent_{nullptr};
};

}

#endif