#ifndef CORPC_NET_CUSTOM_CUSTOM_DATA_H
#define CORPC_NET_CUSTOM_CUSTOM_DATA_H

#include <string>
#include <memory>
#include "corpc/net/abstract_data.h"

namespace corpc {

// Override this class to use your custom protocol
class CustomStruct : public AbstractData {
public:
    typedef std::shared_ptr<CustomStruct> ptr;
    CustomStruct() = default;
    ~CustomStruct() = default;
    CustomStruct(const CustomStruct &) = default;
    CustomStruct &operator=(const CustomStruct &) = default;
    CustomStruct(CustomStruct &&) = default;
    CustomStruct &operator=(CustomStruct &&) = default;

    std::string protocolData;
};

}

#endif