#include <iostream>
#include <google/protobuf/service.h>
#include "corpc/net/pb/pb_rpc_channel.h"
#include "corpc/net/pb/pb_rpc_client_channel.h"
#include "corpc/net/pb/pb_rpc_async_channel.h"
#include "corpc/net/pb/pb_rpc_client_async_channel.h"
#include "corpc/net/pb/pb_rpc_client_block_channel.h"
#include "corpc/net/pb/pb_rpc_controller.h"
#include "corpc/net/pb/pb_rpc_closure.h"
#include "corpc/net/net_address.h"
#include "corpc/common/start.h"
#include "corpc/net/service_register.h"
#include "test_pb_server.pb.h"

corpc::IPAddress::ptr gAddr = std::make_shared<corpc::IPAddress>("127.0.0.1", 20000);

void testClient()
{
    corpc::PbRpcChannel channel(gAddr);
    QueryService_Stub stub(&channel);

    corpc::PbRpcController rpcControllerName;
    rpcControllerName.SetTimeout(15000);

    queryNameReq nameReq;
    queryNameRes nameRes;

    nameReq.set_req_no(111);
    nameReq.set_id(11);
    nameReq.set_type(1);

    std::cout << "call query_name corpc server " << gAddr->toString() << ", req = " << nameReq.ShortDebugString() << std::endl;
    stub.query_name(&rpcControllerName, &nameReq, &nameRes, nullptr);

    if (rpcControllerName.ErrorCode() != 0) {
        std::cout << "Failed to call corpc server query_name request, error code: " << rpcControllerName.ErrorCode() << ", error info: " << rpcControllerName.ErrorText() << std::endl;
        USER_LOG_DEBUG << "Failed to call corpc server query_name request, error code: " << rpcControllerName.ErrorCode() << ", error info: " << rpcControllerName.ErrorText();
        return;
    }

    if (nameRes.ret_code() != 0) {
        std::cout << "call query_name return bad result from corpc server " << gAddr->toString() << ", ret = " << nameRes.ret_code() << ", and res_info = " << nameRes.res_info();
        USER_LOG_DEBUG << "call query_name return bad result from corpc server " << gAddr->toString() << ", ret = " << nameRes.ret_code() << ", and res_info = " << nameRes.res_info();
        return;
    }

    std::cout << "Success get query_name response from corpc server " << gAddr->toString() << ", res = " << nameRes.ShortDebugString() << std::endl;

    corpc::PbRpcController rpcControllerAge;
    rpcControllerAge.SetTimeout(15000);
    
    queryAgeReq ageReq;
    queryAgeRes ageRes;

    ageReq.set_req_no(222);
    ageReq.set_id(22);

    std::cout << "call query_age corpc server " << gAddr->toString() << ", req = " << ageReq.ShortDebugString() << std::endl;
    stub.query_age(&rpcControllerAge, &ageReq, &ageRes, nullptr);

    if (rpcControllerAge.ErrorCode() != 0) {
        std::cout << "Failed to call corpc server query_age request, error code: " << rpcControllerAge.ErrorCode() << ", error info: " << rpcControllerAge.ErrorText() << std::endl;
        USER_LOG_DEBUG << "Failed to call corpc server query_age request, error code: " << rpcControllerAge.ErrorCode() << ", error info: " << rpcControllerAge.ErrorText();
        return;
    }

    if (ageRes.ret_code() != 0) {
        std::cout << "call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes.ret_code() << ", and res_info = " << ageRes.res_info();
        USER_LOG_DEBUG << "call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes.ret_code() << ", and res_info = " << ageRes.res_info();
        return;
    }

    std::cout << "Success get query_age response from corpc server " << gAddr->toString() << ", res = " << ageRes.ShortDebugString() << std::endl;
}

