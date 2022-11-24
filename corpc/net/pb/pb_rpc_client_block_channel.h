#ifndef CORPC_NET_PB_PB_RPC_CLIENT_BLOCK_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_CLIENT_BLOCK_CHANNEL_H

#include <memory>
#include <google/protobuf/service.h>
#include "corpc/net/net_address.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/event_loop.h"
#include "corpc/coroutine/coroutine.h"

namespace corpc {

class PbRpcClientBlockChannel : public google::protobuf::RpcChannel {
public:
    typedef std::shared_ptr<PbRpcClientBlockChannel> ptr;
    PbRpcClientBlockChannel(NetAddress::ptr addr);
    ~PbRpcClientBlockChannel();

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);

private:
    PbRpcChannel::ptr rpcChannel_;
    EventLoop *loop_{nullptr};
    Coroutine::ptr cor_;

private:
    void stop(google::protobuf::Closure *done);
};

}

#endif