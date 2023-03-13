#ifndef CORPC_NET_ABSTRACT_SERVICE_REGISTER_H
#define CORPC_NET_ABSTRACT_SERVICE_REGISTER_H

#include <memory>
#include <string>
#include <vector>
#include <google/protobuf/service.h>
#include "corpc/net/net_address.h"
#include "corpc/net/custom/custom_service.h"

namespace corpc {

class AbstractServiceRegister {
public:
    typedef std::shared_ptr<AbstractServiceRegister> ptr;

    AbstractServiceRegister() {}
    virtual ~AbstractServiceRegister() {}

    virtual void registerService(std::shared_ptr<google::protobuf::Service> service, NetAddress::ptr addr) = 0;
    virtual void registerService(std::shared_ptr<CustomService> service, NetAddress::ptr addr) = 0;
    virtual std::vector<NetAddress::ptr> discoverService(const std::string &serviceName) = 0;
    virtual void clear() = 0;
};

}

#endif