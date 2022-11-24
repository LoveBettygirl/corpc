#ifndef CORPC_NET_PB_PB_RPC_CLIENT_ASYNC_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_CLIENT_ASYNC_CHANNEL_H

#include <google/protobuf/service.h>
#include <future>
#include "corpc/net/pb/pb_data.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/net_address.h"
#include "corpc/net/tcp/tcp_client.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/net/event_loop.h"

namespace corpc {

class PbRpcClientAsyncChannel : public google::protobuf::RpcChannel, public std::enable_shared_from_this<PbRpcClientAsyncChannel> {
public:
    typedef std::shared_ptr<PbRpcClientAsyncChannel> ptr;
    typedef std::shared_ptr<google::protobuf::RpcController> conPtr;
    typedef std::shared_ptr<google::protobuf::Message> msgPtr;
    typedef std::shared_ptr<google::protobuf::Closure> cloPtr;

    PbRpcClientAsyncChannel(NetAddress::ptr addr);
    ~PbRpcClientAsyncChannel();

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);

    PbRpcChannel *getRpcChannel();

    // must call saveCallee before CallMethod
    // in order to save shared_ptr count of req res controller
    void saveCallee(conPtr controller, msgPtr req, msgPtr res, cloPtr closure);

    void wait();

    void setFinished(bool value);

    bool getNeedResume();

    Coroutine::ptr getCurrentCoroutine();

    google::protobuf::RpcController *getControllerPtr();

    google::protobuf::Message *getRequestPtr();

    google::protobuf::Message *getResponsePtr();

    google::protobuf::Closure *getClosurePtr();

    EventLoop *getEventLoop();

    IOThreadPool::ptr getIOThreadPool();

private:
    PbRpcChannel::ptr rpcChannel_;
    Coroutine::ptr pendingCor_;
    Coroutine::ptr currentCor_{nullptr};
    bool isFinished_{false};
    bool needResume_{false};
    bool isPreSet_{false};
    IOThreadPool::ptr ioPool_;
    EventLoop *loop_{nullptr};

private:
    conPtr controller_;
    msgPtr req_;
    msgPtr res_;
    cloPtr closure_;

private:
    void callMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);
    void stop(google::protobuf::Closure *done);
    void waitCor();
};

}

#endif