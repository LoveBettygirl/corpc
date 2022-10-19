#ifndef ABSTRACT_DISPATCHER_H
#define ABSTRACT_DISPATCHER_H

#include <memory>
#include "abstract_data.h"
#include "tcp_connection.h"

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