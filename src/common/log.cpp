#include <ctime>
#include <sys/time.h>
#include <sstream>
#include <sys/syscall.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <algorithm>
#include <semaphore.h>
#include <atomic>
#include <cassert>
#include <functional>

#include "log.h"
#include "runtime.h"
#include "coroutine.h"

namespace corpc {

extern corpc::Logger::ptr gLogger;
extern corpc::Config::ptr gConfig;

void coredumpHandler(int signal_no)
{
    LOG_ERROR << "progress received invalid signal, will exit";
    printf("progress received invalid signal, will exit\n");
    gLogger->flush();
    gLogger->getAsyncLogger()->thread_->join();
    gLogger->getAsyncUserLogger()->thread_->join();

    signal(signal_no, SIG_DFL);
    raise(signal_no);
}

class Coroutine;

static thread_local pid_t t_thread_id = 0;
static pid_t g_pid = 0;

pid_t gettid()
{
    if (t_thread_id == 0) {
        t_thread_id = syscall(SYS_gettid);
    }
    return t_thread_id;
}

bool openLog()
{
    if (!gLogger) {
        return false;
    }
    return true;
}

LogEvent::LogEvent(LogLevel level, const std::string &fileName, int line, const std::string &funcName, LogType type)
    : level_(level),
    fileName_(fileName),
    line_(line),
    funcName_(funcName),
    type_(type)
{
}

LogEvent::~LogEvent()
{
}

std::string levelToString(LogLevel level)
{
    std::string re = "DEBUG";
    switch (level) {
    case NONE:
        re = "NONE";
        return re;
    case DEBUG:
        re = "DEBUG";
        return re;
    case INFO:
        re = "INFO";
        return re;
    case WARN:
        re = "WARN";
        return re;
    case ERROR:
        re = "ERROR";
        return re;
    case FATAL:
        re = "FATAL";
        return re;
    default:
        return re;
    }
}

LogLevel stringToLevel(const std::string &str)
{
    if (str == "NONE")
        return LogLevel::NONE;

    if (str == "DEBUG")
        return LogLevel::DEBUG;

    if (str == "INFO")
        return LogLevel::INFO;

    if (str == "WARN")
        return LogLevel::WARN;

    if (str == "ERROR")
        return LogLevel::ERROR;

    if (str == "FATAL")
        return LogLevel::FATAL;

    return LogLevel::DEBUG;
}

std::string logTypeToString(LogType logtype)
{
    switch (logtype) {
    case INTER_LOG:
        return "internal";
    case USER_LOG:
        return "user";
    default:
        return "";
    }
}

std::stringstream &LogEvent::getStringStream()
{
    gettimeofday(&timeval_, nullptr);

    struct tm time;
    localtime_r(&(timeval_.tv_sec), &time);

    const char *format = "%Y-%m-%d %H:%M:%S";
    char buf[128] = {0};
    strftime(buf, sizeof(buf), format, &time);

    ss_ << "[" << buf << "." << timeval_.tv_usec << "]\t";

    std::string levelStr = levelToString(level_);
    ss_ << "[" << levelStr << "]\t";

    if (g_pid == 0) {
        g_pid = getpid();
    }
    pid_ = g_pid;

    if (t_thread_id == 0) {
        t_thread_id = gettid();
    }
    tid_ = t_thread_id;

    corId_ = Coroutine::getCurrentCoroutine()->getCorId();

    ss_ << "[" << pid_ << "]\t"
        << "[" << tid_ << "]\t"
        << "[" << corId_ << "]\t"
        << "[" << fileName_ << ":" << line_ << "]\t"
        << "[" << funcName_ << "]\t";

    RunTime *runtime = getCurrentRunTime();
    if (runtime) {
        std::string msgNo = runtime->msgNo_;
        if (!msgNo.empty()) {
            ss_ << "[" << msgNo << "]\t";
        }

        std::string interfaceName = runtime->interfaceName_;
        if (!interfaceName.empty()) {
            ss_ << "[" << interfaceName << "]\t";
        }
    }
    return ss_;
}

void LogEvent::log()
{
    ss_ << "\n";
    if (level_ >= gConfig->logLevel_ && type_ == INTER_LOG) {
        gLogger->pushLog(ss_.str());
    }
    else if (level_ >= gConfig->userLogLevel_ && type_ == USER_LOG) {
        gLogger->pushUserLog(ss_.str());
    }
}

LogTemp::LogTemp(LogEvent::ptr event) : event_(event)
{
}

std::stringstream &LogTemp::getStringStream()
{
    return event_->getStringStream();
}

LogTemp::~LogTemp()
{
    event_->log();
    // 打印FATAL级别日志后，应当终止程序运行（错误严重到无法容忍程序继续运行）
    if (event_->getLevel() == FATAL) {
        Abort();
    }
}

Logger::Logger()
{
    // cannot do anything which will call LOG ,otherwise is will coredump
}

Logger::~Logger()
{
    flush();
    asyncLogger_->thread_->join();
    asyncUserLogger_->thread_->join();
}

void Logger::init(const std::string &fileName, const std::string &filePath, int maxSize, int syncInterval)
{
    if (!isInit_) {
        syncInterval_ = syncInterval;
        for (int i = 0; i < 1000000; ++i) {
            buffer_.push_back("");
            userBuffer_.push_back("");
        }

        asyncLogger_ = std::make_shared<AsyncLogger>(fileName, filePath, maxSize, INTER_LOG);
        asyncUserLogger_ = std::make_shared<AsyncLogger>(fileName, filePath, maxSize, USER_LOG);

        signal(SIGSEGV, coredumpHandler);
        signal(SIGABRT, coredumpHandler);
        signal(SIGTERM, coredumpHandler);
        signal(SIGKILL, coredumpHandler);
        signal(SIGINT, coredumpHandler);
        signal(SIGSTKFLT, coredumpHandler);

        // ignore SIGPIPE
        signal(SIGPIPE, SIG_IGN);
        isInit_ = true;
    }
}

void Logger::start()
{
    // TODO: 实现定时任务，让线程间隔syncInteval写日志文件
    TimerEvent::ptr event = std::make_shared<TimerEvent>(syncInteval, true, std::bind(&Logger::loopFunc, this));
    Reactor::GetReactor()->getTimer()->addTimerEvent(event);
}

void Logger::loopFunc()
{
    std::vector<std::string> userTemp;
    std::unique_lock<std::mutex> lock1(userBuffMutex_);
    userTemp.swap(userBuffer_);
    lock1.unlock();

    std::vector<std::string> temp;
    std::unique_lock<std::mutex> lock2(buffMutex_);
    temp.swap(buffer_);
    lock2.unlock();

    // 将一块缓存的所有字符串放进asyncLogger的任务队列中
    // asyncLogger的线程执行的时候，会从任务队列中取出队头的那一块，将那一块所有字符串写入文件
    asyncLogger_->push(temp);
    asyncUserLogger_->push(userTemp);
}

void Logger::pushLog(const std::string &msg)
{
    std::unique_lock<std::mutex> lock(buffMutex_);
    buffer_.push_back(std::move(msg));
}

void Logger::pushUserLog(const std::string &msg)
{
    std::unique_lock<std::mutex> lock(userBuffMutex_);
    userBuffer_.push_back(std::move(msg));
}

void Logger::flush()
{
    loopFunc();
    asyncLogger_->stop();
    asyncLogger_->flush();

    asyncUserLogger_->stop();
    asyncUserLogger_->flush();
}

AsyncLogger::AsyncLogger(const std::string &fileName, const std::string &filePath, int maxSize, LogType logType)
    : fileName_(fileName), filePath_(filePath), maxSize_(maxSize), logType_(logType)
{
    int ret = sem_init(&semaphore_, 0, 0);
    assert(ret == 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread(std::bind(&AsyncLogger::execute, this)));

    ret = sem_wait(&semaphore_);
    assert(ret == 0);
}

AsyncLogger::~AsyncLogger()
{
}

void AsyncLogger::execute()
{
    int ret = sem_post(&semaphore_);
    assert(ret == 0);

    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (tasks_.empty() && !stop_) {
            cond_.wait(lock);
        }
        std::vector<std::string> temp;
        temp.swap(tasks_.front());
        tasks_.pop();
        bool isStop = stop_;
        lock.unlock();

        timeval now;
        gettimeofday(&now, nullptr);

        struct tm nowTime;
        localtime_r(&(now.tv_sec), &nowTime);

        const char *format = "%Y%m%d";
        char date[32] = {0};
        strftime(date, sizeof(date), format, &nowTime);
        if (date_ != std::string(date)) {
            // cross day
            no_ = 0;
            date_ = std::string(date);
            needReopen_ = true;
        }

        if (!fileHandle_) {
            needReopen_ = true;
        }

        std::stringstream ss;
        ss << filePath_ << fileName_ << "_" << date_ << "_" << logTypeToString(logType_) << "_" << no_ << ".log";
        std::string fullFileName = ss.str();

        if (needReopen_) {
            if (fileHandle_) {
                fclose(fileHandle_);
            }

            fileHandle_ = fopen(fullFileName.c_str(), "a");
            needReopen_ = false;
        }

        // 当前日志文件大小超过maxSize，就创建新的日志文件，后续的日志写入这个新的日志文件
        if (ftell(fileHandle_) > maxSize_) {
            fclose(fileHandle_);

            // single log file over max size
            no_++;
            std::stringstream ss2;
            ss2 << filePath_ << fileName_ << "_" << date_ << "_" << logTypeToString(logType_) << "_" << no_ << ".log";
            fullFileName = ss2.str();

            fileHandle_ = fopen(fullFileName.c_str(), "a");
            needReopen_ = false;
        }

        if (!fileHandle_) {
            printf("open log file %s error!", fullFileName.c_str());
        }

        for (auto i : temp) {
            if (!i.empty()) {
                fwrite(i.c_str(), 1, i.length(), fileHandle_);
            }
        }
        temp.clear();
        fflush(fileHandle_);
        if (isStop) {
            break;
        }
    }
    if (!fileHandle_) {
        fclose(fileHandle_);
    }

}

void AsyncLogger::push(std::vector<std::string> &buffer)
{
    if (!buffer.empty()) {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push(buffer);
        lock.unlock();
        cond_.notify_one();
    }
}

void AsyncLogger::flush()
{
    if (fileHandle_) {
        fflush(fileHandle_);
    }
}

void AsyncLogger::stop()
{
    if (!stop_) {
        stop_ = true;
        cond_.notify_one();
    }
}

void Exit(int code)
{
    printf("It's sorry to said we start corpc server error, look up log file to get more details!\n");
    gLogger->flush();
    gLogger->getAsyncLogger()->thread_->join();
    gLogger->getAsyncUserLogger()->thread_->join();

    _exit(code);
}

void Abort()
{
    gLogger->flush();
    gLogger->getAsyncLogger()->thread_->join();
    gLogger->getAsyncUserLogger()->thread_->join();

    abort();
}

}
