#ifndef HTTP_DISPATCHER_H
#define HTTP_DISPATCHER_H

#include <map>
#include <memory>
#include "abstract_dispatcher.h"
#include "http_servlet.h"

namespace corpc {

class HttpDispacther : public AbstractDispatcher {
public:
    HttpDispacther() = default;
    ~HttpDispacther() = default;

    void dispatch(AbstractData *data, TcpConnection *conn);
    void registerServlet(const std::string &path, HttpServlet::ptr servlet);

public:
    std::map<std::string, HttpServlet::ptr> servlets_;
};

}

#endif