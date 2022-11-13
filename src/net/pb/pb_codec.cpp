#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <string.h>
#include "pb_codec.h"
#include "byte_util.h"
#include "log.h"
#include "abstract_data.h"
#include "pb_data.h"
#include "msg_seq.h"

namespace corpc {

static const char PB_START = 0x02; // start char
static const char PB_END = 0x03;   // end char
static const int MSG_REQ_LEN = 20; // default length of msgSeq

PbCodeC::PbCodeC()
{
}

PbCodeC::~PbCodeC()
{
}

void PbCodeC::encode(TcpBuffer *buf, AbstractData *data)
{
    if (!buf || !data) {
        LOG_ERROR << "encode error! buf or data nullptr";
        return;
    }
    LOG_DEBUG << "test encode start";
    PbStruct *temp = dynamic_cast<PbStruct *>(data);

    int len = 0;
    const char *re = encodePbData(temp, len);
    if (re == nullptr || len == 0 || !temp->encodeSucc_) {
        LOG_ERROR << "encode error";
        data->encodeSucc_ = false;
        return;
    }
    LOG_DEBUG << "encode package len = " << len;
    if (buf != nullptr) {
        buf->writeToBuffer(re, len);
        LOG_DEBUG << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
    }
    data = temp;
    if (re) {
        free((void *)re);
        re = nullptr;
    }
    LOG_DEBUG << "test encode end";
}

const char *PbCodeC::encodePbData(PbStruct *data, int &len)
{
    if (data->serviceFullName.empty()) {
        LOG_ERROR << "parse error, serviceFullName is empty";
        data->encodeSucc_ = false;
        return nullptr;
    }
    if (data->msgSeq.empty()) {
        data->msgSeq = MsgSeqUtil::genMsgNumber();
        data->msgSeqLen = data->msgSeq.size();
        LOG_DEBUG << "generate msgno = " << data->msgSeq;
    }

    int32_t pkLen = 2 * sizeof(char) + 6 * sizeof(int32_t) + data->pbData.size() + data->serviceFullName.size() + data->msgSeq.size() + data->errInfo.size();

    LOG_DEBUG << "encode pkLen = " << pkLen;
    char *buf = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buf;
    *temp = PB_START;
    temp++;

    int32_t pkLenNet = htonl(pkLen);
    memcpy(temp, &pkLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    int32_t msgSeqLen = data->msgSeq.size();
    LOG_DEBUG << "msgSeqLen= " << msgSeqLen;
    int32_t msgSeqLenNet = htonl(msgSeqLen);
    memcpy(temp, &msgSeqLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (msgSeqLen != 0) {
        memcpy(temp, &(data->msgSeq[0]), msgSeqLen);
        temp += msgSeqLen;
    }

    int32_t serviceFullNameLen = data->serviceFullName.size();
    LOG_DEBUG << "src serviceFullNameLen = " << serviceFullNameLen;
    int32_t serviceFullNameLenNet = htonl(serviceFullNameLen);
    memcpy(temp, &serviceFullNameLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (serviceFullNameLen != 0) {
        memcpy(temp, &(data->serviceFullName[0]), serviceFullNameLen);
        temp += serviceFullNameLen;
    }

    int32_t errCode = data->errCode;
    LOG_DEBUG << "errCode= " << errCode;
    int32_t errCodeNet = htonl(errCode);
    memcpy(temp, &errCodeNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    int32_t errInfoLen = data->errInfo.size();
    LOG_DEBUG << "errInfoLen= " << errInfoLen;
    int32_t errInfoLenNet = htonl(errInfoLen);
    memcpy(temp, &errInfoLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (errInfoLen != 0) {
        memcpy(temp, &(data->errInfo[0]), errInfoLen);
        temp += errInfoLen;
    }

    memcpy(temp, &(data->pbData[0]), data->pbData.size());
    temp += data->pbData.size();
    LOG_DEBUG << "pbData_len= " << data->pbData.size();

    int32_t checksum = 1;
    int32_t checksumNet = htonl(checksum);
    memcpy(temp, &checksumNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    *temp = PB_END;

    data->pkLen = pkLen;
    data->msgSeqLen = msgSeqLen;
    data->serviceNameLen = serviceFullNameLen;
    data->errInfoLen = errInfoLen;

    // checksum has not been implemented yet, directly skip chcksum
    data->checksum = checksum;
    data->encodeSucc_ = true;

    len = pkLen;

    return buf;
}

void PbCodeC::decode(TcpBuffer *buf, AbstractData *data)
{
    if (!buf || !data) {
        LOG_ERROR << "decode error! buf or data nullptr";
        return;
    }

    std::vector<char> temp = buf->getBufferVector();
    int startIndex = buf->readIndex();
    int endIndex = -1;
    int32_t pkLen = -1;

    bool parseFullPack = false;

    for (int i = startIndex; i < buf->writeIndex(); ++i) {
        // first find start
        if (temp[i] == PB_START) {
            if (i + 1 < buf->writeIndex()) {
                pkLen = getInt32FromNetByte(&temp[i + 1]);
                LOG_DEBUG << "prase pkLen =" << pkLen;
                int j = i + pkLen - 1;
                LOG_DEBUG << "j =" << j << ", i=" << i;

                // 完整的包还没读完，需要继续读，再回来解析
                if (j >= buf->writeIndex()) {
                    LOG_DEBUG << "recv package not complete, or PB_START find error, continue next parse";
                    continue;
                }
                if (temp[j] == PB_END) {
                    startIndex = i;
                    endIndex = j;
                    LOG_DEBUG << "parse succ, now break";
                    parseFullPack = true;
                    break;
                }
            }
        }
    }

    // 完整的包还没读完，需要继续读，再回来解析
    if (!parseFullPack) {
        LOG_DEBUG << "not parse full package, return";
        return;
    }

    buf->recycleRead(endIndex + 1 - startIndex);

    LOG_DEBUG << "buf size=" << buf->getBufferVector().size() << " rd=" << buf->readIndex() << " wd=" << buf->writeIndex();

    PbStruct *pbStruct = dynamic_cast<PbStruct *>(data);
    pbStruct->pkLen = pkLen;
    pbStruct->decodeSucc_ = false;

    int msgSeqLenIndex = startIndex + sizeof(char) + sizeof(int32_t);
    if (msgSeqLenIndex >= endIndex) {
        LOG_ERROR << "parse error, msgSeqLenIndex[" << msgSeqLenIndex << "] >= endIndex[" << endIndex << "]";
        // drop this error package
        return;
    }

    pbStruct->msgSeqLen = getInt32FromNetByte(&temp[msgSeqLenIndex]);
    if (pbStruct->msgSeqLen == 0) {
        LOG_ERROR << "prase error, msgSeq emptr";
        return;
    }

    LOG_DEBUG << "msgSeqLen= " << pbStruct->msgSeqLen;
    int msgSeqIndex = msgSeqLenIndex + sizeof(int32_t);
    LOG_DEBUG << "msgSeqIndex= " << msgSeqIndex;

    char msgSeq[50] = {0};

    memcpy(&msgSeq[0], &temp[msgSeqIndex], pbStruct->msgSeqLen);
    pbStruct->msgSeq = std::string(msgSeq);
    LOG_DEBUG << "msgSeq= " << pbStruct->msgSeq;

    int serviceNameLenIndex = msgSeqIndex + pbStruct->msgSeqLen;
    if (serviceNameLenIndex >= endIndex) {
        LOG_ERROR << "parse error, serviceNameLenIndex[" << serviceNameLenIndex << "] >= endIndex[" << endIndex << "]";
        // drop this error package
        return;
    }

    LOG_DEBUG << "serviceNameLenIndex = " << serviceNameLenIndex;
    int serviceNameIndex = serviceNameLenIndex + sizeof(int32_t);

    if (serviceNameIndex >= endIndex) {
        LOG_ERROR << "parse error, serviceNameIndex[" << serviceNameIndex << "] >= endIndex[" << endIndex << "]";
        return;
    }

    pbStruct->serviceNameLen = getInt32FromNetByte(&temp[serviceNameLenIndex]);

    if (pbStruct->serviceNameLen > pkLen) {
        LOG_ERROR << "parse error, serviceNameLen[" << pbStruct->serviceNameLen << "] >= pkLen [" << pkLen << "]";
        return;
    }
    LOG_DEBUG << "serviceNameLen = " << pbStruct->serviceNameLen;

    char serviceName[512] = {0};

    memcpy(&serviceName[0], &temp[serviceNameIndex], pbStruct->serviceNameLen);
    pbStruct->serviceFullName = std::string(serviceName);
    LOG_DEBUG << "serviceName = " << pbStruct->serviceFullName;

    int errCodeIndex = serviceNameIndex + pbStruct->serviceNameLen;
    pbStruct->errCode = getInt32FromNetByte(&temp[errCodeIndex]);

    int errInfoLenIndex = errCodeIndex + sizeof(int32_t);

    if (errInfoLenIndex >= endIndex) {
        LOG_ERROR << "parse error, errInfoLenIndex[" << errInfoLenIndex << "] >= endIndex[" << endIndex << "]";
        // drop this error package
        return;
    }
    pbStruct->errInfoLen = getInt32FromNetByte(&temp[errInfoLenIndex]);
    LOG_DEBUG << "errInfoLen = " << pbStruct->errInfoLen;
    int errInfoIndex = errInfoLenIndex + sizeof(int32_t);

    char errInfo[512] = {0};

    memcpy(&errInfo[0], &temp[errInfoIndex], pbStruct->errInfoLen);
    pbStruct->errInfo = std::string(errInfo);

    int pbDataLen = pbStruct->pkLen - pbStruct->serviceNameLen - pbStruct->msgSeqLen - pbStruct->errInfoLen - 2 * sizeof(char) - 6 * sizeof(int32_t);

    int pbDataIndex = errInfoIndex + pbStruct->errInfoLen;
    LOG_DEBUG << "pbDataLen= " << pbDataLen << ", pbIndex = " << pbDataIndex;

    if (pbDataIndex >= endIndex) {
        LOG_ERROR << "parse error, pbDataIndex[" << pbDataIndex << "] >= endIndex[" << endIndex << "]";
        return;
    }
    LOG_DEBUG << "pbDataIndex = " << pbDataIndex << ", pbData.length = " << pbDataLen;

    std::string pbDataStr(&temp[pbDataIndex], pbDataLen);
    pbStruct->pbData = pbDataStr;

    LOG_DEBUG << "decode succ, pkLen = " << pkLen << ", serviceName = " << pbStruct->serviceFullName;

    pbStruct->decodeSucc_ = true;
    data = pbStruct;
}

ProtocolType PbCodeC::getProtocolType()
{
    return Pb_Protocol;
}

}
