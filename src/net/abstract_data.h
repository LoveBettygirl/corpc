#ifndef ABSTRACT_DATA_H
#define ABSTRACT_DATA_H

namespace corpc {

class AbstractData {
public:
    AbstractData() = default;
    virtual ~AbstractData(){};

    bool decodeSucc_{false};
    bool encodeSucc_{false};
};

}

#endif