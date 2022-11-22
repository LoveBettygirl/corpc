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

}

#endif