void testBlockClient()
{
    corpc::AbstractServiceRegister::ptr center = corpc::ServiceRegister::queryRegister(corpc::ServiceRegisterCategory::Zk);
    std::vector<corpc::NetAddress::ptr> addrs = center->discoverService("QueryService");

    // corpc::PbRpcClientBlockChannel channel(gAddr);
    corpc::PbRpcClientBlockChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    QueryService_Stub stub(&channel);

    corpc::PbRpcController rpcControllerName;
    rpcControllerName.SetTimeout(15000);

    queryNameReq nameReq;
    queryNameRes nameRes;

    nameReq.set_req_no(111);
    nameReq.set_id(11);
    nameReq.set_type(1);

    std::cout << "call query_name corpc server " << gAddr->toString() << ", req = " << nameReq.ShortDebugString() << std::endl;
    stub.query_name(&rpcControllerName, &nameReq, &nameRes, nullptr);

    if (rpcControllerName.ErrorCode() != 0) {
        std::cout << "Failed to call corpc server query_name request, error code: " << rpcControllerName.ErrorCode() << ", error info: " << rpcControllerName.ErrorText() << std::endl;
        USER_LOG_DEBUG << "Failed to call corpc server query_name request, error code: " << rpcControllerName.ErrorCode() << ", error info: " << rpcControllerName.ErrorText();
        return;
    }

    if (nameRes.ret_code() != 0) {
        std::cout << "call query_name return bad result from corpc server " << gAddr->toString() << ", ret = " << nameRes.ret_code() << ", and res_info = " << nameRes.res_info();
        USER_LOG_DEBUG << "call query_name return bad result from corpc server " << gAddr->toString() << ", ret = " << nameRes.ret_code() << ", and res_info = " << nameRes.res_info();
        return;
    }

    std::cout << "Success get query_name response from corpc server " << gAddr->toString() << ", res = " << nameRes.ShortDebugString() << std::endl;

    corpc::PbRpcController rpcControllerAge;
    rpcControllerAge.SetTimeout(15000);
    
    queryAgeReq ageReq;
    queryAgeRes ageRes;

    ageReq.set_req_no(222);
    ageReq.set_id(22);

    std::cout << "call query_age corpc server " << gAddr->toString() << ", req = " << ageReq.ShortDebugString() << std::endl;
    stub.query_age(&rpcControllerAge, &ageReq, &ageRes, nullptr);

    if (rpcControllerAge.ErrorCode() != 0) {
        std::cout << "Failed to call corpc server query_age request, error code: " << rpcControllerAge.ErrorCode() << ", error info: " << rpcControllerAge.ErrorText() << std::endl;
        USER_LOG_DEBUG << "Failed to call corpc server query_age request, error code: " << rpcControllerAge.ErrorCode() << ", error info: " << rpcControllerAge.ErrorText();
        return;
    }

    if (ageRes.ret_code() != 0) {
        std::cout << "call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes.ret_code() << ", and res_info = " << ageRes.res_info();
        USER_LOG_DEBUG << "call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes.ret_code() << ", and res_info = " << ageRes.res_info();
        return;
    }

    std::cout << "Success get query_age response from corpc server " << gAddr->toString() << ", res = " << ageRes.ShortDebugString() << std::endl;
}

