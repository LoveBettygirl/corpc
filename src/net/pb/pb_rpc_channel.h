#ifndef CORPC_NET_PB_PB_RPC_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_CHANNEL_H

#include <memory>
#include <google/protobuf/service.h>
#include "net_address.h"
#include "tcp_client.h"

namespace corpc {

class PbRpcChannel : public google::protobuf::RpcChannel {
public:
    typedef std::shared_ptr<PbRpcChannel> ptr;
    PbRpcChannel(NetAddress::ptr addr);
    ~PbRpcChannel() = default;

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);

private:
    NetAddress::ptr addr_;
};

}

#endif