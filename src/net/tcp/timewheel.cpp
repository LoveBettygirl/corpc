#include <queue>
#include <vector>
#include "abstract_slot.h"
#include "timewheel.h"
#include "timer.h"

namespace corpc {

TcpTimeWheel::TcpTimeWheel(EventLoop *loop, int bucketCount, int interval /*= 10*/)
    : loop_(loop), bucketCount_(bucketCount), interval_(interval)
{

    for (int i = 0; i < bucketCount; ++i) {
        std::vector<TcpConnectionSlot::ptr> temp;
        wheel_.push(temp);
    }

    event_ = std::make_shared<TimerEvent>(interval_ * 1000, true, std::bind(&TcpTimeWheel::loopFunc, this));
    loop_->getTimer()->addTimerEvent(event_);
}

TcpTimeWheel::~TcpTimeWheel()
{
    loop_->getTimer()->delTimerEvent(event_);
}

void TcpTimeWheel::loopFunc()
{
    // 每次定时事件触发，就弹出队头，插入新队尾
    // 弹出的队头，如果里面所有的slot的shared_ptr引用计数为0，随着slot析构函数调用，调用回调函数cb_，tcp连接就释放了
    LOG_DEBUG << "pop src bucket";
    wheel_.pop();
    std::vector<TcpConnectionSlot::ptr> temp;
    wheel_.push(temp);
    LOG_DEBUG << "push new bucket";
}

void TcpTimeWheel::fresh(TcpConnectionSlot::ptr slot)
{
    // 当一个tcp连接又开始有数据，需要刷新对应的slot在队列中的位置
    // 无需遍历队列检查slot是否在队列中，直接将其插入队尾即可
    // slot的shared_ptr引用计数为0，tcp连接就自动释放了
    LOG_DEBUG << "fresh connection";
    wheel_.back().emplace_back(slot);
}

}