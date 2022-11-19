#ifndef CORPC_NET_PB_PB_RPC_ASYNC_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_ASYNC_CHANNEL_H

#include <google/protobuf/service.h>
#include <future>
#include "pb_data.h"
#include "pb_rpc_channel.h"
#include "pb_rpc_controller.h"
#include "net_address.h"
#include "tcp_client.h"
#include "coroutine.h"

namespace corpc {

class PbRpcAsyncChannel : public google::protobuf::RpcChannel, public std::enable_shared_from_this<PbRpcAsyncChannel> {

public:
    typedef std::shared_ptr<PbRpcAsyncChannel> ptr;
    typedef std::shared_ptr<google::protobuf::RpcController> conPtr;
    typedef std::shared_ptr<google::protobuf::Message> msgPtr;
    typedef std::shared_ptr<google::protobuf::Closure> cloPtr;

    PbRpcAsyncChannel(NetAddress::ptr addr);
    ~PbRpcAsyncChannel();

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

    IOThread *getIOThread();

    Coroutine *getCurrentCoroutine();

    google::protobuf::RpcController *getControllerPtr();

    google::protobuf::Message *getRequestPtr();

    google::protobuf::Message *getResponsePtr();

    google::protobuf::Closure *getClosurePtr();

private:
    PbRpcChannel::ptr rpcChannel_;
    Coroutine::ptr pendingCor_;
    Coroutine *currentCor_{nullptr};
    IOThread *currentIothread_{nullptr};
    bool isFinished_{false};
    bool needResume_{false};
    bool isPreSet_{false};

private:
    conPtr controller_;
    msgPtr req_;
    msgPtr res_;
    cloPtr closure_;
};

}

#endif