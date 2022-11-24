#include <memory>
#include <future>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "corpc/net/net_address.h"
#include "corpc/net/tcp/io_thread.h"
#include "corpc/common/error_code.h"
#include "corpc/net/tcp/tcp_client.h"
#include "corpc/net/pb/pb_rpc_client_async_channel.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_codec.h"
#include "corpc/net/pb/pb_data.h"
#include "corpc/common/log.h"
#include "corpc/common/runtime.h"
#include "corpc/common/msg_seq.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/common/start.h"
#include "corpc/net/pb/pb_rpc_closure.h"

namespace corpc {

PbRpcClientAsyncChannel::PbRpcClientAsyncChannel(NetAddress::ptr addr)
{
    Coroutine::getCurrentCoroutine();
    rpcChannel_ = std::make_shared<PbRpcChannel>(addr);
    loop_ = corpc::EventLoop::getEventLoop();
    loop_->setEventLoopType(SubLoop);
    ioPool_ = std::make_shared<IOThreadPool>(1);
    ioPool_->start();
}

PbRpcClientAsyncChannel::~PbRpcClientAsyncChannel()
{
    if (currentCor_) {
        getCoroutinePool()->returnCoroutine(currentCor_);
    }
    if (pendingCor_) {
        getCoroutinePool()->returnCoroutine(pendingCor_);
    }
}

PbRpcChannel *PbRpcClientAsyncChannel::getRpcChannel()
{
    return rpcChannel_.get();
}

void PbRpcClientAsyncChannel::saveCallee(conPtr controller, msgPtr req, msgPtr res, cloPtr closure)
{
    std::shared_ptr<corpc::PbRpcClosure> clo = std::make_shared<corpc::PbRpcClosure>(std::bind(&PbRpcClientAsyncChannel::stop, this, closure.get())); 
    controller_ = controller;
    req_ = req;
    res_ = res;
    closure_ = clo;
    isPreSet_ = true;
}

void PbRpcClientAsyncChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
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
    currentCor_ = getCoroutinePool()->getCoroutineInstanse(); // 子协程
    getCurrentCoroutine()->setCallBack(std::bind(&PbRpcClientAsyncChannel::callMethod, this, method, controller, request, response, closure_.get()));
    loop_->addCoroutine(getCurrentCoroutine());
    loop_->loop();
}

void PbRpcClientAsyncChannel::callMethod(const google::protobuf::MethodDescriptor *method,
                                    google::protobuf::RpcController *controller,
                                    const google::protobuf::Message *request,
                                    google::protobuf::Message *response,
                                    google::protobuf::Closure *done)
{
    PbRpcController *rpcController = dynamic_cast<PbRpcController *>(controller);
    RunTime *runtime = getCurrentRunTime();
    if (runtime) {
        rpcController->setMsgSeq(runtime->msgNo_);
        LOG_INFO << "get from RunTime succ, msgno=" << runtime->msgNo_;
    }
    else {
        rpcController->setMsgSeq(MsgSeqUtil::genMsgNumber());
        LOG_INFO << "get from RunTime error, generate new msgno=" << rpcController->msgSeq();
    }

    std::shared_ptr<PbRpcClientAsyncChannel> sPtr = shared_from_this();

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
                Coroutine::resume(sPtr->getCurrentCoroutine().get());
            }
            sPtr.reset();
        };

        if (sPtr->getEventLoop()->isLooping()) {
            sPtr->getEventLoop()->addTask(callBack, true);
        }
        else {
            if (sPtr->getClosurePtr() != nullptr) {
                sPtr->getClosurePtr()->Run();
            }
            sPtr->setFinished(true);
        }
        sPtr.reset();
    };
    pendingCor_ = getIOThreadPool()->addCoroutineToRandomThread(cb, false);
    if (loop_) {
        loop_->stop();
    }
}

void PbRpcClientAsyncChannel::stop(google::protobuf::Closure *done)
{
    if (done) {
        done->Run();
    }
    if (loop_) {
        loop_->stop();
    }
}

IOThreadPool::ptr PbRpcClientAsyncChannel::getIOThreadPool()
{
    return ioPool_;
}

EventLoop *PbRpcClientAsyncChannel::getEventLoop()
{
    return loop_;
}

void PbRpcClientAsyncChannel::waitCor()
{
    needResume_ = true;
    if (isFinished_) { // rpc调用是否执行完，执行完就不用yield了
        return;
    }
    Coroutine::yield();
}

void PbRpcClientAsyncChannel::wait()
{
    if (!currentCor_) {
        return;
    }
    getCurrentCoroutine()->setCallBack(std::bind(&PbRpcClientAsyncChannel::waitCor, this));
    loop_->addCoroutine(getCurrentCoroutine());
    loop_->loop();
}

void PbRpcClientAsyncChannel::setFinished(bool value)
{
    isFinished_ = true;
}

Coroutine::ptr PbRpcClientAsyncChannel::getCurrentCoroutine()
{
    return currentCor_;
}

bool PbRpcClientAsyncChannel::getNeedResume()
{
    return needResume_;
}

google::protobuf::RpcController *PbRpcClientAsyncChannel::getControllerPtr()
{
    return controller_.get();
}

google::protobuf::Message *PbRpcClientAsyncChannel::getRequestPtr()
{
    return req_.get();
}

google::protobuf::Message *PbRpcClientAsyncChannel::getResponsePtr()
{
    return res_.get();
}

google::protobuf::Closure *PbRpcClientAsyncChannel::getClosurePtr()
{
    return closure_.get();
}

}