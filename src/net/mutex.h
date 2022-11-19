#ifndef CORPC_NET_MUTEX_H
#define CORPC_NET_MUTEX_H

#include <pthread.h>
#include <memory>
#include <queue>
#include <mutex>

// this file copy form sylar

namespace corpc {

template <class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T &mutex)
        : mutex_(mutex) {
        mutex_.lock();
        locked_ = true;
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!locked_) {
            mutex_.lock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    /// mutex
    T &mutex_;
    /// 是否已上锁
    bool locked_;
};

template <class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T &mutex)
        : mutex_(mutex) {
        mutex_.rdlock();
        locked_ = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!locked_) {
            mutex_.rdlock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    /// mutex
    T &mutex_;
    /// 是否已上锁
    bool locked_;
};

/**
 * @brief 局部写锁模板实现
 */
template <class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T &mutex)
        : mutex_(mutex) {
        mutex_.wrlock();
        locked_ = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!locked_) {
            mutex_.wrlock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    T &mutex_;
    bool locked_;
};

class RWMutex
{
public:
    /// 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&lock_, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&lock_);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&lock_);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&lock_);
    }

    void unlock() {
        pthread_rwlock_unlock(&lock_);
    }

private:
    pthread_rwlock_t lock_;
};

class Coroutine;

class CoroutineMutex {
public:
    typedef ScopedLockImpl<CoroutineMutex> Lock;

    CoroutineMutex();
    ~CoroutineMutex();

    void lock();
    void unlock();
private:
    bool lock_{false};
    std::mutex mutex_;
    std::queue<Coroutine*> sleepCors_;
};

}
#endif