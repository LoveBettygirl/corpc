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
#include "msg_req.h"

namespace corpc {

static const char PB_START = 0x02; // start char
static const char PB_END = 0x03;   // end char
static const int MSG_REQ_LEN = 20; // default length of msg_req

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
    if (data->service_full_name.empty()) {
        LOG_ERROR << "parse error, service_full_name is empty";
        data->encodeSucc_ = false;
        return nullptr;
    }
    if (data->msg_req.empty()) {
        data->msg_req = MsgReqUtil::genMsgNumber();
        data->msg_req_len = data->msg_req.size();
        LOG_DEBUG << "generate msgno = " << data->msg_req;
    }

    int32_t pkLen = 2 * sizeof(char) + 6 * sizeof(int32_t) + data->pb_data.size() + data->service_full_name.size() + data->msg_req.size() + data->err_info.size();

    LOG_DEBUG << "encode pkLen = " << pkLen;
    char *buf = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buf;
    *temp = PB_START;
    temp++;

    int32_t pkLenNet = htonl(pkLen);
    memcpy(temp, &pkLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    int32_t msgReqLen = data->msg_req.size();
    LOG_DEBUG << "msgReqLen= " << msgReqLen;
    int32_t msgReqLenNet = htonl(msgReqLen);
    memcpy(temp, &msgReqLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (msgReqLen != 0) {
        memcpy(temp, &(data->msg_req[0]), msgReqLen);
        temp += msgReqLen;
    }

    int32_t serviceFullNameLen = data->service_full_name.size();
    LOG_DEBUG << "src serviceFullNameLen = " << serviceFullNameLen;
    int32_t serviceFullNameLenNet = htonl(serviceFullNameLen);
    memcpy(temp, &serviceFullNameLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (serviceFullNameLen != 0) {
        memcpy(temp, &(data->service_full_name[0]), serviceFullNameLen);
        temp += serviceFullNameLen;
    }

    int32_t errCode = data->err_code;
    LOG_DEBUG << "errCode= " << errCode;
    int32_t errCodeNet = htonl(errCode);
    memcpy(temp, &errCodeNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    int32_t errInfoLen = data->err_info.size();
    LOG_DEBUG << "errInfoLen= " << errInfoLen;
    int32_t errInfoLenNet = htonl(errInfoLen);
    memcpy(temp, &errInfoLenNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    if (errInfoLen != 0) {
        memcpy(temp, &(data->err_info[0]), errInfoLen);
        temp += errInfoLen;
    }

    memcpy(temp, &(data->pb_data[0]), data->pb_data.size());
    temp += data->pb_data.size();
    LOG_DEBUG << "pb_data_len= " << data->pb_data.size();

    int32_t checksum = 1;
    int32_t checksumNet = htonl(checksum);
    memcpy(temp, &checksumNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    *temp = PB_END;

    data->pk_len = pkLen;
    data->msg_req_len = msgReqLen;
    data->service_name_len = serviceFullNameLen;
    data->err_info_len = errInfoLen;

    // checksum has not been implemented yet, directly skip chcksum
    data->check_num = checksum;
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
    pbStruct->pk_len = pkLen;
    pbStruct->decodeSucc_ = false;

    int msgReqLenIndex = startIndex + sizeof(char) + sizeof(int32_t);
    if (msgReqLenIndex >= endIndex) {
        LOG_ERROR << "parse error, msgReqLenIndex[" << msgReqLenIndex << "] >= endIndex[" << endIndex << "]";
        // drop this error package
        return;
    }

    pbStruct->msg_req_len = getInt32FromNetByte(&temp[msgReqLenIndex]);
    if (pbStruct->msg_req_len == 0) {
        LOG_ERROR << "prase error, msg_req emptr";
        return;
    }

    LOG_DEBUG << "msg_req_len= " << pbStruct->msg_req_len;
    int msgReqIndex = msgReqLenIndex + sizeof(int32_t);
    LOG_DEBUG << "msgReqIndex= " << msgReqIndex;

    char msgReq[50] = {0};

    memcpy(&msgReq[0], &temp[msgReqIndex], pbStruct->msg_req_len);
    pbStruct->msg_req = std::string(msgReq);
    LOG_DEBUG << "msgReq= " << pbStruct->msg_req;

    int serviceNameLenIndex = msgReqIndex + pbStruct->msg_req_len;
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

    pbStruct->service_name_len = getInt32FromNetByte(&temp[serviceNameLenIndex]);

    if (pbStruct->service_name_len > pkLen) {
        LOG_ERROR << "parse error, service_name_len[" << pbStruct->service_name_len << "] >= pkLen [" << pkLen << "]";
        return;
    }
    LOG_DEBUG << "service_name_len = " << pbStruct->service_name_len;

    char serviceName[512] = {0};

    memcpy(&serviceName[0], &temp[serviceNameIndex], pbStruct->service_name_len);
    pbStruct->service_full_name = std::string(serviceName);
    LOG_DEBUG << "serviceName = " << pbStruct->service_full_name;

    int errCodeIndex = serviceNameIndex + pbStruct->service_name_len;
    pbStruct->err_code = getInt32FromNetByte(&temp[errCodeIndex]);

    int errInfoLenIndex = errCodeIndex + sizeof(int32_t);

    if (errInfoLenIndex >= endIndex) {
        LOG_ERROR << "parse error, errInfoLenIndex[" << errInfoLenIndex << "] >= endIndex[" << endIndex << "]";
        // drop this error package
        return;
    }
    pbStruct->err_info_len = getInt32FromNetByte(&temp[errInfoLenIndex]);
    LOG_DEBUG << "errInfoLen = " << pbStruct->err_info_len;
    int errInfoIndex = errInfoLenIndex + sizeof(int32_t);

    char errInfo[512] = {0};

    memcpy(&errInfo[0], &temp[errInfoIndex], pbStruct->err_info_len);
    pbStruct->err_info = std::string(errInfo);

    int pbDataLen = pbStruct->pk_len - pbStruct->service_name_len - pbStruct->msg_req_len - pbStruct->err_info_len - 2 * sizeof(char) - 6 * sizeof(int32_t);

    int pbDataIndex = errInfoIndex + pbStruct->err_info_len;
    LOG_DEBUG << "pbDataLen= " << pbDataLen << ", pbIndex = " << pbDataIndex;

    if (pbDataIndex >= endIndex) {
        LOG_ERROR << "parse error, pbDataIndex[" << pbDataIndex << "] >= endIndex[" << endIndex << "]";
        return;
    }
    LOG_DEBUG << "pbDataIndex = " << pbDataIndex << ", pbData.length = " << pbDataLen;

    std::string pbDataStr(&temp[pbDataIndex], pbDataLen);
    pbStruct->pb_data = pbDataStr;

    LOG_DEBUG << "decode succ, pkLen = " << pkLen << ", serviceName = " << pbStruct->service_full_name;

    pbStruct->decodeSucc_ = true;
    data = pbStruct;
}

ProtocolType PbCodeC::getProtocolType()
{
    return Pb_Protocol;
}

}
