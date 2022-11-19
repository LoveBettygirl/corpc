#ifndef CORPC_COMMOM_START_H
#define CORPC_COMMOM_START_H

#include <google/protobuf/service.h>
#include <memory>
#include <cstdio>
#include <functional>
#include "log.h"
#include "tcp_server.h"
#include "timer.h"

namespace corpc {

#define REGISTER_HTTP_SERVLET(path, servlet)                                                                                       \
    do {                                                                                                                           \
        if (!corpc::GetServer()->registerHttpServlet(path, std::make_shared<servlet>())) {                                       \
            printf("Start corpc server error, because register http servelt error, please look up rpc log get more details!\n"); \
            corpc::Exit(0);                                                                                                      \
        }                                                                                                                          \
    } while (0)

#define REGISTER_SERVICE(service)                                                                                                      \
    do {                                                                                                                                \
        if (!corpc::GetServer()->registerService(std::make_shared<service>())) {                                                      \
            printf("Start corpc server error, because register protobuf service error, please look up rpc log get more details!\n"); \
            corpc::Exit(0);                                                                                                          \
        }                                                                                                                              \
    } while (0)

void initConfig(const char *file);
void startRpcServer();
TcpServer::ptr getServer();
int getIOThreadPoolSize();
Config::ptr getConfig();
void addTimerEvent(TimerEvent::ptr event);

}

#endif