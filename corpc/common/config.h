#ifndef CORPC_COMMOM_CONFIG_H
#define CORPC_COMMOM_CONFIG_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <memory>
#include <map>
#include "corpc/common/const.h"
#include "corpc/net/load_balance.h"

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
    std::string logPath;
    std::string logPrefix;
    int logMaxSize{0};
    LogLevel logLevel{LogLevel::DEBUG};
    LogLevel userLogLevel{LogLevel::DEBUG};
    int logSyncInterval{500};

    // coroutine params
    int corStackSize{0};
    int corPoolSize{0};

    int msgSeqLen{0};

    int maxConnectTimeout{0}; // ms
    int iothreadNum{0};

    int timewheelBucketNum{0};
    int timewheelInterval{0};

    ServiceRegisterCategory serviceRegister;
    std::string zkIp;
    int zkPort{0};
    int zkTimeout{0};

private:
    std::string filePath_;
    YAML::Node yamlFile_;
};

}

#endif