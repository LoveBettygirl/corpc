#include <google/protobuf/service.h>
#include "corpc/common/start.h"
#include "corpc/common/log.h"
#include "corpc/common/config.h"
#include "corpc/net/tcp/tcp_server.h"
#include "corpc/coroutine/coroutine_hook.h"

namespace corpc {

corpc::Config::ptr gConfig;
corpc::Logger::ptr gLogger;
corpc::TcpServer::ptr gServer;

static int gInitConfig = 0;

void initConfig(const char *file)
{
    corpc::setHook(false);
    corpc::setHook(true);

    if (gInitConfig == 0) {
        gConfig = std::make_shared<corpc::Config>(file);
        gConfig->readConf();
        gInitConfig = 1;
    }
}

TcpServer::ptr getServer()
{
    return gServer;
}

void startRpcServer()
{
    gLogger->start();
    gServer->start();
}

int getIOThreadPoolSize()
{
    return gServer->getIOThreadPool()->getIOThreadPoolSize();
}

Config::ptr getConfig()
{
    return gConfig;
}

void addTimerEvent(TimerEvent::ptr event)
{
}

}