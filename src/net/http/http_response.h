#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <memory>

#include "abstract_data.h"
#include "http_define.h"

namespace corpc
{

class HttpResponse : public AbstractData {
public:
    typedef std::shared_ptr<HttpResponse> ptr;

public:
    std::string responseVersion_;
    int responseCode_;
    std::string responseInfo_;
    std::string responseBody_;
};

}

#endif