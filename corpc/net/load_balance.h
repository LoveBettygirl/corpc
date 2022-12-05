#ifndef CORPC_NET_LOAD_BALANCE_H
#define CORPC_NET_LOAD_BALANCE_H

#include <memory>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <iostream>
#include "corpc/common/md5.h"
#include "corpc/common/const.h"

namespace corpc {

class LoadBalanceStrategy {
public:
    using ptr = std::shared_ptr<LoadBalanceStrategy>;
    virtual ~LoadBalanceStrategy() {}

    // 选择节点（一致性哈希需要用到请求的服务名和参数信息，其他方法不需要）
    virtual std::string select(std::vector<std::string> &list, const std::string &invocation) = 0;
};

// 随机策略
class RandomLoadBalanceStrategy : public LoadBalanceStrategy {
public:
    std::string select(std::vector<std::string> &list, const std::string &invocation) {
        srand((unsigned)time(nullptr));
        return list[rand() % list.size()];
    }
};

// 轮询策略
class RoundLoadBalanceStrategy : public LoadBalanceStrategy {
public:
    std::string select(std::vector<std::string>& list, const std::string &invocation) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index_ >= (int)list.size()) {
            index_ = 0;
        }
        return list[index_++];
    }
private:
    int index_ = 0;
    std::mutex mutex_;
};

// 一致性哈希
class ConsistentHashLoadBalanceStrategy : public LoadBalanceStrategy {
public:
    std::string select(std::vector<std::string> &list, const std::string &invocation) {
        size_t identityHashCode = hashCode(list);
        std::lock_guard<std::mutex> lock(mutex_); // C++的map不是线程安全的
        auto it = selectors_.find(invocation); // 对每个服务创建一个选择器
        std::shared_ptr<ConsistentHashSelector> selector;
        // check for updates
        // 检查invokers列表是否扩容或者缩容，如果不存在对应选择器，或者出现了扩缩容（会导致服务器列表的hash值不一致）则初始化选择器
        if (it == selectors_.end() || it->second->identityHashCode != identityHashCode) {
            selectors_.insert({invocation, std::make_shared<ConsistentHashSelector>(list, 160, identityHashCode)});
            selector = selectors_[invocation];
        }
        else {
            selector = it->second;
        }
        return selector->select(invocation); // 同一个参数，请求会打到同一个虚拟节点上
    }
private:
    class ConsistentHashSelector {
    public:
        ConsistentHashSelector(std::vector<std::string> &invokers, int replicaNumber, int identityHashCode) : identityHashCode(identityHashCode) {
            // 创建ConsistentHashSelector时会生成所有虚拟节点，
            for (const std::string &invoker : invokers) {
                // 每个实际节点扩展为160个虚拟节点，每4个为一组
                for (int i = 0; i < replicaNumber / 4; i++) {
                    // 同组的虚拟节点的md5相同
                    std::vector<uint8_t> digest = md5(invoker + std::to_string(i));
                    // 根据md5算法为每4个结点生成一个消息摘要，摘要长为16字节128位。 md5就是一个长16字节占128位的bit数组
                    // md5共16字节，这一个组里的每个虚拟节点占用生成的md5数组中的4个字节
                    for (int h = 0; h < 4; h++) {
                        // hash运算，分别对0—3,4—7,8—11,12—15的字节进行位运算。
                        size_t m = hash(digest, h);
                        virtualInvokers[m] = invoker;
                    }
                }
            }
        }

        std::map<size_t, std::string> virtualInvokers;
        const int identityHashCode;

        static size_t hash(const std::vector<uint8_t> &digest, int number) {
            return (((size_t) (digest[3 + number * 4] & 0xFF) << 24)
                    | ((size_t) (digest[2 + number * 4] & 0xFF) << 16)
                    | ((size_t) (digest[1 + number * 4] & 0xFF) << 8)
                    | (digest[number * 4] & 0xFF))
                    & 0xFFFFFFFFL;
        }

        static std::vector<uint8_t> md5(const std::string &key) {
            MD5 obj;
            return obj.digest(key);
        }

        std::string select(const std::string &rpcServiceKey) {
            std::vector<uint8_t> digest = md5(rpcServiceKey);
            return selectForKey(hash(digest, 0));
        }

        std::string selectForKey(size_t hashCode) {
            auto it = virtualInvokers.lower_bound(hashCode);
            if (it == virtualInvokers.end()) {
                it = virtualInvokers.begin();
            }
            return it->second;
        }
    };

    // 计算vector的hashCode
    size_t hashCode(const std::vector<std::string> &list) {
        size_t result = 1;
        for (const std::string & item : list) {
            result = 31 * result + std::hash<std::string>()(item);
        }
        return result;
    }

    std::unordered_map<std::string, std::shared_ptr<ConsistentHashSelector>> selectors_;
    std::mutex mutex_;
};

class LoadBalance {
public:
    static LoadBalanceStrategy::ptr queryStrategy(LoadBalanceCategory category);
    static std::string strategy2Str(LoadBalanceCategory category);
    static LoadBalanceCategory str2Strategy(const std::string &str);
private:
    static LoadBalanceStrategy::ptr s_randomStrategy;
    static LoadBalanceStrategy::ptr s_RoundStrategy;
    static LoadBalanceStrategy::ptr s_consistentHashStrategy;
};

}

#endif