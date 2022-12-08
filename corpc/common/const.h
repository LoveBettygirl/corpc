#ifndef CORPC_COMMOM_CONST_H
#define CORPC_COMMOM_CONST_H

namespace corpc {

enum LogType {
    INTER_LOG = 1, // 框架内部日志
    USER_LOG = 2, // 用户日志
};

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4,
    NONE = 100, // don't print log
};

enum class LoadBalanceCategory {
    // 随机算法
    Random,
    // 轮询算法
    Round,
    // 一致性哈希
    ConsistentHash
};

enum class ServiceRegisterCategory {
    // 不进行服务注册
    None,
    // zk
    Zk,
};

}

#endif