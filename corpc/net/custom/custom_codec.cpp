#include "corpc/net/custom/custom_codec.h"
#include "corpc/net/custom/custom_data.h"
#include "corpc/common/log.h"
#include <string>

namespace corpc {

void CustomCodeC::encode(TcpBuffer *buf, AbstractData *data)
{
    LOG_DEBUG << "test encode";
    CustomStruct *customStruct = dynamic_cast<CustomStruct *>(data);
    customStruct->encodeSucc_ = false;

    LOG_DEBUG << "encode protocol data is: " << customStruct->protocolData;

    buf->writeToBuffer(customStruct->protocolData.c_str(), customStruct->protocolData.size());
    LOG_DEBUG << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
    customStruct->encodeSucc_ = true;
    LOG_DEBUG << "test encode end";
}

void CustomCodeC::decode(TcpBuffer *buf, AbstractData *data)
{
    LOG_DEBUG << "test custom decode start";
    if (!buf || !data) {
        LOG_ERROR << "decode error! buf or data nullptr";
        return;
    }
    CustomStruct *customStruct = dynamic_cast<CustomStruct *>(data);
    if (!customStruct) {
        LOG_ERROR << "not CustomStruct type";
        return;
    }

    customStruct->protocolData = buf->getBufferString();

    customStruct->decodeSucc_ = true;
    data = customStruct;

    LOG_DEBUG << "test custom decode end";
}

ProtocolType CustomCodeC::getProtocolType()
{
    return Custom_Protocol;
}

}