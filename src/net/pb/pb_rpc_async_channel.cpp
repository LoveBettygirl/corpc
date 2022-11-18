#include <memory>
#include <future>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "net_address.h"
#include "io_thread.h"
#include "error_code.h"
#include "tcp_client.h"
#include "pb_rpc_async_channel.h"
#include "pb_rpc_channel.h"
#include "pb_rpc_controller.h"
#include "pb_codec.h"
#include "pb_data.h"
#include "log.h"
#include "runtime.h"
#include "msg_seq.h"
#include "coroutine_pool.h"
#include "coroutine.h"
#include "start.h"

namespace corpc {

PbRpcAsyncChannel::PbRpcAsyncChannel(NetAddress::ptr addr)
{
    rpcChannel_ = std::make_shared<PbRpcChannel>(addr);
    currentIothread_ = IOThread::getCurrentIOThread();
    currentCor_ = Coroutine::getCurrentCoroutine();
}

PbRpcAsyncChannel::~PbRpcAsyncChannel()
{
    getCoroutinePool()->returnCoroutine(pendingCor_);
}

PbRpcChannel *PbRpcAsyncChannel::getRpcChannel()
{
    return rpcChannel_.get();
}

void PbRpcAsyncChannel::saveCallee(conPtr controller, msgPtr req, msgPtr res, cloPtr closure)
{
    controller_ = controller;
    req_ = req;
    res_ = res;
    closure_ = closure;
    isPreSet_ = true;
}

void PbRpcAsyncChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                                    google::protobuf::RpcController *controller,
                                    const google::protobuf::Message *request,
                                    google::protobuf::Message *response,
                                    google::protobuf::Closure *done)
{

    PbRpcController *rpcController = dynamic_cast<PbRpcController *>(controller);
    if (!isPreSet_) {
        LOG_ERROR << "Error! must call [saveCallee()] function before [CallMethod()]";
        PbRpcController *rpcController = dynamic_cast<PbRpcController *>(controller);
        rpcController->setError(ERROR_NOT_SET_ASYNC_PRE_CALL, "Error! must call [saveCallee()] function before [CallMethod()];");
        isFinished_ = true;
        return;
    }
    RunTime *runtime = getCurrentRunTime();
    if (runtime) {
        rpcController->setMsgSeq(runtime->msgNo_);
        LOG_INFO << "get from RunTime succ, msgno=" << runtime->msgNo_;
    }
    else {
        rpcController->setMsgSeq(MsgSeqUtil::genMsgNumber());
        LOG_INFO << "get from RunTime error, generate new msgno=" << rpcController->msgSeq();
    }

    std::shared_ptr<PbRpcAsyncChannel> sPtr = shared_from_this();

    // cb将在随机分配的新io线程中的子协程中执行
    auto cb = [sPtr, method]() mutable {
        LOG_INFO << "now execute rpc call method by this thread";
        sPtr->getRpcChannel()->CallMethod(method, sPtr->getControllerPtr(), sPtr->getRequestPtr(), sPtr->getResponsePtr(), nullptr);

        LOG_INFO << "execute rpc call method by this thread finish";

        // callBack在之前的主io线程中的主协程中执行
        auto callBack = [sPtr]() mutable {
            LOG_INFO << "async execute rpc call method back old thread";
            // callback function execute in origin thread
            if (sPtr->getClosurePtr() != nullptr) {
                sPtr->getClosurePtr()->Run();
            }
            sPtr->setFinished(true);

            // 如果客户端调用了wait()，说明是需要resume到子协程的
            if (sPtr->getNeedResume()) {
                LOG_INFO << "async execute rpc call method back old thread, need resume";
                Coroutine::resume(sPtr->getCurrentCoroutine());
            }
            sPtr.reset();
        };

        sPtr->getIOThread()->getEventLoop()->addTask(callBack, true);
        sPtr.reset();
    };
    pendingCor_ = getServer()->getIOThreadPool()->addCoroutineToRandomThread(cb, false);
}

void PbRpcAsyncChannel::wait()
{
    needResume_ = true;
    if (isFinished_) { // rpc调用是否执行完，执行完就不用yield了
        return;
    }
    Coroutine::yield();
}

void PbRpcAsyncChannel::setFinished(bool value)
{
    isFinished_ = true;
}

IOThread *PbRpcAsyncChannel::getIOThread()
{
    return currentIothread_;
}

Coroutine *PbRpcAsyncChannel::getCurrentCoroutine()
{
    return currentCor_;
}

bool PbRpcAsyncChannel::getNeedResume()
{
    return needResume_;
}

google::protobuf::RpcController *PbRpcAsyncChannel::getControllerPtr()
{
    return controller_.get();
}

google::protobuf::Message *PbRpcAsyncChannel::getRequestPtr()
{
    return req_.get();
}

google::protobuf::Message *PbRpcAsyncChannel::getResponsePtr()
{
    return res_.get();
}

google::protobuf::Closure *PbRpcAsyncChannel::getClosurePtr()
{
    return closure_.get();
}

}