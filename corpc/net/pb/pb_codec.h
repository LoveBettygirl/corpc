#ifndef CORPC_NET_PB_PB_CODEC_H
#define CORPC_NET_PB_PB_CODEC_H

#include <cstdint>
#include "corpc/net/abstract_codec.h"
#include "corpc/net/abstract_data.h"
#include "corpc/net/pb/pb_data.h"

namespace corpc {

class PbCodeC : public AbstractCodeC {
public:
    PbCodeC();

    ~PbCodeC();

    void encode(TcpBuffer *buf, AbstractData *data) override;
    void decode(TcpBuffer *buf, AbstractData *data) override;
    virtual ProtocolType getProtocolType() override;

    const char *encodePbData(PbStruct *data, int &len);
};

}

#endif