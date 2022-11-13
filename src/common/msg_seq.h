#ifndef MSG_REQ_H
#define MSG_REQ_H

#include <string>

namespace corpc {

class MsgSeqUtil {
public:
    static std::string genMsgNumber();
};

}

#endif