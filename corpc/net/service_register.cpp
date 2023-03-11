#include <algorithm>
#include <cctype>
#include "corpc/net/service_register.h"
#include "corpc/net/register/zk_service_register.h"
#include "corpc/net/register/none_service_register.h"

namespace corpc {

AbstractServiceRegister::ptr ServiceRegister::s_noneServiceRegister;
AbstractServiceRegister::ptr ServiceRegister::s_zkServiceRegister;

AbstractServiceRegister::ptr ServiceRegister::queryRegister(ServiceRegisterCategory category)
{
    switch (category) {
        case ServiceRegisterCategory::None:
            if (!s_noneServiceRegister) {
                s_noneServiceRegister = std::make_shared<NoneServiceRegister>();
            }
            return s_noneServiceRegister;
        case ServiceRegisterCategory::Zk:
            if (!s_zkServiceRegister) {
                s_zkServiceRegister = std::make_shared<ZkServiceRegister>();
            }
            return s_zkServiceRegister;
        default:
            if (!s_noneServiceRegister) {
                s_noneServiceRegister = std::make_shared<NoneServiceRegister>();
            }
            return s_noneServiceRegister;
    }
}

std::string ServiceRegister::category2Str(ServiceRegisterCategory category)
{
    switch (category) {
        case ServiceRegisterCategory::None:
            return "None";
        case ServiceRegisterCategory::Zk:
            return "Zk";
        default:
            return "None";
    }
}

ServiceRegisterCategory ServiceRegister::str2Category(const std::string &str)
{
    std::string strTemp = str;
    std::transform(str.begin(), str.end(), strTemp.begin(), tolower);
    if (strTemp == "zk")
        return ServiceRegisterCategory::Zk;
    return ServiceRegisterCategory::None;
}

}
