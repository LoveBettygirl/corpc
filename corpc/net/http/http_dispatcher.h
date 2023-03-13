#ifndef CORPC_NET_HTTP_HTTP_DISPATCHER_H
#define CORPC_NET_HTTP_HTTP_DISPATCHER_H

#include <map>
#include <memory>
#include "corpc/net/abstract_dispatcher.h"
#include "corpc/net/http/http_servlet.h"

namespace corpc {

class HttpDispacther : public AbstractDispatcher {
public:
    HttpDispacther() = default;
    ~HttpDispacther() = default;

    void dispatch(AbstractData *data, const TcpConnection::ptr &conn) override;
    void registerServlet(const std::string &path, HttpServlet::ptr servlet);

public:
    std::map<std::string, HttpServlet::ptr> servlets_;
};

}

#endif