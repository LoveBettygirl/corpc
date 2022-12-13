#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include "corpc/net/pb/pb_rpc_controller.h"

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

void PbRpcController::SetErrorCode(const int error_code)
{
    errorCode_ = error_code;
}

int PbRpcController::ErrorCode() const
{
    return errorCode_;
}

const std::string &PbRpcController::MsgSeq() const
{
    return msgSeq_;
}

void PbRpcController::SetMsgSeq(const std::string &msgSeq)
{
    msgSeq_ = msgSeq;
}

void PbRpcController::SetError(const int errCode, const std::string &errInfo)
{
    SetFailed(errInfo);
    SetErrorCode(errCode);
}

void PbRpcController::SetPeerAddr(NetAddress::ptr addr)
{
    peerAddr_ = addr;
}

void PbRpcController::SetLocalAddr(NetAddress::ptr addr)
{
    localAddr_ = addr;
}

NetAddress::ptr PbRpcController::PeerAddr()
{
    return peerAddr_;
}

NetAddress::ptr PbRpcController::LocalAddr()
{
    return localAddr_;
}

void PbRpcController::SetTimeout(const int timeout)
{
    timeout_ = timeout;
}

int PbRpcController::Timeout() const
{
    return timeout_;
}

void PbRpcController::SetMaxRetry(const int maxRetry)
{
    maxRetry_ = maxRetry;
}

int PbRpcController::MaxRetry() const
{
    return maxRetry_;
}

void PbRpcController::SetMethodName(const std::string &name)
{
    methodName_ = name;
}

std::string PbRpcController::GetMethodName()
{
    return methodName_;
}

void PbRpcController::SetMethodFullName(const std::string &name)
{
    fullName_ = name;
}

std::string PbRpcController::GetMethodFullName()
{
    return fullName_;
}

}