#ifndef BYTE_UTIL_H
#define BYTE_UTIL_H

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