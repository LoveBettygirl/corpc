#ifndef CORPC_NET_PB_PB_RPC_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_CHANNEL_H

#include <memory>
#include <vector>
#include <google/protobuf/service.h>
#include "corpc/net/net_address.h"
#include "corpc/net/tcp/tcp_client.h"
#include "corpc/net/load_balance.h"

namespace corpc {

class PbRpcChannel : public google::protobuf::RpcChannel {
public:
    typedef std::shared_ptr<PbRpcChannel> ptr;
    PbRpcChannel(NetAddress::ptr addr);
    PbRpcChannel(std::vector<NetAddress::ptr> addrs, LoadBalanceCategory loadBalance = LoadBalanceCategory::Random);
    ~PbRpcChannel() = default;

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);

private:
    std::vector<NetAddress::ptr> addrs_;
    std::vector<NetAddress::ptr> originAddrs_;
    LoadBalanceStrategy::ptr loadBalancer_;
};

}

#endif