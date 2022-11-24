#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "corpc/net/net_address.h"
#include "corpc/net/pb/pb_rpc_client_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_codec.h"
#include "corpc/net/pb/pb_data.h"
#include "corpc/common/log.h"
#include "corpc/common/msg_seq.h"
#include "corpc/common/runtime.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/net/pb/pb_rpc_closure.h"

namespace corpc {

PbRpcClientChannel::PbRpcClientChannel(NetAddress::ptr addr)
{
    int ret = sem_init(&waitSemaphore_, 0, 0);
    assert(ret == 0);
    rpcChannel_ = std::make_shared<PbRpcChannel>(addr);
    ioPool_ = std::make_shared<IOThreadPool>(1);
    ioPool_->start();
}

void PbRpcClientChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                                google::protobuf::RpcController *controller,
                                const google::protobuf::Message *request,
                                google::protobuf::Message *response,
                                google::protobuf::Closure *done)
{
    std::shared_ptr<corpc::PbRpcClosure> closure = std::make_shared<corpc::PbRpcClosure>(std::bind(&PbRpcClientChannel::stop, this, done));
    Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse(); // 子协程
    IOThread *ioThread = ioPool_->getIOThread();
    cor->setCallBack(std::bind(&PbRpcChannel::CallMethod, rpcChannel_.get(), method, controller, request, response, done));
    ioThread->getEventLoop()->addCoroutine(cor);
}

void PbRpcClientChannel::wait()
{
    sem_wait(&waitSemaphore_);
}

void PbRpcClientChannel::stop(google::protobuf::Closure *done)
{
    if (done) {
        done->Run();
    }
    sem_post(&waitSemaphore_);
}

}