#ifndef CORPC_NET_TCP_TCP_CONNECTION_H
#define CORPC_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include <vector>
#include <queue>
#include <functional>
#include "corpc/common/log.h"
#include "corpc/net/channel.h"
#include "corpc/net/event_loop.h"
#include "corpc/net/tcp/tcp_buffer.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/net/tcp/io_thread.h"
#include "corpc/net/tcp/timewheel.h"
#include "corpc/net/tcp/abstract_slot.h"
#include "corpc/net/net_address.h"
#include "corpc/net/mutex.h"
#include "corpc/net/abstract_codec.h"
#include "corpc/net/http/http_request.h"
#include "corpc/net/pb/pb_codec.h"
#include "corpc/net/custom/custom_data.h"
#include "corpc/net/custom/custom_codec.h"

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
    using ConnectionCallback = std::function<void(const TcpConnection::ptr&)>;

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
    bool getResPackageData(const std::string &msgSeq, PbStruct::ptr &pbStruct);
    bool getResPackageData(CustomStruct::ptr &customStruct);
    void registerToTimeWheel();
    Coroutine::ptr getCoroutine();
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setCustomCodeC(CustomCodeC::ptr codec) { codec_ = codec; }
    void setCustomData(std::function<CustomStruct::ptr()> func) { getCustomData_ = func; }

public:
    void mainServerLoopCorFunc();
    void input();
    void execute();
    void output();
    void setOverTimeFlag(bool value);
    bool getOverTimerFlag();
    void initServer();
    void sendInCor(const std::string &data);
    void sendInCor(const char *buf, int size);
    void send(const std::string &data);
    void send(const char *buf, int size);

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
    std::queue<std::shared_ptr<CustomStruct>> replyCustomDatas_;

    std::weak_ptr<AbstractSlot<TcpConnection>> weakSlot_;

    RWMutex mutex_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    std::function<CustomStruct::ptr()> getCustomData_;
};

}

#endif