#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include "pb_rpc_controller.h"

namespace corpc {

void PbRpcController::Reset() {}

bool PbRpcController::Failed() const
{
    return isFailed_;
}

std::string PbRpcController::ErrorText() const
{
    return errorInfo_;
}

void PbRpcController::StartCancel() {}

void PbRpcController::SetFailed(const std::string &reason)
{
    isFailed_ = true;
    errorInfo_ = reason;
}

bool PbRpcController::IsCanceled() const
{
    return false;
}

void PbRpcController::NotifyOnCancel(google::protobuf::Closure *callback)
{
}

void PbRpcController::setErrorCode(const int error_code)
{
    errorCode_ = error_code;
}

int PbRpcController::errorCode() const
{
    return errorCode_;
}

const std::string &PbRpcController::msgSeq() const
{
    return msgReq_;
}

void PbRpcController::setMsgReq(const std::string &msgReq)
{
    msgReq_ = msgReq;
}

void PbRpcController::setError(const int errCode, const std::string &errInfo)
{
    SetFailed(errInfo);
    setErrorCode(errCode);
}

void PbRpcController::setPeerAddr(NetAddress::ptr addr)
{
    peerAddr_ = addr;
}

void PbRpcController::setLocalAddr(NetAddress::ptr addr)
{
    localAddr_ = addr;
}

NetAddress::ptr PbRpcController::peerAddr()
{
    return peerAddr_;
}

NetAddress::ptr PbRpcController::localAddr()
{
    return localAddr_;
}

void PbRpcController::setTimeout(const int timeout)
{
    timeout_ = timeout;
}

int PbRpcController::timeout() const
{
    return timeout_;
}

void PbRpcController::setMethodName(const std::string &name)
{
    methodName_ = name;
}

std::string PbRpcController::getMethodName()
{
    return methodName_;
}

void PbRpcController::setMethodFullName(const std::string &name)
{
    fullName_ = name;
}

std::string PbRpcController::getMethodFullName()
{
    return fullName_;
}

}