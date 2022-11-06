#include <string>
#include <sstream>
#include "http_define.h"

namespace corpc {

std::string gCRLF = "\r\n";
std::string gCRLF_DOUBLE = "\r\n\r\n";

std::string contentTypeText = "text/html;charset=utf-8";
const char *defaultHtmlTemplate = "<html><body><h1>%s</h1><p>%s</p></body></html>";

const char *httpCodeToString(const int code)
{
    switch (code) {
    case HTTP_OK:
        return "OK";

    case HTTP_BADREQUEST:
        return "Bad Request";

    case HTTP_FORBIDDEN:
        return "Forbidden";

    case HTTP_NOTFOUND:
        return "Not Found";

    case HTTP_INTERNALSERVERERROR:
        return "Internal Server Error";

    default:
        return "Unknown code";
    }
}

std::string HttpHeaderComm::getValue(const std::string &key)
{
    return maps_[key.c_str()];
}

int HttpHeaderComm::getHeaderTotalLength()
{
    int len = 0;
    for (auto it : maps_) {
        len += it.first.size() + 1 + it.second.size() + 2;
    }
    return len;
}

void HttpHeaderComm::setKeyValue(const std::string &key, const std::string &value)
{
    maps_[key.c_str()] = value;
}

// 生成http报文头部
std::string HttpHeaderComm::toHttpString()
{
    std::stringstream ss;
    for (auto it : maps_) {
        ss << it.first << ":" << it.second << "\r\n";
    }
    return ss.str();
}

}