#ifndef CORPC_NET_ABSTRACT_DISPATCHER_H
#define CORPC_NET_ABSTRACT_DISPATCHER_H

#include <memory>
#include "corpc/net/abstract_data.h"

namespace corpc {

class TcpConnection;

class AbstractDispatcher {
public:
    typedef std::shared_ptr<AbstractDispatcher> ptr;

    AbstractDispatcher() {}
    virtual ~AbstractDispatcher() {}

    virtual void dispatch(AbstractData *data, TcpConnection *conn) = 0;
};

}

#endif