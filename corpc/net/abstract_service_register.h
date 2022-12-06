#ifndef CORPC_NET_ABSTRACT_SERVICE_REGISTER_H
#define CORPC_NET_ABSTRACT_SERVICE_REGISTER_H

#include <memory>
#include <string>
#include <google/protobuf/service.h>
#include "corpc/net/net_address.h"

namespace corpc {

class AbstractServiceRegister {
public:
    virtual void registerService(std::shared_ptr<google::protobuf::Service> service) = 0;
    virtual NetAddress::ptr discoverService(const std::string &serviceFullName) = 0;
};

}

#endif