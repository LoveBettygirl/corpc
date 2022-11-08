#include <memory>
#include "http_dispatcher.h"
#include "http_request.h"
#include "http_servlet.h"
#include "log.h"
#include "msg_req.h"

namespace corpc {

void HttpDispacther::dispatch(AbstractData *data, TcpConnection *conn)
{
    HttpRequest *resquest = dynamic_cast<HttpRequest *>(data);
    HttpResponse response;
    Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_ = MsgReqUtil::genMsgNumber();
    setCurrentRunTime(Coroutine::getCurrentCoroutine()->getRunTime());

    LOG_INFO << "begin to dispatch client http request, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_;

    std::string urlPath_ = request->requestPath_;
    if (!urlPath_.empty()) {
        auto it = servlets_.find(urlPath_);
        if (it == servlets_.end()) {
            LOG_ERROR << "404, url path{ " << urlPath_ << "}, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_;
            NotFoundHttpServlet servlet;
            Coroutine::getCurrentCoroutine()->getRunTime()->interfaceName_ = servlet.getServletName();
            servlet.setCommParam(resquest, &response);
            servlet.handle(resquest, &response);
        }
        else {
            Coroutine::getCurrentCoroutine()->getRunTime()->interfaceName_ = it->second->getServletName();
            it->second->setCommParam(resquest, &response);
            it->second->handle(resquest, &response);
        }
    }

    conn->getCodec()->encode(conn->getOutBuffer(), &response);

    LOG_INFO << "end dispatch client http request, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_;
}

void HttpDispacther::registerServlet(const std::string &path, HttpServlet::ptr servlet)
{
    auto it = servlets_.find(path);
    if (it == servlets_.end()) {
        LOG_DEBUG << "register servlet success to path {" << path << "}";
        servlets_[path] = servlet;
    }
    else {
        LOG_ERROR << "failed to register, beacuse path {" << path << "} has already register servlet";
    }
}

}