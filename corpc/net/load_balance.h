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

enum class LoadBalanceCategory {
    // 随机算法
    Random,
    // 轮询算法
    Round,
    // 一致性哈希
    ConsistentHash
};

class LoadBalanceStrategy {
public:
    using ptr = std::shared_ptr<LoadBalanceStrategy>;
    virtual ~LoadBalanceStrategy() {}

    // 选择节点
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
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_index >= (int)list.size()) {
            m_index = 0;
        }
        return list[m_index++];
    }
private:
    int m_index = 0;
    std::mutex m_mutex;
};

// 一致性哈希
class ConsistentHashLoadBalanceStrategy : public LoadBalanceStrategy {
public:
    std::string select(std::vector<std::string> &list, const std::string &invocation) {
        size_t identityHashCode = hashCode(list);
        std::lock_guard<std::mutex> lock(m_mutex); // C++的map不是线程安全的
        auto it = selectors.find(invocation);
        std::shared_ptr<ConsistentHashSelector> selector;
        // check for updates
        if (it == selectors.end() || it->second->identityHashCode != identityHashCode) {
            selectors.insert({invocation, std::make_shared<ConsistentHashSelector>(list, 160, identityHashCode)});
            selector = selectors[invocation];
        }
        else {
            selector = it->second;
        }
        return selector->select(invocation);
    }
private:
    class ConsistentHashSelector {
    public:
        ConsistentHashSelector(std::vector<std::string> &invokers, int replicaNumber, int identityHashCode) : identityHashCode(identityHashCode) {
            for (const std::string &invoker : invokers) {
                for (int i = 0; i < replicaNumber / 4; i++) {
                    std::vector<uint8_t> digest = md5(invoker + std::to_string(i));
                    for (int h = 0; h < 4; h++) {
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

    std::unordered_map<std::string, std::shared_ptr<ConsistentHashSelector>> selectors;
    std::mutex m_mutex;
};

class LoadBalance {
public:
    static LoadBalanceStrategy::ptr queryStrategy(LoadBalanceCategory category) {
        switch (category) {
            case LoadBalanceCategory::Random:
                return s_randomStrategy;
            case LoadBalanceCategory::Round:
                return s_RoundStrategy;
            case LoadBalanceCategory::ConsistentHash:
                return s_consistentHashStrategy;
            default:
                return s_randomStrategy;
        }
    }
    static std::string strategy2Str(LoadBalanceCategory category) {
        switch (category) {
            case LoadBalanceCategory::Random:
                return "Random";
            case LoadBalanceCategory::Round:
                return "Round";
            case LoadBalanceCategory::ConsistentHash:
                return "ConsistentHash";
            default:
                return "Random";
        }
    }
    static LoadBalanceCategory str2Strategy(const std::string &str) {
        if (str == "Round")
            return LoadBalanceCategory::Round;
        else if (str == "ConsistentHash")
            return LoadBalanceCategory::ConsistentHash;
        return LoadBalanceCategory::Random;
    }
private:
    static LoadBalanceStrategy::ptr s_randomStrategy;
    static LoadBalanceStrategy::ptr s_RoundStrategy;
    static LoadBalanceStrategy::ptr s_consistentHashStrategy;
};
LoadBalanceStrategy::ptr LoadBalance::s_randomStrategy = std::make_shared<RandomLoadBalanceStrategy>();
LoadBalanceStrategy::ptr LoadBalance::s_RoundStrategy = std::make_shared<RoundLoadBalanceStrategy>();
LoadBalanceStrategy::ptr LoadBalance::s_consistentHashStrategy = std::make_shared<ConsistentHashLoadBalanceStrategy>();

#endif