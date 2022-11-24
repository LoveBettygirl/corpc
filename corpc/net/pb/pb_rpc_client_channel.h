#ifndef CORPC_NET_PB_PB_RPC_CLIENT_CHANNEL_H
#define CORPC_NET_PB_PB_RPC_CLIENT_CHANNEL_H

#include <memory>
#include <google/protobuf/service.h>
#include "corpc/net/net_address.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/tcp/io_thread.h"
#include <semaphore.h>

namespace corpc {

class PbRpcClientChannel : public google::protobuf::RpcChannel {
public:
    typedef std::shared_ptr<PbRpcClientChannel> ptr;
    PbRpcClientChannel(NetAddress::ptr addr);
    ~PbRpcClientChannel() = default;

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);

    void wait();

private:
    PbRpcChannel::ptr rpcChannel_;
    IOThreadPool::ptr ioPool_;
    sem_t waitSemaphore_;

private:
    void stop(google::protobuf::Closure *done);
};

}

#endif