#include <cassert>
#include <cstdio>
#include <memory>
#include <algorithm>
#include "config.h"
#include "log.h"
#include "netaddress.h"
#include "tcp_server.h"

namespace corpc {

extern corpc::Logger::ptr gLogger;
extern corpc::TcpServer::ptr gTcpServer;

Config::Config(const std::string &filePath) : filePath_(filePath)
{
    try {
        yamlFile_ = YAML::LoadFile(filePath_);
    }
    catch (YAML::Exception &e) {
        printf("start corpc server error! read conf file [%s] error info: [%s]\n", filePath_.c_str(), e.what());
        exit(0);
    }
}

void Config::readLogConfig(YAML::Node &node)
{
    YAML::Node logPathNode = node["log_path"];
    if (!logPathNode || !logPathNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log_path] yaml node\n", filePath_.c_str());
        exit(0);
    }
    logPath_ = logPathNode.as<std::string>();

    YAML::Node logPrefixNode = node["log_prefix"];
    if (!logPrefixNode || !logPrefixNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log_prefix] yaml node\n", filePath_.c_str());
        exit(0);
    }
    logPrefix_ = logPrefixNode.as<std::string>();

    YAML::Node logMaxSizeNode = node["log_max_file_size"];
    if (!logMaxSizeNode || !logMaxSizeNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log_max_file_size] yaml node\n", filePath_.c_str());
        exit(0);
    }
    int logMaxSize = std::stoi(logMaxSizeNode.as<std::string>());
    logMaxSize_ = logMaxSize * 1024 * 1024;

    YAML::Node logLevelNode = node["log_level"];
    if (!logLevelNode || !logLevelNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log_level] yaml node\n", filePath_.c_str());
        exit(0);
    }
    std::string logLevel = logLevelNode.as<std::string>();
    logLevel_ = stringToLevel(logLevel);

    YAML::Node userLogLevelNode = node["user_log_level"];
    if (!userLogLevelNode || !userLogLevelNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [user_log_level] yaml node\n", filePath_.c_str());
        exit(0);
    }
    logLevel = userLogLevelNode.as<std::string>();
    logLevel_ = stringToLevel(logLevel);

    YAML::Node logSyncIntervalNode = node["log_sync_interval"];
    if (!logSyncIntervalNode || !logSyncIntervalNode.IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log_sync_interval] yaml node\n", filePath_.c_str());
        exit(0);
    }
    logSyncInterval_ = std::stoi(logSyncIntervalNode.as<std::string>());

    gLogger = std::make_shared<Logger>();
    gLogger->init(logPrefix_, logPath_, logMaxSize_, logSyncInterval_);
}

