#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>
#include "corpc/net/tcp/tcp_server.h"
#include "corpc/net/net_address.h"
#include "corpc/net/mutex.h"
#include "corpc/net/pb/pb_rpc_dispatcher.h"
#include "corpc/common/log.h"
#include "corpc/common/start.h"
#include "test_pb_server.pb.h"

static int i = 0;
corpc::CoroutineMutex gCorMutex;

class QueryServiceImpl : public QueryService {
public:
    QueryServiceImpl() {}
    ~QueryServiceImpl() {}

    void query_name(google::protobuf::RpcController *controller,
                    const ::queryNameReq *request,
                    ::queryNameRes *response,
                    ::google::protobuf::Closure *done) {

        USER_LOG_INFO << "QueryServiceImpl.query_name, req={" << request->ShortDebugString() << "}";

        response->set_id(request->id());
        response->set_name("aaa");

        USER_LOG_INFO << "QueryServiceImpl.query_name, req={" << request->ShortDebugString() << "}, res={" << response->ShortDebugString() << "}";

        if (done) {
            done->Run();
        }
    }

    void query_age(google::protobuf::RpcController *controller,
                   const ::queryAgeReq *request,
                   ::queryAgeRes *response,
                   ::google::protobuf::Closure *done) {

        USER_LOG_INFO << "QueryServiceImpl.query_age, req={" << request->ShortDebugString() << "}";

        response->set_ret_code(0);
        response->set_res_info("OK");
        response->set_req_no(request->req_no());
        response->set_id(request->id());
        response->set_age(100100111);

        gCorMutex.lock();
        USER_LOG_DEBUG << "begin i = " << i;
        sleep(5);
        i++;
        USER_LOG_DEBUG << "end i = " << i;
        gCorMutex.unlock();

        if (done) {
            done->Run();
        }

        USER_LOG_INFO << "QueryServiceImpl.query_age, res={" << response->ShortDebugString() << "}";
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

    REGISTER_SERVICE(QueryServiceImpl);

    corpc::startServer();

    return 0;
}
