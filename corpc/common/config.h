#ifndef CORPC_COMMOM_CONFIG_H
#define CORPC_COMMOM_CONFIG_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <memory>
#include <map>
#include "corpc/common/const.h"

namespace corpc {

class Config {
public:
    typedef std::shared_ptr<Config> ptr;

    Config(const std::string &filePath);

    ~Config();

    void readConf();

    void readLogConfig(YAML::Node &node);

    YAML::Node getYamlNode(const std::string &name);

public:
    // log params
    std::string logPath_;
    std::string logPrefix_;
    int logMaxSize_{0};
    LogLevel logLevel_{LogLevel::DEBUG};
    LogLevel userLogLevel_{LogLevel::DEBUG};
    int logSyncInterval_{500};

    // coroutine params
    int corStackSize_{0};
    int corPoolSize_{0};

    int msgSeqLen_{0};

    int maxConnectTimeout_{0}; // ms
    int iothreadNum_{0};

    int timewheelBucketNum_{0};
    int timewheelInterval_{0};

    bool zkServiceDiscoveryOn_{false};
    std::string zkIp_;
    int zkPort_{0};
    int zkTimeout_{0};
    std::string loadBalanceMethod_;

private:
    std::string filePath_;
    YAML::Node yamlFile_;
};

}

#endif