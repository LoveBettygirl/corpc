#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <memory>
#include <vector>
#include <queue>
#include "log.h"
#include "channel.h"
#include "eventloop.h"
#include "tcp_buffer.h"
#include "coroutine.h"
#include "iothread.h"
#include "timewheel.h"
#include "abstractslot.h"
#include "netaddress.h"
#include "mutex.h"
#include "abstract_codec.h"
#include "http_request.h"

namespace corpc {

class TcpServer;
class TcpClient;
class IOThread;

enum TcpConnectionState {
    NotConnected = 1, // can do io
    Connected = 2,    // can do io
    HalfClosing = 3,  // server call shutdown, write half close. can read,but can't write
    Closed = 4,       // can't do io
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    typedef std::shared_ptr<TcpConnection> ptr;

    TcpConnection(corpc::TcpServer *tcpServer, corpc::IOThread *ioThread, int fd, int buffSize, NetAddress::ptr peerAddr);
    TcpConnection(corpc::TcpClient *tcpClient, corpc::EventLoop *loop, int fd, int buffSize, NetAddress::ptr peerAddr);

    void setUpClient();
    void setUpServer();

    ~TcpConnection();

    void initBuffer(int size);

    enum ConnectionType {
        ServerConnection = 1, // owned by TcpServer
        ClientConnection = 2, // owned by TcpClient
    };

public:
    void shutdownConnection();
    TcpConnectionState getState();
    void setState(const TcpConnectionState &state);
    TcpBuffer *getInBuffer();
    TcpBuffer *getOutBuffer();
    AbstractCodeC::ptr getCodec() const;
    bool getResPackageData(const std::string &msgReq, PbStruct::ptr &pbStruct);
    void registerToTimeWheel();
    Coroutine::ptr getCoroutine();

public:
    void mainServerLoopCorFunc();
    void input();
    void execute();
    void output();
    void setOverTimeFlag(bool value);
    bool getOverTimerFlag();
    void initServer();

private:
    void clearClient();

private:
    TcpServer *tcpServer_{nullptr};
    TcpClient *tcpClient_{nullptr};
    IOThread *ioThread_{nullptr};
    EventLoop *loop_{nullptr};

    int fd_{-1};
    TcpConnectionState state_{TcpConnectionState::Connected};
    ConnectionType connectionType_{ServerConnection};

    NetAddress::ptr peerAddr_;

    TcpBuffer::ptr readBuffer_;
    TcpBuffer::ptr writeBuffer_;

    Coroutine::ptr loopCor_;

    AbstractCodeC::ptr codec_;

    Channel::ptr channel_;

    bool stop_{false};
    bool isOverTime_{false};

    std::map<std::string, std::shared_ptr<PbStruct>> replyDatas_;

    std::weak_ptr<AbstractSlot<TcpConnection>> weakSlot_;

    RWMutex mutex_;
};

}

#endif