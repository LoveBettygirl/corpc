#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include "corpc/net/net_address.h"
#include "corpc/common/log.h"

namespace corpc {

bool IPAddress::checkValidIPAddr(const std::string &addr)
{
    size_t i = addr.find_first_of(":");
    if (i == addr.npos) {
        return false;
    }
    int port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());
    if (port < 0 || port > 65536) {
        return false;
    }

    if (inet_addr(addr.substr(0, i).c_str()) == INADDR_NONE) {
        return false;
    }
    return true;
}

IPAddress::IPAddress(const std::string &ip, uint16_t port)
    : ip_(ip), port_(port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip_.c_str());
    addr_.sin_port = htons(port_);

    LOG_DEBUG << "create ipv4 address succ [" << toString() << "]";
}

IPAddress::IPAddress(sockaddr_in addr) : addr_(addr)
{
    ip_ = std::string(inet_ntoa(addr_.sin_addr));
    port_ = ntohs(addr_.sin_port);
    LOG_DEBUG << "ip[" << ip_ << "], port[" << port_ << "]";
}

IPAddress::IPAddress(const std::string &addr)
{
    size_t i = addr.find_first_of(":");
    if (i == addr.npos) {
        LOG_ERROR << "invalid addr[" << addr << "]";
        return;
    }
    ip_ = addr.substr(0, i);
    port_ = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());

    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip_.c_str());
    addr_.sin_port = htons(port_);
    LOG_DEBUG << "create ipv4 address succ [" << toString() << "]";
}

IPAddress::IPAddress(uint16_t port) : port_(port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port_);

    LOG_DEBUG << "create ipv4 address succ [" << toString() << "]";
}

int IPAddress::getFamily() const
{
    return addr_.sin_family;
}

sockaddr *IPAddress::getSockAddr()
{
    return reinterpret_cast<sockaddr *>(&addr_);
}

std::string IPAddress::toString() const
{
    std::stringstream ss;
    ss << ip_ << ":" << port_;
    return ss.str();
}

socklen_t IPAddress::getSockLen() const
{
    return sizeof(addr_);
}

UnixDomainAddress::UnixDomainAddress(std::string &path) : path_(path)
{

    memset(&addr_, 0, sizeof(addr_));
    unlink(path_.c_str());
    addr_.sun_family = AF_UNIX;
    strcpy(addr_.sun_path, path_.c_str());
}
UnixDomainAddress::UnixDomainAddress(sockaddr_un addr) : addr_(addr)
{
    path_ = addr_.sun_path;
}

int UnixDomainAddress::getFamily() const
{
    return addr_.sun_family;
}

sockaddr *UnixDomainAddress::getSockAddr()
{
    return reinterpret_cast<sockaddr *>(&addr_);
}

socklen_t UnixDomainAddress::getSockLen() const
{
    return sizeof(addr_);
}

std::string UnixDomainAddress::toString() const
{
    return path_;
}

}
