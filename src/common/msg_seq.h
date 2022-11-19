#ifndef CORPC_COMMOM_MSG_REQ_H
#define CORPC_COMMOM_MSG_REQ_H

#include <string>

namespace corpc {

class MsgSeqUtil {
public:
    static std::string genMsgNumber();
};

}

#endif