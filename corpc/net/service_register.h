#ifndef CORPC_NET_SERVICE_REGISTER_H
#define CORPC_NET_SERVICE_REGISTER_H

#include <memory>
#include <string>
#include <vector>
#include <google/protobuf/service.h>
#include "corpc/common/const.h"
#include "corpc/net/abstract_service_register.h"

namespace corpc {

class ServiceRegister {
public:
    static AbstractServiceRegister::ptr queryRegister(ServiceRegisterCategory category);
    static std::string category2Str(ServiceRegisterCategory category);
    static ServiceRegisterCategory str2Category(const std::string &str);
private:
    static AbstractServiceRegister::ptr s_noneServiceRegister;
    static AbstractServiceRegister::ptr s_zkServiceRegister;
};

}

#endif