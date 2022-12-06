#ifndef CORPC_COMMOM_LOG_H
#define CORPC_COMMOM_LOG_H

#include <sstream>
#include <ctime>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include <queue>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include "corpc/common/const.h"
#include "corpc/common/config.h"

namespace corpc {

extern corpc::Config::ptr gConfig;

#define LOG_DEBUG                                                                            \
    if (corpc::openLog() && corpc::LogLevel::DEBUG >= corpc::gConfig->logLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::DEBUG, __FILE__, __LINE__, __func__, corpc::LogType::INTER_LOG))).getStringStream()

#define LOG_INFO                                                                            \
    if (corpc::openLog() && corpc::LogLevel::INFO >= corpc::gConfig->logLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::INFO, __FILE__, __LINE__, __func__, corpc::LogType::INTER_LOG))).getStringStream()

#define LOG_WARN                                                                            \
    if (corpc::openLog() && corpc::LogLevel::WARN >= corpc::gConfig->logLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::WARN, __FILE__, __LINE__, __func__, corpc::LogType::INTER_LOG))).getStringStream()

#define LOG_ERROR                                                                            \
    if (corpc::openLog() && corpc::LogLevel::ERROR >= corpc::gConfig->logLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::ERROR, __FILE__, __LINE__, __func__, corpc::LogType::INTER_LOG))).getStringStream()

#define LOG_FATAL                                                                            \
    if (corpc::openLog() && corpc::LogLevel::FATAL >= corpc::gConfig->logLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::FATAL, __FILE__, __LINE__, __func__, corpc::LogType::INTER_LOG))).getStringStream()

#define USER_LOG_DEBUG                                                                             \
    if (corpc::openLog() && corpc::LogLevel::DEBUG >= corpc::gConfig->userLogLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::DEBUG, __FILE__, __LINE__, __func__, corpc::LogType::USER_LOG))).getStringStream()

#define USER_LOG_INFO                                                                             \
    if (corpc::openLog() && corpc::LogLevel::INFO >= corpc::gConfig->userLogLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::INFO, __FILE__, __LINE__, __func__, corpc::LogType::USER_LOG))).getStringStream()

#define USER_LOG_WARN                                                                             \
    if (corpc::openLog() && corpc::LogLevel::WARN >= corpc::gConfig->userLogLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::WARN, __FILE__, __LINE__, __func__, corpc::LogType::USER_LOG))).getStringStream()

#define USER_LOG_ERROR                                                                             \
    if (corpc::openLog() && corpc::LogLevel::ERROR >= corpc::gConfig->userLogLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::ERROR, __FILE__, __LINE__, __func__, corpc::LogType::USER_LOG))).getStringStream()

#define USER_LOG_FATAL                                                                             \
    if (corpc::openLog() && corpc::LogLevel::FATAL >= corpc::gConfig->userLogLevel) \
    corpc::LogTemp(corpc::LogEvent::ptr(new corpc::LogEvent(corpc::LogLevel::FATAL, __FILE__, __LINE__, __func__, corpc::LogType::USER_LOG))).getStringStream()

pid_t gettid();

LogLevel stringToLevel(const std::string &str);
std::string levelToString(LogLevel level);

bool openLog();

// 代表一条日志的信息
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(LogLevel level, const std::string &fileName, int line, const std::string &funcName, LogType type);

    ~LogEvent();

    std::stringstream &getStringStream();

    void log(); // 打印日志

    LogLevel getLevel() const { return level_; }

private:
    timeval timeval_;
    LogLevel level_;
    pid_t pid_{0};
    pid_t tid_{0};
    int corId_{0};

    std::string fileName_;
    int line_{0};
    std::string funcName_;
    LogType type_;
    std::stringstream ss_;
};

// 封装日志事件，析构函数打印日志
class LogTemp {
public:
    explicit LogTemp(LogEvent::ptr event);

    ~LogTemp();

    std::stringstream &getStringStream();

private:
    LogEvent::ptr event_;
};

class AsyncLogger {
public:
    typedef std::shared_ptr<AsyncLogger> ptr;

    AsyncLogger(const std::string &fileName, const std::string &filePath, int maxSize, LogType logType);
    ~AsyncLogger();

    void push(std::vector<std::string> &buffer);

    void flush();

    void execute();

    void stop();

public:
    std::queue<std::vector<std::string>> tasks_;

private:
    std::string fileName_;
    std::string filePath_;
    int maxSize_{0};
    LogType logType_;
    int no_{0};
    bool needReopen_{false};
    FILE *fileHandle_{nullptr};
    std::string date_;

    std::mutex mutex_;
    std::condition_variable cond_;
    bool stop_{false};

public:
    std::shared_ptr<std::thread> thread_; // 如果不需要该类的对象一创建就执行线程，务必使用指针/智能指针
    sem_t semaphore_;
};

class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger();
    ~Logger();

    void init(const std::string &fileName, const std::string &filePath, int maxSize, int syncInterval);
    void pushLog(const std::string &log_msg);
    void pushUserLog(const std::string &log_msg);
    void loopFunc();

    void flush();

    void start();

    AsyncLogger::ptr getAsyncLogger() { return asyncLogger_; }
    AsyncLogger::ptr getAsyncUserLogger() { return asyncUserLogger_; }

public:
    std::vector<std::string> buffer_;
    std::vector<std::string> userBuffer_;

private:
    std::mutex userBuffMutex_;
    std::mutex buffMutex_;
    bool isInit_{false};
    AsyncLogger::ptr asyncLogger_;
    AsyncLogger::ptr asyncUserLogger_;

    int syncInterval_{0};
};

void Exit(int code);

void Abort();

}

#endif
