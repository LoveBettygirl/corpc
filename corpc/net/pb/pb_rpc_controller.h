#ifndef CORPC_NET_PB_PB_RPC_CONRTOLLER_H
#define CORPC_NET_PB_PB_RPC_CONRTOLLER_H

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <cstdio>
#include <memory>
#include "corpc/net/net_address.h"

namespace corpc {

class PbRpcController : public google::protobuf::RpcController {

public:
    typedef std::shared_ptr<PbRpcController> ptr;
    // Client-side methods ---------------------------------------------

    PbRpcController() = default;
    ~PbRpcController() = default;

    void Reset() override;
    bool Failed() const override;

    // Server-side methods ---------------------------------------------

    std::string ErrorText() const override;
    void StartCancel() override;
    void SetFailed(const std::string &reason) override;
    bool IsCanceled() const override;
    void NotifyOnCancel(google::protobuf::Closure *callback) override;

    // common methods

    int ErrorCode() const;
    void SetErrorCode(const int errorCode);
    const std::string &MsgSeq() const;
    void SetMsgSeq(const std::string &msgSeq);
    void SetError(const int errCode, const std::string &errInfo);
    void SetPeerAddr(NetAddress::ptr addr);
    void SetLocalAddr(NetAddress::ptr addr);

    NetAddress::ptr PeerAddr();
    NetAddress::ptr LocalAddr();

    void SetTimeout(const int timeout);
    int Timeout() const;

    void SetMethodName(const std::string &name);
    std::string GetMethodName();

    void SetMethodFullName(const std::string &name);
    std::string GetMethodFullName();

private:
    int errorCode_{0};      // errorCode, identify one specific error
    std::string errorInfo_; // errorInfo, details description of error
    std::string msgSeq_;    // msgSeq, identify once rpc request and response
    bool isFailed_{false};
    bool isCanceled_{false};
    NetAddress::ptr peerAddr_;
    NetAddress::ptr localAddr_;

    int timeout_{5000};       // max call rpc timeout
    std::string methodName_; // method name
    std::string fullName_;   // full name, like server.method_name
};

}

#endif