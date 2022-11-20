#ifndef CORPC_COMMOM_CONST_H
#define CORPC_COMMOM_CONST_H

namespace corpc {

enum LogType {
    INTER_LOG = 1, // 框架内部日志
    USER_LOG = 2, // 用户日志
};

enum LogLevel {
    NONE = 0, // don't print log
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

}

#endif