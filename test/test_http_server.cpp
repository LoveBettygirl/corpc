#include <google/protobuf/service.h>
#include <atomic>
#include <future>
#include "corpc/common/start.h"
#include "corpc/net/http/http_request.h"
#include "corpc/net/http/http_response.h"
#include "corpc/net/http/http_servlet.h"
#include "corpc/net/http/http_define.h"
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/pb/pb_rpc_async_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_rpc_closure.h"
#include "corpc/net/net_address.h"
#include "test_pb_server.pb.h"

const char *html = "<html><body><h1>Welcome to corpc, just enjoy it!</h1><p>%s</p></body></html>";

corpc::IPAddress::ptr addr = std::make_shared<corpc::IPAddress>("127.0.0.1", 20000);

class BlockCallHttpServlet : public corpc::HttpServlet {
public:
    BlockCallHttpServlet() = default;
    ~BlockCallHttpServlet() = default;

    void handle(corpc::HttpRequest *req, corpc::HttpResponse *res) {
        USER_LOG_DEBUG << "BlockCallHttpServlet get request ";
        USER_LOG_DEBUG << "BlockCallHttpServlet success recive http request, now to get http response";
        setHttpCode(res, corpc::HTTP_OK);
        setHttpContentType(res, "text/html;charset=utf-8");

        queryAgeReq rpcReq;
        queryAgeRes rpcRes;
        USER_LOG_DEBUG << "now to call QueryServer corpc server to query who's id is " << req->queryMaps_["id"];
        rpcReq.set_id(std::atoi(req->queryMaps_["id"].c_str()));

        corpc::PbRpcChannel channel(addr);
        QueryService_Stub stub(&channel);

        corpc::PbRpcController rpcController;
        rpcController.SetTimeout(5000);

        USER_LOG_DEBUG << "BlockCallHttpServlet end to call RPC";
        stub.query_age(&rpcController, &rpcReq, &rpcRes, NULL);
        USER_LOG_DEBUG << "BlockCallHttpServlet end to call RPC";

        if (rpcController.ErrorCode() != 0) {
            USER_LOG_DEBUG << "failed to call QueryServer rpc server";
            char buf[512] = {0};
            sprintf(buf, html, "failed to call QueryServer rpc server");
            setHttpBody(res, std::string(buf));
            return;
        }

        if (rpcRes.ret_code() != 0) {
            std::stringstream ss;
            ss << "QueryServer rpc server return bad result, ret = " << rpcRes.ret_code() << ", and res_info = " << rpcRes.res_info();
            USER_LOG_DEBUG << ss.str();
            char buf[512] = {0};
            sprintf(buf, html, ss.str().c_str());
            setHttpBody(res, std::string(buf));
            return;
        }

        std::stringstream ss;
        ss << "Success!! Your age is " << rpcRes.age() << " and Your id is " << rpcRes.id();

        char buf[512] = {0};
        sprintf(buf, html, ss.str().c_str());
        setHttpBody(res, std::string(buf));
    }

    std::string getServletName() {
        return "BlockCallHttpServlet";
    }
};

class NonBlockCallHttpServlet : public corpc::HttpServlet {
public:
    NonBlockCallHttpServlet() = default;
    ~NonBlockCallHttpServlet() = default;

    void handle(corpc::HttpRequest *req, corpc::HttpResponse *res) {
        USER_LOG_INFO << "NonBlockCallHttpServlet get request";
        USER_LOG_DEBUG << "NonBlockCallHttpServlet success recive http request, now to get http response";
        setHttpCode(res, corpc::HTTP_OK);
        setHttpContentType(res, "text/html;charset=utf-8");

        std::shared_ptr<queryAgeReq> rpcReq = std::make_shared<queryAgeReq>();
        std::shared_ptr<queryAgeRes> rpcRes = std::make_shared<queryAgeRes>();
        USER_LOG_DEBUG << "now to call QueryServer corpc server to query who's id is " << req->queryMaps_["id"];
        rpcReq->set_id(std::atoi(req->queryMaps_["id"].c_str()));

        std::shared_ptr<corpc::PbRpcController> rpcController = std::make_shared<corpc::PbRpcController>();
        rpcController->SetTimeout(10000);

        USER_LOG_DEBUG << "NonBlockCallHttpServlet begin to call RPC async";

        corpc::PbRpcAsyncChannel::ptr asyncChannel =
            std::make_shared<corpc::PbRpcAsyncChannel>(addr);

        auto cb = [rpcRes]() {
            printf("call succ, res = %s\n", rpcRes->ShortDebugString().c_str());
            USER_LOG_DEBUG << "NonBlockCallHttpServlet async call end, res=" << rpcRes->ShortDebugString();
        };

        std::shared_ptr<corpc::PbRpcClosure> closure = std::make_shared<corpc::PbRpcClosure>(cb);
        asyncChannel->saveCallee(rpcController, rpcReq, rpcRes, closure);

        QueryService_Stub stub(asyncChannel.get());

        stub.query_age(rpcController.get(), rpcReq.get(), rpcRes.get(), NULL);
        USER_LOG_DEBUG << "NonBlockCallHttpServlet async end, now you can to some another thing";

        asyncChannel->wait();
        USER_LOG_DEBUG << "wait() back, now to check is rpc call succ";

        if (rpcController->ErrorCode() != 0) {
            USER_LOG_DEBUG << "failed to call QueryServer rpc server";
            char buf[512] = {0};
            sprintf(buf, html, "failed to call QueryServer rpc server");
            setHttpBody(res, std::string(buf));
            return;
        }

        if (rpcRes->ret_code() != 0) {
            std::stringstream ss;
            ss << "QueryServer rpc server return bad result, ret = " << rpcRes->ret_code() << ", and res_info = " << rpcRes->res_info();
            USER_LOG_DEBUG << ss.str();
            char buf[512] = {0};
            sprintf(buf, html, ss.str().c_str());
            setHttpBody(res, std::string(buf));
            return;
        }

        std::stringstream ss;
        ss << "Success!! Your age is " << rpcRes->age() << " and Your id is " << rpcRes->id();

        char buf[512] = {0};
        sprintf(buf, html, ss.str().c_str());
        setHttpBody(res, std::string(buf));
    }

    std::string getServletName() {
        return "NonBlockCallHttpServlet";
    }
};

class QPSHttpServlet : public corpc::HttpServlet {
public:
    QPSHttpServlet() = default;
    ~QPSHttpServlet() = default;

    void handle(corpc::HttpRequest *req, corpc::HttpResponse *res) {
        USER_LOG_DEBUG << "QPSHttpServlet get request";
        setHttpCode(res, corpc::HTTP_OK);
        setHttpContentType(res, "text/html;charset=utf-8");

        std::stringstream ss;
        ss << "QPSHttpServlet Echo Success!! Your id is: " << req->queryMaps_["id"];
        char buf[512] = {0};
        sprintf(buf, html, ss.str().c_str());
        setHttpBody(res, std::string(buf));
        USER_LOG_DEBUG << ss.str();
    }

    std::string getServletName() {
        return "QPSHttpServlet";
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Start corpc server error, input argc is not 2!\n");
        printf("Start corpc server like this: \n");
        printf("./server conf.yaml\n");
        return 0;
    }

    corpc::initConfig(argv[1]);

    REGISTER_HTTP_SERVLET("/qps", QPSHttpServlet);

    REGISTER_HTTP_SERVLET("/block", BlockCallHttpServlet);
    REGISTER_HTTP_SERVLET("/nonblock", NonBlockCallHttpServlet);

    corpc::startRpcServer();
    return 0;
}
