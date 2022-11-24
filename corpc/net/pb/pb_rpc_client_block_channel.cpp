#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "corpc/net/net_address.h"
#include "corpc/net/pb/pb_rpc_client_block_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_codec.h"
#include "corpc/net/pb/pb_data.h"
#include "corpc/common/log.h"
#include "corpc/common/msg_seq.h"
#include "corpc/common/runtime.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/net/pb/pb_rpc_closure.h"

namespace corpc {

PbRpcClientBlockChannel::PbRpcClientBlockChannel(NetAddress::ptr addr)
{
    Coroutine::getCurrentCoroutine();
    rpcChannel_ = std::make_shared<PbRpcChannel>(addr);
    loop_ = corpc::EventLoop::getEventLoop();
    loop_->setEventLoopType(SubLoop);
}

PbRpcClientBlockChannel::~PbRpcClientBlockChannel()
{
    getCoroutinePool()->returnCoroutine(cor_);
}

void PbRpcClientBlockChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                                google::protobuf::RpcController *controller,
                                const google::protobuf::Message *request,
                                google::protobuf::Message *response,
                                google::protobuf::Closure *done)
{
    std::shared_ptr<corpc::PbRpcClosure> closure = std::make_shared<corpc::PbRpcClosure>(std::bind(&PbRpcClientBlockChannel::stop, this, done)); 
    cor_ = getCoroutinePool()->getCoroutineInstanse(); // 子协程
    cor_->setCallBack(std::bind(&PbRpcChannel::CallMethod, rpcChannel_.get(), method, controller, request, response, closure.get()));
    loop_->addCoroutine(cor_);
    loop_->loop();
}

void PbRpcClientBlockChannel::stop(google::protobuf::Closure *done)
{
    if (done) {
        done->Run();
    }
    if (loop_) {
        loop_->stop();
    }
}

}