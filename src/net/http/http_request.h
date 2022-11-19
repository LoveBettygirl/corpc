#ifndef CORPC_NET_HTTP_HTTP_REQUEST_H
#define CORPC_NET_HTTP_HTTP_REQUEST_H

#include <string>
#include <memory>
#include <map>

#include "abstract_data.h"
#include "http_define.h"

namespace corpc {

class HttpRequest : public AbstractData {
public:
    typedef std::shared_ptr<HttpRequest> ptr;

public:
    HttpMethod requestMethod_;
    std::string requestPath_;
    std::string requestQuery_;
    std::string requestVersion_;
    HttpRequestHeader requestHeader_;
    std::string requestBody_;

    std::map<std::string, std::string> queryMaps_;
};

}

#endif