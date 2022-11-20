#ifndef CORPC_NET_HTTP_HTTP_CODEC_H
#define CORPC_NET_HTTP_HTTP_CODEC_H

#include <map>
#include <string>
#include "corpc/net/abstract_data.h"
#include "corpc/net/abstract_codec.h"
#include "corpc/net/http/http_request.h"

namespace corpc {

class HttpCodeC : public AbstractCodeC {
public:
    HttpCodeC();
    ~HttpCodeC();

    void encode(TcpBuffer *buf, AbstractData *data) override;
    void decode(TcpBuffer *buf, AbstractData *data) override;
    ProtocolType getProtocolType() override;

private:
    bool parseHttpRequestLine(HttpRequest *requset, const std::string &temp);
    bool parseHttpRequestHeader(HttpRequest *requset, const std::string &temp);
    bool parseHttpRequestContent(HttpRequest *requset, const std::string &temp);
};

}

#endif
