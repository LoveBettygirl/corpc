#ifndef PB_DATA_H
#define PB_DATA_H

#include <cstdint>
#include <vector>
#include <string>
#include "abstract_data.h"

namespace corpc {

class PbStruct : public AbstractData {
public:
    typedef std::shared_ptr<PbStruct> ptr;
    PbStruct() = default;
    ~PbStruct() = default;
    PbStruct(const PbStruct &) = default;
    PbStruct &operator=(const PbStruct &) = default;
    PbStruct(PbStruct &&) = default;
    PbStruct &operator=(PbStruct &&) = default;

    /**
     *  min of package is: 1 + 4 + 4 + 4 + 4 + 4 + 4 + 1 = 26 bytes
     */

    // char start;                      // indentify start of protocal data
    int32_t pkLen{0};             // len of all package (include start char and end char)
    int32_t msgSeqLen{0};        // len of msgSeq
    std::string msgSeq;           // identify a request
    int32_t serviceNameLen{0};   // len of service full name
    std::string serviceFullName; // service full name, like QueryService.queryName
    int32_t errCode{0};           // errCode, 0 -- call rpc success, otherwise -- call rpc failed. it only be seted by RpcController
    int32_t errInfoLen{0};       // len of errInfo
    std::string errInfo;          // errInfo, empty -- call rpc success, otherwise -- call rpc failed, it will display details of reason why call rpc failed. it only be seted by RpcController
    std::string pbData;           // business pb data
    int32_t checksum{-1};         // checksum of all package. to check legality of data
    // char end;                        // identify end of protocal data
};

}

#endif
