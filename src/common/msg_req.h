#ifndef MSG_REQ_H
#define MSG_REQ_H

#include <string>

namespace corpc {

class MsgReqUtil {
public:
    static std::string genMsgNumber();
};

}

#endif