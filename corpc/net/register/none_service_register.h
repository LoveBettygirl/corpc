#ifndef CORPC_NET_REGISTER_NONE_SERVICE_REGISTER_H
#define CORPC_NET_REGISTER_NONE_SERVICE_REGISTER_H

#include "corpc/net/abstract_service_register.h"

namespace corpc {

class NoneServiceRegister : public AbstractServiceRegister {
public:
    NoneServiceRegister() {}
    ~NoneServiceRegister() {}

    void registerService(std::shared_ptr<google::protobuf::Service> service, NetAddress::ptr addr) {}
    std::vector<NetAddress::ptr> discoverService(const std::string &serviceName) { return std::vector<NetAddress::ptr>(); }
    void clear() {}
};

}

#endif