void Config::readConf()
{
    YAML::Node logNode = yamlFile_["log"];
    if (!logNode || !logNode.IsMap()) {
        printf("start corpc server error! read config file [%s] error, cannot read [log] yaml node\n", filePath_.c_str());
        exit(0);
    }

    readLogConfig(logNode);

    YAML::Node timeWheelNode = yamlFile_["time_wheel"];
    if (!timeWheelNode || !timeWheelNode.IsMap()) {
        printf("start corpc server error! read config file [%s] error, cannot read [time_wheel] yaml node\n", filePath_.c_str());
        exit(0);
    }
    if (!timeWheelNode["bucket_num"] || !timeWheelNode["bucket_num"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [time_wheel.bucket_num] yaml node\n", filePath_.c_str());
        exit(0);
    }
    if (!timeWheelNode["interval"] || !timeWheelNode["interval"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [time_wheel.interval] yaml node\n", filePath_.c_str());
        exit(0);
    }
    timewheelBucketNum_ = std::stoi(timeWheelNode["bucket_num"].as<std::string>());
    timewheelInterval_ = std::stoi(timeWheelNode["interval"].as<std::string>());

    YAML::Node coroutineNode = yamlFile_["coroutine"];
    if (!coroutineNode || !coroutineNode.IsMap()) {
        printf("start corpc server error! read config file [%s] error, cannot read [coroutine] yaml node\n", filePath_.c_str());
        exit(0);
    }
    if (!coroutineNode["coroutine_stack_size"] || !coroutineNode["coroutine_stack_size"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [coroutine.coroutine_stack_size] yaml node\n", filePath_.c_str());
        exit(0);
    }
    if (!coroutineNode["coroutine_pool_size"] || !coroutineNode["coroutine_pool_size"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [coroutine.coroutine_pool_size] yaml node\n", filePath_.c_str());
        exit(0);
    }
    int corStackSize = std::stoi(coroutineNode["coroutine_stack_size"].as<std::string>());
    corStackSize_ = 1024 * corStackSize;
    corPoolSize_ = std::stoi(coroutineNode["coroutine_pool_size"].as<std::string>());

    if (!yamlFile_["msg_seq_len"] || !yamlFile_["msg_seq_len"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [msg_seq_len] yaml node\n", filePath_.c_str());
        exit(0);
    }
    msgSeqLen_ = std::stoi(yamlFile_["msg_seq_len"].as<std::string>());

    if (!yamlFile_["max_connect_timeout"] || !yamlFile_["max_connect_timeout"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [max_connect_timeout] yaml node\n", filePath_.c_str());
        exit(0);
    }
    int maxConnectTimeout = std::stoi(yamlFile_["max_connect_timeout"].as<std::string>());
    maxConnectTimeout_ = maxConnectTimeout * 1000;

    if (!yamlFile_["iothread_num"] || !yamlFile_["iothread_num"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [iothread_num] yaml node\n", filePath_.c_str());
        exit(0);
    }
    iothreadNum_ = std::stoi(yamlFile_["iothread_num"].as<std::string>());

    YAML::Node serverNode = yamlFile_["server"];
    if (!serverNode || !serverNode.IsMap()) {
        printf("start corpc server error! read config file [%s] error, cannot read [server] yaml node\n", filePath_.c_str());
        exit(0);
    }

    if (!serverNode["ip"] || !serverNode["ip"].IsScalar() || 
        !serverNode["port"] || !serverNode["port"].IsScalar() || 
        !serverNode["protocol"] || !serverNode["protocol"].IsScalar()) {
        printf("start corpc server error! read config file [%s] error, cannot read [server.ip] or [server.port] or [server.protocol] yaml node\n", filePath_.c_str());
        exit(0);
    }
    std::string ip = serverNode["ip"].as<std::string>();
    if (ip.empty()) {
        ip = "0.0.0.0";
    }
    int port = std::stoi(serverNode["port"].as<std::string>());
    if (port == 0) {
        printf("start corpc server error! read config file [%s] error, read [server.port] = 0\n", filePath_.c_str());
        exit(0);
    }
    std::string protocol = serverNode["protocol"].as<std::string>();
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), toupper);

    corpc::IPAddress::ptr addr = std::make_shared<corpc::IPAddress>(ip, port);

    if (protocol == "HTTP") {
        gTcpServer = std::make_shared<TcpServer>(addr, Http_Protocol);
    }
    else if (protocol == "PB") {
        gTcpServer = std::make_shared<TcpServer>(addr, Pb_Protocol);
    }

    char buff[512] = {0};
    sprintf(buff, "read config from file [%s]: [log_path: %s], [log_prefix: %s], [log_max_size: %d MB], [log_level: %s], "
                    "[coroutine_stack_size: %d KB], [coroutine_pool_size: %d], "
                    "[msg_seq_len: %d], [max_connect_timeout: %d s], "
                    "[iothread_num:%d], [timewheel_bucket_num: %d], [timewheel_inteval: %d s], [server_ip: %s], [server_Port: %d], [server_protocol: %s]\n",
            filePath_.c_str(), logPath_.c_str(),logPrefix_.c_str(), logMaxSize_ / 1024 / 1024,
            levelToString(logLevel_).c_str(), corStackSize_, corPoolSize_, msgSeqLen_,
            maxConnectTimeout_, iothreadNum_, timewheelBucketNum_, timewheelInterval_, ip.c_str(), port, protocol.c_str());

    std::string s(buff);
    LOG_INFO << s;
}

Config::~Config()
{
}

YAML::Node Config::getYamlNode(const std::string &name)
{
    return yamlFile_[name];
}

}