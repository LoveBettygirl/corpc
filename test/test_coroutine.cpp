#include <google/protobuf/service.h>
#include <iostream>
#include <thread>
#include "corpc/common/log.h"
#include "corpc/coroutine/coroutine_pool.h"
#include "corpc/coroutine/coroutine_hook.h"
#include "corpc/coroutine/coroutine.h"
#include "corpc/net/mutex.h"

corpc::Coroutine::ptr cor;
corpc::Coroutine::ptr cor2;

class Test {
public:
    corpc::CoroutineMutex coroutineMutex_;
    int a = 1;
};
Test test_;

void func1()
{
    std::cout << "cor1 ---- now first resume func1 coroutine by thread 1" << std::endl;
    std::cout << "cor1 ---- now begin to yield func1 coroutine" << std::endl;

    test_.coroutineMutex_.lock();

    std::cout << "cor1 ---- coroutine lock on test_, sleep 5s begin" << std::endl;

    sleep(5);
    std::cout << "cor1 ---- sleep 5s end, now back coroutine lock" << std::endl;

    test_.coroutineMutex_.unlock();

    corpc::Coroutine::yield();
    std::cout << "cor1 ---- func1 coroutine back, now end" << std::endl; // 这句不会执行
}

void func2()
{
    std::cout << "cor2 ---- now first resume func2 coroutine by thread 2" << std::endl;
    std::cout << "cor2 ---- sleep 2s begin" << std::endl;

    sleep(2);
    std::cout << "cor2 ---- coroutine2 want to get coroutine lock of test_" << std::endl;
    test_.coroutineMutex_.lock();

    std::cout << "cor2 ---- coroutine2 get coroutine lock of test_ succ" << std::endl; // 这句不会执行，因为线程1的子协程释放锁之前线程2就执行结束了
}

void thread1Func()
{
    std::cout << "thread 1 begin" << std::endl;
    std::cout << "now begin to resume func1 coroutine in thread 1" << std::endl;
    corpc::Coroutine::resume(cor.get());
    std::cout << "now func1 coroutine back in thread 1" << std::endl;
    std::cout << "thread 1 end" << std::endl;
}

void thread2Func()
{
    corpc::Coroutine::getCurrentCoroutine(); // 创建线程2的主协程
    std::cout << "thread 2 begin" << std::endl;
    std::cout << "now begin to resume func2 coroutine in thread 2" << std::endl;
    corpc::Coroutine::resume(cor2.get());
    std::cout << "now func2 coroutine back in thread 2" << std::endl;
    std::cout << "thread 2 end" << std::endl;
}

int main(int argc, char *argv[])
{
    corpc::setHook(false);
    std::cout << "main begin" << std::endl;
    int stackSize = 128 * 1024;
    char *sp = reinterpret_cast<char *>(malloc(stackSize));
    cor = std::make_shared<corpc::Coroutine>(stackSize, sp, func1); // 创建线程1的主协程和子协程

    char *sp2 = reinterpret_cast<char *>(malloc(stackSize));
    cor2 = std::make_shared<corpc::Coroutine>(stackSize, sp2, func2); // 创建线程2的子协程（注意，线程2的子协程是在线程1中创建的）

    std::thread thread2(thread2Func);

    thread1Func();

    thread2.join();

    std::cout << "main end" << std::endl;
}