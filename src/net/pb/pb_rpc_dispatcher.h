#ifndef PB_RPC_DISPATCHER_H
#define PB_RPC_DISPATCHER_H

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <map>
#include <memory>

#include "abstract_dispatcher.h"
#include "pb_data.h"

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