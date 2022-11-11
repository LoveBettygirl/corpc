#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "error_code.h"
#include "pb_data.h"
#include "pb_rpc_dispatcher.h"
#include "pb_codec.h"
#include "msg_req.h"

namespace corpc {

class TcpBuffer;

void PbRpcDispacther::dispatch(AbstractData *data, TcpConnection *conn)
{
    PbStruct *temp = dynamic_cast<PbStruct *>(data);

    if (temp == nullptr) {
        LOG_ERROR << "dynamic_cast error";
        return;
    }
    Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_ = temp->msgReq;
    setCurrentRunTime(Coroutine::getCurrentCoroutine()->getRunTime());

    LOG_INFO << "begin to dispatch client tinypb request, msgno=" << temp->msgReq;

    std::string serviceName;
    std::string methodName;

    PbStruct replyPk;
    replyPk.serviceFullName = temp->serviceFullName;
    replyPk.msgReq = temp->msgReq;
    if (replyPk.msgReq.empty()) {
        replyPk.msgReq = MsgReqUtil::genMsgNumber();
    }

    if (!parseServiceFullName(temp->serviceFullName, serviceName, methodName)) {
        LOG_ERROR << replyPk.msgReq << "|parse service full name " << temp->serviceFullName << "error";

        replyPk.errCode = ERROR_PARSE_SERVICE_FULL_NAME;
        std::stringstream ss;
        ss << "cannot parse service full name:[" << temp->serviceFullName << "]";
        replyPk.errInfo = ss.str();
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&replyPk));
        return;
    }

    Coroutine::getCurrentCoroutine()->getRunTime()->interfaceName_ = temp->serviceFullName;
    auto it = serviceMap.find(serviceName);
    if (it == serviceMap.end() || !(it->second)) {
        replyPk.errCode = ERROR_SERVICE_NOT_FOUND;
        std::stringstream ss;
        ss << "not found service name:[" << serviceName << "]";
        LOG_ERROR << replyPk.msgReq << "|" << ss.str();
        replyPk.errInfo = ss.str();

        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&replyPk));

        LOG_INFO << "end dispatch client pb request, msgno=" << temp->msgReq;
        return;
    }

    servicePtr service = it->second;

    const google::protobuf::MethodDescriptor *method = service->GetDescriptor()->FindMethodByName(methodName);
    if (!method) {
        replyPk.errCode = ERROR_METHOD_NOT_FOUND;
        std::stringstream ss;
        ss << "not found method name:[" << methodName << "]";
        LOG_ERROR << replyPk.msgReq << "|" << ss.str();
        replyPk.errInfo = ss.str();
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&replyPk));
        return;
    }

    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    LOG_DEBUG << replyPk.msgReq << "|request.name = " << request->GetDescriptor()->full_name();

    if (!request->ParseFromString(temp->pbData)) {
        replyPk.errCode = ERROR_FAILED_SERIALIZE;
        std::stringstream ss;
        ss << "faild to parse request data, request.name:[" << request->GetDescriptor()->full_name() << "]";
        replyPk.errInfo = ss.str();
        LOG_ERROR << replyPk.msgReq << "|" << ss.str();
        delete request;
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&replyPk));
        return;
    }

    LOG_INFO << "============================================================";
    LOG_INFO << replyPk.msgReq << "|Get client request data:" << request->ShortDebugString();
    LOG_INFO << "============================================================";

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    LOG_DEBUG << replyPk.msgReq << "|response.name = " << response->GetDescriptor()->full_name();

    PbRpcController rpcController;
    rpcController.SetMsgReq(replyPk.msgReq);
    rpcController.SetMethodName(methodName);
    rpcController.SetMethodFullName(temp->serviceFullName);

    std::function<void()> replyPackageFunc = []() {};

    PbRpcClosure closure(replyPackageFunc);
    service->CallMethod(method, &rpcController, request, response, &closure);

    LOG_INFO << "Call [" << replyPk.serviceFullName << "] succ, now send reply package";

    if (!(response->SerializeToString(&(replyPk.pbData)))) {
        replyPk.pbData = "";
        LOG_ERROR << replyPk.msgReq << "|reply error! encode reply package error";
        replyPk.errCode = ERROR_FAILED_SERIALIZE;
        replyPk.errInfo = "failed to serilize relpy data";
    }
    else {
        LOG_INFO << "============================================================";
        LOG_INFO << replyPk.msgReq << "|Set server response data:" << response->ShortDebugString();
        LOG_INFO << "============================================================";
    }

    delete request;
    delete response;

    conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&replyPk));
}

bool PbRpcDispacther::parseServiceFullName(const std::string &fullName, std::string &serviceName, std::string &methodName)
{
    // serviceName.methodName
    if (fullName.empty()) {
        LOG_ERROR << "fullName of service empty";
        return false;
    }
    std::size_t i = fullName.find(".");
    if (i == fullName.npos) {
        LOG_ERROR << "not found [.]";
        return false;
    }

    serviceName = fullName.substr(0, i);
    LOG_DEBUG << "serviceName = " << serviceName;
    methodName = fullName.substr(i + 1, fullName.size() - i - 1);
    LOG_DEBUG << "methodName = " << methodName;

    return true;
}

void PbRpcDispacther::registerService(servicePtr service)
{
    std::string serviceName = service->GetDescriptor()->full_name();
    serviceMap_[serviceName] = service;
    LOG_INFO << "succ register service[" << serviceName << "]!";
}

}