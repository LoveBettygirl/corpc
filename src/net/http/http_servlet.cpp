#include <memory>
#include "http_servlet.h"
#include "http_request.h"
#include "http_response.h"
#include "http_define.h"
#include "log.h"

namespace corpc {

extern const char *defaultHtmlTemplate;
extern std::string contentTypeText;

HttpServlet::HttpServlet()
{
}

HttpServlet::~HttpServlet()
{
}

void HttpServlet::handle(HttpRequest *req, HttpResponse *res)
{
}

void HttpServlet::handleNotFound(HttpRequest *req, HttpResponse *res)
{
    LOG_DEBUG << "return 404 html";
    setHttpCode(res, HTTP_NOTFOUND);
    char buf[512] = {0};
    sprintf(buf, defaultHtmlTemplate, std::to_string(HTTP_NOTFOUND).c_str(), httpCodeToString(HTTP_NOTFOUND));
    setHttpContentType(res, contentTypeText);
    setHttpBody(res, std::string(buf));
    setCommParam(req, res);
}

void HttpServlet::setHttpCode(HttpResponse *res, const int code)
{
    res->responseCode_ = code;
    res->responseInfo_ = std::string(httpCodeToString(code));
}

void HttpServlet::setHttpContentType(HttpResponse *res, const std::string &contentType)
{
    res->responseHeader_.maps_["Content-Type"] = contentType;
}

void HttpServlet::setHttpBody(HttpResponse *res, const std::string &body)
{
    res->responseBody_ = body;
    res->responseHeader_.maps_["Content-Length"] = std::to_string(res->responseBody_.size());
}

void HttpServlet::setCommParam(HttpRequest *req, HttpResponse *res)
{
    LOG_DEBUG << "set response version=" << req->requestVersion_;
    res->responseVersion_ = req->requestVersion_;
    if (req->requestHeader_.maps_.find("Connection") != req->requestHeader_.maps_.end()) {
        res->responseHeader_.maps_["Connection"] = req->requestHeader_.maps_["Connection"];
    }
}

NotFoundHttpServlet::NotFoundHttpServlet()
{
}

NotFoundHttpServlet::~NotFoundHttpServlet()
{
}

void NotFoundHttpServlet::handle(HttpRequest *req, HttpResponse *res)
{
    handleNotFound(req, res);
}

std::string NotFoundHttpServlet::getServletName()
{
    return "NotFoundHttpServlet";
}

}