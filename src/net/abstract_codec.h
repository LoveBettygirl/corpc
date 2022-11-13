#ifndef ABSTRACT_CODEC_H
#define ABSTRACT_CODEC_H

#include <string>
#include <memory>
#include "tcp_buffer.h"
#include "abstract_data.h"

namespace corpc {

enum ProtocolType {
    Http_Protocol = 1,
    Pb_Protocol = 2,
};

class AbstractCodeC {
public:
    typedef std::shared_ptr<AbstractCodeC> ptr;

    AbstractCodeC() {}
    virtual ~AbstractCodeC() {}

    virtual void encode(TcpBuffer *buf, AbstractData *data) = 0;
    virtual void decode(TcpBuffer *buf, AbstractData *data) = 0;
    virtual ProtocolType getProtocolType() = 0;
};

}

#endif
