#ifndef CORPC_COMMOM_NONCOPYABLE_H
#define CORPC_COMMOM_NONCOPYABLE_H

namespace corpc {

class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable& noncopyable) = delete;
    Noncopyable& operator=(const Noncopyable& noncopyable) = delete;
};

}

#endif