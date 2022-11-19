#ifndef CORPC_NET_HTTP_HTTP_SERVLET_H
#define CORPC_NET_HTTP_HTTP_SERVLET_H

#include <memory>
#include "http_request.h"
#include "http_response.h"

namespace corpc {

class HttpServlet : public std::enable_shared_from_this<HttpServlet> {
public:
    typedef std::shared_ptr<HttpServlet> ptr;

    HttpServlet();
    virtual ~HttpServlet();

    virtual void handle(HttpRequest *req, HttpResponse *res) = 0;
    virtual std::string getServletName() = 0;
    void handleNotFound(HttpRequest *req, HttpResponse *res);
    void setHttpCode(HttpResponse *res, const int code);
    void setHttpContentType(HttpResponse *res, const std::string &contentType);
    void setHttpBody(HttpResponse *res, const std::string &body);
    void setCommParam(HttpRequest *req, HttpResponse *res);
};

class NotFoundHttpServlet : public HttpServlet {
public:
    NotFoundHttpServlet();
    ~NotFoundHttpServlet();

    void handle(HttpRequest *req, HttpResponse *res);
    std::string getServletName();
};

}

#endif