#ifndef CORPC_NET_CUSTOM_CUSTOM_SERVICE_H
#define CORPC_NET_CUSTOM_CUSTOM_SERVICE_H

#include <memory>
#include <string>

namespace corpc {

// Override this class to use your custom protocol
class CustomService : public std::enable_shared_from_this<CustomService> {
public:
    typedef std::shared_ptr<CustomService> ptr;
    CustomService() {}
    virtual ~CustomService() {}

    virtual std::string getServiceName() { return "CustomService"; }
};

}

#endif