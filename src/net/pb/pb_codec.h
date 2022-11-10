#ifndef PB_CODEC_H
#define PB_CODEC_H

#include <stdint.h>
#include "abstract_codec.h"
#include "abstract_data.h"
#include "pb_data.h"

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