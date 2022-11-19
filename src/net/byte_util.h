#ifndef CORPC_NET_BYTE_UTIL_H
#define CORPC_NET_BYTE_UTIL_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

namespace corpc {

inline int32_t getInt32FromNetByte(const char *buf)
{
    int32_t temp;
    memcpy(&temp, buf, sizeof(temp));
    return ntohl(temp);
}

}

#endif