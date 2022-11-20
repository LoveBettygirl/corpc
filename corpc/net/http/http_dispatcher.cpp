#include <memory>
#include "corpc/net/http/http_dispatcher.h"
#include "corpc/net/http/http_request.h"
#include "corpc/net/http/http_servlet.h"
#include "corpc/common/log.h"
#include "corpc/common/msg_seq.h"
#include "corpc/net/tcp/tcp_connection.h"

namespace corpc {

void HttpDispacther::dispatch(AbstractData *data, TcpConnection *conn)
{
    HttpRequest *request = dynamic_cast<HttpRequest *>(data);
    HttpResponse response;
    Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_ = MsgSeqUtil::genMsgNumber();
    setCurrentRunTime(Coroutine::getCurrentCoroutine()->getRunTime());

    LOG_INFO << "begin to dispatch client http request, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_;

    std::string urlPath_ = request->requestPath_;
    if (!urlPath_.empty()) {
        auto it = servlets_.find(urlPath_);
        if (it == servlets_.end()) {
            LOG_ERROR << "404, url path{ " << urlPath_ << "}, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->msgNo_;
            NotFoundHttpServlet servlet;
            Coroutine::getCurrentCoroutine()->getRunTime()->interfaceName_ = servlet.getServletName();
            servlet.setCommParam(request, &response);
            servlet.handle(request, &response);
        }
        else {
            Coroutine::getCurrentCoroutine()->getRunTime()->interfaceName_ = it->second->getServletName();
            it->second->setCommParam(request, &response);
            it->second->handle(request, &response);
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