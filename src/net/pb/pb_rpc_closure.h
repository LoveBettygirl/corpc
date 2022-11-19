#ifndef CORPC_NET_PB_PB_RPC_CLOSURE_H
#define CORPC_NET_PB_PB_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>
#include <functional>
#include <memory>

namespace corpc {

class PbRpcClosure : public google::protobuf::Closure {
public:
    typedef std::shared_ptr<PbRpcClosure> ptr;
    explicit PbRpcClosure(std::function<void()> cb) : cb_(cb) {}

    ~PbRpcClosure() = default;

    void Run() override {
        if (cb_) {
            cb_();
        }
    }

private:
    std::function<void()> cb_{nullptr};
};

}

#endif