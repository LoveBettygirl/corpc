#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "corpc/net/net_address.h"
#include "corpc/common/error_code.h"
#include "corpc/net/tcp/tcp_client.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_codec.h"
#include "corpc/net/pb/pb_data.h"
#include "corpc/common/log.h"
#include "corpc/common/msg_seq.h"
#include "corpc/common/runtime.h"

namespace corpc {

PbRpcChannel::PbRpcChannel(NetAddress::ptr addr)
{
    addrs_.clear();
    addrs_.push_back(addr);
    originAddrs_ = addrs_;
    loadBalancer_ = LoadBalance::queryStrategy(LoadBalanceCategory::Random);
}

PbRpcChannel::PbRpcChannel(std::vector<NetAddress::ptr> addrs, LoadBalanceCategory loadBalance/* = LoadBalanceCategory::Random*/) : addrs_(addrs), originAddrs_(addrs)
{
    loadBalancer_ = LoadBalance::queryStrategy(loadBalance);
}

void PbRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                                google::protobuf::RpcController *controller,
                                const google::protobuf::Message *request,
                                google::protobuf::Message *response,
                                google::protobuf::Closure *done)
{
    PbStruct pbStruct;
    PbRpcController *rpcController = dynamic_cast<PbRpcController *>(controller);
    if (!rpcController) {
        LOG_ERROR << "call failed. falid to dynamic cast PbRpcController";
        if (done) {
            done->Run();
        }
        return;
    }

    pbStruct.serviceFullName = method->full_name();
    LOG_DEBUG << "call service full name = " << pbStruct.serviceFullName;
    if (!request->SerializeToString(&(pbStruct.pbData))) {
        LOG_ERROR << "serialize send package error";
        if (done) {
            done->Run();
        }
        return;
    }

    if (!rpcController->MsgSeq().empty()) {
        pbStruct.msgSeq = rpcController->MsgSeq();
    }
    else {
        // get current coroutine's msgno to set this request
        RunTime *runtime = getCurrentRunTime();
        if (runtime != nullptr && !runtime->msgNo_.empty()) {
            pbStruct.msgSeq = runtime->msgNo_;
            LOG_DEBUG << "get from RunTime succ, msgno = " << pbStruct.msgSeq;
        }
        else {
            pbStruct.msgSeq = MsgSeqUtil::genMsgNumber();
            LOG_DEBUG << "get from RunTime error, generate new msgno = " << pbStruct.msgSeq;
        }
        rpcController->SetMsgSeq(pbStruct.msgSeq);
    }

    int maxRetry = rpcController->MaxRetry();
    PbStruct::ptr resData;
    int64_t endCall = getNowMs() + rpcController->Timeout();

    // 重试大循环，最多的重复调用次数为 1 + maxRetry
    for (int retryTimes = 0; retryTimes <= maxRetry; retryTimes++) {
        if (addrs_.empty()) {
            addrs_ = originAddrs_;
            if (addrs_.empty()) {
                rpcController->SetError(ERROR_SERVICE_NOT_FOUND, "not found address of service");
                LOG_ERROR << pbStruct.msgSeq << "|call rpc occur client error, serviceFullName=" << pbStruct.serviceFullName << ", error_code="
                        << ERROR_SERVICE_NOT_FOUND << ", errorInfo = not found address of service";
                if (done) {
                    done->Run();
                }
                return;
            }
        }
        NetAddress::ptr addr = loadBalancer_->select(addrs_, pbStruct); // 负载均衡器的选择
        LOG_INFO << "service full name: " << pbStruct.serviceFullName << " server addr: " << addr->toString();
        TcpClient::ptr client = std::make_shared<TcpClient>(addr);
        rpcController->SetLocalAddr(client->getLocalAddr());
        rpcController->SetPeerAddr(client->getPeerAddr());

        AbstractCodeC::ptr codec = client->getConnection()->getCodec();
        codec->encode(client->getConnection()->getOutBuffer(), &pbStruct);
        if (!pbStruct.encodeSucc_) {
            rpcController->SetError(ERROR_FAILED_ENCODE, "encode pb data error");
            if (done) {
                done->Run();
            }
            return;
        }

        LOG_INFO << "============================================================";
        LOG_INFO << pbStruct.msgSeq << "|" << rpcController->PeerAddr()->toString()
                << "|. Set client send request data: " << request->ShortDebugString();
        if (retryTimes > 0) {
            LOG_INFO << "retry times: " << retryTimes;
        }
        LOG_INFO << "============================================================";

        int64_t restTime = endCall - getNowMs();
        client->setTimeout(restTime);

        int ret = client->sendAndRecvPb(pbStruct.msgSeq, resData); // 接收并解码服务端响应
        if (ret == 0) {
            break;
        }
        else if (ret != 0 && ret != ERROR_RPC_TIMEOUT) {
            auto it = addrs_.begin();
            for (; it != addrs_.end(); it++) {
                if ((*it)->toString() == addr->toString()) {
                    break;
                }
            }
            if (it != addrs_.end()) {
                addrs_.erase(it);
            }
            LOG_ERROR << pbStruct.msgSeq << "|call rpc occur client error, serviceFullName=" << pbStruct.serviceFullName << ", error_code="
                        << ret << ", errorInfo = " << client->getErrInfo() << ", to retry......";
            continue;
        }
        else {
            rpcController->SetError(ret, client->getErrInfo());
            LOG_ERROR << pbStruct.msgSeq << "|call rpc occur client error, serviceFullName=" << pbStruct.serviceFullName << ", error_code="
                        << ret << ", errorInfo = " << client->getErrInfo();
            if (done) {
                done->Run();
            }
            return;
        }
    }

    if (!response->ParseFromString(resData->pbData)) {
        rpcController->SetError(ERROR_FAILED_DESERIALIZE, "failed to deserialize data from server");
        LOG_ERROR << pbStruct.msgSeq << "|failed to deserialize data";
        if (done) {
            done->Run();
        }
        return;
    }

    // fix: rpc成功调用，结果返回错误，不能成为框架级错误
    if (resData->errCode != 0) {
        LOG_ERROR << pbStruct.msgSeq << "|server reply error_code=" << resData->errCode << ", errInfo=" << resData->errInfo;
        // rpcController->SetError(resData->errCode, resData->errInfo);
        if (done) {
            done->Run();
        }
        return;
    }

    LOG_INFO << "============================================================";
    LOG_INFO << pbStruct.msgSeq << "|" << rpcController->PeerAddr()->toString()
            << "|call rpc server [" << pbStruct.serviceFullName << "] succ"
            << ". Get server reply response data:" << response->ShortDebugString();
    LOG_INFO << "============================================================";

    // execute callback function
    if (done) {
        done->Run();
    }
}

}