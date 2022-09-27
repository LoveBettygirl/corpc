#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <memory>
#include <map>
#include "const.h"

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

    int msgReqLen_{0};

    int maxConnectTimeout_{0}; // ms
    int iothreadNum_{0};

    int timewheelBucketNum_{0};
    int timewheelInterval_{0};

private:
    std::string filePath_;
    YAML::Node yamlFile_;
};

}

#endif