#ifndef CORPC_NET_HTTP_HTTP_RESPONSE_H
#define CORPC_NET_HTTP_HTTP_RESPONSE_H

#include <string>
#include <memory>

#include "corpc/net/abstract_data.h"
#include "corpc/net/http/http_define.h"

namespace corpc {

class HttpResponse : public AbstractData {
public:
    typedef std::shared_ptr<HttpResponse> ptr;

public:
    std::string responseVersion_;
    int responseCode_;
    std::string responseInfo_;
    HttpResponseHeader responseHeader_;
    std::string responseBody_;
};

}

#endif