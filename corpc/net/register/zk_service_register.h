#ifndef CORPC_NET_REGISTER_ZK_SERVICE_REGISTER_H
#define CORPC_NET_REGISTER_ZK_SERVICE_REGISTER_H

#include <unordered_set>
#include <string>
#include "corpc/net/abstract_service_register.h"
#include "corpc/common/zk_util.h"

namespace corpc {

class ZkServiceRegister : public AbstractServiceRegister {
public:
    ZkServiceRegister();
    ZkServiceRegister(const std::string &ip, int port, int timeout);
    ~ZkServiceRegister();

    void registerService(std::shared_ptr<google::protobuf::Service> service, NetAddress::ptr addr);
    void registerService(std::shared_ptr<CustomService> service, NetAddress::ptr addr);
    std::vector<NetAddress::ptr> discoverService(const std::string &serviceName);
    void clear();

private:
    void init();

    ZkClient zkCli_;

    std::unordered_set<std::string> pathSet_;
    std::unordered_set<std::string> serviceSet_;
};

}

#endif