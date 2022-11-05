#ifndef HTTP_DEFINE_H
#define HTTP_DEFINE_H

#include <string>
#include <map>

namespace corpc {

extern std::string gCRLF;
extern std::string gCRLF_DOUBLE;

extern std::string contentTypeText;
extern const char *defaultHtmlTemplate;

enum HttpMethod {
    GET = 1,
    POST = 2,
};

enum HttpCode {
    HTTP_OK = 200,
    HTTP_BADREQUEST = 400,
    HTTP_FORBIDDEN = 403,
    HTTP_NOTFOUND = 404,
    HTTP_INTERNALSERVERERROR = 500,
};

const char *httpCodeToString(const int code);

class HttpHeaderComm {
public:
    HttpHeaderComm() = default;
    virtual ~HttpHeaderComm() = default;

    int getHeaderTotalLength();
    std::string getValue(const std::string &key);
    void setKeyValue(const std::string &key, const std::string &value);
    std::string toHttpString();

public:
    std::map<std::string, std::string> maps_;
};

}

#endif