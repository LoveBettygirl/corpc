#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <random>
#include "corpc/common/log.h"
#include "corpc/common/config.h"
#include "corpc/common/msg_seq.h"

namespace corpc {

extern corpc::Config::ptr gConfig;

static thread_local std::string tMsgSeqNum;
static thread_local std::string tMaxMsgSeqNum;

static int gRandomfd = -1;

std::string MsgSeqUtil::genMsgNumber()
{
    int msgSeqLen = 20;
    if (gConfig) {
        msgSeqLen = gConfig->msgSeqLen;
    }

    // 如果当前消息序列号tMsgSeqNum是空的，或者已达到最大值tMaxMsgSeqNum
    // 需要重新生成一个随机数作为下一个消息的序列号tMsgSeqNum
    if (tMsgSeqNum.empty() || tMsgSeqNum == tMaxMsgSeqNum) {
        // 用srand/rand产生随机数，其实这种随机性并不好，容易遭受攻击（很多时候，也满足不了需求）。
        // /dev/random和/dev/urandom是Linux系统中提供的随机伪设备，这两个设备的任务，是提供永不为空的随机字节数据流。
        // 这两个设备的差异在于：
        // /dev/random的random依赖于系统中断，因此在系统的中断数不足时，/dev/random设备会一直封锁，
        // 尝试读取的进程就会进入等待状态，直到系统的中断数充分够用, /dev/random设备可以保证数据的随机性。
        // /dev/urandom不依赖系统的中断，也就不会造成进程忙等待，但是数据的随机性也不高。
        // man 页面推荐在大多数“一般”的密码学应用下使用 /dev/urandom 。
        if (gRandomfd == -1) {
            gRandomfd = open("/dev/urandom", O_RDONLY);
        }
        std::string res(msgSeqLen, 0); // 读出来的字符串长度应和msgSeqLen一致
        if ((read(gRandomfd, &res[0], msgSeqLen)) != msgSeqLen) {
            LOG_ERROR << "read /dev/urandom data less " << msgSeqLen << " bytes";
            return "";
        }
        tMaxMsgSeqNum = "";
        for (int i = 0; i < msgSeqLen; ++i) {
            uint8_t x = ((uint8_t)(res[i])) % 10;
            res[i] = x + '0';
            tMaxMsgSeqNum += "9";
        }
        tMsgSeqNum = res;
    }
    else {
        // 将消息序列号tMsgSeqNum进行+1的操作
        int i = tMsgSeqNum.size() - 1;
        while (tMsgSeqNum[i] == '9' && i >= 0) {
            i--;
        }
        if (i >= 0) {
            tMsgSeqNum[i] += 1;
            for (size_t j = i + 1; j < tMsgSeqNum.size(); ++j) {
                tMsgSeqNum[j] = '0';
            }
        }
    }
    return tMsgSeqNum;
}

}