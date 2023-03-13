#ifndef CORPC_NET_CUSTOM_CUSTOM_DISPATCHER_H
#define CORPC_NET_CUSTOM_CUSTOM_DISPATCHER_H

#include <memory>
#include "corpc/net/abstract_dispatcher.h"

namespace corpc {

class TcpConnection;

// Override this class to use your custom protocol
class CustomDispatcher : public AbstractDispatcher {
public:
    CustomDispatcher() {}
    ~CustomDispatcher() {}

    void dispatch(AbstractData *data, const TcpConnection::ptr &conn) override {}
};

}

#endif