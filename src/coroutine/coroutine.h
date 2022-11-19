#ifndef CORPC_COROUTINE_COROUTINE_H
#define CORPC_COROUTINE_COROUTINE_H

#include <memory>
#include <functional>
#include <string>
#include "coctx.h"
#include "runtime.h"

namespace corpc {

int getCoroutineIndex();
RunTime* getCurrentRunTime();
void setCurrentRunTime(RunTime *v);

class Coroutine {
public:
    typedef std::shared_ptr<Coroutine> ptr;

private:
    Coroutine();

public:
    Coroutine(int size, char *stack_ptr);
    Coroutine(int size, char *stack_ptr, std::function<void()> cb);
    ~Coroutine();

    bool setCallBack(std::function<void()> cb);
    int getCorId() const { return corId_; }
    void setIsInCoFunc(const bool v) { isInCofunc_ = v; }
    bool getIsInCoFunc() const { return isInCofunc_; }
    void setIndex(int index) { index_ = index; }
    int getIndex() { return index_; }
    char *getStackPtr() { return stackSp_; }
    int getStackSize() { return stackSize_; }
    void setCanResume(bool v) { canResume_ = v; }
    RunTime* getRunTime() { return &runtime_; }

    static void yield();
    static void resume(Coroutine *cor);
    static Coroutine *getCurrentCoroutine();
    static Coroutine *getMainCoroutine();
    static bool isMainCoroutine();

private:
    int corId_{0};
    coctx coctx_;              // coroutine regs
    int stackSize_{0};        // size of stack memory space
    char *stackSp_{nullptr};     // coroutine's stack memory space, you can malloc or mmap get some memory to init this value
    bool isInCofunc_{false}; // true when call CoFunction, false when CoFunction finished
    RunTime runtime_;

    bool canResume_{true};

    int index_{-1}; // index in coroutine pool

public:
    std::function<void()> callback_;
};

}

#endif