void testNonBlockClient()
{
    corpc::PbRpcClientChannel::ptr channel = std::make_shared<corpc::PbRpcClientChannel>(gAddr);
    QueryService_Stub stub(channel.get());

    std::shared_ptr<corpc::PbRpcController> rpcControllerAge = std::make_shared<corpc::PbRpcController>();
    rpcControllerAge->SetTimeout(15000);
    
    std::shared_ptr<queryAgeReq> ageReq = std::make_shared<queryAgeReq>();
    std::shared_ptr<queryAgeRes> ageRes = std::make_shared<queryAgeRes>();

    ageReq->set_req_no(222);
    ageReq->set_id(22);

    auto cbAge = [ageRes, rpcControllerAge]() {
        if (rpcControllerAge->ErrorCode() != 0) {
            std::cout << "Failed to nonblock call corpc server query_age request, error code: " << rpcControllerAge->ErrorCode() << ", error info: " << rpcControllerAge->ErrorText() << std::endl;
            USER_LOG_DEBUG << "Failed to nonblock call corpc server query_age request, error code: " << rpcControllerAge->ErrorCode() << ", error info: " << rpcControllerAge->ErrorText();
            return;
        }

        if (ageRes->ret_code() != 0) {
            std::cout << "nonblock call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes->ret_code() << ", and res_info = " << ageRes->res_info();
            USER_LOG_DEBUG << "nonblock call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes->ret_code() << ", and res_info = " << ageRes->res_info();
            return;
        }

        std::cout << "Success get query_age response from corpc server " << gAddr->toString() << ", res = " << ageRes->ShortDebugString() << std::endl;
    };

    std::shared_ptr<corpc::PbRpcClosure> closureAge = std::make_shared<corpc::PbRpcClosure>(cbAge); 

    std::cout << "nonblock call query_age corpc server " << gAddr->toString() << ", req = " << ageReq->ShortDebugString() << std::endl;
    stub.query_age(rpcControllerAge.get(), ageReq.get(), ageRes.get(), closureAge.get());
    std::cout << "waiting for nonblock call query_age result......" << std::endl;
    channel->wait();
}

void testClientAsync()
{
    corpc::PbRpcClientAsyncChannel::ptr channel = std::make_shared<corpc::PbRpcClientAsyncChannel>(gAddr);
    QueryService_Stub stub(channel.get());

    std::shared_ptr<corpc::PbRpcController> rpcControllerAge = std::make_shared<corpc::PbRpcController>();
    rpcControllerAge->SetTimeout(15000);
    
    std::shared_ptr<queryAgeReq> ageReq = std::make_shared<queryAgeReq>();
    std::shared_ptr<queryAgeRes> ageRes = std::make_shared<queryAgeRes>();

    ageReq->set_req_no(222);
    ageReq->set_id(22);

    auto cbAge = [ageRes, rpcControllerAge]() {
        if (rpcControllerAge->ErrorCode() != 0) {
            std::cout << "Failed to async call corpc server query_age request, error code: " << rpcControllerAge->ErrorCode() << ", error info: " << rpcControllerAge->ErrorText() << std::endl;
            USER_LOG_DEBUG << "Failed to async call corpc server query_age request, error code: " << rpcControllerAge->ErrorCode() << ", error info: " << rpcControllerAge->ErrorText();
            return;
        }

        if (ageRes->ret_code() != 0) {
            std::cout << "async call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes->ret_code() << ", and res_info = " << ageRes->res_info();
            USER_LOG_DEBUG << "async call query_age return bad result from corpc server " << gAddr->toString() << ", ret = " << ageRes->ret_code() << ", and res_info = " << ageRes->res_info();
            return;
        }

        std::cout << "Success get query_age response from corpc server " << gAddr->toString() << ", res = " << ageRes->ShortDebugString() << std::endl;
    };

    std::shared_ptr<corpc::PbRpcClosure> closureAge = std::make_shared<corpc::PbRpcClosure>(cbAge); 

    channel->saveCallee(rpcControllerAge, ageReq, ageRes, closureAge);

    std::cout << "async call query_age corpc server " << gAddr->toString() << ", req = " << ageReq->ShortDebugString() << std::endl;
    stub.query_age(rpcControllerAge.get(), ageReq.get(), ageRes.get(), closureAge.get());
    std::cout << "waiting for async call query_age result......" << std::endl;
    channel->wait();
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Start corpc server error, input argc is not 2!\n");
        printf("Start corpc server like this: \n");
        printf("./server conf.yaml\n");
        return 0;
    }

    corpc::initConfig(argv[1]);

    // testClient();
    testBlockClient();
    // testNonBlockClient();
    // testClientAsync();

    return 0;
}
