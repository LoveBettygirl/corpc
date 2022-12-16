#include <algorithm>
#include <sstream>
#include "corpc/net/http/http_codec.h"
#include "corpc/common/log.h"
#include "corpc/common/string_util.h"
#include "corpc/net/abstract_data.h"
#include "corpc/net/abstract_codec.h"
#include "corpc/net/http/http_request.h"
#include "corpc/net/http/http_response.h"

namespace corpc {

HttpCodeC::HttpCodeC()
{
}

HttpCodeC::~HttpCodeC()
{
}

// HttpResponse对象转http响应报文
void HttpCodeC::encode(TcpBuffer *buf, AbstractData *data)
{
    LOG_DEBUG << "test encode";
    HttpResponse *response = dynamic_cast<HttpResponse *>(data);
    response->encodeSucc_ = false;

    std::stringstream ss;
    ss << response->responseVersion_ << " " << response->responseCode_ << " "
        << response->responseInfo_ << gCRLF
        << response->responseHeader_.toHttpString()
        << gCRLF
        << response->responseBody_;
    std::string httpRes = ss.str();
    LOG_DEBUG << "encode http response is: " << httpRes;

    buf->writeToBuffer(httpRes.c_str(), httpRes.size());
    LOG_DEBUG << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
    response->encodeSucc_ = true;
    LOG_DEBUG << "test encode end";
}

// http请求报文转HttpRequest对象
void HttpCodeC::decode(TcpBuffer *buf, AbstractData *data)
{
    LOG_DEBUG << "test http decode start";
    std::string strs = "";
    if (!buf || !data) {
        LOG_ERROR << "decode error! buf or data nullptr";
        return;
    }
    HttpRequest *request = dynamic_cast<HttpRequest *>(data);
    if (!request) {
        LOG_ERROR << "not HttpRequest type";
        return;
    }

    strs = buf->getBufferString();

    bool isParseRequestLine = false;
    bool isParseRequestHeader = false;
    bool isParseRequestContent = false;
    int readSize = 0;
    std::string temp(strs);
    LOG_DEBUG << "pending to parse str:" << temp << ", total size =" << temp.size();
    int len = temp.size();
    while (true) {
        // 解析请求行
        if (!isParseRequestLine) {
            size_t i = temp.find(gCRLF);
            if (i == temp.npos) {
                LOG_DEBUG << "not found CRLF in buffer";
                return;
            }
            // 请求行内容还没完全读到应用层缓冲区中
            if (i == temp.size() - 2) {
                LOG_DEBUG << "need to read more data";
                return;
            }
            isParseRequestLine = parseHttpRequestLine(request, temp.substr(0, i));
            if (!isParseRequestLine) {
                return;
            }
            temp = temp.substr(i + 2, len - 2 - i);
            len = temp.size();
            readSize = readSize + i + 2;
        }

        // 解析请求头
        if (!isParseRequestHeader) {
            size_t j = temp.find(gCRLF_DOUBLE);
            if (j == temp.npos) {
                LOG_DEBUG << "not found CRLF CRLF in buffer";
                return;
            }
            isParseRequestHeader = parseHttpRequestHeader(request, temp.substr(0, j));
            if (!isParseRequestHeader) {
                return;
            }
            temp = temp.substr(j + 4, len - 4 - j);
            len = temp.size();
            readSize = readSize + j + 4;
        }

        // 解析请求体
        if (!isParseRequestContent) {
            auto it = request->requestHeader_.maps_.find("Content-Length");
            int contentLen = 0;
            if (it != request->requestHeader_.maps_.end()) {
                contentLen = std::stoi(it->second);
            }
            // 请求体内容还没完全读到应用层缓冲区中
            if ((int)strs.size() - readSize < contentLen) {
                LOG_DEBUG << "need to read more data";
                return;
            }
            if (request->requestMethod_ == POST && contentLen != 0) {
                isParseRequestContent = parseHttpRequestContent(request, temp.substr(0, contentLen));
                if (!isParseRequestContent) {
                    return;
                }
                readSize = readSize + contentLen;
            }
            else {
                isParseRequestContent = true;
            }
        }

        // 已正常解析完http请求
        if (isParseRequestLine && isParseRequestHeader && isParseRequestHeader) {
            LOG_DEBUG << "parse http request success, read size is " << readSize << " bytes";
            buf->recycleRead(readSize);
            break;
        }
    }

    request->decodeSucc_ = true;
    data = request;

    LOG_DEBUG << "test http decode end";
}

bool HttpCodeC::parseHttpRequestLine(HttpRequest *requset, const std::string &temp)
{
    size_t s1 = temp.find_first_of(" ");
    size_t s2 = temp.find_last_of(" ");

    if (s1 == temp.npos || s2 == temp.npos || s1 == s2) {
        LOG_ERROR << "error read Http Requser Line, space is not 2";
        return false;
    }
    std::string method = temp.substr(0, s1);
    std::transform(method.begin(), method.end(), method.begin(), toupper);
    if (method == "GET") {
        requset->requestMethod_ = HttpMethod::GET;
    }
    else if (method == "POST") {
        requset->requestMethod_ = HttpMethod::POST;
    }
    else {
        LOG_ERROR << "parse http request request line error, not support http method:" << method;
        return false;
    }

    std::string version = temp.substr(s2 + 1, temp.size() - s2 - 1);
    std::transform(version.begin(), version.end(), version.begin(), toupper);
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        LOG_ERROR << "parse http request request line error, not support http version:" << version;
        return false;
    }
    requset->requestVersion_ = version;

    std::string url = temp.substr(s1 + 1, s2 - s1 - 1);
    size_t j = url.find("://");

    if (j != url.npos && j + 3 >= url.size()) {
        LOG_ERROR << "parse http request request line error, bad url:" << url;
        return false;
    }
    int l = 0;
    if (j == url.npos) {
        LOG_DEBUG << "url only have path, url is" << url;
    }
    else {
        url = url.substr(j + 3, s2 - s1 - j - 4);
        LOG_DEBUG << "delete http prefix, url = " << url;
        j = url.find_first_of("/");
        l = url.size();
        if (j == url.npos || j == url.size() - 1) {
            LOG_DEBUG << "http request root path, and query is empty";
            return true;
        }
        url = url.substr(j, l - j);
    }

    l = url.size();
    j = url.find_first_of("?");
    if (j == url.npos) {
        requset->requestPath_ = url;
        LOG_DEBUG << "http request path:" << requset->requestPath_ << "and query is empty";
        return true;
    }
    requset->requestPath_ = url.substr(0, j);
    requset->requestQuery_ = url.substr(j + 1, l - j - 1);
    LOG_DEBUG << "http request path:" << requset->requestPath_ << ", and query:" << requset->requestQuery_;
    StringUtil::splitStrToMap(requset->requestQuery_, "&", "=", requset->queryMaps_);
    return true;
}

bool HttpCodeC::parseHttpRequestHeader(HttpRequest *requset, const std::string &str)
{
    if (str.empty() || str.size() < 4 || str == gCRLF_DOUBLE) {
        return true;
    }
    std::string temp = str;
    StringUtil::splitStrToMap(temp, gCRLF, ":", requset->requestHeader_.maps_);
    return true;
}

bool HttpCodeC::parseHttpRequestContent(HttpRequest *requset, const std::string &str)
{
    if (str.empty()) {
        return true;
    }
    requset->requestBody_ = str;
    return true;
}

ProtocolType HttpCodeC::getProtocolType()
{
    return Http_Protocol;
}

}
