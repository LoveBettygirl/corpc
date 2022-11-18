#ifndef ABSTRACT_SLOT_H
#define ABSTRACT_SLOT_H

#include <memory>
#include <functional>

namespace corpc {

// 模板参数T是TcpConnection，一个TcpConnection和AbstractSlot一一对应
// 封装了TcpConnection和销毁时对应的回调函数（销毁tcp连接）
template <class T>
class AbstractSlot {
public:
    typedef std::shared_ptr<AbstractSlot> ptr;
    typedef std::weak_ptr<T> weakPtr;
    typedef std::shared_ptr<T> sharedPtr;

    AbstractSlot(weakPtr ptr, std::function<void(sharedPtr)> cb) : weakPtr_(ptr), cb_(cb) {}
    ~AbstractSlot() {
        // 可用来判断连接是否被释放
        sharedPtr ptr = weakPtr_.lock();
        if (ptr) {
            cb_(ptr);
        }
    }

private:
    weakPtr weakPtr_; // 必须用weak_ptr，防止TcpTimeWheel中的队列中的智能指针和该对象的智能指针出现循环引用
    std::function<void(sharedPtr)> cb_; // 析构时调用
};

}
#endif