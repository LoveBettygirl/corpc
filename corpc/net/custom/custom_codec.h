#ifndef CORPC_NET_CUSTOM_CUSTOM_CODEC_H
#define CORPC_NET_CUSTOM_CUSTOM_CODEC_H

#include "corpc/net/abstract_data.h"
#include "corpc/net/abstract_codec.h"

namespace corpc {

// Override this class to use your custom protocol
class CustomCodeC : public AbstractCodeC {
public:
    CustomCodeC() {}
    ~CustomCodeC() {}

    void encode(TcpBuffer *buf, AbstractData *data) override;
    void decode(TcpBuffer *buf, AbstractData *data) override;
    ProtocolType getProtocolType() override;
};

}

#endif