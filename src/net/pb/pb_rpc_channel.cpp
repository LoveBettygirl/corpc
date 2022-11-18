#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "net_address.h"
#include "error_code.h"
#include "tcp_client.h"
#include "pb_rpc_channel.h"
#include "pb_rpc_controller.h"
#include "pb_codec.h"
#include "pb_data.h"
#include "log.h"
#include "msg_seq.h"
#include "runtime.h"

namespace corpc {

PbRpcChannel::PbRpcChannel(NetAddress::ptr addr) : addr_(addr)
{
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
        return;
    }

    TcpClient::ptr client = std::make_shared<TcpClient>(addr_);
    rpcController->setLocalAddr(client->getLocalAddr());
    rpcController->setPeerAddr(client->getPeerAddr());

    pbStruct.serviceFullName = method->full_name();
    LOG_DEBUG << "call service full name = " << pbStruct.serviceFullName;
    if (!request->SerializeToString(&(pbStruct.pbData))) {
        LOG_ERROR << "serialize send package error";
        return;
    }

    if (!rpcController->msgSeq().empty()) {
        pbStruct.msgSeq = rpcController->msgSeq();
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
        rpcController->setMsgSeq(pbStruct.msgSeq);
    }

    AbstractCodeC::ptr codec = client->getConnection()->getCodec();
    codec->encode(client->getConnection()->getOutBuffer(), &pbStruct);
    if (!pbStruct.encodeSucc_) {
        rpcController->setError(ERROR_FAILED_ENCODE, "encode pb data error");
        return;
    }

    LOG_INFO << "============================================================";
    LOG_INFO << pbStruct.msgSeq << "|" << rpcController->peerAddr()->toString()
            << "|. Set client send request data:" << request->ShortDebugString();
    LOG_INFO << "============================================================";
    client->setTimeout(rpcController->timeout());

    PbStruct::ptr resData;
    int ret = client->sendAndRecvPb(pbStruct.msgSeq, resData); // 接收并解码服务端响应
    if (ret != 0) {
        rpcController->setError(ret, client->getErrInfo());
        LOG_ERROR << pbStruct.msgSeq << "|call rpc occur client error, serviceFullName=" << pbStruct.serviceFullName << ", error_code="
                    << ret << ", errorInfo = " << client->getErrInfo();
        return;
    }

    if (!response->ParseFromString(resData->pbData)) {
        rpcController->setError(ERROR_FAILED_DESERIALIZE, "failed to deserialize data from server");
        LOG_ERROR << pbStruct.msgSeq << "|failed to deserialize data";
        return;
    }
    if (resData->errCode != 0) {
        LOG_ERROR << pbStruct.msgSeq << "|server reply error_code=" << resData->errCode << ", errInfo=" << resData->errInfo;
        rpcController->setError(resData->errCode, resData->errInfo);
        return;
    }

    LOG_INFO << "============================================================";
    LOG_INFO << pbStruct.msgSeq << "|" << rpcController->peerAddr()->toString()
            << "|call rpc server [" << pbStruct.serviceFullName << "] succ"
            << ". Get server reply response data:" << response->ShortDebugString();
    LOG_INFO << "============================================================";

    // execute callback function
    if (done) {
        done->Run();
    }
}

}