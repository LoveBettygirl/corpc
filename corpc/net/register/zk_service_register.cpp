#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include "corpc/net/register/zk_service_register.h"

namespace corpc {

ZkServiceRegister::ZkServiceRegister()
{
    // 启动zkclient客户端
    zkCli_.start();
    zkCli_.closeLog();
    // 加入根节点
    zkCli_.create(ROOT_PATH, nullptr, 0);
}

void ZkServiceRegister::registerService(std::shared_ptr<google::protobuf::Service> service, NetAddress::ptr addr)
{
    std::string serviceName = service->GetDescriptor()->full_name();
    serviceSet_.insert(serviceName);
    // znode路径：/corpc/serviceName/ip:port
    std::string servicePath = ROOT_PATH;
    servicePath += "/" + serviceName;
    servicePath += "/" + addr->toString();
    zkCli_.create(servicePath.c_str(), nullptr, 0);
    pathSet_.insert(servicePath);
}

std::vector<NetAddress::ptr> ZkServiceRegister::discoverService(const std::string &serviceName)
{
    std::string servicePath = ROOT_PATH;
    servicePath += "/" + serviceName;
    std::vector<std::string> strNodes = zkCli_.getChildrenNodes(servicePath);
    std::vector<NetAddress::ptr> nodes;
    for (const auto &node : strNodes) {
        nodes.push_back(std::make_shared<corpc::IPAddress>(node));
    }
    return nodes;
}

void ZkServiceRegister::clear()
{
    if (zkCli_.getIsConnected()) {
        // 先删除地址对应的节点
        for (const std::string &path : pathSet_) {
            zkCli_.deleteNode(path.c_str());
        }
        for (auto &sp : serviceSet_) {
            std::string servicePath = ROOT_PATH;
            servicePath += "/" + sp;
            if (zkCli_.getChildrenNodes(servicePath).empty()) {
                zkCli_.deleteNode(servicePath.c_str());
            }
        }
        if (zkCli_.getChildrenNodes(ROOT_PATH).empty()) {
            zkCli_.deleteNode(ROOT_PATH);
        }
        // 断开与zkserver的连接
        zkCli_.stop();
    }
}

ZkServiceRegister::~ZkServiceRegister()
{
    clear();
}

}