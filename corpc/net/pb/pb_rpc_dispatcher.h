#ifndef CORPC_NET_PB_PB_RPC_DISPATCHER_H
#define CORPC_NET_PB_PB_RPC_DISPATCHER_H

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <map>
#include <memory>

#include "corpc/net/abstract_dispatcher.h"
#include "corpc/net/pb/pb_data.h"

namespace corpc {

class PbRpcDispacther : public AbstractDispatcher {
public:
    typedef std::shared_ptr<google::protobuf::Service> servicePtr;

    PbRpcDispacther() = default;
    ~PbRpcDispacther() = default;

    void dispatch(AbstractData *data, TcpConnection *conn) override;
    bool parseServiceFullName(const std::string &fullName, std::string &serviceName, std::string &methodName);
    void registerService(servicePtr service);

public:
    // all services should be registerd on there before progress start
    // key: service_name
    std::map<std::string, servicePtr> serviceMap_;
};

}

#endif